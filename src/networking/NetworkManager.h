#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "Connection.h"
#include "KeepAlive.h"
#include "PacketDispatcher.h"
#include "PingManager.h"
#include "Protocol.h"
#include "../core/WorldManager.h"

// -----------------------------------------------------------------------------
// NetworkManager
//
// Owns the server session: the TCP connection, the packet dispatcher, and the
// keep-alive and ping timers.
//
// It also holds the authenticated session state (player id, username, session
// token) that the server hands back in LoginSuccess, because every screen after
// login needs it and it must survive a screen change.
// -----------------------------------------------------------------------------
class NetworkManager
{
public:
    NetworkManager();
    ~NetworkManager();

    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    // Starts Winsock and registers the packet types. Does not connect.
    bool initialize();

    // Opens a session and sends the handshake. Blocking, with a short timeout.
    bool connect(const std::string& host, uint16_t port);

    // Non-blocking connect. beginConnect() returns immediately; pollConnect()
    // is called once a frame until it stops reporting Pending. The handshake
    // is sent automatically on success, so a Connected result means the
    // session is ready to use.
    using ConnectProgress = Connection::ConnectProgress;

    bool            beginConnect(const std::string& host, uint16_t port);
    ConnectProgress pollConnect();

    void disconnect();

    bool isConnected() const;

    // Description of the last connection or session failure; empty if none.
    std::string getLastError() const;

    // Host and port of the most recent connect attempt, for status text.
    const std::string& getHost() const { return m_host; }
    uint16_t getPort() const { return m_port; }

    bool sendPacket(const std::shared_ptr<Packet>& packet);

    // --- Requests --------------------------------------------------------
    bool sendHandshake();
    bool sendLogin(const std::string& username, const std::string& password);
    bool sendRegister(const std::string& username,
                      const std::string& email,
                      const std::string& password);
    bool sendWorldJoin(const std::string& worldName);

    // Asks the server what worlds it has. The reply populates WorldManager,
    // which the world browser reads - it had a setter and no source, so the
    // browser could only ever show its empty state.
    bool sendWorldListRequest();

    // Leaves the current world. The session stays open, so the player returns
    // to World Selection rather than being disconnected.
    bool sendWorldLeave();

    // Sends a chat line. The server attributes it to this connection's
    // authenticated player, so no sender id is passed. Returns false if the
    // message is empty or the session is not usable.
    bool sendChat(const std::string& message);

    // Reports the local player's position. Coordinates are in TILES, not
    // pixels: the server stores positions as integer tile coordinates and
    // validates them against the world's tile bounds and its speed limit.
    bool sendPlayerMove(float tileX, float tileY,
                        float velocityX = 0.0f, float velocityY = 0.0f);

    // --- Handlers --------------------------------------------------------
    void addPacketHandler(Opcode opcode, const std::shared_ptr<PacketHandler>& handler);
    void removePacketHandler(Opcode opcode, const std::shared_ptr<PacketHandler>& handler);

    // Pumps received packets and the keep-alive/ping timers. Call once a frame.
    void update(float deltaTime);

    // --- Session state ----------------------------------------------------
    bool isAuthenticated() const { return m_authenticated; }

    // This client's own entity id, as the server assigned it. Zero until
    // LoginSuccess arrives.
    //
    // Every packet the server sends about a player is keyed on this, so it is
    // how the client tells its own from everyone else's. Before it was used,
    // that question was answered by "an id that is not in the roster must be
    // me" - which is true only for as long as the roster is complete and the
    // server's send rules never change.
    uint64_t getEntityId() const { return m_entityId; }
    bool isSelf(uint64_t id) const { return m_entityId != 0 && id == m_entityId; }
    const std::string& getUsername() const { return m_username; }
    const std::string& getSessionToken() const { return m_sessionToken; }

    // Called by the login flow once LoginSuccess arrives.
    void setSession(uint64_t entityId, const std::string& username, const std::string& token);
    void clearSession();

    // --- Other players ----------------------------------------------------
    // One player currently in the world, as last reported by the server.
    // Coordinates are in tiles, matching the wire format.
    // Six palette indices. Shared shape for the local player (from PlayerData)
    // and for everyone else (from PlayerSpawn), so one renderer draws both.
    struct Appearance
    {
        uint8_t hair     = 0;
        uint8_t skin     = 0;
        uint8_t eyes     = 0;
        uint8_t shirt    = 0;
        uint8_t trousers = 0;
        uint8_t boots    = 0;
    };

    struct RemotePlayer
    {
        uint64_t    id = 0;
        std::string username;
        float       tileX = 0.0f;
        float       tileY = 0.0f;
        Appearance  look{};
    };

    // The roster is kept here rather than on the gameplay screen because the
    // server sends PlayerSpawn as soon as the world is joined - while the
    // loading screen is still up, seconds before the gameplay screen exists.
    // Those packets used to be dispatched to nobody and lost.
    const std::unordered_map<uint64_t, RemotePlayer>& getRemotePlayers() const
    {
        return m_RemotePlayers;
    }

    // True when the id belongs to another player. A PlayerMove for anything
    // else is the server correcting our own position.
    bool isRemotePlayer(uint64_t id) const
    {
        return m_RemotePlayers.find(id) != m_RemotePlayers.end();
    }

    // --- Character stats --------------------------------------------------
    // The local player's own stats, as last reported. Everything here comes
    // from the server; the client never invents a value. `known` stays false
    // until PlayerData arrives, and the HUD leaves the fields blank until it
    // does rather than showing a made-up starting figure.
    struct CharacterStats
    {
        bool     known      = false;
        Appearance look{};
        int32_t  level      = 0;
        int32_t  experience = 0;
        uint32_t experienceToNextLevel = 0;
        uint32_t health     = 0;
        uint32_t maxHealth  = 0;

        // Where the server says this character is, in its tile coordinates.
        // `hasPosition` is separate from `known` so a spawn is only ever taken
        // from a packet that actually carried one - the client must never fall
        // back to 0,0, which is a real corner of the world and would drop the
        // player into bedrock.
        bool     hasPosition = false;
        float    tileX       = 0.0f;
        float    tileY       = 0.0f;
    };

    const CharacterStats& getCharacterStats() const { return m_Stats; }

    uint32_t getStatsRevision() const { return m_StatsRevision; }

    // --- Inventory --------------------------------------------------------
    struct InventorySlot
    {
        uint16_t itemId     = 0;   // Zero means the slot is empty.
        uint16_t quantity   = 0;
        uint16_t durability = 0;

        bool IsEmpty() const { return itemId == 0 || quantity == 0; }
    };

    // Asks the server for a full inventory sync. There is no outbound way to
    // change a slot: the server rejects client-authored InventoryUpdate as a
    // protocol violation, so every change here originates server-side.
    bool sendInventoryRequest();

    // Slots as last reported, indexed by slot number. Held here rather than on
    // a screen because the reply can arrive during a screen change.
    const std::unordered_map<uint8_t, InventorySlot>& getInventory() const
    {
        return m_Inventory;
    }

    // Bumped whenever the inventory changes, so a view can redraw only then.
    uint32_t getInventoryRevision() const { return m_InventoryRevision; }

    // --- World entry ------------------------------------------------------
    // True once the server has answered a world-join request with WorldState.
    // Tracked here rather than on the loading screen because the reply can
    // arrive during the screen transition, before that screen exists.
    bool isWorldConfirmed() const { return m_WorldConfirmed; }
    const std::string& getCurrentWorld() const { return m_CurrentWorld; }
    uint32_t getWorldTimeOfDay() const { return m_WorldTimeOfDay; }

    // --- Terrain ----------------------------------------------------------
    // One chunk of foreground tile ids, as delivered by the server.
    struct TerrainChunk
    {
        int32_t              chunkX = 0;
        int32_t              chunkY = 0;
        std::vector<uint8_t> tiles;   // 16*16, row-major
    };

    // Chunks received for the current world, keyed by packed chunk
    // coordinate. Held here for the same reason as the inventory and the
    // player roster: the server sends the whole world immediately after
    // WorldState, which is while the loading screen is up and seconds before
    // GameScreen exists to receive it.
    const std::unordered_map<uint64_t, TerrainChunk>& getTerrain() const
    {
        return m_Terrain;
    }

    // Bumped on every chunk received, so a consumer can tell whether the
    // world it built is still current.
    uint32_t getTerrainRevision() const { return m_TerrainRevision; }

    // Packs a chunk coordinate into the terrain map's key.
    static uint64_t terrainKey(int32_t chunkX, int32_t chunkY)
    {
        return (static_cast<uint64_t>(static_cast<uint32_t>(chunkX)) << 32) |
                static_cast<uint32_t>(chunkY);
    }

    // --- World edits ------------------------------------------------------
    // Asks the server to break or place a block. Neither applies anything
    // locally: the server validates reach, rate and inventory, then broadcasts
    // the result. Applying optimistically would let a rejected edit leave a
    // hole only this player can see.
    bool sendBlockBreak(int32_t tileX, int32_t tileY);
    bool sendBlockPlace(int32_t tileX, int32_t tileY, uint16_t itemId);

    // One accepted edit, as broadcast by the server.
    struct TileEdit
    {
        int32_t tileX  = 0;
        int32_t tileY  = 0;
        uint8_t tileId = 0;   // 0 = air, i.e. the block was broken
    };

    // Edits that have arrived but not yet been applied to the drawable world.
    // GameScreen drains this each frame; anything that arrives before it
    // exists simply waits here.
    std::vector<TileEdit> takePendingTileEdits()
    {
        std::vector<TileEdit> out;
        out.swap(m_PendingTileEdits);
        return out;
    }

    const NetworkStatistics& getStatistics() const;

    uint32_t getLastRoundTripTimeMs() const;
    uint32_t getAverageRoundTripTimeMs() const;

private:
    void onPacketReceived(const std::shared_ptr<Packet>& packet);

    std::unique_ptr<Connection>       m_connection;
    std::unique_ptr<PacketDispatcher> m_dispatcher;
    std::unique_ptr<KeepAlive>        m_keepAlive;
    std::unique_ptr<PingManager>      m_pingManager;

    std::string m_host;
    uint16_t    m_port = 0;

    bool        m_WorldConfirmed = false;
    std::string m_CurrentWorld;
    uint32_t    m_WorldTimeOfDay = 0;

    std::unordered_map<uint64_t, RemotePlayer> m_RemotePlayers;

    std::unordered_map<uint8_t, InventorySlot> m_Inventory;
    uint32_t                                   m_InventoryRevision = 0;

    std::unordered_map<uint64_t, TerrainChunk> m_Terrain;
    uint32_t                                   m_TerrainRevision = 0;
    std::vector<TileEdit>                      m_PendingTileEdits;

    // Applies an accepted edit to the stored chunk and queues it for the
    // world. Keeping the store in step means a later rebuild from m_Terrain
    // still reflects every edit.
    void recordTileEdit(int32_t tileX, int32_t tileY, uint8_t tileId);

    CharacterStats m_Stats;
    uint32_t       m_StatsRevision = 0;

    // PlayerData is sent unicast to a joining client to describe its own
    // character, and *also* broadcast to everyone else when a player changes
    // appearance. Nothing distinguished the two, so a client applied another
    // player's level, experience and health as its own the moment that player
    // restyled themselves. That filter now uses m_entityId below, which
    // LoginSuccess establishes before any of this arrives - one id, from the
    // earliest authoritative source, rather than a second one latched here.

    bool        m_authenticated = false;
    uint64_t    m_entityId      = 0;
    std::string m_username;
    std::string m_sessionToken;

    bool m_initialized = false;
};

#endif // NETWORK_MANAGER_H
