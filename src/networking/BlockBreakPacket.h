#pragma once

#include "Packet.h"

#include <cstdint>

// Sent when the player breaks a block, and sent back by the server to every
// client once the break is accepted.
//
// The server is the authority: it checks reach, rate and whether the tile can
// be broken, then broadcasts this packet on success. The client must not apply
// the edit when it sends -- it applies it when the broadcast arrives, so a
// rejected break never leaves a hole that only one player can see.
//
// Field order and types mirror
// Server/src/network/Packets/BlockBreakPacket.cpp exactly.
class BlockBreakPacket final : public Packet
{
public:
    std::int32_t  X      = 0;
    std::int32_t  Y      = 0;
    std::int32_t  Z      = 0;   // layer; 0 is the foreground
    std::uint16_t ToolID = 0;   // for XP and durability; 0 means bare hands
    std::uint8_t  Face   = 0;   // which face was clicked; unused by this client

    Opcode getOpcode() const override { return Opcode::BlockBreak; }

    const char* getName() const override { return "BlockBreakPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(X);
        buffer.write(Y);
        buffer.write(Z);
        buffer.write(ToolID);
        buffer.write(Face);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        X      = buffer.read<std::int32_t>();
        Y      = buffer.read<std::int32_t>();
        Z      = buffer.read<std::int32_t>();
        ToolID = buffer.read<std::uint16_t>();
        Face   = buffer.read<std::uint8_t>();
    }
};
