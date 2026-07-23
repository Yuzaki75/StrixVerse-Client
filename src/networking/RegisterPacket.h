#ifndef REGISTER_PACKET_H
#define REGISTER_PACKET_H

#include "Packet.h"
#include <string>

class RegisterPacket : public Packet {
public:
    RegisterPacket() = default;
    RegisterPacket(const std::string& username, const std::string& password, const std::string& email)
        : m_username(username), m_password(password), m_email(email) {}

    PacketType getType() const override { return PacketType::Register; }

    void serialize(PacketBuffer& buffer) const override {
        buffer.writeString(m_username);
        buffer.writeString(m_password);
        buffer.writeString(m_email);
    }

    void deserialize(PacketBuffer& buffer) override {
        m_username = buffer.readString();
        m_password = buffer.readString();
        m_email = buffer.readString();
    }

    // Getters
    const std::string& getUsername() const { return m_username; }
    const std::string& getPassword() const { return m_password; }
    const std::string& getEmail() const { return m_email; }

    // Setters
    void setUsername(const std::string& username) { m_username = username; }
    void setPassword(const std::string& password) { m_password = password; }
    void setEmail(const std::string& email) { m_email = email; }

protected:
    std::shared_ptr<Packet> createInstance() const override {
        return std::make_shared<RegisterPacket>();
    }

private:
    std::string m_username;
    std::string m_password;
    std::string m_email;
};

#endif // REGISTER_PACKET_H
