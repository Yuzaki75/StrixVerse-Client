#pragma once

#include "Packet.h"
#include "Protocol.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

// Server -> client: every buff running on the receiving player.
//
// A complete replacement, not a delta: the whole set is sent whenever it
// changes, so a client that missed one packet is corrected by the next rather
// than drifting. The client counts the remaining seconds down between packets
// purely so the bar moves smoothly - it never decides that a buff has ended,
// and it never sends one.
//
// Field order and types mirror Server/src/network/Packets/PlayerBuffsPacket.cpp
// exactly.
class PlayerBuffsPacket final : public Packet
{
public:
    // Must equal PlayerBuffsPacket::FormatVersion on the server.
    static constexpr std::uint8_t FormatVersion = 1;
    static constexpr std::uint8_t MaxBuffs      = 32;

    bool Valid = false;

    struct Entry
    {
        std::string   Id;
        std::string   Name;
        std::uint32_t RemainingMs = 0;
        std::uint32_t TotalMs     = 0;
    };

    std::vector<Entry> Buffs;

    Opcode getOpcode() const override { return Opcode::PlayerBuffs; }

    const char* getName() const override { return "PlayerBuffsPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);

        const std::uint8_t count =
            static_cast<std::uint8_t>(std::min<std::size_t>(Buffs.size(), MaxBuffs));
        buffer.write(count);

        for (std::uint8_t i = 0; i < count; ++i)
        {
            buffer.writeString(Buffs[i].Id, ProtocolLimits::MaxUsernameLength);
            buffer.writeString(Buffs[i].Name, ProtocolLimits::MaxUsernameLength);
            buffer.write(Buffs[i].RemainingMs);
            buffer.write(Buffs[i].TotalMs);
        }
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Valid = false;
        Buffs.clear();

        if (buffer.read<std::uint8_t>() != FormatVersion)
            return;

        const std::uint8_t count = buffer.read<std::uint8_t>();

        // Clamp before reserving: the count is the peer's claim.
        const std::uint8_t safeCount = std::min<std::uint8_t>(count, MaxBuffs);
        Buffs.reserve(safeCount);

        for (std::uint8_t i = 0; i < safeCount; ++i)
        {
            Entry entry;
            entry.Id          = buffer.readString(ProtocolLimits::MaxUsernameLength);
            entry.Name        = buffer.readString(ProtocolLimits::MaxUsernameLength);
            entry.RemainingMs = buffer.read<std::uint32_t>();
            entry.TotalMs     = buffer.read<std::uint32_t>();
            Buffs.push_back(std::move(entry));
        }

        Valid = true;
    }
};
