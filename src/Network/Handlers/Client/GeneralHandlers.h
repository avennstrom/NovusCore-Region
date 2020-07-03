#pragma once
#include <memory>

class MessageHandler;
class NetworkClient;
struct NetworkPacket;
namespace Client
{
    class GeneralHandlers
    {
    public:
        static void Setup(MessageHandler*);
        static bool HandleRequestAddress(std::shared_ptr<NetworkClient>, NetworkPacket*);
    };
}