#pragma once

#include "Packet.h"
#include "Protocol.h"

#include <string>

// The local player's own character stats.
//
// Field order and types mirror
// Server/src/network/Packets/CharacterDataPacket.cpp exactly. The server names
// the class CharacterDataPacket but sends it under the PlayerData opcode; the
// name here follows the opcode, which is what the wire actually identifies.
class PlayerDataPacket final : public Packet
{
public:
    // Must equal CharacterDataPacket::FormatVersion on the server. The byte
    // exists because this layout is maintained by hand in two repositories,
    // and the two have now drifted five times, each time read at the wrong
    // offsets without anything noticing - a wrong offset still yields a
    // number. An unknown version is refused rather than guessed at.
    //
    //   1 - through TileY
    //   2 - same as 1, plus Gems
    //
    // The most recent drift is what this constant being 1 while the server sent
    // 2 actually cost: every PlayerData was refused, so the client had no name,
    // no level, no health and - worse - no spawn position, fell back to the
    // middle of the world, and was then visibly yanked back by the server's
    // position correction on every single login.
    static constexpr uint8_t FormatVersion = 2;

    // True once a packet of a version this client understands has been read.
    // Nothing downstream should trust the fields until it is.
    bool Valid = false;

    uint64_t    CharacterID = 0;
    std::string Name;

    // Appearance is six palette indices, one byte each, written raw by the
    // server (Entity::Appearance in CharacterDataPacket::Serialize).
    //
    // This was declared as a length-prefixed std::string, so the client
    // consumed a bogus 4-byte length from the six colour bytes and every field
    // after it was read at the wrong offset -- which is why the HUD showed
    // "LVL 65536" and "HP 6553600" instead of level 1 and 100 health.
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

    int32_t     Level      = 1;
    int32_t     Experience = 0;
    uint32_t    Health     = 0;
    uint32_t    MaxHealth  = 0;
    uint32_t    ExperienceToNextLevel = 0;

    // Where the server says this character is, in its own tile coordinates
    // (Y up). This is the authoritative spawn: before it existed the client
    // invented one at the middle of the world and searched outward for a gap
    // it fit in, which is how players ended up wedged between a tree and a
    // cliff instead of standing where they logged out.
    int32_t     TileX = 0;
    int32_t     TileY = 0;

    // Added in format 2: the recipient's own wallet, server-authoritative like
    // everything else here. Zero when talking to a format-1 server.
    uint32_t    Gems = 0;

    Opcode getOpcode() const override { return Opcode::PlayerData; }

    const char* getName() const override { return "PlayerDataPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);
        buffer.write(CharacterID);
        buffer.writeString(Name, ProtocolLimits::MaxUsernameLength);
        buffer.write(Appearance.hair);
        buffer.write(Appearance.skin);
        buffer.write(Appearance.eyes);
        buffer.write(Appearance.shirt);
        buffer.write(Appearance.trousers);
        buffer.write(Appearance.boots);
        buffer.write(Level);
        buffer.write(Experience);
        buffer.write(Health);
        buffer.write(MaxHealth);
        buffer.write(ExperienceToNextLevel);
        buffer.write(TileX);
        buffer.write(TileY);
        buffer.write(Gems);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Valid = false;

        // Both known versions are accepted, exactly as the server accepts them,
        // so a client and server one revision apart still agree on everything
        // up to the point where they diverge. Anything else stops rather than
        // reading the rest at offsets that no longer mean what this build
        // thinks they mean: every field keeps its default and `Valid` stays
        // false, so a caller that ignores this cannot silently act on rubbish.
        const uint8_t version = buffer.read<uint8_t>();
        if (version != 1 && version != FormatVersion)
        {
            return;
        }

        CharacterID = buffer.read<uint64_t>();
        Name        = buffer.readString(ProtocolLimits::MaxUsernameLength);
        Appearance.hair     = buffer.read<uint8_t>();
        Appearance.skin     = buffer.read<uint8_t>();
        Appearance.eyes     = buffer.read<uint8_t>();
        Appearance.shirt    = buffer.read<uint8_t>();
        Appearance.trousers = buffer.read<uint8_t>();
        Appearance.boots    = buffer.read<uint8_t>();
        Level       = buffer.read<int32_t>();
        Experience  = buffer.read<int32_t>();
        Health      = buffer.read<uint32_t>();
        MaxHealth   = buffer.read<uint32_t>();
        ExperienceToNextLevel = buffer.read<uint32_t>();
        TileX = buffer.read<int32_t>();
        TileY = buffer.read<int32_t>();

        // Only format 2 carries it; a format-1 server stops after TileY and
        // reading further would run off the end of the payload.
        if (version >= 2)
        {
            Gems = buffer.read<uint32_t>();
        }

        Valid = true;
    }
};
