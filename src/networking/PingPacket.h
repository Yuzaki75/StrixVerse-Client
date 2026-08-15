#pragma once

#include "Packet.h"


// Latency probe; the peer echoes the timestamp back in a PongPacket.
// Field order and types mirror Server/src/network/Packets/PingPacket.cpp exactly.
class PingPacket final : public Packet
{
public:
    uint64_t Timestamp = 0;

    Opcode getOpcode() const override { return Opcode::Ping; }

    const char* getName() const override { return "PingPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(Timestamp);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Timestamp = buffer.read<uint64_t>();
    }
};
