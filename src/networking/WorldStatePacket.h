#pragma once

#include "Packet.h"

#include <string>

// The server's reply to a WorldJoinPacket: confirms which world the player is
// in and the current time of day.
// Field order and types mirror Server/src/network/Packets/WorldStatePacket.cpp exactly.
class WorldStatePacket final : public Packet
{
public:
    std::string WorldName;
    uint32_t    TimeOfDay  = 0;      // Server ticks.
    bool        IsDaytime  = true;

    Opcode getOpcode() const override { return Opcode::WorldState; }

    const char* getName() const override { return "WorldStatePacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.writeString(WorldName, ProtocolLimits::MaxWorldNameLength);
        buffer.write(TimeOfDay);
        buffer.write(IsDaytime);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        WorldName = buffer.readString(ProtocolLimits::MaxWorldNameLength);
        TimeOfDay = buffer.read<uint32_t>();
        IsDaytime = buffer.read<bool>();
    }
};
