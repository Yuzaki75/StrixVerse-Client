#include "NetworkManager.h"

#include "ChatMessagePacket.h"
#include "DisconnectPacket.h"
#include "HandshakePacket.h"
#include "InventoryPacket.h"
#include "InventoryUpdatePacket.h"
#include "LoginPacket.h"
#include "PacketRegistry.h"
#include "PingPacket.h"
#include "PlayerDataPacket.h"
#include "PlayerMovePacket.h"
#include "PlayerRemovePacket.h"
#include "PlayerSpawnPacket.h"
#include "PongPacket.h"
#include "RegisterPacket.h"
#include "WorldJoinPacket.h"
#include "WorldLeavePacket.h"
#include "BlockBreakPacket.h"
#include "BlockPlacePacket.h"
#include "ChunkLoadPacket.h"
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

    // Terrain. The server sends the whole playable world immediately after
    // WorldState, so these arrive while the loading screen is up. Collected
    // here and handed to GameScreen when it is built.
    m_dispatcher->addHandler(
        Opcode::ChunkLoad,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* load = static_cast<const ChunkLoadPacket*>(packet.get());

                // A short chunk would leave holes that read as walkable air,
                // so a partial chunk is dropped rather than half-applied.
                if (load->Tiles.size() != ChunkLoadPacket::kTileCount)
                {
                    Logger::Warning(std::format(
                        "NetworkManager: chunk ({}, {}) carried {} tiles, expected {}; ignored.",
                        load->ChunkX, load->ChunkY,
                        load->Tiles.size(), ChunkLoadPacket::kTileCount));
                    return;
                }

                TerrainChunk chunk;
                chunk.chunkX = load->ChunkX;
                chunk.chunkY = load->ChunkY;
                chunk.tiles  = load->Tiles;

                m_Terrain[terrainKey(load->ChunkX, load->ChunkY)] = std::move(chunk);
                ++m_TerrainRevision;
            }));

    // World edits, echoed by the server once accepted. These arrive for edits
    // made by anyone, including this client -- the server is the only thing
    // that decides an edit happened.
    m_dispatcher->addHandler(
        Opcode::BlockBreak,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* broken = static_cast<const BlockBreakPacket*>(packet.get());
                recordTileEdit(broken->X, broken->Y, 0);   // 0 = air
            }));

    m_dispatcher->addHandler(
        Opcode::BlockPlace,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* placed = static_cast<const BlockPlacePacket*>(packet.get());

                // The wire carries the ITEM id, and the world stores TILE ids.
                // They coincide for the block items the generator uses, so the
                // value is passed through; anything that does not fit a byte
                // is clamped rather than wrapping into an unrelated tile.
                const uint16_t itemId = placed->ItemID;
                recordTileEdit(placed->X, placed->Y,
                               static_cast<uint8_t>(itemId > 255 ? 255 : itemId));
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

                // What they look like. The server sends this with the spawn
                // and the client used to stop reading before it, so everyone
                // else was drawn identically.
                entry.look.hair     = spawn->Appearance.hair;
                entry.look.skin     = spawn->Appearance.skin;
                entry.look.eyes     = spawn->Appearance.eyes;
                entry.look.shirt    = spawn->Appearance.shirt;
                entry.look.trousers = spawn->Appearance.trousers;
                entry.look.boots    = spawn->Appearance.boots;
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

    // Character stats. Sent on world join, which happens while the loading
    // screen is up, so this is recorded here rather than on the HUD.
    m_dispatcher->addHandler(
        Opcode::PlayerData,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* data = static_cast<const PlayerDataPacket*>(packet.get());

                m_Stats.known      = true;
                m_Stats.look.hair     = data->Appearance.hair;
                m_Stats.look.skin     = data->Appearance.skin;
                m_Stats.look.eyes     = data->Appearance.eyes;
                m_Stats.look.shirt    = data->Appearance.shirt;
                m_Stats.look.trousers = data->Appearance.trousers;
                m_Stats.look.boots    = data->Appearance.boots;
                m_Stats.level      = data->Level;
                m_Stats.experience = data->Experience;
                m_Stats.experienceToNextLevel = data->ExperienceToNextLevel;
                m_Stats.health     = data->Health;
                m_Stats.maxHealth  = data->MaxHealth;

                ++m_StatsRevision;

                Logger::Info(std::format("NetworkManager: stats - level {}, xp {}, hp {}/{}.",
                                         data->Level, data->Experience,
                                         data->Health, data->MaxHealth));
            }));

    // Inventory, kept here for the same reason as the roster: the reply to the
    // request can land while a screen change is in flight.
    m_dispatcher->addHandler(
        Opcode::Inventory,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* inventory = static_cast<const InventoryPacket*>(packet.get());

                // A full sync replaces what we had; slots absent from it are
                // empty, not merely unchanged.
                m_Inventory.clear();

                for (const auto& entry : inventory->DecodeSlots())
                {
                    if (entry.itemId == 0)
                        continue;

                    InventorySlot& slot = m_Inventory[entry.slot];
                    slot.itemId   = entry.itemId;
                    slot.quantity = entry.quantity;
                }

                ++m_InventoryRevision;

                Logger::Info(std::format("NetworkManager: inventory synced, {} occupied slot(s).",
                                         m_Inventory.size()));
            }));

    m_dispatcher->addHandler(
        Opcode::InventoryUpdate,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* update = static_cast<const InventoryUpdatePacket*>(packet.get());

                if (update->ItemID == 0 || update->Quantity == 0)
                {
                    m_Inventory.erase(update->SlotIndex);
                }
                else
                {
                    InventorySlot& slot = m_Inventory[update->SlotIndex];
                    slot.itemId     = update->ItemID;
                    slot.quantity   = update->Quantity;
                    slot.durability = update->Durability;
                }

                ++m_InventoryRevision;
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

bool NetworkManager::beginConnect(const std::string& host, uint16_t port)
{
    if (!m_initialized || !m_connection)
    {
        Logger::Error("NetworkManager: beginConnect called before initialize.");
        return false;
    }

    m_host = host;
    m_port = port;

    clearSession();

    return m_connection->beginConnect(host, port);
}

NetworkManager::ConnectProgress NetworkManager::pollConnect()
{
    if (!m_connection)
        return ConnectProgress::Failed;

    switch (m_connection->pollConnect())
    {
    case Connection::ConnectProgress::Pending:
        return ConnectProgress::Pending;

    case Connection::ConnectProgress::Failed:
        return ConnectProgress::Failed;

    case Connection::ConnectProgress::Connected:
        break;
    }

    // The socket is up. The handshake is the first frame of a session and has
    // to go out before the caller treats the connection as usable, so it is
    // sent here rather than left to the caller to remember.
    if (!sendHandshake())
    {
        m_connection->disconnect();
        return ConnectProgress::Failed;
    }

    return ConnectProgress::Connected;
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

bool NetworkManager::sendInventoryRequest()
{
    if (!isConnected())
        return false;

    auto packet = std::make_shared<InventoryPacket>();

    // The server answers for the connection's own player and ignores both
    // fields; they are set only so nothing goes out uninitialised.
    packet->PlayerID = m_playerId;

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

bool NetworkManager::sendBlockBreak(int32_t tileX, int32_t tileY)
{
    if (!isConnected())
        return false;

    auto packet = std::make_shared<BlockBreakPacket>();
    packet->X = tileX;
    packet->Y = tileY;
    packet->Z = 0;          // foreground layer
    packet->ToolID = 0;     // bare hands until tools are selectable
    packet->Face = 0;

    return sendPacket(packet);
}

bool NetworkManager::sendBlockPlace(int32_t tileX, int32_t tileY, uint16_t itemId)
{
    if (!isConnected())
        return false;

    auto packet = std::make_shared<BlockPlacePacket>();
    packet->X = tileX;
    packet->Y = tileY;
    packet->Z = 0;
    packet->ItemID = itemId;
    packet->Face = 0;

    return sendPacket(packet);
}

void NetworkManager::recordTileEdit(int32_t tileX, int32_t tileY, uint8_t tileId)
{
    constexpr int32_t kChunkSize = 16;

    // Floor division, not truncation: a negative coordinate would otherwise
    // land in the wrong chunk. The server should never send one, but this is
    // network input and the cost of being right is a comparison.
    const int32_t chunkX = (tileX >= 0) ? tileX / kChunkSize
                                        : ((tileX + 1) / kChunkSize) - 1;
    const int32_t chunkY = (tileY >= 0) ? tileY / kChunkSize
                                        : ((tileY + 1) / kChunkSize) - 1;

    auto it = m_Terrain.find(terrainKey(chunkX, chunkY));
    if (it != m_Terrain.end())
    {
        const int32_t localX = tileX - chunkX * kChunkSize;
        const int32_t localY = tileY - chunkY * kChunkSize;
        const std::size_t index =
            static_cast<std::size_t>(localY) * kChunkSize + static_cast<std::size_t>(localX);

        if (index < it->second.tiles.size())
        {
            it->second.tiles[index] = tileId;
        }
    }

    m_PendingTileEdits.push_back(TileEdit{tileX, tileY, tileId});
    ++m_TerrainRevision;
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

    m_Inventory.clear();
    ++m_InventoryRevision;

    m_Stats = CharacterStats{};
    ++m_StatsRevision;

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
