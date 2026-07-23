#ifndef NETWORK_STATISTICS_H
#define NETWORK_STATISTICS_H

#include <cstdint>
#include <string>

class NetworkStatistics {
public:
    NetworkStatistics();
    ~NetworkStatistics() = default;

    // Call when bytes are sent
    void bytesSent(uint32_t count);
    // Call when bytes are received
    void bytesReceived(uint32_t count);
    // Call when a packet is sent
    void packetsSent(uint32_t count = 1);
    // Call when a packet is received
    void packetsReceived(uint32_t count = 1);
    // Call when a packet is lost (detected via sequence numbers, etc.)
    void packetsLost(uint32_t count = 1);
    // Call when a duplicate packet is received
    void duplicatePacketReceived(uint32_t count = 1);
    // Call when a packet is malformed/invalid
    void invalidPacketReceived(uint32_t count = 1);

    // Getters
    uint64_t getTotalBytesSent() const { return m_totalBytesSent; }
    uint64_t getTotalBytesReceived() const { return m_totalBytesReceived; }
    uint64_t getTotalPacketsSent() const { return m_totalPacketsSent; }
    uint64_t getTotalPacketsReceived() const { return m_totalPacketsReceived; }
    uint64_t getTotalPacketsLost() const { return m_totalPacketsLost; }
    uint64_t getTotalDuplicatePackets() const { return m_totalDuplicatePackets; }
    uint64_t getTotalInvalidPackets() const { return m_totalInvalidPackets; }

    // Reset all statistics
    void reset();

private:
    uint64_t m_totalBytesSent;
    uint64_t m_totalBytesReceived;
    uint64_t m_totalPacketsSent;
    uint64_t m_totalPacketsReceived;
    uint64_t m_totalPacketsLost;
    uint64_t m_totalDuplicatePackets;
    uint64_t m_totalInvalidPackets;
};

#endif // NETWORK_STATISTICS_H
