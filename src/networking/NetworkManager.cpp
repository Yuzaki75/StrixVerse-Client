#include "NetworkManager.h"
#include "Packet.h"
#include <iostream>

NetworkManager::NetworkManager()
    : m_connection(std::make_unique<Connection>()),
      m_dispatcher(std::make_unique<PacketDispatcher>()),
      m_keepAlive(nullptr),
      m_pingManager(nullptr)
{
}

NetworkManager::~NetworkManager()
{
    disconnect();
}

bool NetworkManager::initialize()
{
    // Initialize Winsock via the Connection class
    if (!Connection::initialize()) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return false;
    }

    // Create KeepAlive and PingManager, passing the connection as the sender
    m_keepAlive = std::make_unique<KeepAlive>(m_connection.get(), 10.0f); // 10 seconds interval
    m_pingManager = std::make_unique<PingManager>(m_connection.get(), 5.0f); // 5 seconds interval

    return true;
}

bool NetworkManager::connect(const std::string& host, uint16_t port)
{
    if (!m_connection) {
        return false;
    }
    return m_connection->connect(host, port);
}

void NetworkManager::disconnect()
{
    if (m_connection) {
        m_connection->disconnect();
    }
}

bool NetworkManager::isConnected() const
{
    return m_connection && m_connection->isConnected();
}

void NetworkManager::sendPacket(const std::shared_ptr<Packet>& packet)
{
    if (m_connection) {
        m_connection->sendPacket(packet);
    }
}

void NetworkManager::addPacketHandler(PacketType type, const std::shared_ptr<PacketHandler>& handler)
{
    if (m_dispatcher) {
        m_dispatcher->addHandler(type, handler);
    }
}

void NetworkManager::removePacketHandler(PacketType type, const std::shared_ptr<PacketHandler>& handler)
{
    if (m_dispatcher) {
        m_dispatcher->removeHandler(type, handler);
    }
}

void NetworkManager::update(float deltaTime)
{
    if (m_connection && m_connection->isConnected()) {
        // Process incoming packets and dispatch them to handlers
        m_connection->processReceivedPackets([this](const std::shared_ptr<Packet>& packet) {
            if (m_dispatcher) {
                m_dispatcher->dispatch(packet);
            }
        });
    }

    // Update keep-alive and ping manager
    if (m_keepAlive) {
        m_keepAlive->update(deltaTime);
    }
    if (m_pingManager) {
        m_pingManager->update(deltaTime);
    }
}

NetworkStatistics NetworkManager::getStatistics() const
{
    if (m_connection) {
        return m_connection->getStatistics();
    }
    return NetworkStatistics(); // Return default-constructed stats if no connection
}

uint32_t NetworkManager::getLastRoundTripTimeMs() const
{
    if (m_pingManager) {
        return m_pingManager->getLastRoundTripTimeMs();
    }
    return 0;
}

uint32_t NetworkManager::getAverageRoundTripTimeMs() const
{
    if (m_pingManager) {
        return m_pingManager->getAverageRoundTripTimeMs();
    }
    return 0;
}

void NetworkManager::onPacketReceived(const std::shared_ptr<Packet>& packet)
{
    // This method is not used because we pass a lambda to processReceivedPackets.
    // But we keep it for completeness if needed elsewhere.
    if (m_dispatcher) {
        m_dispatcher->dispatch(packet);
    }
}