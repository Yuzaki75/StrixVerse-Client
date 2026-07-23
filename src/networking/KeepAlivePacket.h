#ifndef KEEP_ALIVE_PACKET_H
#define KEEP_ALIVE_PACKET_H

#include "Packet.h"

class KeepAlivePacket : public Packet {
public:
    KeepAlivePacket() = default;

    PacketType getType() const override { return PacketType::KeepAlive; }

    void serialize(PacketBuffer& buffer) const override {
        // No payload
    }

    void deserialize(PacketBuffer& buffer) override {
        // No payload to read
    }

protected:
    std::shared_ptr<Packet> createInstance() const override {
        return std::make_shared<KeepAlivePacket>();
    }
};

#endif // KEEP_ALIVE_PACKET_H
