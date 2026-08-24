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
    //   1 - count, then per world: Name, Players
    //   2 - adds OwnerName and Flags per world
    static constexpr uint8_t  FormatVersion = 2;
    static constexpr uint16_t MaxWorlds     = 256;

    // False until a packet of a version this build understands has been read.
    bool Valid = false;

    // OwnerName is empty for an unclaimed world. Flags is a bitfield rather
    // than three bools because the set will grow - the world's own settings
    // already carry allow_building and allow_breaking, and those become
    // interesting to the browser the moment a player can be a guest.
    enum EntryFlags : uint8_t
    {
        FlagProtected     = 1 << 0,   // the Strix Core is enforcing permissions
        FlagAllowVisitors = 1 << 1,   // non-members may enter at all
    };

    struct Entry
    {
        std::string Name;
        uint16_t    Players = 0;
        std::string OwnerName;
        uint8_t     Flags = 0;

        bool IsClaimed() const { return !OwnerName.empty(); }
        bool IsProtected() const { return (Flags & FlagProtected) != 0; }
        bool AllowsVisitors() const { return (Flags & FlagAllowVisitors) != 0; }
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
            buffer.writeString(Worlds[i].OwnerName, ProtocolLimits::MaxUsernameLength);
            buffer.write(Worlds[i].Flags);
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
            entry.Name      = buffer.readString(ProtocolLimits::MaxWorldNameLength);
            entry.Players   = buffer.read<uint16_t>();
            entry.OwnerName = buffer.readString(ProtocolLimits::MaxUsernameLength);
            entry.Flags     = buffer.read<uint8_t>();
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
