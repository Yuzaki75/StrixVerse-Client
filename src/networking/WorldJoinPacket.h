#pragma once

#include "Packet.h"

#include <string>

// Request to enter a world. The server ignores the client-supplied PlayerID
// and spawn point and uses the session's own values.
// Field order and types mirror Server/src/network/Packets/WorldJoinPacket.cpp exactly.
class WorldJoinPacket final : public Packet
{
public:
    uint64_t PlayerID = 0;
    std::string WorldName;
    float SpawnX = 0.0f;
    float SpawnY = 0.0f;

    Opcode getOpcode() const override { return Opcode::WorldJoin; }

    const char* getName() const override { return "WorldJoinPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(PlayerID);
        buffer.writeString(WorldName, ProtocolLimits::MaxWorldNameLength);
        buffer.write(SpawnX);
        buffer.write(SpawnY);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        PlayerID = buffer.read<uint64_t>();
        WorldName = buffer.readString(ProtocolLimits::MaxWorldNameLength);
        SpawnX = buffer.read<float>();
        SpawnY = buffer.read<float>();
    }
};
