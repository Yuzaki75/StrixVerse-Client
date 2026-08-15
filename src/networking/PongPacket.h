#pragma once

#include "Packet.h"


// Reply to a PingPacket, echoing the original timestamp.
// Field order and types mirror Server/src/network/Packets/PongPacket.cpp exactly.
class PongPacket final : public Packet
{
public:
    uint64_t Timestamp = 0;

    Opcode getOpcode() const override { return Opcode::Pong; }

    const char* getName() const override { return "PongPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(Timestamp);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Timestamp = buffer.read<uint64_t>();
    }
};
