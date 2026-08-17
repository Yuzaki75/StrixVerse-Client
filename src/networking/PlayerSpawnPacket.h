#pragma once

#include "Packet.h"

#include <cstdint>
#include <string>

// Another player entered view.
// Field order and types mirror Server/src/network/Packets/PlayerSpawnPacket.cpp exactly.
class PlayerSpawnPacket final : public Packet
{
public:
    uint64_t EntityID = 0;
    std::string Username;
    float X = 0.0f;
    float Y = 0.0f;
    float Direction = 0.0f;

    // Six palette indices, one byte each, written raw by the server after
    // Direction -- the same Entity::Appearance that PlayerData carries for your
    // own character.
    //
    // These were not read at all. The bytes were on the wire and the client
    // stopped at Direction, so every remote player drew with the default look
    // while your own appearance worked, because that one arrives separately on
    // PlayerData. The result was a world where everybody else was a clone.
    struct AppearanceIndices
    {
        uint8_t hair     = 0;
        uint8_t skin     = 0;
        uint8_t eyes     = 0;
        uint8_t shirt    = 0;
        uint8_t trousers = 0;
        uint8_t boots    = 0;
    };

    AppearanceIndices Appearance{};

    Opcode getOpcode() const override { return Opcode::PlayerSpawn; }

    const char* getName() const override { return "PlayerSpawnPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(EntityID);
        buffer.writeString(Username, ProtocolLimits::MaxUsernameLength);
        buffer.write(X);
        buffer.write(Y);
        buffer.write(Direction);
        buffer.write(Appearance.hair);
        buffer.write(Appearance.skin);
        buffer.write(Appearance.eyes);
        buffer.write(Appearance.shirt);
        buffer.write(Appearance.trousers);
        buffer.write(Appearance.boots);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        EntityID = buffer.read<uint64_t>();
        Username = buffer.readString(ProtocolLimits::MaxUsernameLength);
        X = buffer.read<float>();
        Y = buffer.read<float>();
        Direction = buffer.read<float>();
        Appearance.hair     = buffer.read<uint8_t>();
        Appearance.skin     = buffer.read<uint8_t>();
        Appearance.eyes     = buffer.read<uint8_t>();
        Appearance.shirt    = buffer.read<uint8_t>();
        Appearance.trousers = buffer.read<uint8_t>();
        Appearance.boots    = buffer.read<uint8_t>();
    }
};
