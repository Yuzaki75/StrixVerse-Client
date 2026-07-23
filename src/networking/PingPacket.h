#ifndef PING_PACKET_H
#define PING_PACKET_H

#include "Packet.h"
#include <cstdint>

class PingPacket : public Packet {
public:
    PingPacket() = default;
    explicit PingPacket(uint64_t timestamp) : m_timestamp(timestamp) {}

    PacketType getType() const override { return PacketType::Ping; }

    void serialize(PacketBuffer& buffer) const override {
        buffer.write(m_timestamp);
    }

    void deserialize(PacketBuffer& buffer) override {
        m_timestamp = buffer.read<uint64_t>();
    }

    uint64_t getTimestamp() const { return m_timestamp; }
    void setTimestamp(uint64_t timestamp) { m_timestamp = timestamp; }

protected:
    std::shared_ptr<Packet> createInstance() const override {
        return std::make_shared<PingPacket>();
    }

private:
    uint64_t m_timestamp;
};

#endif // PING_PACKET_H
