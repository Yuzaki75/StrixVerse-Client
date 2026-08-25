#pragma once

#include "Packet.h"
#include "Protocol.h"

#include <cstdint>
#include <string>
#include <vector>

// World management: the wrench panel's request and response set.
//
// Field order and types mirror
// Server/src/network/Packets/WorldManagePackets.cpp exactly. Four hand-mirrored
// layouts in this project have already drifted and been read at the wrong
// offset - silently, because a wrong offset still yields a number - so the two
// files are kept in the same order and read side by side when either changes.
//
// The client asks; it never decides. A role named here is a request, and the
// server re-checks who is asking and what they may do before any of it lands.

// Matches MaxWorldRosterEntries on the server. Both sides clamp, because a
// count read from the wire decides how many times a loop runs.
inline constexpr std::uint16_t kMaxWorldRosterEntries = 64;

// ---------------------------------------------------------------------------
// Client -> server
// ---------------------------------------------------------------------------

class InteractStrixCorePacket final : public Packet
{
public:
    static constexpr std::uint8_t FormatVersion = 1;

    // Mirrors the server's flag so the two files stay readable
    // side by side. The client never deserialises its own
    // requests, so nothing here reads it.
    bool Valid = false;

    std::int32_t X = 0;
    std::int32_t Y = 0;

    Opcode getOpcode() const override { return Opcode::InteractStrixCore; }
    const char* getName() const override { return "InteractStrixCorePacket"; }

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

        Valid = true;
    }
};

class InviteWorldMemberPacket final : public Packet
{
public:
    static constexpr std::uint8_t FormatVersion = 1;

    bool Valid = false;

    std::string  Username;
    std::uint8_t Role = 0;

    Opcode getOpcode() const override { return Opcode::InviteWorldMember; }
    const char* getName() const override { return "InviteWorldMemberPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);
        buffer.writeString(Username, ProtocolLimits::MaxUsernameLength);
        buffer.write(Role);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        if (buffer.read<std::uint8_t>() != FormatVersion)
            return;

        Username = buffer.readString(ProtocolLimits::MaxUsernameLength);
        Role     = buffer.read<std::uint8_t>();

        Valid = true;
    }
};

class RemoveWorldMemberPacket final : public Packet
{
public:
    static constexpr std::uint8_t FormatVersion = 1;

    bool Valid = false;

    std::string Username;

    Opcode getOpcode() const override { return Opcode::RemoveWorldMember; }
    const char* getName() const override { return "RemoveWorldMemberPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);
        buffer.writeString(Username, ProtocolLimits::MaxUsernameLength);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        if (buffer.read<std::uint8_t>() != FormatVersion)
            return;

        Username = buffer.readString(ProtocolLimits::MaxUsernameLength);

        Valid = true;
    }
};

class ChangeWorldRolePacket final : public Packet
{
public:
    static constexpr std::uint8_t FormatVersion = 1;

    bool Valid = false;

    std::string  Username;
    std::uint8_t Role = 0;

    Opcode getOpcode() const override { return Opcode::ChangeWorldRole; }
    const char* getName() const override { return "ChangeWorldRolePacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);
        buffer.writeString(Username, ProtocolLimits::MaxUsernameLength);
        buffer.write(Role);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        if (buffer.read<std::uint8_t>() != FormatVersion)
            return;

        Username = buffer.readString(ProtocolLimits::MaxUsernameLength);
        Role     = buffer.read<std::uint8_t>();

        Valid = true;
    }
};

class SetWorldSettingsPacket final : public Packet
{
public:
    static constexpr std::uint8_t FormatVersion = 1;

    bool Valid = false;

    std::uint8_t ProtectionOn  = 0;
    std::uint8_t AllowBuilding = 0;
    std::uint8_t AllowBreaking = 0;
    std::uint8_t AllowVisitors = 0;

    Opcode getOpcode() const override { return Opcode::SetWorldSettings; }
    const char* getName() const override { return "SetWorldSettingsPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);
        buffer.write(ProtectionOn);
        buffer.write(AllowBuilding);
        buffer.write(AllowBreaking);
        buffer.write(AllowVisitors);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        if (buffer.read<std::uint8_t>() != FormatVersion)
            return;

        ProtectionOn  = buffer.read<std::uint8_t>();
        AllowBuilding = buffer.read<std::uint8_t>();
        AllowBreaking = buffer.read<std::uint8_t>();
        AllowVisitors = buffer.read<std::uint8_t>();

        Valid = true;
    }
};

class BanWorldPlayerPacket final : public Packet
{
public:
    static constexpr std::uint8_t FormatVersion = 1;

    bool Valid = false;

    std::string  Username;
    std::uint8_t Banned = 1;     // 0 lifts the ban
    std::string  Reason;

    // Optional tail: how long the ban lasts, in seconds. 0 = permanent.
    // Mirrors the server's tolerant tail - an older server reads its absence
    // as permanent, which is what an omitted duration means anyway.
    std::uint32_t DurationSeconds = 0;

    Opcode getOpcode() const override { return Opcode::BanWorldPlayer; }
    const char* getName() const override { return "BanWorldPlayerPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);
        buffer.writeString(Username, ProtocolLimits::MaxUsernameLength);
        buffer.write(Banned);
        buffer.writeString(Reason, ProtocolLimits::MaxChatMessageLength);
        buffer.write(DurationSeconds);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        if (buffer.read<std::uint8_t>() != FormatVersion)
            return;

        Username = buffer.readString(ProtocolLimits::MaxUsernameLength);
        Banned   = buffer.read<std::uint8_t>();
        Reason   = buffer.readString(ProtocolLimits::MaxChatMessageLength);

        if (buffer.remaining() >= sizeof(std::uint32_t))
            DurationSeconds = buffer.read<std::uint32_t>();

        Valid = true;
    }
};

// ---------------------------------------------------------------------------
// Server -> client
// ---------------------------------------------------------------------------

// The world, and what this client may do in it.
//
// The Viewer* fields are statements about the reader, filled per recipient.
// They are what lets the panel show only the controls this player may use
// without the client ever deciding anything - it is being told.
class WorldInfoPacket final : public Packet
{
public:
    static constexpr std::uint8_t FormatVersion = 1;

    bool Valid = false;

    std::string WorldName;
    std::string OwnerName;       // empty when the world is unclaimed

    std::uint8_t  CoreLevel     = 0;
    std::int32_t  CoreX         = 0;
    std::int32_t  CoreY         = 0;

    std::uint8_t  ProtectionOn  = 0;
    std::uint8_t  AllowBuilding = 0;
    std::uint8_t  AllowBreaking = 0;
    std::uint8_t  AllowVisitors = 0;

    std::uint16_t MemberCount   = 0;

    std::uint8_t  ViewerRole      = 0;
    std::uint8_t  ViewerCanManage = 0;
    std::uint8_t  ViewerIsOwner   = 0;

    Opcode getOpcode() const override { return Opcode::WorldInfo; }
    const char* getName() const override { return "WorldInfoPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);
        buffer.writeString(WorldName, ProtocolLimits::MaxWorldNameLength);
        buffer.writeString(OwnerName, ProtocolLimits::MaxUsernameLength);
        buffer.write(CoreLevel);
        buffer.write(CoreX);
        buffer.write(CoreY);
        buffer.write(ProtectionOn);
        buffer.write(AllowBuilding);
        buffer.write(AllowBreaking);
        buffer.write(AllowVisitors);
        buffer.write(MemberCount);
        buffer.write(ViewerRole);
        buffer.write(ViewerCanManage);
        buffer.write(ViewerIsOwner);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Valid = false;

        if (buffer.read<std::uint8_t>() != FormatVersion)
            return;

        WorldName       = buffer.readString(ProtocolLimits::MaxWorldNameLength);
        OwnerName       = buffer.readString(ProtocolLimits::MaxUsernameLength);
        CoreLevel       = buffer.read<std::uint8_t>();
        CoreX           = buffer.read<std::int32_t>();
        CoreY           = buffer.read<std::int32_t>();
        ProtectionOn    = buffer.read<std::uint8_t>();
        AllowBuilding   = buffer.read<std::uint8_t>();
        AllowBreaking   = buffer.read<std::uint8_t>();
        AllowVisitors   = buffer.read<std::uint8_t>();
        MemberCount     = buffer.read<std::uint16_t>();
        ViewerRole      = buffer.read<std::uint8_t>();
        ViewerCanManage = buffer.read<std::uint8_t>();
        ViewerIsOwner   = buffer.read<std::uint8_t>();

        Valid = true;
    }
};

// Who belongs to the world, and who is barred from it.
//
// Both lists ride together because the panel refreshes them on the same events;
// sent separately, the two tabs could disagree for a frame. A player whose role
// may not see the roster is sent nothing at all rather than an empty list.
class WorldMembersPacket final : public Packet
{
public:
    static constexpr std::uint8_t FormatVersion = 1;

    bool Valid = false;

    struct Entry
    {
        std::string  Username;
        std::uint8_t Role = 0;       // members only; unused for bans
    };

    std::string        WorldName;
    std::vector<Entry> Members;
    std::vector<Entry> Bans;

    Opcode getOpcode() const override { return Opcode::WorldMembers; }
    const char* getName() const override { return "WorldMembersPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);
        buffer.writeString(WorldName, ProtocolLimits::MaxWorldNameLength);
        writeRoster(buffer, Members);
        writeRoster(buffer, Bans);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Valid = false;

        if (buffer.read<std::uint8_t>() != FormatVersion)
            return;

        WorldName = buffer.readString(ProtocolLimits::MaxWorldNameLength);
        readRoster(buffer, Members);
        readRoster(buffer, Bans);

        Valid = true;
    }

private:
    static void writeRoster(PacketBuffer& buffer, const std::vector<Entry>& entries)
    {
        const std::uint16_t count =
            entries.size() > kMaxWorldRosterEntries
                ? kMaxWorldRosterEntries
                : static_cast<std::uint16_t>(entries.size());

        buffer.write(count);

        for (std::uint16_t i = 0; i < count; ++i)
        {
            buffer.writeString(entries[i].Username, ProtocolLimits::MaxUsernameLength);
            buffer.write(entries[i].Role);
        }
    }

    static void readRoster(PacketBuffer& buffer, std::vector<Entry>& entries)
    {
        std::uint16_t count = buffer.read<std::uint16_t>();

        // The count comes off the wire and decides how many times this loop
        // runs, so it is clamped before it is used and never reserved on raw.
        if (count > kMaxWorldRosterEntries)
            count = kMaxWorldRosterEntries;

        entries.clear();
        entries.reserve(count);

        for (std::uint16_t i = 0; i < count; ++i)
        {
            Entry entry;
            entry.Username = buffer.readString(ProtocolLimits::MaxUsernameLength);
            entry.Role     = buffer.read<std::uint8_t>();
            entries.push_back(std::move(entry));
        }
    }
};

// Server -> client: a line the server wrote, not a player.
//
// Every management reply used to ride ChatMessagePacket with SenderID left at
// zero, which the client rendered as "Player 0: ...". Giving the server its own
// channel is what lets the chat log tell a refusal from something a player
// said, and lets a severity pick the colour.
//
// Severity matches the notification stack the HUD already keeps:
//   0 information, 1 warning, 2 success, 3 error.
class WorldNotificationPacket final : public Packet
{
public:
    static constexpr std::uint8_t FormatVersion = 1;

    bool Valid = false;

    std::uint8_t Severity = 0;
    std::string  Message;

    Opcode getOpcode() const override { return Opcode::WorldNotification; }
    const char* getName() const override { return "WorldNotificationPacket"; }

    void serialize(PacketBuffer& buffer) const override
    {
        buffer.write(FormatVersion);
        buffer.write(Severity);
        buffer.writeString(Message, ProtocolLimits::MaxChatMessageLength);
    }

    void deserialize(PacketBuffer& buffer) override
    {
        Valid = false;

        if (buffer.read<std::uint8_t>() != FormatVersion)
            return;

        Severity = buffer.read<std::uint8_t>();
        Message  = buffer.readString(ProtocolLimits::MaxChatMessageLength);

        Valid = true;
    }
};
