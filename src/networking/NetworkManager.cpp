#include "NetworkManager.h"

#include "ChatMessagePacket.h"
#include "DisconnectPacket.h"
#include "HandshakePacket.h"
#include "LoginPacket.h"
#include "PacketRegistry.h"
#include "PingPacket.h"
#include "PlayerMovePacket.h"
#include "PlayerRemovePacket.h"
#include "PlayerSpawnPacket.h"
#include "PongPacket.h"
#include "RegisterPacket.h"
#include "WorldJoinPacket.h"
#include "WorldLeavePacket.h"
#include "WorldStatePacket.h"
#include "../core/Logger.h"
#include "../core/Version.h"

#include <chrono>
#include <format>

NetworkManager::NetworkManager() = default;

NetworkManager::~NetworkManager()
{
    disconnect();

    if (m_initialized)
        Connection::cleanup();
}

bool NetworkManager::initialize()
{
    if (m_initialized)
        return true;

    if (!Connection::initialize())
    {
        Logger::Error("NetworkManager: failed to initialise the socket layer.");
        return false;
    }

    PacketRegistry::registerAllPacketTypes();

    m_connection = std::make_unique<Connection>();
    m_dispatcher = std::make_unique<PacketDispatcher>();

    // The server drops idle sessions, so the heartbeat runs whenever connected;
    // the ping manager measures latency for the HUD and loading screen.
    m_keepAlive   = std::make_unique<KeepAlive>(m_connection.get(), 10.0f);
    m_pingManager = std::make_unique<PingManager>(m_connection.get(), 5.0f);

    // The server answers a Ping with a Pong; feed that back into the timers.
    m_dispatcher->addHandler(
        Opcode::Pong,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* pong = static_cast<const PongPacket*>(packet.get());

                if (m_pingManager)
                    m_pingManager->onPongReceived(pong->Timestamp);

                if (m_keepAlive)
                    m_keepAlive->onPongReceived();
            }));

    // A server-initiated Ping must be answered, or the server will time us out.
    m_dispatcher->addHandler(
        Opcode::Ping,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* ping = static_cast<const PingPacket*>(packet.get());

                auto pong = std::make_shared<PongPacket>();
                pong->Timestamp = ping->Timestamp;
                sendPacket(pong);
            }));

    // The server answers a world join with WorldState. It is recorded here so
    // the loading screen can pick it up whenever it happens to be created.
    m_dispatcher->addHandler(
        Opcode::WorldState,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* state = static_cast<const WorldStatePacket*>(packet.get());

                m_CurrentWorld   = state->WorldName;
                m_WorldTimeOfDay = state->TimeOfDay;
                m_WorldConfirmed = true;

                Logger::Info(std::format("NetworkManager: server placed us in world '{}'.",
                                         state->WorldName));
            }));

    // The roster is maintained here so it survives the screen change between
    // joining a world and the gameplay screen being built.
    m_dispatcher->addHandler(
        Opcode::PlayerSpawn,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* spawn = static_cast<const PlayerSpawnPacket*>(packet.get());

                RemotePlayer& entry = m_RemotePlayers[spawn->EntityID];
                entry.id       = spawn->EntityID;
                entry.username = spawn->Username;
                entry.tileX    = spawn->X;
                entry.tileY    = spawn->Y;
            }));

    m_dispatcher->addHandler(
        Opcode::PlayerRemove,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* remove = static_cast<const PlayerRemovePacket*>(packet.get());
                m_RemotePlayers.erase(remove->EntityID);
            }));

    m_dispatcher->addHandler(
        Opcode::PlayerMove,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* move = static_cast<const PlayerMovePacket*>(packet.get());

                // Only track players we already know about; an unknown id is
                // the server correcting us, not a new player.
                const auto it = m_RemotePlayers.find(move->PlayerID);
                if (it == m_RemotePlayers.end())
                    return;

                it->second.tileX = move->X;
                it->second.tileY = move->Y;
            }));

    // A Disconnect is the server explaining why it is closing the session.
    m_dispatcher->addHandler(
        Opcode::Disconnect,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* disconnectPacket = static_cast<const DisconnectPacket*>(packet.get());
                Logger::Warning(std::format("NetworkManager: server disconnected us - {}",
                                            disconnectPacket->Reason));
                clearSession();
            }));

    m_initialized = true;
    Logger::Info("NetworkManager: initialised.");
    return true;
}

bool NetworkManager::connect(const std::string& host, uint16_t port)
{
    if (!m_initialized || !m_connection)
    {
        Logger::Error("NetworkManager: connect called before initialize.");
        return false;
    }

    m_host = host;
    m_port = port;

    clearSession();

    if (!m_connection->connect(host, port))
        return false;

    // The server expects the handshake as the first frame of a session.
    if (!sendHandshake())
    {
        m_connection->disconnect();
        return false;
    }

    return true;
}

void NetworkManager::disconnect()
{
    if (!m_connection)
        return;

    // Tell the server why, if the session is still usable.
    if (m_connection->isConnected())
    {
        auto packet = std::make_shared<DisconnectPacket>();
        packet->Reason = "client closed the session";
        m_connection->sendPacket(packet);
    }

    m_connection->disconnect();
    clearSession();
}

bool NetworkManager::isConnected() const
{
    return m_connection && m_connection->isConnected();
}

std::string NetworkManager::getLastError() const
{
    return m_connection ? m_connection->getLastError() : std::string();
}

bool NetworkManager::sendPacket(const std::shared_ptr<Packet>& packet)
{
    return m_connection && m_connection->sendPacket(packet);
}

bool NetworkManager::sendHandshake()
{
    auto packet = std::make_shared<HandshakePacket>();
    packet->ClientVersion      = Version::GetClientVersion();
    packet->ClientPlatform     = "windows";
    packet->ClientArchitecture = "x64";

    return sendPacket(packet);
}

bool NetworkManager::sendLogin(const std::string& username, const std::string& password)
{
    auto packet = std::make_shared<LoginPacket>();
    packet->Username = username;
    packet->Password = password;

    return sendPacket(packet);
}

bool NetworkManager::sendRegister(const std::string& username,
                                  const std::string& email,
                                  const std::string& password)
{
    auto packet = std::make_shared<RegisterPacket>();
    packet->Username = username;
    packet->Email    = email;
    packet->Password = password;

    return sendPacket(packet);
}

bool NetworkManager::sendWorldJoin(const std::string& worldName)
{
    // A new request invalidates the previous confirmation and roster.
    m_WorldConfirmed = false;
    m_CurrentWorld.clear();
    m_RemotePlayers.clear();

    auto packet = std::make_shared<WorldJoinPacket>();

    // The server ignores the client-supplied id and spawn point and uses the
    // session's authenticated player and the world's own spawn.
    packet->PlayerID  = m_playerId;
    packet->WorldName = worldName;
    packet->SpawnX    = 0.0f;
    packet->SpawnY    = 0.0f;

    return sendPacket(packet);
}

bool NetworkManager::sendWorldLeave()
{
    // Leaving invalidates the world confirmation and everyone we could see.
    m_WorldConfirmed = false;
    m_CurrentWorld.clear();
    m_RemotePlayers.clear();

    if (!isConnected())
        return false;

    return sendPacket(std::make_shared<WorldLeavePacket>());
}

bool NetworkManager::sendChat(const std::string& message)
{
    if (message.empty() || !isConnected())
        return false;

    auto packet = std::make_shared<ChatMessagePacket>();

    // The server overwrites this with the authenticated player's id; it is set
    // only so the field is not left uninitialised on the wire.
    packet->SenderID = m_playerId;
    packet->Message  = message.size() > ProtocolLimits::MaxChatMessageLength
                           ? message.substr(0, ProtocolLimits::MaxChatMessageLength)
                           : message;

    return sendPacket(packet);
}

bool NetworkManager::sendPlayerMove(float tileX, float tileY,
                                    float velocityX, float velocityY)
{
    if (!isConnected())
        return false;

    auto packet = std::make_shared<PlayerMovePacket>();

    // The server ignores this id and uses the connection's own player, but the
    // field still has to be on the wire.
    packet->PlayerID  = m_playerId;
    packet->X         = tileX;
    packet->Y         = tileY;
    packet->Z         = 0.0f;
    packet->VelocityX = velocityX;
    packet->VelocityY = velocityY;
    packet->VelocityZ = 0.0f;

    return sendPacket(packet);
}

void NetworkManager::addPacketHandler(Opcode opcode, const std::shared_ptr<PacketHandler>& handler)
{
    if (m_dispatcher)
        m_dispatcher->addHandler(opcode, handler);
}

void NetworkManager::removePacketHandler(Opcode opcode, const std::shared_ptr<PacketHandler>& handler)
{
    if (m_dispatcher)
        m_dispatcher->removeHandler(opcode, handler);
}

void NetworkManager::onPacketReceived(const std::shared_ptr<Packet>& packet)
{
    if (m_dispatcher)
        m_dispatcher->dispatch(packet);
}

void NetworkManager::update(float deltaTime)
{
    if (!m_connection)
        return;

    // Handlers run here, on the game thread.
    m_connection->processReceivedPackets(
        [this](const std::shared_ptr<Packet>& packet) { onPacketReceived(packet); });

    if (!m_connection->isConnected())
        return;

    if (m_keepAlive)
        m_keepAlive->update(deltaTime);

    if (m_pingManager)
        m_pingManager->update(deltaTime);
}

void NetworkManager::setSession(uint64_t playerId,
                                const std::string& username,
                                const std::string& token)
{
    m_playerId      = playerId;
    m_username      = username;
    m_sessionToken  = token;
    m_authenticated = true;

    Logger::Info(std::format("NetworkManager: session established for '{}' (player {}).",
                             username, playerId));
}

void NetworkManager::clearSession()
{
    m_WorldConfirmed = false;
    m_CurrentWorld.clear();
    m_WorldTimeOfDay = 0;
    m_RemotePlayers.clear();

    m_authenticated = false;
    m_playerId      = 0;
    m_username.clear();
    m_sessionToken.clear();
}

const NetworkStatistics& NetworkManager::getStatistics() const
{
    static const NetworkStatistics empty;
    return m_connection ? m_connection->getStatistics() : empty;
}

uint32_t NetworkManager::getLastRoundTripTimeMs() const
{
    return m_pingManager ? m_pingManager->getLastRoundTripTimeMs() : 0;
}

uint32_t NetworkManager::getAverageRoundTripTimeMs() const
{
    return m_pingManager ? m_pingManager->getAverageRoundTripTimeMs() : 0;
}
