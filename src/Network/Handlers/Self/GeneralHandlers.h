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
        static bool HandleConnected(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool HandleSendAddress(std::shared_ptr<NetworkClient>, NetworkPacket*);
    };
}