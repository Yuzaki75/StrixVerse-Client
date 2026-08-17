#pragma once

#include "Packet.h"

#include <cstdint>

// Sent when the player places a block, and sent back by the server to every
// client once the placement is accepted.
//
// As with BlockBreakPacket the server is the authority -- it verifies the
// player actually holds the item and consumes it -- so the client sends and
// waits for the broadcast rather than placing optimistically.
//
// Field order and types mirror
// Server/src/network/Packets/BlockPlacePacket.cpp exactly.
class BlockPlacePacket final : public Packet
{
public:
    std::int32_t  X      = 0;
    std::int32_t  Y      = 0;
    std::int32_t  Z      = 0;   // layer; 0 is the foreground
    std::uint16_t ItemID = 0;   // the item being placed, verified server-side
    std::uint8_t  Face   = 0;   // which face was clicked; unused by this client

    Opcode getOpcode() const override { return Opcode::BlockPlace; }

    const char* getName() const override { return "BlockPlacePacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(X);
        buffer.write(Y);
        buffer.write(Z);
        buffer.write(ItemID);
        buffer.write(Face);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        X      = buffer.read<std::int32_t>();
        Y      = buffer.read<std::int32_t>();
        Z      = buffer.read<std::int32_t>();
        ItemID = buffer.read<std::uint16_t>();
        Face   = buffer.read<std::uint8_t>();
    }
};
