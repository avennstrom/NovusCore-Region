#pragma once
#include <NovusTypes.h>
#include <Utils/ConcurrentQueue.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>

struct ConnectionSingleton
{
    ConnectionSingleton() : packetQueue(256) { }

    std::shared_ptr<NetworkClient> networkClient;
    moodycamel::ConcurrentQueue<std::shared_ptr<NetworkPacket>> packetQueue;
};