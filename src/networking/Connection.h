#ifndef CONNECTION_H
#define CONNECTION_H

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "NetworkStatistics.h"
#include "Packet.h"
#include "PacketSender.h"
#include "Protocol.h"

// -----------------------------------------------------------------------------
// Connection
//
// TCP transport for one server session.
//
// Frames are built and parsed exactly as the server does in
// Server/src/network/Socket/TcpSession.cpp:
//
//     [ opcode : uint16 big-endian ][ length : uint32 big-endian ][ payload ]
//
// A background thread drains the socket into a reassembly buffer and pushes
// completed packets onto a queue; the game thread consumes them through
// processReceivedPackets(), so packet handlers never run on the socket thread.
// -----------------------------------------------------------------------------
class Connection : public PacketSender
{
public:
    enum class State
    {
        Disconnected,
        Connecting,
        Connected,
        Failed
    };

    Connection();
    ~Connection() override;

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    // Winsock startup/teardown, reference counted.
    static bool initialize();
    static void cleanup();

    // Blocking connect. Returns false and sets the failure reason on error.
    // Blocking connect. Kept for callers that genuinely want to wait; it is
    // now beginConnect() followed by pollConnect() until the deadline.
    bool connect(const std::string& host, uint16_t port);

    // Non-blocking connect, in two parts, so the caller can keep drawing
    // frames while the socket comes up. A dead host used to stall the whole
    // window for the full timeout with no feedback.
    enum class ConnectProgress
    {
        Pending,     // still waiting; call again next frame
        Connected,
        Failed
    };

    // Starts resolving and connecting. Returns false if it failed outright,
    // in which case pollConnect() must not be called.
    bool beginConnect(const std::string& host, uint16_t port);

    // Advances a pending connect without blocking. Safe to call only after a
    // successful beginConnect().
    ConnectProgress pollConnect();

    void disconnect();

    bool isConnected() const { return m_state.load() == State::Connected; }
    State getState() const { return m_state.load(); }

    // Why the last connect or session failed; empty when there was no failure.
    std::string getLastError() const;

    bool sendPacket(const std::shared_ptr<Packet>& packet) override;

    // Drains the received queue on the calling thread.
    void processReceivedPackets(const std::function<void(const std::shared_ptr<Packet>&)>& callback);

    const NetworkStatistics& getStatistics() const { return m_stats; }

private:
    void receiveThread();

    // Pulls every complete frame out of the reassembly buffer.
    void parseFrames();

    void setFailure(const std::string& reason);
    void closeSocket();

    SOCKET m_socket = INVALID_SOCKET;

    // Pending non-blocking connect. Held between beginConnect() and the
    // pollConnect() that resolves it.
    std::string           m_pendingHost;
    uint16_t              m_pendingPort = 0;
    std::chrono::steady_clock::time_point m_connectDeadline{};

    // Shared tail of both connect paths: applies the socket options that must
    // be set once the connection is actually up.
    void finishConnect();

    std::atomic<State> m_state{State::Disconnected};
    std::atomic<bool>  m_running{false};

    std::thread m_receiveThread;

    // Reassembly buffer: TCP gives no message boundaries, so a frame may
    // arrive split across reads or several frames may arrive in one read.
    std::vector<uint8_t> m_receiveBuffer;
    std::size_t          m_readPosition = 0;

    std::deque<std::shared_ptr<Packet>> m_receiveQueue;
    mutable std::mutex                  m_receiveQueueMutex;

    mutable std::mutex m_errorMutex;
    std::string        m_lastError;

    // Sends are serialised so two threads cannot interleave halves of a frame.
    std::mutex m_sendMutex;

    NetworkStatistics m_stats;

    static std::mutex s_wsaMutex;
    static int        s_wsaRefCount;
};

#endif // CONNECTION_H
