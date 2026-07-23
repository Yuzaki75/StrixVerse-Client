#include "NetworkStatistics.h"

NetworkStatistics::NetworkStatistics()
    : m_totalBytesSent(0), m_totalBytesReceived(0),
      m_totalPacketsSent(0), m_totalPacketsReceived(0),
      m_totalPacketsLost(0), m_totalDuplicatePackets(0),
      m_totalInvalidPackets(0) {}

void NetworkStatistics::bytesSent(uint32_t count) {
    m_totalBytesSent += count;
}

void NetworkStatistics::bytesReceived(uint32_t count) {
    m_totalBytesReceived += count;
}

void NetworkStatistics::packetsSent(uint32_t count) {
    m_totalPacketsSent += count;
}

void NetworkStatistics::packetsReceived(uint32_t count) {
    m_totalPacketsReceived += count;
}

void NetworkStatistics::packetsLost(uint32_t count) {
    m_totalPacketsLost += count;
}

void NetworkStatistics::duplicatePacketReceived(uint32_t count) {
    m_totalDuplicatePackets += count;
}

void NetworkStatistics::invalidPacketReceived(uint32_t count) {
    m_totalInvalidPackets += count;
}

void NetworkStatistics::reset() {
    m_totalBytesSent = 0;
    m_totalBytesReceived = 0;
    m_totalPacketsSent = 0;
    m_totalPacketsReceived = 0;
    m_totalPacketsLost = 0;
    m_totalDuplicatePackets = 0;
    m_totalInvalidPackets = 0;
}
