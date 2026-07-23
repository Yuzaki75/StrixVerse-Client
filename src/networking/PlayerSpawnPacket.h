#ifndef PLAYER_SPAWN_PACKET_H
#define PLAYER_SPAWN_PACKET_H

#include "Packet.h"

class PlayerSpawnPacket : public Packet {
public:
    PlayerSpawnPacket() = default;
    PlayerSpawnPacket(uint32_t entityId, uint32_t playerId, float x, float y, uint16_t spriteId)
        : m_entityId(entityId), m_playerId(playerId), m_x(x), m_y(y), m_spriteId(spriteId) {}

    PacketType getType() const override { return PacketType::PlayerSpawn; }

    void serialize(PacketBuffer& buffer) const override {
        buffer.write(m_entityId);
        buffer.write(m_playerId);
        buffer.write(m_x);
        buffer.write(m_y);
        buffer.write(m_spriteId);
    }

    void deserialize(PacketBuffer& buffer) override {
        m_entityId = buffer.read<uint32_t>();
        m_playerId = buffer.read<uint32_t>();
        m_x = buffer.read<float>();
        m_y = buffer.read<float>();
        m_spriteId = buffer.read<uint16_t>();
    }

    // Getters
    uint32_t getEntityId() const { return m_entityId; }
    uint32_t getPlayerId() const { return m_playerId; }
    float getX() const { return m_x; }
    float getY() const { return m_y; }
    uint16_t getSpriteId() const { return m_spriteId; }

    // Setters
    void setEntityId(uint32_t id) { m_entityId = id; }
    void setPlayerId(uint32_t id) { m_playerId = id; }
    void setPosition(float x, float y) { m_x = x; m_y = y; }
    void setSpriteId(uint16_t id) { m_spriteId = id; }

protected:
    std::shared_ptr<Packet> createInstance() const override {
        return std::make_shared<PlayerSpawnPacket>();
    }

private:
    uint32_t m_entityId;
    uint32_t m_playerId;
    float m_x;
    float m_y;
    uint16_t m_spriteId;
};

#endif // PLAYER_SPAWN_PACKET_H
