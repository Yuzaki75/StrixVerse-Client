#pragma once

#include "Packet.h"

#include <string>

// Server accepted the credentials. SessionToken must be echoed on reconnect.
// Field order and types mirror Server/src/network/Packets/LoginSuccessPacket.cpp exactly.
class LoginSuccessPacket final : public Packet
{
public:
    uint64_t PlayerID = 0;
    std::string Username;
    std::string SessionToken;

    Opcode getOpcode() const override { return Opcode::LoginSuccess; }

    const char* getName() const override { return "LoginSuccessPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(PlayerID);
        buffer.writeString(Username, ProtocolLimits::MaxUsernameLength);
        buffer.writeString(SessionToken, ProtocolLimits::MaxStringLength);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        PlayerID = buffer.read<uint64_t>();
        Username = buffer.readString(ProtocolLimits::MaxUsernameLength);
        SessionToken = buffer.readString(ProtocolLimits::MaxStringLength);
    }
};
