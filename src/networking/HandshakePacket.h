#pragma once

#include "Packet.h"

#include <string>

// First frame the client sends; identifies the build to the server.
// Field order and types mirror Server/src/network/Packets/HandshakePacket.cpp exactly.
class HandshakePacket final : public Packet
{
public:
    std::string ClientVersion;
    std::string ClientPlatform;
    std::string ClientArchitecture;

    Opcode getOpcode() const override { return Opcode::Handshake; }

    const char* getName() const override { return "HandshakePacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.writeString(ClientVersion, ProtocolLimits::MaxStringLength);
        buffer.writeString(ClientPlatform, ProtocolLimits::MaxStringLength);
        buffer.writeString(ClientArchitecture, ProtocolLimits::MaxStringLength);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        ClientVersion = buffer.readString(ProtocolLimits::MaxStringLength);
        ClientPlatform = buffer.readString(ProtocolLimits::MaxStringLength);
        ClientArchitecture = buffer.readString(ProtocolLimits::MaxStringLength);
    }
};
