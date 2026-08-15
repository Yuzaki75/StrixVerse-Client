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
    uint64_t getPlayerId() const { return m_playerId; }
    const std::string& getUsername() const { return m_username; }
    const std::string& getSessionToken() const { return m_sessionToken; }

    // Called by the login flow once LoginSuccess arrives.
    void setSession(uint64_t playerId, const std::string& username, const std::string& token);
    void clearSession();

    // --- Other players ----------------------------------------------------
    // One player currently in the world, as last reported by the server.
    // Coordinates are in tiles, matching the wire format.
    struct RemotePlayer
    {
        uint64_t    id = 0;
        std::string username;
        float       tileX = 0.0f;
        float       tileY = 0.0f;
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

    // --- World entry ------------------------------------------------------
    // True once the server has answered a world-join request with WorldState.
    // Tracked here rather than on the loading screen because the reply can
    // arrive during the screen transition, before that screen exists.
    bool isWorldConfirmed() const { return m_WorldConfirmed; }
    const std::string& getCurrentWorld() const { return m_CurrentWorld; }
    uint32_t getWorldTimeOfDay() const { return m_WorldTimeOfDay; }

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

    bool        m_authenticated = false;
    uint64_t    m_playerId      = 0;
    std::string m_username;
    std::string m_sessionToken;

    bool m_initialized = false;
};

#endif // NETWORK_MANAGER_H
