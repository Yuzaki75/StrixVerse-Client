#ifndef CONNECTION_H
#define CONNECTION_H

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <memory>
#include <string>
#include <chrono>
#include "Packet.h"
#include "PacketSerializer.h"
#include "PacketSender.h"
#include "NetworkStatistics.h"

class Connection : public PacketSender {
public:
    Connection();
    virtual ~Connection();

    // Initialize Winsock (should be called once per process)
    static bool initialize();
    // Cleanup Winsock
    static void cleanup();

    // Connect to the server
    bool connect(const std::string& host, uint16_t port);
    // Disconnect from the server
    void disconnect();
    // Check if connected
    bool isConnected() const;
    // GetConnectionState() const;

    // Send a packet (thread-safe)
    bool sendPacket(const std::shared_ptr<Packet>& packet) override;

    // Process incoming packets (call this regularly to handle received packets)
    void processReceivedPackets(std::function<void(const std::shared_ptr<Packet>&)> callback);

    // Get statistics
    const NetworkStatistics& getStatistics() const { return m_stats; }

private:
    enum class ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Disconnecting
    };

    // Thread functions
    void receiveThread();
    void sendThread();

    // Helper to send all queued packets
    void flushSendQueue();

    // Socket
    SOCKET m_socket;
    // Server address
    sockaddr_in m_serverAddr;
    // Connection state
    std::atomic<ConnectionState> m_state;
    // Threads
    std::thread m_receiveThread;
    std::thread m_sendThread;
    // Flags to stop threads
    std::atomic<bool> m_running;

    // Queues
    std::queue<std::shared_ptr<Packet>> m_sendQueue;
    mutable std::mutex m_sendQueueMutex;
    std::condition_variable m_sendQueueCond;

    std::queue<std::shared_ptr<Packet>> m_receiveQueue;
    mutable std::mutex m_receiveQueueMutex;
    std::condition_variable m_receiveQueueCond;

    // Buffer for receiving data
    static constexpr size_t RECV_BUFFER_SIZE = 4096;
    char m_recvBuffer[RECV_BUFFER_SIZE];
    size_t m_recvBufferPos; // How many bytes are currently in the buffer

    // Statistics
    NetworkStatistics m_stats;
};

#endif // CONNECTION_H
