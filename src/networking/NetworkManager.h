#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
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

    // Asks the server to claim the Strix Core at this tile. The server decides
    // whether there is one there, whether it is reachable, and whether it is
    // still unclaimed.
    bool sendClaimStrixCore(int32_t tileX, int32_t tileY);

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

        // Standing in the current world, as the server resolved it at spawn.
        // Held here rather than looked up per frame because the roster is the
        // only place that wants it and the server is the only source of it.
        uint8_t     worldRole = 0;
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

    // --- Strix Core -------------------------------------------------------
    // The Core of the world this client is in, as last reported. `known` stays
    // false until the server says something about one, so nothing draws a Core
    // that may not exist.
    struct CoreState
    {
        bool        known         = false;
        int32_t     tileX         = 0;
        int32_t     tileY         = 0;
        int32_t     level         = 0;     // 0 unclaimed, 1..4 claimed
        bool        protectionOn  = false;
        bool        viewerIsOwner = false; // about this client, not the Core
        std::string ownerName;
    };

    const CoreState& getCoreState() const { return m_Core; }
    uint32_t getCoreRevision() const { return m_CoreRevision; }

    // --- World management -------------------------------------------------
    //
    // What the server says about the world this client is standing in, and
    // what this client may do in it. Every field is filled from WorldInfo; none
    // of it is worked out here. The panel reads these to decide which controls
    // to show, which is not the same as deciding whether they are allowed - the
    // server re-checks every request on arrival regardless of what was shown.
    struct WorldManageState
    {
        bool        valid         = false;   // false until the first WorldInfo
        std::string worldName;
        std::string ownerName;               // empty while unclaimed

        int32_t     coreLevel     = 0;
        int32_t     coreX         = 0;
        int32_t     coreY         = 0;

        bool        protectionOn  = false;
        bool        allowBuilding = false;
        bool        allowBreaking = false;
        bool        allowVisitors = false;

        uint16_t    memberCount   = 0;

        uint8_t     viewerRole    = 0;       // WorldRole, as the server resolved it
        bool        canManage     = false;
        bool        isOwner       = false;
    };

    struct WorldRosterEntry
    {
        std::string username;
        uint8_t     role = 0;                // members only; unused for bans
    };

    const WorldManageState& getWorldManageState() const { return m_WorldManage; }
    uint32_t getWorldInfoRevision() const { return m_WorldInfoRevision; }

    // Bumped when the server confirms the player has left a world.
    //
    // The server answers WorldLeave only on success and stays silent on
    // refusal -- it refuses when the player is not actually standing at the
    // door. So a screen watches this counter rather than assuming the press
    // worked: silence correctly means nothing happened.
    uint32_t getWorldLeftRevision() const { return m_WorldLeftRevision; }

    const std::vector<WorldRosterEntry>& getWorldMembers() const { return m_WorldMembers; }
    const std::vector<WorldRosterEntry>& getWorldBans() const { return m_WorldBans; }
    uint32_t getWorldMembersRevision() const { return m_WorldMembersRevision; }

    // Asks the server to open management for the Core at this tile. The server
    // decides whether there is a Core there, whether the player is close enough
    // and what they may see; the panel opens when WorldInfo comes back, never
    // on the click.
    bool sendInteractStrixCore(int32_t tileX, int32_t tileY);

    bool sendInviteWorldMember(const std::string& username, uint8_t role);
    bool sendRemoveWorldMember(const std::string& username);
    bool sendChangeWorldRole(const std::string& username, uint8_t role);
    bool sendSetWorldSettings(bool protectionOn, bool allowBuilding,
                              bool allowBreaking, bool allowVisitors);
    bool sendBanWorldPlayer(const std::string& username, bool banned,
                            const std::string& reason);

    // --- Server notifications --------------------------------------------
    //
    // Notification packets carry short world-management notices (world saved,
    // protection toggled, ...). They are routed to the registered handler, or
    // queued when none is set yet - the packet can arrive while the loading
    // screen is up, before the HUD that displays them exists. Nothing is lost
    // either way.
    void SetNotificationHandler(
        std::function<void(const std::string& message, int severity)> handler);

    // Takes one queued notification, oldest first. False when the queue is
    // empty; `out` and `severity` are then untouched.
    bool PopPendingNotification(std::string& out, int& severity);

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

    // --- Chunk loading progress -------------------------------------------
    // How many ChunkLoad packets have been fully applied for the current
    // world join, and how many the server announced. Nothing in the protocol
    // carries an expected total today, so the expected count stays zero,
    // which the loading screen treats as "unknown" and animates as an
    // indeterminate bar.
    uint32_t ChunksReceived() const { return m_ChunksReceived.load(std::memory_order_relaxed); }
    uint32_t ChunksExpected() const { return m_ChunksExpected.load(std::memory_order_relaxed); }

    // Clears both counters. Called when a world join begins, so the loading
    // screen measures this join only and never inherits the previous one.
    void ResetChunkProgress()
    {
        m_ChunksReceived.store(0, std::memory_order_relaxed);
        m_ChunksExpected.store(0, std::memory_order_relaxed);
    }

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
    // `toolItemId` is the selected hotbar item's id and feeds the server's
    // XP/durability handling; 0 (the default) means bare hands, so callers
    // that do not track tools stay valid.
    bool sendBlockBreak(int32_t tileX, int32_t tileY, uint16_t toolItemId = 0);
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

    // Delivers a notification to the handler, or queues it when there is none.
    void pushNotification(const std::string& message, int severity);

    std::unique_ptr<Connection>       m_connection;
    std::unique_ptr<PacketDispatcher> m_dispatcher;
    std::unique_ptr<KeepAlive>        m_keepAlive;
    std::unique_ptr<PingManager>      m_pingManager;

    std::function<void(const std::string& message, int severity)> m_NotificationHandler;

    struct PendingNotification
    {
        std::string message;
        int         severity = 0;
    };

    // Capped so a server that floods notifications cannot grow memory without
    // bound; the oldest drops when the cap is passed.
    static constexpr std::size_t kMaxPendingNotifications = 32;
    std::deque<PendingNotification> m_PendingNotifications;

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

    // Progress counters for the loading screen. Atomic because the protocol
    // may one day announce an expected total from a handler running off the
    // game thread; today both are touched only where packets are dispatched.
    std::atomic<uint32_t> m_ChunksReceived{0};
    std::atomic<uint32_t> m_ChunksExpected{0};

    // Applies an accepted edit to the stored chunk and queues it for the
    // world. Keeping the store in step means a later rebuild from m_Terrain
    // still reflects every edit.
    void recordTileEdit(int32_t tileX, int32_t tileY, uint8_t tileId);

    CoreState      m_Core;
    uint32_t       m_CoreRevision = 0;

    WorldManageState              m_WorldManage;
    uint32_t                      m_WorldInfoRevision = 0;
    uint32_t                      m_WorldLeftRevision = 0;

    std::vector<WorldRosterEntry> m_WorldMembers;
    std::vector<WorldRosterEntry> m_WorldBans;
    uint32_t                      m_WorldMembersRevision = 0;

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
