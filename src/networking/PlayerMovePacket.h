#pragma once

#include "Packet.h"


// Movement update for a player.
// Field order and types mirror Server/src/network/Packets/PlayerMovePacket.cpp exactly.
class PlayerMovePacket final : public Packet
{
public:
    uint64_t PlayerID = 0;
    float X = 0.0f;
    float Y = 0.0f;
    float Z = 0.0f;
    float VelocityX = 0.0f;
    float VelocityY = 0.0f;
    float VelocityZ = 0.0f;

    Opcode getOpcode() const override { return Opcode::PlayerMove; }

    const char* getName() const override { return "PlayerMovePacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(PlayerID);
        buffer.write(X);
        buffer.write(Y);
        buffer.write(Z);
        buffer.write(VelocityX);
        buffer.write(VelocityY);
        buffer.write(VelocityZ);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        PlayerID = buffer.read<uint64_t>();
        X = buffer.read<float>();
        Y = buffer.read<float>();
        Z = buffer.read<float>();
        VelocityX = buffer.read<float>();
        VelocityY = buffer.read<float>();
        VelocityZ = buffer.read<float>();
    }
};
