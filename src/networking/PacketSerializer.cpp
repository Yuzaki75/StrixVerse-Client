#include "PacketSerializer.h"
#include "PacketFactory.h"
#include <stdexcept>

void PacketSerializer::serialize(const Packet& packet, PacketBuffer& buffer) {
    // Write the packet type as a uint8_t (or whatever enum size we use)
    uint8_t type = static_cast<uint8_t>(packet.getType());
    buffer.write(type);
    // Then let the packet serialize its own data
    packet.serialize(buffer);
}

bool PacketSerializer::deserialize(PacketBuffer& buffer, std::shared_ptr<Packet>& outPacket) {
    if (buffer.remaining() < sizeof(uint8_t)) {
        return false; // Not enough data for type
    }
    uint8_t type = buffer.read<uint8_t>();
    // Create the packet using the factory
    auto packet = PacketFactory::createPacket(static_cast<PacketType>(type));
    if (!packet) {
        return false; // Unknown type
    }
    // Deserialize the packet's data
    packet->deserialize(buffer);
    outPacket = packet;
    return true;
}
