#ifndef PLAYER_REMOVE_PACKET_H
#define PLAYER_REMOVE_PACKET_H

#include "Packet.h"

class PlayerRemovePacket : public Packet {
public:
    PlayerRemovePacket() = default;
    explicit PlayerRemovePacket(uint32_t entityId) : m_entityId(entityId) {}

    PacketType getType() const override { return PacketType::PlayerRemove; }

    void serialize(PacketBuffer& buffer) const override {
        buffer.write(m_entityId);
    }

    void deserialize(PacketBuffer& buffer) override {
        m_entityId = buffer.read<uint32_t>();
    }

    // Getter
    uint32_t getEntityId() const { return m_entityId; }

    // Setter
    void setEntityId(uint32_t id) { m_entityId = id; }

protected:
    std::shared_ptr<Packet> createInstance() const override {
        return std::make_shared<PlayerRemovePacket>();
    }

private:
    uint32_t m_entityId;
};

#endif // PLAYER_REMOVE_PACKET_H
