#include "PingManager.h"
#include "PacketSender.h"

static IPacketSender* g_packetSender = nullptr;

void setGlobalPacketSenderForPingManager(IPacketSender* sender) {
    g_packetSender = sender;
}

PingManager::PingManager(float sendIntervalSeconds, size_t maxHistory)
    : m_sendIntervalSeconds(sendIntervalSeconds), m_timeSinceLastPing(0.0f),
      m_maxHistory(maxHistory), m_awaitingPong(false) {}

void PingManager::update(float deltaTime) {
    m_timeSinceLastPing += deltaTime;
    if (m_timeSinceLastPing >= m_sendIntervalSeconds && !m_awaitingPong) {
        sendPing();
        m_timeSinceLastPing = 0.0f;
    }
}

void PingManager::onPongReceived(uint64_t timestamp) {
    if (m_awaitingPong) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_pingSendTime);
        uint32_t rtt = static_cast<uint32_t>(duration.count());
        m_rttHistory.push_back(rtt);
        if (m_rttHistory.size() > m_maxHistory) {
            m_rttHistory.erase(m_rttHistory.begin());
        }
        m_awaitingPong = false;
    }
}

void PingManager::sendPing() {
    if (g_packetSender) {
        auto ping = std::make_shared<PingPacket>();
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        ping->setTimestamp(timestamp);
        m_pingSendTime = std::chrono::steady_clock::now();
        m_awaitingPong = true;
        g_packetSender->sendPacket(ping);
    }
}

uint32_t PingManager::getLastRoundTripTimeMs() const {
    if (m_rttHistory.empty()) return 0;
    return m_rttHistory.back();
}

uint32_t PingManager::getAverageRoundTripTimeMs() const {
    if (m_rttHistory.empty()) return 0;
    uint64_t sum = 0;
    for (auto rtt : m_rttHistory) {
        sum += rtt;
    }
    return static_cast<uint32_t>(sum / m_rttHistory.size());
}

uint32_t PingManager::getMinRoundTripTimeMs() const {
    if (m_rttHistory.empty()) return 0;
    return *std::min_element(m_rttHistory.begin(), m_rttHistory.end());
}

uint32_t PingManager::getMaxRoundTripTimeMs() const {
    if (m_rttHistory.empty()) return 0;
    return *std::max_element(m_rttHistory.begin(), m_rttHistory.end());
}

void PingManager::clearHistory() {
    m_rttHistory.clear();
}
