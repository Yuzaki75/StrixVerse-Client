#pragma once

#include "Packet.h"

#include <string>

// Credentials submitted by the login screen.
// Field order and types mirror Server/src/network/Packets/LoginPacket.cpp exactly.
class LoginPacket final : public Packet
{
public:
    std::string Username;
    std::string Password;

    Opcode getOpcode() const override { return Opcode::Login; }

    const char* getName() const override { return "LoginPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.writeString(Username, ProtocolLimits::MaxUsernameLength);
        buffer.writeString(Password, ProtocolLimits::MaxPasswordLength);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Username = buffer.readString(ProtocolLimits::MaxUsernameLength);
        Password = buffer.readString(ProtocolLimits::MaxPasswordLength);
    }
};
