#ifndef PLAYER_STATE_PACKET_H
#define PLAYER_STATE_PACKET_H

#include "Packet.h"

class PlayerStatePacket : public Packet {
public:
    PlayerStatePacket() = default;
    PlayerStatePacket(uint32_t entityId, uint16_t health, uint16_t mana, uint8_t state)
        : m_entityId(entityId), m_health(health), m_mana(mana), m_state(state) {}

    PacketType getType() const override { return PacketType::PlayerState; }

    void serialize(PacketBuffer& buffer) const override {
        buffer.write(m_entityId);
        buffer.write(m_health);
        buffer.write(m_mana);
        buffer.write(m_state);
    }

    void deserialize(PacketBuffer& buffer) override {
        m_entityId = buffer.read<uint32_t>();
        m_health = buffer.read<uint16_t>();
        m_mana = buffer.read<uint16_t>();
        m_state = buffer.read<uint8_t>();
    }

    // Getters
    uint32_t getEntityId() const { return m_entityId; }
    uint16_t getHealth() const { return m_health; }
    uint16_t getMana() const { return m_mana; }
    uint8_t getState() const { return m_state; }

    // Setters
    void setEntityId(uint32_t id) { m_entityId = id; }
    void setHealth(uint16_t health) { m_health = health; }
    void setMana(uint16_t mana) { m_mana = mana; }
    void setState(uint8_t state) { m_state = state; }

protected:
    std::shared_ptr<Packet> createInstance() const override {
        return std::make_shared<PlayerStatePacket>();
    }

private:
    uint32_t m_entityId;
    uint16_t m_health;
    uint16_t m_mana;
    uint8_t m_state;
};

#endif // PLAYER_STATE_PACKET_H
