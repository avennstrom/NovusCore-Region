#include "ConnectionSystems.h"
#include <entt.hpp>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkServer.h>
#include "../../Components/Network/ConnectionSingleton.h"
#include "../../Components/Network/AuthenticationSingleton.h"
#include "../../Components/Network/ConnectionComponent.h"
#include "../../Components/Network/ConnectionDeferredSingleton.h"
#include "../../../Utils/ServiceLocator.h"
#include <tracy/Tracy.hpp>

void ConnectionUpdateSystem::Update(entt::registry& registry)
{
    ZoneScopedNC("ConnectionUpdateSystem::Update", tracy::Color::Blue)
    ConnectionSingleton& connectionSingleton = registry.ctx<ConnectionSingleton>();
    if (connectionSingleton.networkClient)
    {
        std::shared_ptr<NetworkPacket> packet = nullptr;

        MessageHandler* networkMessageHandler = ServiceLocator::GetSelfMessageHandler();
        while (connectionSingleton.packetQueue.try_dequeue(packet))
        {
#ifdef NC_Debug
            NC_LOG_SUCCESS("[Network/ClientSocket]: CMD: %u, Size: %u", packet->header.opcode, packet->header.size);
#endif // NC_Debug

            if (!networkMessageHandler->CallHandler(connectionSingleton.networkClient, packet.get()))
            {
                connectionSingleton.networkClient->Close(asio::error::shut_down);
                return;
            }
        }
    }

    MessageHandler* clientMessageHandler = ServiceLocator::GetClientMessageHandler();
    auto view = registry.view<ConnectionComponent>();
    view.each([&registry, &clientMessageHandler](const auto, ConnectionComponent& connection)
        {
            std::shared_ptr<NetworkPacket> packet;
            while (connection.packetQueue.try_dequeue(packet))
            {
#ifdef NC_Debug
                NC_LOG_SUCCESS("[Network/ServerSocket]: CMD: %u, Size: %u", packet->header.opcode, packet->header.size);
#endif // NC_Debug

                if (!clientMessageHandler->CallHandler(connection.connection, packet.get()))
                {
                    connection.connection->Close(asio::error::shut_down);
                    return;
                }
            }
        });
}

void ConnectionUpdateSystem::Server_HandleConnect(NetworkServer* server, asio::ip::tcp::socket* socket, const asio::error_code& error)
{
    if (!error)
    {
#ifdef NC_Debug
        NC_LOG_SUCCESS("[Network/Socket]: Client connected from (%s)", socket->remote_endpoint().address().to_string().c_str());
#endif // NC_Debug

        socket->non_blocking(true);
        socket->set_option(asio::socket_base::send_buffer_size(NETWORK_BUFFER_SIZE));
        socket->set_option(asio::socket_base::receive_buffer_size(NETWORK_BUFFER_SIZE));
        socket->set_option(asio::ip::tcp::no_delay(true));

        entt::registry* registry = ServiceLocator::GetRegistry();
        auto& connectionDeferredSingleton = registry->ctx<ConnectionDeferredSingleton>();
        connectionDeferredSingleton.newConnectionQueue.enqueue(socket);
    }
}

void ConnectionUpdateSystem::Client_HandleRead(BaseSocket* socket)
{
    NetworkClient* client = static_cast<NetworkClient*>(socket);

    std::shared_ptr<Bytebuffer> buffer = client->GetReceiveBuffer();
    entt::registry* registry = ServiceLocator::GetRegistry();

    entt::entity entity = static_cast<entt::entity>(client->GetEntityId());
    ConnectionComponent& connectionComponent = registry->get<ConnectionComponent>(entity);

    while (buffer->GetActiveSize())
    {
        Opcode opcode = Opcode::INVALID;
        u16 size = 0;

        buffer->Get(opcode);
        buffer->GetU16(size);

        if (size > NETWORK_BUFFER_SIZE)
        {
            client->Close(asio::error::shut_down);
            return;
        }

        std::shared_ptr<NetworkPacket> packet = NetworkPacket::Borrow();
        {
            // Header
            {
                packet->header.opcode = opcode;
                packet->header.size = size;
            }

            // Payload
            {
                if (size)
                {
                    packet->payload = Bytebuffer::Borrow<NETWORK_BUFFER_SIZE>();
                    packet->payload->size = size;
                    packet->payload->writtenData = size;
                    std::memcpy(packet->payload->GetDataPointer(), buffer->GetReadPointer(), size);
                }
            }

            connectionComponent.packetQueue.enqueue(packet);
        }

        buffer->readData += size;
    }

    client->Listen();
}
void ConnectionUpdateSystem::Client_HandleDisconnect(BaseSocket* socket)
{
#ifdef NC_Debug
    NC_LOG_WARNING("[Network/Socket]: Client disconnected from (%s)", socket->socket()->remote_endpoint().address().to_string().c_str());
#endif // NC_Debug

    entt::registry* registry = ServiceLocator::GetRegistry();
    auto& connectionDeferredSingleton = registry->ctx<ConnectionDeferredSingleton>();

    NetworkClient* client = static_cast<NetworkClient*>(socket);
    connectionDeferredSingleton.droppedConnectionQueue.enqueue(static_cast<entt::entity>(client->GetEntityId()));
}
void ConnectionUpdateSystem::Self_HandleConnect(BaseSocket* socket, bool connected)
{
    if (connected)
    {
#ifdef NC_Debug
        NC_LOG_SUCCESS("[Network/Socket]: Successfully connected to (%s, %u)", socket->socket()->remote_endpoint().address().to_string().c_str(), socket->socket()->remote_endpoint().port());
#endif // NC_Debug

        entt::registry* registry = ServiceLocator::GetRegistry();
        AuthenticationSingleton& authentication = registry->ctx<AuthenticationSingleton>();

        /* Send Initial Packet */
        std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<512>();

        authentication.srp.username = "region";
        authentication.srp.password = "password";

        // If StartAuthentication fails, it means A failed to generate and thus we cannot connect
        if (!authentication.srp.StartAuthentication())
            return;

        buffer->Put(Opcode::CMSG_LOGON_CHALLENGE);
        buffer->SkipWrite(sizeof(u16));

        u16 size = static_cast<u16>(buffer->writtenData);
        buffer->PutString(authentication.srp.username);
        buffer->PutBytes(authentication.srp.aBuffer->GetDataPointer(), authentication.srp.aBuffer->size);

        u16 writtenData = static_cast<u16>(buffer->writtenData) - size;

        buffer->Put<u16>(writtenData, 2);
        socket->Send(buffer.get());

        NetworkClient* networkClient = static_cast<NetworkClient*>(socket);
        networkClient->SetStatus(ConnectionStatus::AUTH_CHALLENGE);
        socket->AsyncRead();
    }
    else
    {
#ifdef NC_Debug
        NC_LOG_WARNING("[Network/Socket]: Failed connecting to (%s, %u)", socket->socket()->remote_endpoint().address().to_string().c_str(), socket->socket()->remote_endpoint().port());
#endif // NC_Debug
    }
}
void ConnectionUpdateSystem::Self_HandleRead(BaseSocket* socket)
{
    entt::registry* registry = ServiceLocator::GetRegistry();
    ConnectionSingleton& connectionSingleton = registry->ctx<ConnectionSingleton>();

    NetworkClient* client = static_cast<NetworkClient*>(socket);
    std::shared_ptr<Bytebuffer> buffer = client->GetReceiveBuffer();

    while (buffer->GetActiveSize())
    {
        Opcode opcode = Opcode::INVALID;
        u16 size = 0;

        buffer->Get(opcode);
        buffer->GetU16(size);

        if (size > NETWORK_BUFFER_SIZE)
        {
            client->Close(asio::error::shut_down);
            return;
        }

        std::shared_ptr<NetworkPacket> packet = NetworkPacket::Borrow();
        {
            // Header
            {
                packet->header.opcode = opcode;
                packet->header.size = size;
            }

            // Payload
            {
                if (size)
                {
                    packet->payload = Bytebuffer::Borrow<NETWORK_BUFFER_SIZE>();
                    packet->payload->size = size;
                    packet->payload->writtenData = size;
                    std::memcpy(packet->payload->GetDataPointer(), buffer->GetReadPointer(), size);
                }
            }

            connectionSingleton.packetQueue.enqueue(packet);
        }

        buffer->readData += size;
    }

    client->Listen();
}
void ConnectionUpdateSystem::Self_HandleDisconnect(BaseSocket* socket)
{
#ifdef NC_Debug
    NC_LOG_WARNING("[Network/Socket]: Disconnected from (%s, %u)", socket->socket()->remote_endpoint().address().to_string().c_str(), socket->socket()->remote_endpoint().port());
#endif // NC_Debug
}

void ConnectionDeferredSystem::Update(entt::registry& registry)
{
    ConnectionDeferredSingleton& connectionDeferredSingleton = registry.ctx<ConnectionDeferredSingleton>();

    if (connectionDeferredSingleton.newConnectionQueue.size_approx() > 0)
    {
        asio::ip::tcp::socket* socket;
        while (connectionDeferredSingleton.newConnectionQueue.try_dequeue(socket))
        {
            entt::entity entity = registry.create();

            ConnectionComponent& connectionComponent = registry.emplace<ConnectionComponent>(entity);
            connectionComponent.connection = std::make_shared<NetworkClient>(socket, entt::to_integral(entity));

            connectionComponent.connection->SetReadHandler(std::bind(&ConnectionUpdateSystem::Client_HandleRead, std::placeholders::_1));
            connectionComponent.connection->SetDisconnectHandler(std::bind(&ConnectionUpdateSystem::Client_HandleDisconnect, std::placeholders::_1));
            connectionComponent.connection->Listen();

            connectionDeferredSingleton.networkServer->AddConnection(connectionComponent.connection);
        }
    }

    if (connectionDeferredSingleton.droppedConnectionQueue.size_approx() > 0)
    {
        entt::entity entity;
        while (connectionDeferredSingleton.droppedConnectionQueue.try_dequeue(entity))
        {
            registry.destroy(entity);
        }
    }
}