#pragma once

#include "Packet.h"


// A player left view or disconnected.
// Field order and types mirror Server/src/network/Packets/PlayerRemovePacket.cpp exactly.
class PlayerRemovePacket final : public Packet
{
public:
    uint64_t EntityID = 0;

    Opcode getOpcode() const override { return Opcode::PlayerRemove; }

    const char* getName() const override { return "PlayerRemovePacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(EntityID);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        EntityID = buffer.read<uint64_t>();
    }
};
