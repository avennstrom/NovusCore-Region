#include "AuthHandlers.h"
#include <entt.hpp>
#include <Networking/NetworkPacket.h>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkClient.h>
#include <Networking/AddressType.h>
#include <Utils/ByteBuffer.h>
#include "../../../../Utils/ServiceLocator.h"
#include "../../../../ECS/Components/Network/AuthenticationSingleton.h"
#include "../../../../ECS/Components/Network/ConnectionDeferredSingleton.h"

// @TODO: Remove Temporary Includes when they're no longer needed
#include <Utils/DebugHandler.h>

namespace InternalSocket
{
    void AuthHandlers::Setup(MessageHandler* messageHandler)
    {
        messageHandler->SetMessageHandler(Opcode::SMSG_LOGON_CHALLENGE, { ConnectionStatus::AUTH_CHALLENGE, sizeof(ServerLogonChallenge), AuthHandlers::HandshakeHandler });
        messageHandler->SetMessageHandler(Opcode::SMSG_LOGON_HANDSHAKE, { ConnectionStatus::AUTH_HANDSHAKE, sizeof(ServerLogonHandshake), AuthHandlers::HandshakeResponseHandler });
    }
    bool AuthHandlers::HandshakeHandler(std::shared_ptr<NetworkClient> networkClient, std::shared_ptr<NetworkPacket>& packet)
    {
        ServerLogonChallenge logonChallenge;
        logonChallenge.Deserialize(packet->payload);

        entt::registry* registry = ServiceLocator::GetRegistry();
        AuthenticationSingleton& authenticationSingleton = registry->ctx<AuthenticationSingleton>();

        // If "ProcessChallenge" fails, we have either hit a bad memory allocation or a SRP-6a safety check, thus we should close the connection
        if (!authenticationSingleton.srp.ProcessChallenge(logonChallenge.s, logonChallenge.B))
        {
            networkClient->Close(asio::error::no_data);
            return true;
        }

        std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<36>();
        ClientLogonHandshake clientResponse;

        std::memcpy(clientResponse.M1, authenticationSingleton.srp.M, 32);

        buffer->Put(Opcode::CMSG_LOGON_HANDSHAKE);
        buffer->PutU16(0);

        u16 payloadSize = clientResponse.Serialize(buffer);
        buffer->Put<u16>(payloadSize, 2);
        networkClient->Send(buffer);

        networkClient->SetStatus(ConnectionStatus::AUTH_HANDSHAKE);
        return true;
    }
    bool AuthHandlers::HandshakeResponseHandler(std::shared_ptr<NetworkClient> networkClient, std::shared_ptr<NetworkPacket>& packet)
    {
        // Handle handshake response
        ServerLogonHandshake logonResponse;
        logonResponse.Deserialize(packet->payload);

        entt::registry* registry = ServiceLocator::GetRegistry();
        AuthenticationSingleton& authenticationSingleton = registry->ctx<AuthenticationSingleton>();
        ConnectionDeferredSingleton& connectionDeferredSingleton = registry->ctx<ConnectionDeferredSingleton>();

        if (!authenticationSingleton.srp.VerifySession(logonResponse.HAMK))
        {
            NC_LOG_WARNING("Unsuccessful Login");
            networkClient->Close(asio::error::no_permission);
            return true;
        }
        else
        {
            NC_LOG_SUCCESS("Successful Login");
        }

        std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
        buffer->Put(Opcode::CMSG_CONNECTED);
        buffer->PutU16(8);
        buffer->Put(AddressType::REGION);
        buffer->PutU8(0);

        std::shared_ptr<NetworkServer> server = connectionDeferredSingleton.networkServer;
        auto& localEndpoint = networkClient->socket()->local_endpoint();
        buffer->PutU32(localEndpoint.address().to_v4().to_uint());
        buffer->PutU16(server->GetPort());

        networkClient->Send(buffer);

        networkClient->SetStatus(ConnectionStatus::AUTH_SUCCESS);
        return true;
    }
}