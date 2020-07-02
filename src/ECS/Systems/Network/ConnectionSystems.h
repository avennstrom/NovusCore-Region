#pragma once
#include <asio.hpp>
#include <entity/fwd.hpp>
#include <Utils/ConcurrentQueue.h>

class NetworkServer;
class BaseSocket;
namespace moddycamel
{
    class ConcurrentQueue;
}
class ConnectionUpdateSystem
{
public:
    static void Update(entt::registry& registry);

    // Handlers for Network Server
    static void Server_HandleConnect(NetworkServer* server, asio::ip::tcp::socket* socket, const asio::error_code& error);

    // Handlers for Network Client
    static void Client_HandleRead(BaseSocket* socket);
    static void Client_HandleDisconnect(BaseSocket* socket);
    static void Self_HandleConnect(BaseSocket* socket, bool connected);
    static void Self_HandleRead(BaseSocket* socket);
    static void Self_HandleDisconnect(BaseSocket* socket);
};

class ConnectionDeferredSystem
{
public:
    static void Update(entt::registry& registry);
};