#include "Connection.h"

#include <iostream>
#include <sstream>

// Static members
int Connection::s_wsaRefCount = 0;

// Static members
bool Connection::initialize()
{
    if (s_wsaRefCount == 0)
    {
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            return false;
        }
    }
    s_wsaRefCount++;
    return true;
}

void Connection::cleanup()
{
    if (s_wsaRefCount > 0)
    {
        s_wsaRefCount--;
        if (s_wsaRefCount == 0)
        {
            WSACleanup();
        }
    }
}

Connection::Connection()
    : m_socket(INVALID_SOCKET), m_state(ConnectionState::Disconnected), m_running(false), m_recvBufferPos(0)
{
}

Connection::~Connection()
{
    disconnect();
    if (m_receiveThread.joinable())
        m_receiveThread.join();
    if (m_sendThread.joinable())
        m_sendThread.join();
}

bool Connection::connect(const std::string& host, uint16_t port)
{
    if (m_state.load() != ConnectionState::Disconnected)
        return false;

    // Ensure Winsock is initialized using reference counting
    if (!Connection::initialize())
        return false;

    m_state.store(ConnectionState::Connecting);

    // Create socket
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET)
    {
        std::cerr << "socket() failed: " << WSAGetLastError() << std::endl;
        m_state.store(ConnectionState::Disconnected);
        Connection::cleanup(); // Cleanup on failure
        return false;
    }

    // Set timeouts so recv/send can check m_running
    int timeout = 500; // 0.5 second
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    // Resolve host
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    addrinfo* result = nullptr;
    int res = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
    if (res != 0)
    {
        std::cerr << "getaddrinfo failed: " << res << std::endl;
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        m_state.store(ConnectionState::Disconnected);
        Connection::cleanup(); // Cleanup on failure
        return false;
    }

    // Use the first result
    m_serverAddr = *reinterpret_cast<sockaddr_in*>(result->ai_addr);
    freeaddrinfo(result);

    // Connect (blocking)
    if (::connect(m_socket, reinterpret_cast<sockaddr*>(&m_serverAddr), sizeof(m_serverAddr)) == SOCKET_ERROR)
    {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK)
        {
            std::cerr << "connect() failed: " << error << std::endl;
            closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            m_state.store(ConnectionState::Disconnected);
            Connection::cleanup(); // Cleanup on failure
            return false;
        }
        // For non-blocking we would need to wait; but we treat as connected anyway.
    }

    m_state.store(ConnectionState::Connected);

    // Start threads
    m_running = true;
    m_receiveThread = std::thread(&Connection::receiveThread, this);
    m_sendThread = std::thread(&Connection::sendThread, this);

    return true;
}

void Connection::disconnect()
{
    if (m_state.exchange(ConnectionState::Disconnecting) == ConnectionState::Disconnected)
        return;

    m_running = false;

    // Shutdown socket to cause blocking calls to return
    if (m_socket != INVALID_SOCKET)
    {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    // Wake up any waiting threads
    {
        std::lock_guard<std::mutex> lock(m_sendQueueMutex);
        m_sendQueueCond.notify_all();
    }
    {
        std::lock_guard<std::mutex> lock(m_receiveQueueMutex);
        m_receiveQueueCond.notify_all();
    }

    if (m_receiveThread.joinable())
        m_receiveThread.join();
    if (m_sendThread.joinable())
        m_sendThread.join();

    m_state.store(ConnectionState::Disconnected);

    // Clear queues
    std::queue<std::shared_ptr<Packet>> empty;
    {
        std::lock_guard<std::mutex> lock(m_sendQueueMutex);
        m_sendQueue.swap(empty);
    }
    {
        std::lock_guard<std::mutex> lock(m_receiveQueueMutex);
        m_receiveQueue.swap(empty);
    }

    // Cleanup Winsock reference count
    Connection::cleanup();
}

bool Connection::isConnected() const
{
    return m_state.load() == ConnectionState::Connected;
}

bool Connection::sendPacket(const std::shared_ptr<Packet>& packet)
{
    if (!isConnected())
        return false;

    {
        std::lock_guard<std::mutex> lock(m_sendQueueMutex);
        m_sendQueue.push(packet);
    }
    m_sendQueueCond.notify_one();
    return true;
}

void Connection::processReceivedPackets(std::function<void(const std::shared_ptr<Packet>&)> callback)
{
    std::vector<std::shared_ptr<Packet>> packets;
    {
        std::lock_guard<std::mutex> lock(m_receiveQueueMutex);
        while (!m_receiveQueue.empty())
        {
            packets.push_back(m_receiveQueue.front());
            m_receiveQueue.pop();
        }
    }
    for (auto& pkt : packets)
    {
        callback(pkt);
    }
}

void Connection::receiveThread()
{
    while (m_running.load())
    {
        if (m_socket == INVALID_SOCKET)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // First, try to process any complete packets already in the buffer
        bool progress = true;
        while (progress && m_running.load())
        {
            progress = false;
            // We need at least one byte to attempt to read a packet (packet ID)
            if (m_recvBufferPos < sizeof(uint8_t))
                break;

            // Copy buffer data into a PacketBuffer for deserialization
            std::vector<char> packetData(m_recvBuffer, m_recvBuffer + m_recvBufferPos);
            PacketBuffer pb;
            pb.write(packetData.data(), packetData.size());
            pb.resetReadPos();

            std::shared_ptr<Packet> pkt;
            try
            {
                if (PacketSerializer::deserialize(pb, pkt))
                {
                    // Determine consumed bytes
                    size_t consumed = packetData.size() - pb.remaining();
                    if (consumed > 0)
                    {
                        progress = true;
                        // Shift remaining bytes to front
                        if (consumed < m_recvBufferPos)
                        {
                            memmove(m_recvBuffer, m_recvBuffer + consumed, m_recvBufferPos - consumed);
                        }
                        m_recvBufferPos -= consumed;
                        m_stats.packetsReceived();

                        // Enqueue packet for consumer
                        {
                            std::lock_guard<std::mutex> lock(m_receiveQueueMutex);
                            m_receiveQueue.push(pkt);
                        }
                        m_receiveQueueCond.notify_one();
                        continue; // try to process another packet
                    }
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "Packet deserialization failed: " << e.what() << std::endl;
                // Consider the connection corrupted? We'll break.
                break;
            }
            catch (...)
            {
                std::cerr << "Unknown exception during packet deserialization" << std::endl;
                break;
            }

            // If we get here, we couldn't deserialize a packet (not enough data or error)
            break;
        }

        // If we have processed all possible packets, check if buffer is full
        if (m_recvBufferPos >= RECV_BUFFER_SIZE)
        {
            std::cerr << "Receive buffer overflow: possible oversized packet or stuck data" << std::endl;
            // We cannot proceed; break to avoid infinite loop
            break;
        }

        // Receive more data
        int bytesToRecv = static_cast<int>(RECV_BUFFER_SIZE - m_recvBufferPos);
        int bytesRead = recv(m_socket,
                             m_recvBuffer + static_cast<int>(m_recvBufferPos),
                             bytesToRecv,
                             0);

        if (bytesRead == SOCKET_ERROR)
        {
            int error = WSAGetLastError();
            if (error == WSAETIMEDOUT)
                continue; // timeout, try again
            else if (error == WSAECONNRESET)
            {
                std::cerr << "Connection reset by peer." << std::endl;
                break;
            }
            else
            {
                std::cerr << "recv() failed: " << error << std::endl;
                break;
            }
        }
        else if (bytesRead == 0)
        {
            // Connection closed gracefully
            std::cerr << "Connection closed by remote." << std::endl;
            break;
        }
        else
        {
            m_recvBufferPos += static_cast<size_t>(bytesRead);
            m_stats.bytesReceived(static_cast<uint32_t>(bytesRead));
            // Loop again to process any newly available packets
        }
    }

    m_running.store(false);
}

void Connection::sendThread()
{
    while (m_running.load())
    {
        std::unique_lock<std::mutex> lock(m_sendQueueMutex);
        m_sendQueueCond.wait(lock, [this] { return !m_running.load() || !m_sendQueue.empty(); });

        if (!m_running.load())
            break;

        // Take all pending packets
        std::queue<std::shared_ptr<Packet>> localQueue;
        std::swap(m_sendQueue, localQueue);
        lock.unlock();

        while (!localQueue.empty() && m_running.load())
        {
            auto pkt = std::move(localQueue.front());
            localQueue.pop();

            // Serialize packet
            PacketBuffer pb;
            PacketSerializer::serialize(*pkt, pb);
            const char* data = pb.data();
            size_t remaining = pb.size();

            while (remaining > 0 && m_running.load())
            {
                int sent = send(m_socket, data, static_cast<int>(remaining), 0);
                if (sent == SOCKET_ERROR)
                {
                    int error = WSAGetLastError();
                    if (error == WSAETIMEDOUT)
                        continue; // retry
                    else
                    {
                        std::cerr << "send() failed: " << error << std::endl;
                        m_running.store(false);
                        break;
                    }
                }
                else if (sent == 0)
                {
                    // Connection closed?
                    break;
                }
                else
                {
                    data += sent;
                    remaining -= static_cast<size_t>(sent);
                    m_stats.bytesSent(static_cast<uint32_t>(sent));
                    m_stats.packetsSent();
                }
            }
        }
    }
}