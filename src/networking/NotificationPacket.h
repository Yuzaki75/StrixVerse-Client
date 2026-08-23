#pragma once

#include "Packet.h"
#include "Protocol.h"

#include <cstdint>
#include <string>

// A server-pushed HUD notification for world-management events: the world was
// saved, protection was toggled, and so on. Routed to the notification stack,
// never into the chat log.
//
// Severity: 0 info, 1 warn, 2 success. The client styles the notice by it.
//
// Field order and types mirror Server/src/network/Packets/NotificationPacket.cpp
// exactly.
class NotificationPacket final : public Packet
{
public:
    std::string  Message;
    std::uint8_t Severity = 0;   // 0 info, 1 warn, 2 success

    Opcode getOpcode() const override { return Opcode::Notification; }

    const char* getName() const override { return "NotificationPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.writeString(Message, ProtocolLimits::MaxChatMessageLength);
        buffer.write(Severity);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Message  = buffer.readString(ProtocolLimits::MaxChatMessageLength);
        Severity = buffer.read<std::uint8_t>();
    }
};
