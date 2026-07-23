#ifndef PACKET_SERIALIZER_H
#define PACKET_SERIALIZER_H

#include <memory>
#include "Packet.h"
#include "PacketBuffer.h"

class PacketSerializer {
public:
    // Serialize a packet to buffer (includes type byte)
    static void serialize(const Packet& packet, PacketBuffer& buffer);
    
    // Deserialize a packet from buffer (assumes type byte is at current position)
    // Returns true if successful, false if not enough data or unknown type
    static bool deserialize(PacketBuffer& buffer, std::shared_ptr<Packet>& outPacket);
};

#endif // PACKET_SERIALIZER_H
