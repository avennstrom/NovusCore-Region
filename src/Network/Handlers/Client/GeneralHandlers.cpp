#include "..\..\..\..\..\NovusCore-LoadBalancer\src\Network\Handlers\GeneralHandlers.h"
#include "GeneralHandlers.h"
#include <Networking/MessageHandler.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>
#include <Networking/PacketUtils.h>
#include <Networking/AddressType.h>
#include "../../../Utils/ServiceLocator.h"
#include "../../../ECS/Components/Network/ConnectionSingleton.h"

namespace Client
{
    void GeneralHandlers::Setup(MessageHandler* messageHandler)
    {
        messageHandler->SetMessageHandler(Opcode::MSG_REQUEST_ADDRESS, { ConnectionStatus::AUTH_NONE, 0, GeneralHandlers::MSG_REQUEST_ADDRESS });
    }

    bool GeneralHandlers::MSG_REQUEST_ADDRESS(std::shared_ptr<NetworkClient> networkClient, NetworkPacket* packet)
    {
        // We expect an empty packet
        if (packet->header.size > 0)
            return false;

        std::shared_ptr<Bytebuffer> buffer = Bytebuffer::Borrow<128>();
        if (!PacketUtils::Write_MSG_REQUEST_ADDRESS(buffer, AddressType::AUTH, networkClient->GetEntity()))
            return false;

        // Send the buffer to the Novus Service
        entt::registry* registry = ServiceLocator::GetRegistry();
        auto& connection = registry->ctx<ConnectionSingleton>();
        connection.networkClient->Send(buffer.get());

        return true;
    }
}