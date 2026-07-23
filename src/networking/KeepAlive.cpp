#include "KeepAlive.h"
#include "PingPacket.h"
#include "PongPacket.h"
#include "PacketSender.h" // We'll need a way to send packets

// Forward declaration of a sender interface; in reality, this would be provided by the network layer
class IPacketSender {
public:
    virtual ~IPacketSender() = default;
    virtual bool sendPacket(const std::shared_ptr<Packet>& packet) = 0;
};

// We'll store a pointer to the sender; in a full implementation, this would be set by the network manager
static IPacketSender* g_packetSender = nullptr;

// This is a simplistic approach; in reality, we'd use dependency injection
void setGlobalPacketSender(IPacketSender* sender) {
    g_packetSender = sender;
}

KeepAlive::KeepAlive(float sendIntervalSeconds)
    : m_sendIntervalSeconds(sendIntervalSeconds), m_timeSinceLastPing(0.0f),
      m_lastRoundTripTimeMs(0), m_awaitingPong(false), m_connected(false) {}

void KeepAlive::update(float deltaTime) {
    m_timeSinceLastPing += deltaTime;
    if (m_timeSinceLastPing >= m_sendIntervalSeconds && !m_awaitingPong) {
        sendPing();
        m_timeSinceLastPing = 0.0f;
    }
}

void KeepAlive::onPongReceived() {
    if (m_awaitingPong) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_pingSendTime);
        m_lastRoundTripTimeMs = static_cast<uint32_t>(duration.count());
        m_awaitingPong = false;
        m_connected = true;
    }
}

void KeepAlive::sendPing() {
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
