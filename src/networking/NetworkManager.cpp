#include "NetworkManager.h"

#include "ChatMessagePacket.h"
#include "DisconnectPacket.h"
#include "HandshakePacket.h"
#include "InventoryPacket.h"
#include "InventoryUpdatePacket.h"
#include "LoginPacket.h"
#include "PacketRegistry.h"
#include "PingPacket.h"
#include "PlayerBuffsPacket.h"
#include "PlayerDataPacket.h"
#include "UseItemPacket.h"
#include "WorldListPacket.h"
#include "StrixCorePacket.h"
#include "WorldManagePackets.h"
#include "PlayerMovePacket.h"
#include "PlayerRemovePacket.h"
#include "PlayerSpawnPacket.h"
#include "PongPacket.h"
#include "RegisterPacket.h"
#include "WorldJoinPacket.h"
#include "WorldLeavePacket.h"
#include "BlockBreakPacket.h"
#include "BlockPlacePacket.h"
#include "TileChangePacket.h"
#include "ChunkLoadPacket.h"
#include "WorldStatePacket.h"
#include "../core/Logger.h"
#include "../core/ServiceLocator.h"
#include "../core/Engine.h"
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

                // The chunk is fully applied to the store; the loading screen
                // watches this counter for its progress bar.
                m_ChunksReceived.fetch_add(1, std::memory_order_relaxed);
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
                recordTileEdit(broken->X, broken->Y, 0, broken->Z);   // 0 = air
            }));

    m_dispatcher->addHandler(
        Opcode::BlockPlace,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* placed = static_cast<const BlockPlacePacket*>(packet.get());

                // The server says which tile it wrote. It used to send only the
                // item id, which this treated as a tile id and clamped above
                // 255 - and the two are different spaces, so Dirt (item 1000,
                // tile 1) drew as tile 255, the unknown-tile grey. Every block
                // anyone placed was the wrong colour.
                const uint16_t tileId = placed->TileID;
                if (tileId == 0 || tileId > 255)
                {
                    Logger::Warning(std::format(
                        "NetworkManager: placement at ({}, {}) carried tile id {}, which this "
                        "client cannot store; ignored.", placed->X, placed->Y, tileId));
                    return;
                }

                recordTileEdit(placed->X, placed->Y, static_cast<uint8_t>(tileId),
                               placed->Z);
            }));

    // Everything the world changes on its own. Block place and break carry a
    // player's own edits; this carries the rest -- a seed becoming a sapling, a
    // plant maturing into its block, and whatever later systems change without
    // anyone having clicked.
    //
    // There was no handler and no packet class for this at all, so every one of
    // those changes arrived and was discarded. Planting has worked end to end
    // on the server for some time and simply never appeared on screen.
    m_dispatcher->addHandler(
        Opcode::TileChange,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* change = static_cast<const TileChangePacket*>(packet.get());

                // Same clamp as BlockPlace, and for the same reason: a tile id
                // is one byte in this client's storage, so anything outside
                // that range would silently become a different tile.
                if (change->TileID == 0 || change->TileID > 255)
                {
                    Logger::Warning(std::format(
                        "NetworkManager: tile change at ({}, {}) carried tile id {}, which this "
                        "client cannot store; ignored.",
                        change->X, change->Y, change->TileID));
                    return;
                }

                recordTileEdit(change->X, change->Y,
                               static_cast<uint8_t>(change->TileID),
                               change->Z);
            }));

    // The server confirming we are out of the world. Sent only on success; a
    // refusal is silence, which is why nothing here assumes the press worked
    // and the screen waits on this counter instead.
    m_dispatcher->addHandler(
        Opcode::WorldLeave,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>&)
            {
                // The session stays authenticated -- the server despawns the
                // player but keeps the connection logged in -- so rejoining
                // needs no fresh handshake, only another WorldJoin.
                ++m_WorldLeftRevision;
                Logger::Info("NetworkManager: left the world; the session is still open.");
            }));

    // The roster is maintained here so it survives the screen change between
    // joining a world and the gameplay screen being built.
    m_dispatcher->addHandler(
        Opcode::PlayerSpawn,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* spawn = static_cast<const PlayerSpawnPacket*>(packet.get());

                // A packet whose version this build does not recognise leaves
                // every field at its default. Entering that in the roster would
                // put a nameless player at tile (0,0) - the same reasoning as
                // the PlayerData handler below.
                if (!spawn->Valid)
                {
                    Logger::Warning("NetworkManager: ignoring PlayerSpawn in an "
                                    "unrecognised wire format.");
                    return;
                }

                // Never enter ourselves in the roster of other players. The
                // server excludes the joiner from its own spawn broadcast
                // today, so this has not bitten - but the roster is what the
                // rest of the client uses to answer "is this someone else?",
                // and one such packet would put a second copy of this player
                // in the world and start treating our own corrections as
                // somebody else's movement.
                if (isSelf(spawn->EntityID))
                    return;

                RemotePlayer& entry = m_RemotePlayers[spawn->EntityID];
                entry.id       = spawn->EntityID;
                entry.username = spawn->Username;
                entry.tileX    = spawn->X;
                entry.tileY    = spawn->Y;
                entry.worldRole = spawn->WorldRole;

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

                // Our own id here is the server correcting us, and the
                // roster must never gain an entry for ourselves - that would
                // draw a second copy of this player standing where the server
                // thinks we are. Asking outright is better than the previous
                // "an id I do not recognise must be me", which was only true
                // while the roster happened to be complete.
                if (isSelf(move->PlayerID))
                    return;

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

                // A packet whose version this build does not recognise leaves
                // every field at its default. Acting on those would show a
                // level-0 character with no health and spawn them at tile 0,0,
                // which is worse than showing nothing.
                if (!data->Valid)
                {
                    Logger::Warning("NetworkManager: ignoring PlayerData in an "
                                    "unrecognised wire format.");
                    return;
                }

                // Only our own character. The same packet is broadcast to
                // everyone else when a player restyles themselves, so without
                // this a client adopted the restyling player's level, health
                // and position as its own.
                //
                // LoginSuccess established our entity id long before this
                // arrives. Falling back to accepting the first packet only
                // matters if it somehow did not, and is better than dropping
                // our own stats on the floor.
                if (m_entityId != 0 && data->CharacterID != m_entityId)
                {
                    return;
                }

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

                // The authoritative spawn. Kept here rather than on the
                // gameplay screen because this packet arrives while the
                // loading screen is still up, seconds before that screen
                // exists - the same reason the roster and inventory live here.
                m_Stats.tileX      = static_cast<float>(data->TileX);
                m_Stats.tileY      = static_cast<float>(data->TileY);
                m_Stats.hasPosition = true;

                ++m_StatsRevision;

                Logger::Info(std::format("NetworkManager: stats - level {}, xp {}, hp {}/{}, "
                                         "at tile {},{}.",
                                         data->Level, data->Experience,
                                         data->Health, data->MaxHealth,
                                         data->TileX, data->TileY));
            }));

    // What is running on this player, as a complete replacement. The server
    // sends the whole set on every change, so there is nothing to merge.
    m_dispatcher->addHandler(
        Opcode::PlayerBuffs,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* buffs = static_cast<const PlayerBuffsPacket*>(packet.get());

                if (!buffs->Valid)
                {
                    Logger::Warning("NetworkManager: ignoring a buff set in an "
                                    "unrecognised wire format.");
                    return;
                }

                m_Buffs.clear();
                m_Buffs.reserve(buffs->Buffs.size());

                for (const auto& entry : buffs->Buffs)
                {
                    BuffState state;
                    state.id   = entry.Id;
                    state.name = entry.Name;
                    state.remainingSeconds =
                        static_cast<float>(entry.RemainingMs) / 1000.0f;
                    state.totalSeconds =
                        static_cast<float>(entry.TotalMs) / 1000.0f;
                    m_Buffs.push_back(std::move(state));
                }

                ++m_BuffRevision;
            }));

    m_dispatcher->addHandler(
        Opcode::WorldList,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* list = static_cast<const WorldListPacket*>(packet.get());

                if (!list->Valid)
                {
                    Logger::Warning("NetworkManager: ignoring a world list in an "
                                    "unrecognised wire format.");
                    return;
                }

                std::vector<WorldInfo> worlds;
                worlds.reserve(list->Worlds.size());

                for (const auto& entry : list->Worlds)
                {
                    WorldInfo info;
                    info.name           = entry.Name;
                    info.players        = static_cast<int>(entry.Players);
                    info.owner          = entry.OwnerName;
                    info.protectedWorld = entry.IsProtected();
                    info.allowsVisitors = entry.AllowsVisitors();

                    // Type, description and capacity are still not modelled on
                    // the server, so they are left empty rather than invented -
                    // the browser already renders a world without them.
                    worlds.push_back(std::move(info));
                }

                // Reached through the Engine, which owns it - WorldManager is
                // not itself in the ServiceLocator, so looking it up there
                // silently returned null and the catalogue went nowhere.
                if (auto engine = ServiceLocator::Get<Engine>())
                {
                    if (WorldManager* worldManager = engine->GetWorldManager())
                        worldManager->SetAvailableWorlds(std::move(worlds));
                }
            }));

    m_dispatcher->addHandler(
        Opcode::StrixCoreUpdated,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* core = static_cast<const StrixCoreUpdatedPacket*>(packet.get());
                if (!core->Valid)
                    return;

                // The tile swap that makes the claim visible. Recorded through
                // the same path as any other world edit, so GameScreen applies
                // it on its next update whether or not it existed when this
                // arrived.
                if (core->TileID > 0 && core->TileID <= 255)
                    recordTileEdit(core->X, core->Y, static_cast<uint8_t>(core->TileID));

                m_Core.known         = true;
                m_Core.tileX         = core->X;
                m_Core.tileY         = core->Y;
                m_Core.level         = core->CoreLevel;
                m_Core.protectionOn  = core->ProtectionOn != 0;
                m_Core.viewerIsOwner = core->ViewerIsOwner != 0;
                m_Core.ownerName     = core->OwnerName;
                ++m_CoreRevision;

                Logger::Info(std::format(
                    "NetworkManager: Strix Core at ({}, {}) is level {}, owned by '{}'{}.",
                    core->X, core->Y, static_cast<int>(core->CoreLevel),
                    core->OwnerName, core->ViewerIsOwner ? " (that is us)" : ""));
            }));

    // The world, and what this client may do in it. Everything here is the
    // server's answer; nothing is derived locally. The revision counter is what
    // GameScreen watches to refresh the management panel, matching how stats
    // and inventory already work.
    m_dispatcher->addHandler(
        Opcode::WorldInfo,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* info = static_cast<const WorldInfoPacket*>(packet.get());
                if (!info->Valid)
                    return;

                m_WorldManage.valid         = true;
                m_WorldManage.worldName     = info->WorldName;
                m_WorldManage.ownerName     = info->OwnerName;
                m_WorldManage.coreLevel     = info->CoreLevel;
                m_WorldManage.coreX         = info->CoreX;
                m_WorldManage.coreY         = info->CoreY;
                m_WorldManage.protectionOn  = info->ProtectionOn  != 0;
                m_WorldManage.allowBuilding = info->AllowBuilding != 0;
                m_WorldManage.allowBreaking = info->AllowBreaking != 0;
                m_WorldManage.allowVisitors = info->AllowVisitors != 0;
                m_WorldManage.memberCount   = info->MemberCount;
                m_WorldManage.viewerRole    = info->ViewerRole;
                m_WorldManage.canManage     = info->ViewerCanManage != 0;
                m_WorldManage.isOwner       = info->ViewerIsOwner   != 0;
                ++m_WorldInfoRevision;

                Logger::Info(std::format(
                    "NetworkManager: world '{}' owner '{}' level {} protection {} "
                    "members {} (we are role {}{})",
                    info->WorldName, info->OwnerName, static_cast<int>(info->CoreLevel),
                    info->ProtectionOn ? "on" : "off", info->MemberCount,
                    static_cast<int>(info->ViewerRole),
                    info->ViewerCanManage ? ", may manage" : ""));
            }));

    // The roster. Only sent to someone whose role may see it, so an arrival is
    // itself the server saying this client may act on the list.
    // The server's own voice: every management reply, plus the world-saved
    // and protection-toggled notices. Arrives here rather than through player
    // chat, which is what stops a refusal rendering as "Player 0: ...".
    //
    // Delivered to the registered handler, or queued when the HUD has not
    // bound itself yet.
    m_dispatcher->addHandler(
        Opcode::WorldNotification,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* notice =
                    static_cast<const WorldNotificationPacket*>(packet.get());
                if (!notice->Valid || notice->Message.empty())
                    return;

                // Straight into the queue the HUD already drains, so a notice
                // that arrives before the HUD exists is not lost.
                pushNotification(notice->Message, static_cast<int>(notice->Severity));
            }));

    m_dispatcher->addHandler(
        Opcode::WorldMembers,
        std::make_shared<FunctionPacketHandler>(
            [this](const std::shared_ptr<Packet>& packet)
            {
                const auto* roster = static_cast<const WorldMembersPacket*>(packet.get());
                if (!roster->Valid)
                    return;

                m_WorldMembers.clear();
                for (const auto& entry : roster->Members)
                    m_WorldMembers.push_back({entry.Username, entry.Role});

                m_WorldBans.clear();
                for (const auto& entry : roster->Bans)
                    m_WorldBans.push_back({entry.Username, entry.Role});

                ++m_WorldMembersRevision;

                Logger::Info(std::format(
                    "NetworkManager: world '{}' roster - {} member(s), {} ban(s)",
                    roster->WorldName, m_WorldMembers.size(), m_WorldBans.size()));
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

bool NetworkManager::sendLogin(const std::string& username, const std::string& password,
                               const std::string& totpCode)
{
    auto packet = std::make_shared<LoginPacket>();
    packet->Username = username;
    packet->Password = password;
    packet->TotpCode = totpCode;

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

    // The loading screen reads the chunk counters from this join onwards.
    ResetChunkProgress();

    auto packet = std::make_shared<WorldJoinPacket>();

    // The server ignores the client-supplied id and spawn point and uses the
    // session's authenticated player and the world's own spawn.
    packet->PlayerID  = m_entityId;
    packet->WorldName = worldName;
    packet->SpawnX    = 0.0f;
    packet->SpawnY    = 0.0f;

    return sendPacket(packet);
}

bool NetworkManager::sendClaimStrixCore(int32_t tileX, int32_t tileY)
{
    if (!isConnected())
        return false;

    auto packet = std::make_shared<ClaimStrixCorePacket>();
    packet->X = tileX;
    packet->Y = tileY;
    return sendPacket(packet);
}

bool NetworkManager::sendInteractStrixCore(int32_t tileX, int32_t tileY)
{
    if (!isConnected())
        return false;

    auto packet = std::make_shared<InteractStrixCorePacket>();
    packet->X = tileX;
    packet->Y = tileY;
    return sendPacket(packet);
}

bool NetworkManager::sendInviteWorldMember(const std::string& username, uint8_t role)
{
    if (!isConnected() || username.empty())
        return false;

    auto packet = std::make_shared<InviteWorldMemberPacket>();
    packet->Username = username;
    packet->Role     = role;
    return sendPacket(packet);
}

bool NetworkManager::sendRemoveWorldMember(const std::string& username)
{
    if (!isConnected() || username.empty())
        return false;

    auto packet = std::make_shared<RemoveWorldMemberPacket>();
    packet->Username = username;
    return sendPacket(packet);
}

bool NetworkManager::sendChangeWorldRole(const std::string& username, uint8_t role)
{
    if (!isConnected() || username.empty())
        return false;

    auto packet = std::make_shared<ChangeWorldRolePacket>();
    packet->Username = username;
    packet->Role     = role;
    return sendPacket(packet);
}

bool NetworkManager::sendSetWorldSettings(bool protectionOn, bool allowBuilding,
                                          bool allowBreaking, bool allowVisitors)
{
    if (!isConnected())
        return false;

    auto packet = std::make_shared<SetWorldSettingsPacket>();
    packet->ProtectionOn  = protectionOn  ? 1 : 0;
    packet->AllowBuilding = allowBuilding ? 1 : 0;
    packet->AllowBreaking = allowBreaking ? 1 : 0;
    packet->AllowVisitors = allowVisitors ? 1 : 0;
    return sendPacket(packet);
}

bool NetworkManager::sendBanWorldPlayer(const std::string& username, bool banned,
                                        const std::string& reason)
{
    if (!isConnected() || username.empty())
        return false;

    auto packet = std::make_shared<BanWorldPlayerPacket>();
    packet->Username = username;
    packet->Banned   = banned ? 1 : 0;
    packet->Reason   = reason;
    return sendPacket(packet);
}

bool NetworkManager::sendWorldListRequest()
{
    if (!isConnected())
        return false;

    return sendPacket(std::make_shared<WorldListRequestPacket>());
}

bool NetworkManager::sendInventoryRequest()
{
    if (!isConnected())
        return false;

    auto packet = std::make_shared<InventoryPacket>();

    // The server answers for the connection's own player and ignores both
    // fields; they are set only so nothing goes out uninitialised.
    packet->PlayerID = m_entityId;

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
    packet->SenderID = m_entityId;
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
    packet->PlayerID  = m_entityId;
    packet->X         = tileX;
    packet->Y         = tileY;
    packet->Z         = 0.0f;
    packet->VelocityX = velocityX;
    packet->VelocityY = velocityY;
    packet->VelocityZ = 0.0f;

    return sendPacket(packet);
}

bool NetworkManager::sendBlockBreak(int32_t tileX, int32_t tileY, uint16_t toolItemId,
                                    int32_t tileZ)
{
    if (!isConnected())
        return false;

    auto packet = std::make_shared<BlockBreakPacket>();
    packet->X = tileX;
    packet->Y = tileY;
    packet->Z = tileZ;
    packet->ToolID = toolItemId;
    packet->Face = 0;

    return sendPacket(packet);
}

bool NetworkManager::sendBlockPlace(int32_t tileX, int32_t tileY, uint16_t itemId,
                                    int32_t tileZ)
{
    if (!isConnected())
        return false;

    auto packet = std::make_shared<BlockPlacePacket>();
    packet->X = tileX;
    packet->Y = tileY;
    packet->Z = tileZ;
    packet->ItemID = itemId;
    packet->Face = 0;

    return sendPacket(packet);
}

bool NetworkManager::sendUseItem(uint8_t inventorySlot, uint16_t itemId)
{
    if (!isConnected())
        return false;

    auto packet = std::make_shared<UseItemPacket>();

    // PlayerID is left at zero deliberately. The server resolves who is asking
    // from the connection, and a client-supplied id would be a value it must
    // not trust - the same reason block edits do not carry one.
    packet->SlotIndex = inventorySlot;
    packet->ItemID    = itemId;
    packet->Quantity  = 1;

    return sendPacket(packet);
}

void NetworkManager::recordTileEdit(int32_t tileX, int32_t tileY, uint8_t tileId,
                                    int32_t tileZ)
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

    m_PendingTileEdits.push_back(TileEdit{tileX, tileY, tileZ, tileId});
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

void NetworkManager::SetNotificationHandler(
    std::function<void(const std::string& message, int severity)> handler)
{
    m_NotificationHandler = std::move(handler);

    // Anything that queued up before a handler existed is delivered now,
    // oldest first, so the notices keep their order.
    while (!m_PendingNotifications.empty())
    {
        PendingNotification pending = std::move(m_PendingNotifications.front());
        m_PendingNotifications.pop_front();

        if (m_NotificationHandler)
            m_NotificationHandler(pending.message, pending.severity);
    }
}

bool NetworkManager::PopPendingNotification(std::string& out, int& severity)
{
    if (m_PendingNotifications.empty())
        return false;

    PendingNotification pending = std::move(m_PendingNotifications.front());
    m_PendingNotifications.pop_front();

    out      = std::move(pending.message);
    severity = pending.severity;
    return true;
}

void NetworkManager::pushNotification(const std::string& message, int severity)
{
    if (m_NotificationHandler)
    {
        m_NotificationHandler(message, severity);
        return;
    }

    if (m_PendingNotifications.size() >= kMaxPendingNotifications)
        m_PendingNotifications.pop_front();

    m_PendingNotifications.push_back({message, severity});
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

void NetworkManager::setSession(uint64_t entityId,
                                const std::string& username,
                                const std::string& token)
{
    m_entityId      = entityId;
    m_username      = username;
    m_sessionToken  = token;
    m_authenticated = true;

    Logger::Info(std::format("NetworkManager: session established for '{}' (entity {}).",
                             username, entityId));
}

void NetworkManager::clearSession()
{
    m_WorldConfirmed = false;
    m_CurrentWorld.clear();
    m_WorldTimeOfDay = 0;
    m_RemotePlayers.clear();

    m_Inventory.clear();
    ++m_InventoryRevision;

    m_Buffs.clear();
    ++m_BuffRevision;

    m_Core = CoreState{};
    ++m_CoreRevision;

    m_Stats = CharacterStats{};
    ++m_StatsRevision;

    m_authenticated = false;
    m_entityId      = 0;
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
