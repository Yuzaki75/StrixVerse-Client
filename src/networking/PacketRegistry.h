#ifndef PACKET_REGISTRY_H
#define PACKET_REGISTRY_H

#include <functional>
#include <unordered_map>
#include <memory>
#include "Packet.h"

class PacketRegistry {
public:
    using CreatorFunc = std::function<std::shared_ptr<Packet>()>;

    // Register a creator function for a packet type
    static void registerPacket(PacketType type, CreatorFunc creator);
    
    // Create a packet of the given type using the registered creator
    static std::shared_ptr<Packet> createPacket(PacketType type);
    
    // Clear all registrations (mainly for testing)
    static void clear();

private:
    PacketRegistry() = default;
    static std::unordered_map<PacketType, CreatorFunc>& getMap();
};

#endif // PACKET_REGISTRY_H
