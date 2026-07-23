#ifndef PLAYER_MOVE_PACKET_H
#define PLAYER_MOVE_PACKET_H

#include "Packet.h"

class PlayerMovePacket : public Packet {
public:
    PlayerMovePacket() = default;
    PlayerMovePacket(uint32_t entityId, float x, float y)
        : m_entityId(entityId), m_x(x), m_y(y) {}

    PacketType getType() const override { return PacketType::PlayerMove; }

    void serialize(PacketBuffer& buffer) const override {
        buffer.write(m_entityId);
        buffer.write(m_x);
        buffer.write(m_y);
    }

    void deserialize(PacketBuffer& buffer) override {
        m_entityId = buffer.read<uint32_t>();
        m_x = buffer.read<float>();
        m_y = buffer.read<float>();
    }

    // Getters
    uint32_t getEntityId() const { return m_entityId; }
    float getX() const { return m_x; }
    float getY() const { return m_y; }

    // Setters
    void setEntityId(uint32_t id) { m_entityId = id; }
    void setPosition(float x, float y) { m_x = x; m_y = y; }

protected:
    std::shared_ptr<Packet> createInstance() const override {
        return std::make_shared<PlayerMovePacket>();
    }

private:
    uint32_t m_entityId;
    float m_x;
    float m_y;
};

#endif // PLAYER_MOVE_PACKET_H
