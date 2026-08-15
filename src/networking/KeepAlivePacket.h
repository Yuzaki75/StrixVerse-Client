#pragma once

#include "Packet.h"


// Idle-timeout heartbeat.
// Field order and types mirror Server/src/network/Packets/KeepAlivePacket.cpp exactly.
class KeepAlivePacket final : public Packet
{
public:
    uint64_t Timestamp = 0;

    Opcode getOpcode() const override { return Opcode::KeepAlive; }

    const char* getName() const override { return "KeepAlivePacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(Timestamp);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Timestamp = buffer.read<uint64_t>();
    }
};
