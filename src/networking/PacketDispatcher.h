#ifndef PACKET_DISPATCHER_H
#define PACKET_DISPATCHER_H

#include <memory>
#include <vector>
#include <unordered_map>
#include "Packet.h"
#include "PacketHandler.h"

class PacketDispatcher {
public:
    // Add a handler for a specific packet type
    void addHandler(PacketType type, std::shared_ptr<PacketHandler> handler);

    // Remove a handler for a specific packet type
    void removeHandler(PacketType type, std::shared_ptr<PacketHandler> handler);

    // Dispatch a packet to all registered handlers for its type
    void dispatch(const std::shared_ptr<Packet>& packet);

private:
    std::unordered_map<PacketType, std::vector<std::shared_ptr<PacketHandler>>> m_handlers;
};

#endif // PACKET_DISPATCHER_H