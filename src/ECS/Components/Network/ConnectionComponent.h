#pragma once
#include <NovusTypes.h>
#include <Utils/ConcurrentQueue.h>
#include <Networking/NetworkPacket.h>
#include <Networking/NetworkClient.h>

enum class PacketPriority
{
    LOW,
    MEDIUM,
    HIGH
};

#define LOW_PRIORITY_TIME 1
#define MEDIUM_PRIORITY_TIME 0.5f

struct ConnectionComponent
{
    ConnectionComponent() : packetQueue(256) { }

    std::shared_ptr<NetworkClient> connection;
    moodycamel::ConcurrentQueue<std::shared_ptr<NetworkPacket>> packetQueue;
};