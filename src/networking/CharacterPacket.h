#ifndef CHARACTER_PACKET_H
#define CHARACTER_PACKET_H

#include "Packet.h"
#include <string>
#include <cstdint>

class CharacterPacket : public Packet {
public:
    CharacterPacket() = default;
    CharacterPacket(uint32_t playerId, const std::string& name, uint16_t level, uint32_t experience)
        : m_playerId(playerId), m_name(name), m_level(level), m_experience(experience) {}

    PacketType getType() const override { return PacketType::Character; }

    void serialize(PacketBuffer& buffer) const override {
        buffer.write(m_playerId);
        buffer.writeString(m_name);
        buffer.write(m_level);
        buffer.write(m_experience);
    }

    void deserialize(PacketBuffer& buffer) override {
        m_playerId = buffer.read<uint32_t>();
        m_name = buffer.readString();
        m_level = buffer.read<uint16_t>();
        m_experience = buffer.read<uint32_t>();
    }

    // Getters
    uint32_t getPlayerId() const { return m_playerId; }
    const std::string& getName() const { return m_name; }
    uint16_t getLevel() const { return m_level; }
    uint32_t getExperience() const { return m_experience; }

    // Setters
    void setPlayerId(uint32_t id) { m_playerId = id; }
    void setName(const std::string& name) { m_name = name; }
    void setLevel(uint16_t level) { m_level = level; }
    void setExperience(uint32_t exp) { m_experience = exp; }

protected:
    std::shared_ptr<Packet> createInstance() const override {
        return std::make_shared<CharacterPacket>();
    }

private:
    uint32_t m_playerId;
    std::string m_name;
    uint16_t m_level;
    uint32_t m_experience;
};

#endif // CHARACTER_PACKET_H
