#include "Connection.h"

#include "PacketRegistry.h"
#include "../core/Logger.h"

#include <chrono>
#include <cstring>
#include <format>

#pragma comment(lib, "Ws2_32.lib")

std::mutex Connection::s_wsaMutex;
int        Connection::s_wsaRefCount = 0;

namespace
{
    // How long recv() blocks before returning so the thread can notice a
    // shutdown request.
    constexpr DWORD kReceiveTimeoutMs = 200;

    constexpr int kConnectTimeoutMs = 5000;

    std::string DescribeSocketError(int error)
    {
        switch (error)
        {
        case WSAECONNREFUSED: return "connection refused";
        case WSAETIMEDOUT:    return "connection timed out";
        case WSAEHOSTUNREACH: return "host unreachable";
        case WSAENETUNREACH:  return "network unreachable";
        case WSAECONNRESET:   return "connection reset by peer";
        case WSAECONNABORTED: return "connection aborted";
        default:              return "socket error " + std::to_string(error);
        }
    }
}

Connection::Connection() = default;

Connection::~Connection()
{
    disconnect();
}

bool Connection::initialize()
{
    std::lock_guard<std::mutex> lock(s_wsaMutex);

    if (s_wsaRefCount > 0)
    {
        ++s_wsaRefCount;
        return true;
    }

    WSADATA wsaData{};
    const int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        Logger::Error(std::format("Connection: WSAStartup failed ({}).", result));
        return false;
    }

    s_wsaRefCount = 1;
    return true;
}

void Connection::cleanup()
{
    std::lock_guard<std::mutex> lock(s_wsaMutex);

    if (s_wsaRefCount == 0)
        return;

    if (--s_wsaRefCount == 0)
        WSACleanup();
}

std::string Connection::getLastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}

void Connection::setFailure(const std::string& reason)
{
    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastError = reason;
    }

    m_state.store(State::Failed);
}

void Connection::closeSocket()
{
    if (m_socket != INVALID_SOCKET)
    {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

bool Connection::connect(const std::string& host, uint16_t port)
{
    if (m_state.load() == State::Connected)
        return true;

    disconnect();

    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_lastError.clear();
    }

    m_state.store(State::Connecting);

    // Resolve the host so a name works as well as a literal address.
    addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* resolved = nullptr;
    const std::string service = std::to_string(port);

    if (getaddrinfo(host.c_str(), service.c_str(), &hints, &resolved) != 0 || !resolved)
    {
        setFailure(std::format("could not resolve '{}'", host));
        Logger::Error(std::format("Connection: {}", getLastError()));
        return false;
    }

    m_socket = socket(resolved->ai_family, resolved->ai_socktype, resolved->ai_protocol);
    if (m_socket == INVALID_SOCKET)
    {
        setFailure(DescribeSocketError(WSAGetLastError()));
        freeaddrinfo(resolved);
        return false;
    }

    // Connect with a timeout: a non-blocking connect plus select, so a dead
    // host does not stall the caller for the OS default of ~20 seconds.
    u_long nonBlocking = 1;
    ioctlsocket(m_socket, FIONBIO, &nonBlocking);

    bool connected = false;

    if (::connect(m_socket, resolved->ai_addr, static_cast<int>(resolved->ai_addrlen)) == 0)
    {
        connected = true;
    }
    else if (WSAGetLastError() == WSAEWOULDBLOCK)
    {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_socket, &writeSet);

        timeval timeout{};
        timeout.tv_sec  = kConnectTimeoutMs / 1000;
        timeout.tv_usec = (kConnectTimeoutMs % 1000) * 1000;

        const int ready = select(0, nullptr, &writeSet, nullptr, &timeout);
        if (ready > 0)
        {
            int soError = 0;
            int length  = sizeof(soError);
            getsockopt(m_socket, SOL_SOCKET, SO_ERROR,
                       reinterpret_cast<char*>(&soError), &length);

            if (soError == 0)
                connected = true;
            else
                setFailure(DescribeSocketError(soError));
        }
        else
        {
            setFailure("connection timed out");
        }
    }
    else
    {
        setFailure(DescribeSocketError(WSAGetLastError()));
    }

    freeaddrinfo(resolved);

    if (!connected)
    {
        closeSocket();
        Logger::Error(std::format("Connection: failed to connect to {}:{} - {}",
                                  host, port, getLastError()));
        return false;
    }

    nonBlocking = 0;
    ioctlsocket(m_socket, FIONBIO, &nonBlocking);

    // A read timeout lets the receive thread poll m_running.
    DWORD timeoutMs = kReceiveTimeoutMs;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));

    // Latency matters more than packing for this traffic.
    BOOL noDelay = TRUE;
    setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY,
               reinterpret_cast<const char*>(&noDelay), sizeof(noDelay));

    m_receiveBuffer.clear();
    m_readPosition = 0;

    {
        std::lock_guard<std::mutex> lock(m_receiveQueueMutex);
        m_receiveQueue.clear();
    }

    m_state.store(State::Connected);
    m_running.store(true);
    m_receiveThread = std::thread(&Connection::receiveThread, this);

    Logger::Info(std::format("Connection: connected to {}:{}", host, port));
    return true;
}

void Connection::disconnect()
{
    const bool wasRunning = m_running.exchange(false);

    // Closing the socket unblocks a recv() that is mid-timeout.
    closeSocket();

    if (m_receiveThread.joinable())
        m_receiveThread.join();

    {
        std::lock_guard<std::mutex> lock(m_receiveQueueMutex);
        m_receiveQueue.clear();
    }

    m_receiveBuffer.clear();
    m_readPosition = 0;

    // A failure reason set by the receive thread must survive teardown so the
    // UI can show why the session ended.
    if (m_state.load() != State::Failed)
        m_state.store(State::Disconnected);

    if (wasRunning)
        Logger::Info("Connection: disconnected.");
}

bool Connection::sendPacket(const std::shared_ptr<Packet>& packet)
{
    if (!packet || !isConnected())
        return false;

    PacketBuffer payload;

    try
    {
        packet->serialize(payload);
    }
    catch (const std::exception& error)
    {
        Logger::Error(std::format("Connection: failed to serialize {}: {}",
                                  packet->getName(), error.what()));
        return false;
    }

    if (payload.size() > ProtocolLimits::MaxPayloadSize)
    {
        Logger::Error(std::format("Connection: {} payload of {} bytes exceeds the {} byte limit.",
                                  packet->getName(), payload.size(),
                                  ProtocolLimits::MaxPayloadSize));
        return false;
    }

    // Header is network byte order; payload is native, matching the server.
    const uint16_t opcodeNet = htons(static_cast<uint16_t>(packet->getOpcode()));
    const uint32_t lengthNet = htonl(static_cast<uint32_t>(payload.size()));

    std::vector<char> frame;
    frame.reserve(ProtocolLimits::HeaderSize + payload.size());

    const char* opcodeBytes = reinterpret_cast<const char*>(&opcodeNet);
    const char* lengthBytes = reinterpret_cast<const char*>(&lengthNet);

    frame.insert(frame.end(), opcodeBytes, opcodeBytes + sizeof(opcodeNet));
    frame.insert(frame.end(), lengthBytes, lengthBytes + sizeof(lengthNet));
    frame.insert(frame.end(), payload.data(), payload.data() + payload.size());

    std::lock_guard<std::mutex> lock(m_sendMutex);

    std::size_t sent = 0;
    while (sent < frame.size())
    {
        const int result = send(m_socket,
                                frame.data() + sent,
                                static_cast<int>(frame.size() - sent),
                                0);

        if (result == SOCKET_ERROR)
        {
            const int error = WSAGetLastError();
            setFailure(DescribeSocketError(error));
            Logger::Error(std::format("Connection: send failed - {}", getLastError()));
            return false;
        }

        sent += static_cast<std::size_t>(result);
    }

    m_stats.bytesSent(static_cast<uint32_t>(frame.size()));
    m_stats.packetsSent();

    return true;
}

void Connection::receiveThread()
{
    std::vector<char> chunk(8192);

    while (m_running.load())
    {
        const int received = recv(m_socket, chunk.data(), static_cast<int>(chunk.size()), 0);

        if (received > 0)
        {
            m_stats.bytesReceived(static_cast<uint32_t>(received));

            m_receiveBuffer.insert(m_receiveBuffer.end(),
                                   reinterpret_cast<uint8_t*>(chunk.data()),
                                   reinterpret_cast<uint8_t*>(chunk.data()) + received);

            if (m_receiveBuffer.size() > ProtocolLimits::MaxReceiveBufferSize)
            {
                setFailure("server sent more unparsed data than the receive buffer allows");
                Logger::Error("Connection: receive buffer overflow; dropping the session.");
                m_running.store(false);
                break;
            }

            parseFrames();
            continue;
        }

        if (received == 0)
        {
            // Orderly shutdown by the peer.
            if (m_running.load())
            {
                setFailure("server closed the connection");
                Logger::Warning("Connection: server closed the connection.");
            }
            m_running.store(false);
            break;
        }

        const int error = WSAGetLastError();

        // A read timeout is how the loop stays responsive to disconnect().
        if (error == WSAETIMEDOUT || error == WSAEWOULDBLOCK)
            continue;

        if (m_running.load())
        {
            setFailure(DescribeSocketError(error));
            Logger::Error(std::format("Connection: recv failed - {}", getLastError()));
        }

        m_running.store(false);
        break;
    }
}

void Connection::parseFrames()
{
    while (true)
    {
        const std::size_t available = m_receiveBuffer.size() - m_readPosition;

        if (available < ProtocolLimits::HeaderSize)
            break;

        const uint8_t* cursor = m_receiveBuffer.data() + m_readPosition;

        uint16_t opcodeRaw    = 0;
        uint32_t payloadLength = 0;
        std::memcpy(&opcodeRaw, cursor, sizeof(opcodeRaw));
        std::memcpy(&payloadLength, cursor + sizeof(opcodeRaw), sizeof(payloadLength));

        opcodeRaw     = ntohs(opcodeRaw);
        payloadLength = ntohl(payloadLength);

        // Reject an absurd declared length before it is used for anything;
        // otherwise the loop waits forever for data that will never arrive.
        if (payloadLength > ProtocolLimits::MaxPayloadSize)
        {
            setFailure("server declared an oversized packet");
            Logger::Error(std::format("Connection: declared payload of {} bytes exceeds the limit.",
                                      payloadLength));
            m_running.store(false);
            return;
        }

        const std::size_t frameSize = ProtocolLimits::HeaderSize + payloadLength;

        // Wait for the rest of a partially received frame.
        if (available < frameSize)
            break;

        const uint8_t* payloadStart = cursor + ProtocolLimits::HeaderSize;

        m_readPosition += frameSize;

        const Opcode opcode = static_cast<Opcode>(opcodeRaw);

        std::shared_ptr<Packet> packet = PacketRegistry::createPacket(opcode);
        if (!packet)
        {
            // Unknown opcodes are skipped, not fatal: the server may be newer.
            m_stats.invalidPacketReceived();
            Logger::Warning(std::format("Connection: ignoring unknown opcode {}.", opcodeRaw));
            continue;
        }

        PacketBuffer payload;
        if (payloadLength > 0)
            payload.write(payloadStart, payloadLength);

        try
        {
            packet->deserialize(payload);
        }
        catch (const std::exception& error)
        {
            // A malformed packet is the peer's fault, not ours; drop the packet
            // and keep the session, since the frame boundary is still known.
            m_stats.invalidPacketReceived();
            Logger::Warning(std::format("Connection: malformed {} (opcode {}): {}",
                                        packet->getName(), opcodeRaw, error.what()));
            continue;
        }

        m_stats.packetsReceived();

        {
            std::lock_guard<std::mutex> lock(m_receiveQueueMutex);
            m_receiveQueue.push_back(std::move(packet));
        }
    }

    // Release storage for frames already consumed, so a long session's buffer
    // does not grow without bound.
    if (m_readPosition > 0)
    {
        m_receiveBuffer.erase(m_receiveBuffer.begin(),
                              m_receiveBuffer.begin() + static_cast<ptrdiff_t>(m_readPosition));
        m_readPosition = 0;
    }
}

void Connection::processReceivedPackets(
    const std::function<void(const std::shared_ptr<Packet>&)>& callback)
{
    if (!callback)
        return;

    std::deque<std::shared_ptr<Packet>> pending;

    {
        std::lock_guard<std::mutex> lock(m_receiveQueueMutex);
        pending.swap(m_receiveQueue);
    }

    // Handlers run on the caller's thread (the game thread), outside the lock,
    // so a handler may safely send packets or tear the connection down.
    for (const auto& packet : pending)
        callback(packet);
}
