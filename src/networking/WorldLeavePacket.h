#pragma once

#include "Packet.h"

// Tells the server the player is leaving the world.
// Mirrors Server/src/network/Packets/WorldLeavePacket.cpp, which carries no
// payload - the connection identifies the player.
class WorldLeavePacket final : public Packet
{
public:
    Opcode getOpcode() const override { return Opcode::WorldLeave; }

    const char* getName() const override { return "WorldLeavePacket"; }

    void serialize(PacketBuffer& buffer) const override { (void)buffer; }

    void deserialize(PacketBuffer& buffer) override { (void)buffer; }
};
