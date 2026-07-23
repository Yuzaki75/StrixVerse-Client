#ifndef PACKET_FACTORY_H
#define PACKET_FACTORY_H

#include <memory>
#include "Packet.h"
#include "PacketRegistry.h"

class PacketFactory {
public:
    // Create a packet of the given type
    static std::shared_ptr<Packet> createPacket(PacketType type) {
        return PacketRegistry::createPacket(type);
    }
    
    // Register a packet type (to be called by each packet's static initializer)
    static void registerPacket(PacketType type, std::function<std::shared_ptr<Packet>()> creator) {
        PacketRegistry::registerPacket(type, std::move(creator));
    }
};

#endif // PACKET_FACTORY_H
