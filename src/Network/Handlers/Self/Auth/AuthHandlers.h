#pragma once
#include <memory>

class MessageHandler;
class NetworkClient;
struct NetworkPacket;
namespace InternalSocket
{
    class AuthHandlers
    {
    public:
        static void Setup(MessageHandler*);
        static bool HandshakeHandler(std::shared_ptr<NetworkClient>, NetworkPacket*);
        static bool HandshakeResponseHandler(std::shared_ptr<NetworkClient>, NetworkPacket*);
    };
}