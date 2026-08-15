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
    bool connect(const std::string& host, uint16_t port);

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
