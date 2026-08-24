#pragma once

#include "Packet.h"
#include "Protocol.h"

// A chat line, in both directions.
// Field order and types mirror Server/src/network/Packets/ChatMessagePacket.cpp
// exactly.
//
// Outbound, SenderID is ignored by the server: it attributes every message to
// the authenticated player on the connection, so a client cannot impersonate
// anyone by filling this in. Inbound, it identifies who actually said it.
class ChatMessagePacket final : public Packet
{
public:
    uint64_t    SenderID = 0;
    std::string Message;

    // Attribution tail, appended by the server after the message. The server
    // overwrites both with the authenticated sender's identity, so they are
    // authoritative on arrival and meaningless outbound.
    std::string SenderName;
    uint8_t     SenderRole = 0;   // World::WorldRole value

    Opcode getOpcode() const override { return Opcode::ChatMessage; }

    const char* getName() const override { return "ChatMessagePacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(SenderID);
        buffer.writeString(Message, ProtocolLimits::MaxChatMessageLength);
        buffer.writeString(SenderName, ProtocolLimits::MaxUsernameLength);
        buffer.write(SenderRole);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        SenderID = buffer.read<uint64_t>();
        Message  = buffer.readString(ProtocolLimits::MaxChatMessageLength);

        // Tolerant tail: absent on frames from servers built before chat
        // carried attribution. Mirrors the server's own tolerant read.
        if (buffer.remaining() >= sizeof(uint8_t))
        {
            SenderName = buffer.readString(ProtocolLimits::MaxUsernameLength);
            SenderRole = buffer.read<uint8_t>();
        }
    }
};
