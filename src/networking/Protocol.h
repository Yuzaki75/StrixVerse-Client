#pragma once

#include <cstddef>
#include <cstdint>

// -----------------------------------------------------------------------------
// StrixVerse wire protocol
//
// This is the client's copy of the contract the server defines in
// Server/src/network/Packet/Opcode.h and PacketLimits.h. The values here must
// stay identical to the server's; they are the reason a client build can talk
// to a server build at all.
//
// Frame layout:
//
//     [ opcode : uint16, big-endian ]
//     [ length : uint32, big-endian ]  -- payload byte count, excluding header
//     [ payload : length bytes       ]
//
// The header is in network byte order (the server uses htons/htonl), while the
// payload body is written in native little-endian order by PacketBuffer, and
// strings are a uint32 little-endian length followed by the raw characters.
// That split is deliberate on the server side, so the client mirrors it exactly
// rather than "tidying" one half and desynchronising the two.
// -----------------------------------------------------------------------------
enum class Opcode : uint16_t
{
    None = 0,

    // Connection
    Handshake = 1,
    Ping,
    Pong,
    Disconnect,

    // Authentication
    Login = 10,
    LoginSuccess,
    LoginFailed,
    Logout,
    Register = 15,

    // World synchronisation
    WorldJoin = 20,
    WorldLeave,
    WorldState,

    // World catalogue. WorldBrowser could never list anything: it read a
    // catalogue no packet ever filled, so its empty state ("No world list from
    // the server") was the only state it had, and the screen worked solely as
    // a text box you typed a name into.
    WorldListRequest = 23,   // client -> server, no body
    WorldList        = 24,   // server -> client, the worlds it has
    EntityUpdate = 25,
    EntitySpawn  = 26,
    EntityRemove = 27,

    // Player
    PlayerSpawn = 30,
    PlayerMove,
    PlayerPosition,
    PlayerDirection,
    PlayerAnimation,
    PlayerRemove,
    PlayerData = 40,

    // Chunk system
    ChunkLoad = 50,
    ChunkUnload,
    ChunkRequest = 52,

    // Inventory system
    Inventory = 60,
    InventoryUpdate,
    ItemPickup,
    ItemDrop,
    Equipment,
    UseItem,

    // World
    TileChange = 70,
    TileUpdate,
    BlockPlace,
    BlockBreak,
    ChunkData,
    WorldData,

    // Chat
    ChatMessage = 80,

    // Keep-alive
    KeepAlive = 90
};

namespace ProtocolLimits
{
    // Mirrors Server/src/network/Packet/PacketLimits.h.
    inline constexpr std::size_t HeaderSize = sizeof(uint16_t) + sizeof(uint32_t);

    inline constexpr std::size_t MaxPayloadSize       = 64 * 1024;
    inline constexpr std::size_t MaxReceiveBufferSize = 256 * 1024;

    inline constexpr std::size_t MaxStringLength      = 4 * 1024;
    inline constexpr std::size_t MaxUsernameLength    = 32;
    inline constexpr std::size_t MaxEmailLength       = 254;
    inline constexpr std::size_t MaxPasswordLength    = 128;
    inline constexpr std::size_t MaxChatMessageLength = 256;
    inline constexpr std::size_t MaxWorldNameLength   = 64;
    inline constexpr std::size_t MaxReasonLength      = 256;
}
