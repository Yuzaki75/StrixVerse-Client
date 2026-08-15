#pragma once

#include "Packet.h"

#include <string>

// New account details submitted by the register screen.
// Field order and types mirror Server/src/network/Packets/RegisterPacket.cpp exactly.
class RegisterPacket final : public Packet
{
public:
    std::string Username;
    std::string Email;
    std::string Password;

    Opcode getOpcode() const override { return Opcode::Register; }

    const char* getName() const override { return "RegisterPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.writeString(Username, ProtocolLimits::MaxUsernameLength);
        buffer.writeString(Email, ProtocolLimits::MaxEmailLength);
        buffer.writeString(Password, ProtocolLimits::MaxPasswordLength);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Username = buffer.readString(ProtocolLimits::MaxUsernameLength);
        Email = buffer.readString(ProtocolLimits::MaxEmailLength);
        Password = buffer.readString(ProtocolLimits::MaxPasswordLength);
    }
};
