#ifndef KEEP_ALIVE_H
#define KEEP_ALIVE_H

#include <chrono>
#include <cstdint>
#include "Packet.h"
#include "PacketSender.h"

class KeepAlive {
public:
    explicit KeepAlive(PacketSender* sender, float sendIntervalSeconds = 10.0f);
    ~KeepAlive() = default;

    // Update the keep-alive timer; call this regularly (e.g., every frame)
    void update(float deltaTime);

    // Call this when a PongPacket is received
    void onPongReceived();

    // Get the last round-trip time in milliseconds
    uint32_t getLastRoundTripTimeMs() const { return m_lastRoundTripTimeMs; }

    // Get whether we have received a response to the last ping
    bool isConnected() const { return m_connected; }

private:
    void sendPing();

    PacketSender* m_sender;
    float m_sendIntervalSeconds;
    float m_timeSinceLastPing;
    uint32_t m_lastRoundTripTimeMs;
    std::chrono::steady_clock::time_point m_pingSendTime;
    bool m_awaitingPong;
    bool m_connected;
};

#endif // KEEP_ALIVE_H