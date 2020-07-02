#pragma once
#include <memory>

class MessageHandler;
class NetworkClient;
struct NetworkPacket;
namespace InternalSocket
{
    class GeneralHandlers
    {
    public:
        static void Setup(MessageHandler*);
        static bool SMSG_CONNECTED(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool SMSG_SEND_ADDRESS(std::shared_ptr<NetworkClient>, NetworkPacket*);
    };
}