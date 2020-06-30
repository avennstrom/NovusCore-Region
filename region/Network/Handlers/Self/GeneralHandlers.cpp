#include "GeneralHandlers.h"
#include <entt.hpp>
#include <Networking/MessageHandler.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>
#include <Networking/PacketUtils.h>
#include <Networking/AddressType.h>
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/Network/ConnectionComponent.h"

void InternalSocket::GeneralHandlers::Setup(MessageHandler* messageHandler)
{
    messageHandler->SetMessageHandler(Opcode::SMSG_CONNECTED, InternalSocket::GeneralHandlers::SMSG_CONNECTED);
    messageHandler->SetMessageHandler(Opcode::SMSG_SEND_ADDRESS, InternalSocket::GeneralHandlers::SMSG_SEND_ADDRESS);
}

bool InternalSocket::GeneralHandlers::SMSG_CONNECTED(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
{
    if (networkClient->GetStatus() != ConnectionStatus::AUTH_SUCCESS)
        return false;

    networkClient->SetStatus(ConnectionStatus::CONNECTED);
    return true;
}
bool InternalSocket::GeneralHandlers::SMSG_SEND_ADDRESS(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
{
    if (networkClient->GetStatus() != ConnectionStatus::CONNECTED)
        return false;
    
    u8 status = 0;
    u32 address = 0;
    u16 port = 0;
    entt::entity entity = entt::null;

    if (!packet->payload->GetU8(status))
        return false;

    if (status > 0)
    {
        if (!packet->payload->GetU32(address))
            return false;

        if (!packet->payload->GetU16(port))
            return false;
    }

    if (!packet->payload->Get(entity))
        return false;

    std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
    if (!PacketUtils::Write_SMSG_SEND_ADDRESS(buffer, status, address, port))
        return false;

    entt::registry* registry = ServiceLocator::GetRegistry();
    auto& connectionComponent = registry->get<ConnectionComponent>(entity);
    connectionComponent.connection->Send(buffer.get());
    return true;
}
