#ifndef SESSION_PACKET_H
#define SESSION_PACKET_H

#include "Packet.h"
#include <string>
#include <cstdint>

class SessionPacket : public Packet {
public:
    SessionPacket() = default;
    SessionPacket(uint32_t sessionId, const std::string& username, uint32_t playerId)
        : m_sessionId(sessionId), m_username(username), m_playerId(playerId) {}

    PacketType getType() const override { return PacketType::Session; }

    void serialize(PacketBuffer& buffer) const override {
        buffer.write(m_sessionId);
        buffer.writeString(m_username);
        buffer.write(m_playerId);
    }

    void deserialize(PacketBuffer& buffer) override {
        m_sessionId = buffer.read<uint32_t>();
        m_username = buffer.readString();
        m_playerId = buffer.read<uint32_t>();
    }

    // Getters
    uint32_t getSessionId() const { return m_sessionId; }
    const std::string& getUsername() const { return m_username; }
    uint32_t getPlayerId() const { return m_playerId; }

    // Setters
    void setSessionId(uint32_t id) { m_sessionId = id; }
    void setUsername(const std::string& username) { m_username = username; }
    void setPlayerId(uint32_t id) { m_playerId = id; }

protected:
    std::shared_ptr<Packet> createInstance() const override {
        return std::make_shared<SessionPacket>();
    }

private:
    uint32_t m_sessionId;
    std::string m_username;
    uint32_t m_playerId;
};

#endif // SESSION_PACKET_H
