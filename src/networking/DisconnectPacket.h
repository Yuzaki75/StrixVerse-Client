#pragma once

#include "Packet.h"

#include <string>

// Sent by either side to close the session with an explanation.
// Field order and types mirror Server/src/network/Packets/DisconnectPacket.cpp exactly.
class DisconnectPacket final : public Packet
{
public:
    std::string Reason;

    Opcode getOpcode() const override { return Opcode::Disconnect; }

    const char* getName() const override { return "DisconnectPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.writeString(Reason, ProtocolLimits::MaxReasonLength);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Reason = buffer.readString(ProtocolLimits::MaxReasonLength);
    }
};
