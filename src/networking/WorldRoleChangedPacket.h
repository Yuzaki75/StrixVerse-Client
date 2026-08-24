#pragma once

#include "Packet.h"
#include "Protocol.h"

#include <cstdint>
#include <string>

// Server -> client: someone's standing in this world changed.
//
// Field order and types mirror
// Server/src/network/packets/WorldManagePackets.h exactly (FormatVersion byte,
// EntityID, Username, Role, Pending). Broadcast to the whole world on invite
// accept, role change, removal and ban, so the player list can re-colour a
// name the moment the server decides it, not the next time a panel opens.
class WorldRoleChangedPacket final : public Packet
{
public:
    static constexpr std::uint8_t FormatVersion = 1;

    bool        Valid = false;
    std::uint64_t EntityID = 0;
    std::string Username;
    std::uint8_t Role = 0;      // World::WorldRole value; Visitor once removed
    std::uint8_t Pending = 0;   // 1 = an offer, not an active role

    Opcode getOpcode() const override { return Opcode::WorldRoleChanged; }

    const char* getName() const override { return "WorldRoleChangedPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);
        buffer.write(EntityID);
        buffer.writeString(Username, ProtocolLimits::MaxUsernameLength);
        buffer.write(Role);
        buffer.write(Pending);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Valid = false;

        if (buffer.read<std::uint8_t>() != FormatVersion)
            return;

        EntityID = buffer.read<std::uint64_t>();
        Username = buffer.readString(ProtocolLimits::MaxUsernameLength);
        Role     = buffer.read<std::uint8_t>();
        Pending  = buffer.read<std::uint8_t>();

        Valid = true;
    }
};
