#pragma once

#include "Packet.h"
#include "Protocol.h"

#include <cstdint>
#include <string>

// Client -> server: "claim the Strix Core at this tile".
//
// Field order and types mirror
// Server/src/network/Packets/StrixCorePacket.cpp exactly.
class ClaimStrixCorePacket final : public Packet
{
public:
    static constexpr std::uint8_t FormatVersion = 1;

    std::int32_t X = 0;
    std::int32_t Y = 0;

    Opcode getOpcode() const override { return Opcode::ClaimStrixCore; }
    const char* getName() const override { return "ClaimStrixCorePacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);
        buffer.write(X);
        buffer.write(Y);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        if (buffer.read<std::uint8_t>() != FormatVersion)
            return;

        X = buffer.read<std::int32_t>();
        Y = buffer.read<std::int32_t>();
    }
};

// Server -> client: what the Core is now.
//
// ViewerIsOwner is a statement about the reader, not about the Core: the server
// fills it per recipient. The client is told what it may do rather than working
// it out, and every request is re-checked on arrival anyway.
class StrixCoreUpdatedPacket final : public Packet
{
public:
    static constexpr std::uint8_t FormatVersion = 1;

    bool Valid = false;

    std::int32_t  X = 0;
    std::int32_t  Y = 0;
    std::uint16_t TileID = 0;
    std::uint8_t  CoreLevel = 0;
    std::uint8_t  ProtectionOn = 0;
    std::uint8_t  ViewerIsOwner = 0;
    std::string   WorldName;
    std::string   OwnerName;

    Opcode getOpcode() const override { return Opcode::StrixCoreUpdated; }
    const char* getName() const override { return "StrixCoreUpdatedPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);
        buffer.write(X);
        buffer.write(Y);
        buffer.write(TileID);
        buffer.write(CoreLevel);
        buffer.write(ProtectionOn);
        buffer.write(ViewerIsOwner);
        buffer.writeString(WorldName, ProtocolLimits::MaxWorldNameLength);
        buffer.writeString(OwnerName, ProtocolLimits::MaxUsernameLength);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Valid = false;

        if (buffer.read<std::uint8_t>() != FormatVersion)
            return;

        X             = buffer.read<std::int32_t>();
        Y             = buffer.read<std::int32_t>();
        TileID        = buffer.read<std::uint16_t>();
        CoreLevel     = buffer.read<std::uint8_t>();
        ProtectionOn  = buffer.read<std::uint8_t>();
        ViewerIsOwner = buffer.read<std::uint8_t>();
        WorldName     = buffer.readString(ProtocolLimits::MaxWorldNameLength);
        OwnerName     = buffer.readString(ProtocolLimits::MaxUsernameLength);

        Valid = true;
    }
};
