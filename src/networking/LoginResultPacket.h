#ifndef LOGIN_RESULT_PACKET_H
#define LOGIN_RESULT_PACKET_H

#include "Packet.h"
#include <string>

class LoginResultPacket : public Packet {
public:
    enum class Result : uint8_t {
        Success = 0,
        InvalidCredentials = 1,
        AccountBanned = 2,
        AccountNotFound = 3,
        ServerFull = 4,
        Maintenance = 5
    };

    LoginResultPacket() = default;
    LoginResultPacket(Result result, const std::string& message = "")
        : m_result(result), m_message(message) {}

    PacketType getType() const override { return PacketType::LoginResult; }

    void serialize(PacketBuffer& buffer) const override {
        buffer.write(static_cast<uint8_t>(m_result));
        buffer.writeString(m_message);
    }

    void deserialize(PacketBuffer& buffer) override {
        m_result = static_cast<Result>(buffer.read<uint8_t>());
        m_message = buffer.readString();
    }

    // Getters
    Result getResult() const { return m_result; }
    const std::string& getMessage() const { return m_message; }

    // Setters
    void setResult(Result result) { m_result = result; }
    void setMessage(const std::string& message) { m_message = message; }

protected:
    std::shared_ptr<Packet> createInstance() const override {
        return std::make_shared<LoginResultPacket>();
    }

private:
    Result m_result;
    std::string m_message;
};

#endif // LOGIN_RESULT_PACKET_H
