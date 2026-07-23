#ifndef LOGIN_PACKET_H
#define LOGIN_PACKET_H

#include "Packet.h"
#include <string>

class LoginPacket : public Packet {
public:
    LoginPacket() = default;
    LoginPacket(const std::string& username, const std::string& password)
        : m_username(username), m_password(password) {}

    PacketType getType() const override { return PacketType::Login; }

    void serialize(PacketBuffer& buffer) const override {
        buffer.writeString(m_username);
        buffer.writeString(m_password);
    }

    void deserialize(PacketBuffer& buffer) override {
        m_username = buffer.readString();
        m_password = buffer.readString();
    }

    // Getters
    const std::string& getUsername() const { return m_username; }
    const std::string& getPassword() const { return m_password; }

    // Setters
    void setUsername(const std::string& username) { m_username = username; }
    void setPassword(const std::string& password) { m_password = password; }

protected:
    std::shared_ptr<Packet> createInstance() const override {
        return std::make_shared<LoginPacket>();
    }

private:
    std::string m_username;
    std::string m_password;
};

#endif // LOGIN_PACKET_H
