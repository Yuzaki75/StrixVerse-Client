#ifndef PING_MANAGER_H
#define PING_MANAGER_H

#include <vector>
#include <cstdint>
#include "PacketSender.h"

class PingManager {
public:
    explicit PingManager(PacketSender* sender, float sendIntervalSeconds = 5.0f, size_t maxHistory = 100);
    ~PingManager() = default;

    void update(float deltaTime);
    void onPongReceived(uint64_t timestamp);

    // Get the latest round-trip time in milliseconds
    uint32_t getLastRoundTripTimeMs() const;
    // Get the average round-trip time over the history
    uint32_t getAverageRoundTripTimeMs() const;
    // Get the minimum RTT in the history
    uint32_t getMinRoundTripTimeMs() const;
    // Get the maximum RTT in the history
    uint32_t getMaxRoundTripTimeMs() const;

    // Clear history
    void clearHistory();

private:
    void sendPing();

    PacketSender* m_sender;
    float m_sendIntervalSeconds;
    float m_timeSinceLastPing;
    std::vector<uint32_t> m_rttHistory;
    size_t m_maxHistory;
    std::chrono::steady_clock::time_point m_pingSendTime;
    bool m_awaitingPong;
};

#endif // PING_MANAGER_H