#pragma once

#include "Packet.h"

#include <string>

// Another player entered view.
// Field order and types mirror Server/src/network/Packets/PlayerSpawnPacket.cpp exactly.
class PlayerSpawnPacket final : public Packet
{
public:
    uint64_t EntityID = 0;
    std::string Username;
    float X = 0.0f;
    float Y = 0.0f;
    float Direction = 0.0f;

    Opcode getOpcode() const override { return Opcode::PlayerSpawn; }

    const char* getName() const override { return "PlayerSpawnPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(EntityID);
        buffer.writeString(Username, ProtocolLimits::MaxUsernameLength);
        buffer.write(X);
        buffer.write(Y);
        buffer.write(Direction);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        EntityID = buffer.read<uint64_t>();
        Username = buffer.readString(ProtocolLimits::MaxUsernameLength);
        X = buffer.read<float>();
        Y = buffer.read<float>();
        Direction = buffer.read<float>();
    }
};
