#pragma once

#include "Packet.h"

#include <cstdint>

// A tile changed for a reason that was not a player's own block edit.
//
// Server to client only. Block placement and breaking have their own packets
// and their own echoes; this carries everything else the world does on its own
// -- a planted seed becoming a sapling, a plant maturing into its block -- and
// anything a future system changes without a player having clicked it.
//
// This had no class and no handler on the client at all, so every one of those
// changes was received and dropped. Planting worked end to end on the server
// and was invisible on screen.
//
// Field order and types mirror
// Server/src/network/Packets/TileChangePacket.cpp exactly. Note TileID is a
// uint32 on the wire even though a tile id is one byte in storage.
class TileChangePacket final : public Packet
{
public:
    std::int32_t  X      = 0;
    std::int32_t  Y      = 0;
    std::int32_t  Z      = 0;   // layer; 0 is the foreground
    std::uint32_t TileID = 0;   // the tile the coordinate now holds

    Opcode getOpcode() const override { return Opcode::TileChange; }

    const char* getName() const override { return "TileChangePacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(X);
        buffer.write(Y);
        buffer.write(Z);
        buffer.write(TileID);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        X      = buffer.read<std::int32_t>();
        Y      = buffer.read<std::int32_t>();
        Z      = buffer.read<std::int32_t>();
        TileID = buffer.read<std::uint32_t>();
    }
};
