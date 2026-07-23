#ifndef HANDSHAKE_PACKET_H
#define HANDSHAKE_PACKET_H

#include "Packet.h"
#include <string>

class HandshakePacket : public Packet {
public:
    HandshakePacket() = default;
    HandshakePacket(uint32_t protocolVersion, const std::string& clientVersion, uint64_t randomToken)
        : m_protocolVersion(protocolVersion), m_clientVersion(clientVersion), m_randomToken(randomToken) {}

    PacketType getType() const override { return PacketType::Handshake; }

    void serialize(PacketBuffer& buffer) const override {
        buffer.write(m_protocolVersion);
        buffer.writeString(m_clientVersion);
        buffer.write(m_randomToken);
    }

    void deserialize(PacketBuffer& buffer) override {
        m_protocolVersion = buffer.read<uint32_t>();
        m_clientVersion = buffer.readString();
        m_randomToken = buffer.read<uint64_t>();
    }

    // Getters
    uint32_t getProtocolVersion() const { return m_protocolVersion; }
    const std::string& getClientVersion() const { return m_clientVersion; }
    uint64_t getRandomToken() const { return m_randomToken; }

    // Setters
    void setProtocolVersion(uint32_t v) { m_protocolVersion = v; }
    void setClientVersion(const std::string& v) { m_clientVersion = v; }
    void setRandomToken(uint64_t v) { m_randomToken = v; }

protected:
    std::shared_ptr<Packet> createInstance() const override {
        return std::make_shared<HandshakePacket>();
    }

private:
    uint32_t m_protocolVersion;
    std::string m_clientVersion;
    uint64_t m_randomToken;
};

#endif // HANDSHAKE_PACKET_H
