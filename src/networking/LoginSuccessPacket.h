#pragma once

#include "Packet.h"

#include <string>

// Server accepted the credentials. SessionToken must be echoed on reconnect.
// Field order and types mirror Server/src/network/Packets/LoginSuccessPacket.cpp exactly.
class LoginSuccessPacket final : public Packet
{
public:
    // The player's runtime **entity** id, not their account id - the two are
    // separate spaces and were both called PlayerID on both sides. Every
    // packet about a player after login (spawn, move, remove, chat, character
    // data) is keyed on this one, so it is what lets this client tell its own
    // packets from everyone else's. Mirrors LoginSuccessPacket::EntityID on
    // the server; the wire layout is unchanged, only the name.
    uint64_t EntityID = 0;
    std::string Username;
    std::string SessionToken;

    Opcode getOpcode() const override { return Opcode::LoginSuccess; }

    const char* getName() const override { return "LoginSuccessPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(EntityID);
        buffer.writeString(Username, ProtocolLimits::MaxUsernameLength);
        buffer.writeString(SessionToken, ProtocolLimits::MaxStringLength);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        EntityID = buffer.read<uint64_t>();
        Username = buffer.readString(ProtocolLimits::MaxUsernameLength);
        SessionToken = buffer.readString(ProtocolLimits::MaxStringLength);
    }
};
