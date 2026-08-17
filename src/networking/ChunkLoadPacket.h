#pragma once

#include "Packet.h"

#include <cstdint>
#include <vector>

// One chunk of terrain, sent by the server on world join and in reply to a
// ChunkRequest. Inbound only -- the client never sends this.
//
// Field order and types mirror
// Server/src/network/Packets/ChunkLoadPacket.cpp exactly:
//   int32  ChunkX
//   int32  ChunkY
//   uint32 tile count
//   uint8  tile ids, row-major, count of them
//
// Tiles carry the FOREGROUND id only. Chunks are 16x16 on both sides, so a
// well-formed packet always carries exactly 256 ids; anything else is a
// protocol error rather than a partial chunk.
class ChunkLoadPacket final : public Packet
{
public:
    static constexpr std::size_t kChunkWidth  = 16;
    static constexpr std::size_t kChunkHeight = 16;
    static constexpr std::size_t kTileCount   = kChunkWidth * kChunkHeight;

    std::int32_t              ChunkX = 0;
    std::int32_t              ChunkY = 0;
    std::vector<std::uint8_t> Tiles;

    Opcode getOpcode() const override { return Opcode::ChunkLoad; }

    const char* getName() const override { return "ChunkLoadPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        // Present for symmetry with the base class. The client has no reason
        // to send terrain, and the server would reject it if it did.
        buffer.write(ChunkX);
        buffer.write(ChunkY);
        buffer.write(static_cast<std::uint32_t>(Tiles.size()));
        if (!Tiles.empty())
        {
            buffer.write(Tiles.data(), Tiles.size());
        }
    }

    void deserialize(PacketBuffer& buffer) override
    {
        ChunkX = buffer.read<std::int32_t>();
        ChunkY = buffer.read<std::int32_t>();

        const std::uint32_t declared = buffer.read<std::uint32_t>();

        // Validate before allocating. The count arrives from the network, so
        // it is checked against both the fixed chunk size and the bytes
        // actually left in the buffer -- otherwise a 12-byte packet could ask
        // for a multi-gigabyte allocation.
        if (declared > kTileCount || declared > buffer.remaining())
        {
            throw std::out_of_range("ChunkLoadPacket: implausible tile count");
        }

        Tiles.resize(declared);
        if (declared > 0)
        {
            buffer.read(Tiles.data(), declared);
        }
    }
};
