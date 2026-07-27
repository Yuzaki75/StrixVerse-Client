#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <memory>
#include <functional>
#include <string>
#include "Connection.h"
#include "PacketDispatcher.h"
#include "KeepAlive.h"
#include "PingManager.h"

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // Delete copy/move
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    // Initialize the network manager (initialize Winsock, create connection, etc.)
    bool initialize();

    // Connect to server
    bool connect(const std::string& host, uint16_t port);

    // Disconnect from server
    void disconnect();

    // Check if connected
    bool isConnected() const;

    // Send a packet to the server
    void sendPacket(const std::shared_ptr<Packet>& packet);

    // Register a packet handler for a specific packet type
    void addPacketHandler(PacketType type, const std::shared_ptr<PacketHandler>& handler);

    // Remove a packet handler
    void removePacketHandler(PacketType type, const std::shared_ptr<PacketHandler>& handler);

    // Update network manager (call once per frame)
    void update(float deltaTime);

    // Get network statistics
    NetworkStatistics getStatistics() const;

    // Get round-trip time statistics from ping manager
    uint32_t getLastRoundTripTimeMs() const;
    uint32_t getAverageRoundTripTimeMs() const;

private:
    // Callback for received packets
    void onPacketReceived(const std::shared_ptr<Packet>& packet);

    std::unique_ptr<Connection> m_connection;
    std::unique_ptr<PacketDispatcher> m_dispatcher;
    std::unique_ptr<KeepAlive> m_keepAlive;
    std::unique_ptr<PingManager> m_pingManager;
};

#endif // NETWORK_MANAGER_H