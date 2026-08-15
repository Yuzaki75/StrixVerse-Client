#pragma once

#include "Packet.h"

#include <string>

// Server rejected the credentials; Reason is shown on the login screen.
// Field order and types mirror Server/src/network/Packets/LoginFailedPacket.cpp exactly.
class LoginFailedPacket final : public Packet
{
public:
    std::string Reason;

    Opcode getOpcode() const override { return Opcode::LoginFailed; }

    const char* getName() const override { return "LoginFailedPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.writeString(Reason, ProtocolLimits::MaxReasonLength);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Reason = buffer.readString(ProtocolLimits::MaxReasonLength);
    }
};
