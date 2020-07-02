#pragma once
#include <NovusTypes.h>
#include <asio.hpp>
#include <Utils/ConcurrentQueue.h>
#include <Networking/NetworkServer.h>

struct ConnectionDeferredSingleton
{
    ConnectionDeferredSingleton() : newConnectionQueue(64), droppedConnectionQueue(32) { }

    std::shared_ptr<NetworkServer> networkServer;
    moodycamel::ConcurrentQueue<asio::ip::tcp::socket*> newConnectionQueue;
    moodycamel::ConcurrentQueue<entt::entity> droppedConnectionQueue;
};