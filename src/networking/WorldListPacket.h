#pragma once

#include "Packet.h"
#include "Protocol.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

// The catalogue of worlds the server has.
//
// Field order and types mirror Server/src/network/Packets/WorldListPacket.cpp
// exactly. WorldBrowserScreen reads its list from WorldManager, which had a
// setter and no source: every session showed "No world list from the server"
// and the screen worked only as a box to type a name into.
class WorldListPacket final : public Packet
{
public:
    // Must equal WorldListPacket::FormatVersion on the server.
    static constexpr uint8_t  FormatVersion = 1;
    static constexpr uint16_t MaxWorlds     = 256;

    // False until a packet of a version this build understands has been read.
    bool Valid = false;

    struct Entry
    {
        std::string Name;
        uint16_t    Players = 0;
    };

    std::vector<Entry> Worlds;

    Opcode getOpcode() const override { return Opcode::WorldList; }

    const char* getName() const override { return "WorldListPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);

        const uint16_t count =
            static_cast<uint16_t>(std::min<std::size_t>(Worlds.size(), MaxWorlds));
        buffer.write(count);

        for (uint16_t i = 0; i < count; ++i)
        {
            buffer.writeString(Worlds[i].Name, ProtocolLimits::MaxWorldNameLength);
            buffer.write(Worlds[i].Players);
        }
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Valid = false;
        Worlds.clear();

        const uint8_t version = buffer.read<uint8_t>();
        if (version != FormatVersion)
        {
            return;
        }

        const uint16_t count = buffer.read<uint16_t>();

        // Clamp before reserving. The count is the peer's claim, and reserving
        // on it unchecked turns a six-byte packet into a large allocation.
        const uint16_t safeCount = std::min<uint16_t>(count, MaxWorlds);
        Worlds.reserve(safeCount);

        for (uint16_t i = 0; i < safeCount; ++i)
        {
            Entry entry;
            entry.Name    = buffer.readString(ProtocolLimits::MaxWorldNameLength);
            entry.Players = buffer.read<uint16_t>();
            Worlds.push_back(std::move(entry));
        }

        Valid = true;
    }
};

// Client -> server: "what worlds have you got?". No body; the opcode is the
// whole message.
class WorldListRequestPacket final : public Packet
{
public:
    Opcode getOpcode() const override { return Opcode::WorldListRequest; }

    const char* getName() const override { return "WorldListRequestPacket"; }

    void serialize(PacketBuffer&) const override {}
    void deserialize(PacketBuffer&) override {}
};
