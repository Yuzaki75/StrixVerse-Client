#include "KeepAlive.h"
#include "KeepAlivePacket.h"

KeepAlive::KeepAlive(PacketSender* sender, float sendIntervalSeconds)
    : m_sender(sender), m_sendIntervalSeconds(sendIntervalSeconds), m_timeSinceLastPing(0.0f),
      m_lastRoundTripTimeMs(0), m_awaitingPong(false), m_connected(false)
{
}

void KeepAlive::update(float deltaTime)
{
    m_timeSinceLastPing += deltaTime;
    if (m_timeSinceLastPing >= m_sendIntervalSeconds && !m_awaitingPong) {
        sendPing();
        m_timeSinceLastPing = 0.0f;
    }
}

void KeepAlive::onPongReceived()
{
    if (m_awaitingPong) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_pingSendTime);
        m_lastRoundTripTimeMs = static_cast<uint32_t>(duration.count());
        m_awaitingPong = false;
        m_connected = true;
    }
}

void KeepAlive::sendPing()
{
    if (m_sender) {
        auto heartbeat = std::make_shared<KeepAlivePacket>();
        heartbeat->Timestamp = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());

        m_pingSendTime = std::chrono::steady_clock::now();
        m_awaitingPong = true;
        m_sender->sendPacket(heartbeat);
    }
}