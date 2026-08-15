#include "GameScreen.h"

#include "../core/AssetManager.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../core/ServiceLocator.h"
#include "../ecs/Camera2DComponent.h"
#include "../ecs/ColliderComponent.h"
#include "../ecs/CollisionSystem.h"
#include "../ecs/ComponentManager.h"
#include "../ecs/EntityManager.h"
#include "../ecs/InputComponent.h"
#include "../ecs/PlayerComponent.h"
#include "../ecs/SpriteComponent.h"
#include "../ecs/SystemManager.h"
#include "../ecs/TransformComponent.h"
#include "../ecs/VelocityComponent.h"
#include "../graphics/Texture.h"
#include "../ecs/NetworkComponent.h"
#include "../networking/ChatMessagePacket.h"
#include "../networking/NetworkManager.h"
#include "../networking/PlayerMovePacket.h"
#include "../networking/PlayerRemovePacket.h"
#include "../networking/PlayerSpawnPacket.h"
#include "../ui/UIButton.h"
#include "../ui/UILabel.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

#include <cmath>
#include <cstdlib>
#include <format>

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    // One tile on screen, in world units. The renderer and the collision system
    // both need this and must be given the same value.
    constexpr float kTileSize = 32.0f;

    // The local player's collision box, as a fraction of a tile.
    constexpr float kPlayerWidth  = kTileSize * 0.75f;
    constexpr float kPlayerHeight = kTileSize * 1.5f;

    // Position updates are sent at this rate while moving. The server's limit
    // is 12 tiles per second, and the player covers about 3, so this leaves a
    // wide margin under the speed check.
    constexpr float kMoveSendInterval = 0.1f;

    // Movement below this is not worth a packet.
    constexpr float kMoveEpsilonTiles = 0.02f;
}

GameScreen::GameScreen(Engine* engine)
    : Screen(engine)
{
}

void GameScreen::OnEnter()
{
    if (!uiManager_)
    {
        LOG_ERROR("GameScreen: UIManager not available");
        return;
    }

    CreateRoot();

    InitializeUI();
    InitializeHUD();
    InitializeWorld();
    InitializeActors();

    // Anyone already in the world was announced while the loading screen was
    // up, seconds before this screen existed. NetworkManager kept that roster,
    // so they are spawned here rather than being lost. They are not announced
    // as joining - they were already present.
    if (engine_)
    {
        for (const auto& [id, player] : engine_->getNetworkManager().getRemotePlayers())
            OnPlayerSpawn(id, player.username, player.tileX, player.tileY, false);
    }
}

void GameScreen::InitializeUI()
{
    const float originX = DesignOriginX();
    const float originY = DesignOriginY();

    const std::string worldName = engine_ ? engine_->GetSelectedWorldName() : std::string();

    // The HUD occupies the top-left stat column and the top-right chat panel,
    // so the screen's own chrome sits along the bottom edge to avoid it.
    worldLabel_ = std::make_shared<UILabel>();
    worldLabel_->setText(worldName.empty() ? "StrixVerse" : worldName);
    worldLabel_->setFont(DisplayFont(UITheme::Display::Label));
    worldLabel_->setTextColor(UITheme::Text);
    worldLabel_->setAlignment(UILabel::Alignment::Center);
    worldLabel_->setPosition(originX, originY + UIScale::kDesignHeight - S(46.0f));
    worldLabel_->setSize(UIScale::kDesignWidth, S(14.0f));
    root_->addChild(worldLabel_);

    const float buttonWidth  = S(90.0f);
    const float buttonHeight = S(24.0f);

    settingsButton_ = std::make_shared<UIButton>();
    settingsButton_->setText("SETTINGS");
    settingsButton_->setFont(DisplayFont(UITheme::Display::Small));
    settingsButton_->setVariant(UIButton::Variant::Purple);
    settingsButton_->setPosition(originX + UIScale::kDesignWidth - buttonWidth - S(20.0f),
                                 originY + UIScale::kDesignHeight - buttonHeight - S(20.0f));
    settingsButton_->setSize(buttonWidth, buttonHeight);
    settingsButton_->setOnClick([this]() { OnSettingsButtonClicked(); });
    root_->addChild(settingsButton_);
}

void GameScreen::InitializeHUD()
{
    hud_ = std::make_unique<HUD>(engine_);
    hud_->Initialize();

    // Outbound: what the player types goes to the server, and is echoed here
    // because the server broadcasts to everyone except the sender.
    hud_->SetChatSubmitHandler(
        [this](const std::string& message) { SubmitChat(message); });

    RegisterNetworkHandlers();

    // Placeholder values until the server drives these.
    hud_->SetHealth(100.0f, 100.0f);
    hud_->SetMana(50.0f, 50.0f);
    hud_->SetLevel(5);
    hud_->SetExperience(350, 500);
    hud_->SetCoins(150);
    hud_->SetGems(5);
    hud_->AddChatMessage("Welcome to StrixVerse!");
}

void GameScreen::InitializeWorld()
{
    auto entityManager    = ServiceLocator::Get<StrixVerse::ECS::EntityManager>();
    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();
    auto systemManager    = ServiceLocator::Get<StrixVerse::ECS::SystemManager>();

    if (!entityManager || !componentManager || !systemManager)
    {
        LOG_ERROR("GameScreen: Failed to get ECS managers from ServiceLocator");
        return;
    }

    (void)entityManager;
    (void)componentManager;

    // The world starts empty and stays empty. There is no sample terrain here:
    // its contents come from a generator or from the server's chunk packets,
    // and inventing tiles would only mask which of those is actually running.
    //
    // >>> Plug a world generator in here. Anything that fills this World is
    //     drawn automatically - TileRendererSystem reads it every frame, so no
    //     further wiring is needed. For example:
    //
    //         world_->GenerateFlatWorld(4, 4, 1, Tile::Type::Grass);
    //
    world_ = std::make_unique<StrixVerse::World::World>();

    // The tile renderer belongs to the Engine and outlives this screen; the
    // screen only lends it the world to draw.
    if (auto tileRenderer = systemManager->getSystem<StrixVerse::ECS::TileRendererSystem>())
    {
        tileRenderer->SetWorld(world_.get());
        tileRenderer->SetTileSize(kTileSize);
    }
    else
    {
        LOG_ERROR("GameScreen: TileRendererSystem is not registered; tiles will not draw");
    }

    // Collision reads the same world, and must agree with the renderer about
    // how big a tile is or the two disagree about where the walls are.
    if (auto collision = systemManager->getSystem<StrixVerse::ECS::CollisionSystem>())
    {
        collision->SetWorld(world_.get());
        collision->SetTileSize(kTileSize);
    }
    else
    {
        LOG_ERROR("GameScreen: CollisionSystem is not registered; movement will be unbounded");
    }

    LOG_INFO(std::format("GameScreen: entered '{}' ({} x {} tiles)",
                         engine_ ? engine_->GetSelectedWorldName() : std::string(),
                         world_->GetWidthInTiles(),
                         world_->GetHeightInTiles()));
}

void GameScreen::FindFreeSpawn(float& x, float& y) const
{
    auto systemManager = ServiceLocator::Get<StrixVerse::ECS::SystemManager>();
    if (!systemManager)
        return;

    auto collision = systemManager->getSystem<StrixVerse::ECS::CollisionSystem>();
    if (!collision)
        return;

    if (!collision->IsAreaBlocked(x, y, kPlayerWidth, kPlayerHeight))
        return;

    // The middle of the world can easily be inside water. Search outwards in
    // rings for the nearest tile the player actually fits in, rather than
    // spawning them somewhere they cannot move out of.
    constexpr int kMaxRings = 64;

    const float startX = x;
    const float startY = y;

    for (int ring = 1; ring <= kMaxRings; ++ring)
    {
        for (int dy = -ring; dy <= ring; ++dy)
        {
            for (int dx = -ring; dx <= ring; ++dx)
            {
                // Only the perimeter of this ring is new.
                if (std::abs(dx) != ring && std::abs(dy) != ring)
                    continue;

                const float candidateX = startX + static_cast<float>(dx) * kTileSize;
                const float candidateY = startY + static_cast<float>(dy) * kTileSize;

                if (!collision->IsAreaBlocked(candidateX, candidateY, kPlayerWidth, kPlayerHeight))
                {
                    x = candidateX;
                    y = candidateY;
                    return;
                }
            }
        }
    }

    LOG_WARN("GameScreen: no clear spawn found near the world centre");
}

void GameScreen::InitializeActors()
{
    auto entityManager    = ServiceLocator::Get<StrixVerse::ECS::EntityManager>();
    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();

    if (!entityManager || !componentManager)
        return;

    AssetManager* assets = Assets();
    if (!assets)
        return;

    // A 1x1 white texture scaled to the sprite size and tinted per entity. The
    // player has no artwork yet, so this stands in without inventing an asset.
    // Remote players reuse it, so it is held for the screen's lifetime.
    playerTexture_ = assets->LoadTexture("assets/ui/textures/white.png");
    if (!playerTexture_)
    {
        LOG_ERROR("GameScreen: white texture unavailable; the player cannot be drawn");
        return;
    }

    const std::shared_ptr<Texture>& white = playerTexture_;

    // Spawn in the middle of the world rather than on its corner. An empty
    // world has no middle, so the origin is the fallback.
    float spawnX = 0.0f;
    float spawnY = 0.0f;

    if (world_ && world_->GetWidthInTiles() > 0 && world_->GetHeightInTiles() > 0)
    {
        spawnX = static_cast<float>(world_->GetWidthInTiles()) * 0.5f * kTileSize;
        spawnY = static_cast<float>(world_->GetHeightInTiles()) * 0.5f * kTileSize;

        FindFreeSpawn(spawnX, spawnY);
    }

    // --- Local player -----------------------------------------------------
    playerEntity_ = entityManager->createEntity();

    StrixVerse::ECS::Transform transform;
    transform.position.x = spawnX;
    transform.position.y = spawnY;
    // The texture is 1x1, so the scale is the on-screen size in pixels.
    transform.scale = {kPlayerWidth, kPlayerHeight};
    componentManager->addComponent<StrixVerse::ECS::Transform>(playerEntity_, transform);

    // The collider matches the sprite, so what you see is what stops you.
    StrixVerse::ECS::ColliderComponent collider;
    collider.width   = kPlayerWidth;
    collider.height  = kPlayerHeight;
    collider.enabled = true;
    componentManager->addComponent<StrixVerse::ECS::ColliderComponent>(playerEntity_, collider);

    StrixVerse::ECS::SpriteComponent sprite;
    sprite.textureID = white->GetRendererID();
    sprite.r = 0.31f;   // UITheme::Primary, so the player reads as "ours".
    sprite.g = 0.55f;
    sprite.b = 1.0f;
    sprite.a = 1.0f;
    sprite.layer = 10;  // Above the tiles.
    componentManager->addComponent<StrixVerse::ECS::SpriteComponent>(playerEntity_, sprite);

    componentManager->addComponent<StrixVerse::ECS::VelocityComponent>(
        playerEntity_, StrixVerse::ECS::VelocityComponent{});
    componentManager->addComponent<StrixVerse::ECS::InputComponent>(
        playerEntity_, StrixVerse::ECS::InputComponent{});

    StrixVerse::ECS::PlayerComponent player;
    player.username = engine_ ? engine_->GetSignedInUser() : std::string();
    componentManager->addComponent<StrixVerse::ECS::PlayerComponent>(playerEntity_, player);

    // --- Camera that follows the player -----------------------------------
    cameraEntity_ = entityManager->createEntity();

    componentManager->addComponent<StrixVerse::ECS::Transform>(
        cameraEntity_, StrixVerse::ECS::Transform{});

    StrixVerse::ECS::Camera2DComponent camera;
    camera.followTarget = true;
    camera.targetEntity = playerEntity_;
    camera.zoom         = 1.0f;
    componentManager->addComponent<StrixVerse::ECS::Camera2DComponent>(cameraEntity_, camera);

    LOG_INFO(std::format("GameScreen: spawned player entity {} with camera {}",
                         playerEntity_.getIndex(), cameraEntity_.getIndex()));
}

void GameScreen::DestroyActors()
{
    auto entityManager = ServiceLocator::Get<StrixVerse::ECS::EntityManager>();
    if (!entityManager)
        return;

    for (const auto& [id, entity] : remotePlayers_)
        entityManager->destroyEntity(entity);

    remotePlayers_.clear();
    remoteNames_.clear();

    for (StrixVerse::ECS::Entity* entity : {&playerEntity_, &cameraEntity_})
    {
        if (*entity != StrixVerse::ECS::NULL_ENTITY)
        {
            entityManager->destroyEntity(*entity);
            *entity = StrixVerse::ECS::NULL_ENTITY;
        }
    }

    playerTexture_.reset();
}

void GameScreen::Update(float deltaTime)
{
    if (hud_)
        hud_->Update(deltaTime);

    PublishLocalPosition(deltaTime);
}

void GameScreen::OnKeyDown(int key, bool, bool)
{
    // This only runs while nothing holds keyboard focus, so Enter here always
    // means "start typing" and never "send".
    if (key == UIKey::Enter)
    {
        if (hud_)
            hud_->FocusChatInput();
        return;
    }

    if (key == UIKey::Escape)
        OnSettingsButtonClicked();
}

void GameScreen::SubmitChat(const std::string& message)
{
    if (message.empty())
        return;

    const std::string author = engine_ && !engine_->GetSignedInUser().empty()
                                   ? engine_->GetSignedInUser()
                                   : std::string("You");

    if (!engine_)
        return;

    NetworkManager& network = engine_->getNetworkManager();

    if (!network.isConnected())
    {
        if (hud_)
            hud_->AddChatMessage("[not connected] " + message);
        return;
    }

    if (!network.sendChat(message))
    {
        if (hud_)
            hud_->AddChatMessage("[failed to send] " + message);
        return;
    }

    // The server broadcasts to every client except the sender, so the local
    // echo is what puts our own line in the log.
    if (hud_)
        hud_->AddChatMessage(author + ": " + message);
}

void GameScreen::OnChatReceived(uint64_t senderId, const std::string& message)
{
    if (!hud_ || message.empty())
        return;

    hud_->AddChatMessage(DisplayNameFor(senderId) + ": " + message);
}

std::string GameScreen::DisplayNameFor(uint64_t entityId) const
{
    // The chat packet carries only an id. PlayerSpawn is what supplies the
    // name, so anyone who joined before us - or whose spawn we missed - falls
    // back to the id rather than being shown a made-up name.
    const auto it = remoteNames_.find(entityId);
    return it != remoteNames_.end() ? it->second : std::format("Player {}", entityId);
}

void GameScreen::RegisterNetworkHandlers()
{
    if (!engine_)
        return;

    NetworkManager& network = engine_->getNetworkManager();

    chatHandler_ = std::make_shared<FunctionPacketHandler>(
        [this](const std::shared_ptr<Packet>& packet)
        {
            const auto* chat = static_cast<const ChatMessagePacket*>(packet.get());
            OnChatReceived(chat->SenderID, chat->Message);
        });
    network.addPacketHandler(Opcode::ChatMessage, chatHandler_);

    spawnHandler_ = std::make_shared<FunctionPacketHandler>(
        [this](const std::shared_ptr<Packet>& packet)
        {
            const auto* spawn = static_cast<const PlayerSpawnPacket*>(packet.get());
            OnPlayerSpawn(spawn->EntityID, spawn->Username, spawn->X, spawn->Y);
        });
    network.addPacketHandler(Opcode::PlayerSpawn, spawnHandler_);

    moveHandler_ = std::make_shared<FunctionPacketHandler>(
        [this](const std::shared_ptr<Packet>& packet)
        {
            const auto* move = static_cast<const PlayerMovePacket*>(packet.get());
            OnPlayerMove(move->PlayerID, move->X, move->Y);
        });
    network.addPacketHandler(Opcode::PlayerMove, moveHandler_);

    removeHandler_ = std::make_shared<FunctionPacketHandler>(
        [this](const std::shared_ptr<Packet>& packet)
        {
            const auto* remove = static_cast<const PlayerRemovePacket*>(packet.get());
            OnPlayerRemove(remove->EntityID);
        });
    network.addPacketHandler(Opcode::PlayerRemove, removeHandler_);
}

void GameScreen::UnregisterNetworkHandlers()
{
    if (!engine_)
        return;

    NetworkManager& network = engine_->getNetworkManager();

    if (chatHandler_)   network.removePacketHandler(Opcode::ChatMessage, chatHandler_);
    if (spawnHandler_)  network.removePacketHandler(Opcode::PlayerSpawn, spawnHandler_);
    if (moveHandler_)   network.removePacketHandler(Opcode::PlayerMove, moveHandler_);
    if (removeHandler_) network.removePacketHandler(Opcode::PlayerRemove, removeHandler_);

    chatHandler_.reset();
    spawnHandler_.reset();
    moveHandler_.reset();
    removeHandler_.reset();
}

void GameScreen::OnPlayerSpawn(uint64_t entityId, const std::string& username,
                               float tileX, float tileY, bool announce)
{
    auto entityManager    = ServiceLocator::Get<StrixVerse::ECS::EntityManager>();
    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();

    if (!entityManager || !componentManager || !playerTexture_)
        return;

    remoteNames_[entityId] = username;

    // A re-spawn for someone already present just moves them.
    if (const auto existing = remotePlayers_.find(entityId); existing != remotePlayers_.end())
    {
        OnPlayerMove(entityId, tileX, tileY);
        return;
    }

    const float x = tileX * kTileSize;
    const float y = tileY * kTileSize;

    StrixVerse::ECS::Entity entity = entityManager->createEntity();

    StrixVerse::ECS::Transform transform;
    transform.position.x = x;
    transform.position.y = y;
    transform.scale      = {kPlayerWidth, kPlayerHeight};
    componentManager->addComponent<StrixVerse::ECS::Transform>(entity, transform);

    StrixVerse::ECS::SpriteComponent sprite;
    sprite.textureID = playerTexture_->GetRendererID();
    sprite.r = 0.42f;   // Purple, so other players read differently from us.
    sprite.g = 0.36f;
    sprite.b = 0.91f;
    sprite.a = 1.0f;
    sprite.layer = 9;   // Just under the local player.
    componentManager->addComponent<StrixVerse::ECS::SpriteComponent>(entity, sprite);

    // NetworkSyncSystem copies these onto the transform each frame; nothing
    // else drives a remote player, so it has no velocity or input.
    StrixVerse::ECS::NetworkComponent network;
    network.networkID     = entityId;
    network.isLocalPlayer = false;
    network.x             = x;
    network.y             = y;
    componentManager->addComponent<StrixVerse::ECS::NetworkComponent>(entity, network);

    remotePlayers_[entityId] = entity;

    if (announce && hud_)
        hud_->AddChatMessage(username + " joined.");

    LOG_INFO(std::format("GameScreen: '{}' (id {}) spawned at tile {:.1f},{:.1f}",
                         username, entityId, tileX, tileY));
}

void GameScreen::OnPlayerMove(uint64_t entityId, float tileX, float tileY)
{
    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();
    if (!componentManager)
        return;

    const auto it = remotePlayers_.find(entityId);

    if (it == remotePlayers_.end())
    {
        // The server never relays our own movement back to us - Broadcast
        // excludes the sender - so a PlayerMove for anyone outside the roster
        // is the correction it sends when it rejects a move of ours. The
        // roster is NetworkManager's, which has been current since before this
        // screen existed, so an unknown id here really is unknown.
        if (engine_ && engine_->getNetworkManager().isRemotePlayer(entityId))
            return;   // Known player, but no entity yet: its spawn is pending.

        ApplyServerCorrection(tileX, tileY);
        return;
    }

    if (auto* network = componentManager->getComponent<StrixVerse::ECS::NetworkComponent>(it->second))
    {
        network->x = tileX * kTileSize;
        network->y = tileY * kTileSize;
    }
}

void GameScreen::OnPlayerRemove(uint64_t entityId)
{
    auto entityManager = ServiceLocator::Get<StrixVerse::ECS::EntityManager>();

    const auto it = remotePlayers_.find(entityId);
    if (it == remotePlayers_.end())
        return;

    if (entityManager)
        entityManager->destroyEntity(it->second);

    if (hud_)
        hud_->AddChatMessage(DisplayNameFor(entityId) + " left.");

    remotePlayers_.erase(it);
    remoteNames_.erase(entityId);
}

void GameScreen::ApplyServerCorrection(float tileX, float tileY)
{
    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();

    if (!componentManager || playerEntity_ == StrixVerse::ECS::NULL_ENTITY)
        return;

    auto* transform = componentManager->getComponent<StrixVerse::ECS::Transform>(playerEntity_);
    if (!transform)
        return;

    transform->position.x = tileX * kTileSize;
    transform->position.y = tileY * kTileSize;

    // Do not immediately re-send the position we were just corrected to.
    lastSentTileX_ = tileX;
    lastSentTileY_ = tileY;
    hasSentMove_   = true;

    LOG_WARN(std::format("GameScreen: server corrected our position to tile {:.1f},{:.1f}",
                         tileX, tileY));
}

void GameScreen::PublishLocalPosition(float deltaTime)
{
    if (!engine_ || playerEntity_ == StrixVerse::ECS::NULL_ENTITY)
        return;

    NetworkManager& network = engine_->getNetworkManager();
    if (!network.isConnected())
        return;

    moveSendTimer_ += deltaTime;
    if (moveSendTimer_ < kMoveSendInterval)
        return;

    moveSendTimer_ = 0.0f;

    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();
    if (!componentManager)
        return;

    const auto* transform = componentManager->getComponent<StrixVerse::ECS::Transform>(playerEntity_);
    if (!transform)
        return;

    // The server stores tile coordinates and validates against tile bounds, so
    // the conversion has to happen here rather than on the wire.
    const float tileX = transform->position.x / kTileSize;
    const float tileY = transform->position.y / kTileSize;

    if (hasSentMove_ &&
        std::fabs(tileX - lastSentTileX_) < kMoveEpsilonTiles &&
        std::fabs(tileY - lastSentTileY_) < kMoveEpsilonTiles)
    {
        return;   // Standing still: nothing worth a packet.
    }

    const auto* velocity =
        componentManager->getComponent<StrixVerse::ECS::VelocityComponent>(playerEntity_);

    const float vx = velocity ? velocity->vx / kTileSize : 0.0f;
    const float vy = velocity ? velocity->vy / kTileSize : 0.0f;

    if (network.sendPlayerMove(tileX, tileY, vx, vy))
    {
        lastSentTileX_ = tileX;
        lastSentTileY_ = tileY;
        hasSentMove_   = true;
    }
}

void GameScreen::OnSettingsButtonClicked()
{
    RequestScreenChange(ScreenID::Settings);
}

void GameScreen::OnExit()
{
    // Unregister before anything else goes away: a packet arriving after this
    // point would otherwise reach a handler holding a dangling screen.
    UnregisterNetworkHandlers();

    hud_.reset();

    DestroyActors();

    // Both systems hold a raw pointer into the world, so they must stop
    // referencing it before the world is destroyed.
    if (auto systemManager = ServiceLocator::Get<StrixVerse::ECS::SystemManager>())
    {
        if (auto tileRenderer = systemManager->getSystem<StrixVerse::ECS::TileRendererSystem>())
            tileRenderer->SetWorld(nullptr);

        if (auto collision = systemManager->getSystem<StrixVerse::ECS::CollisionSystem>())
            collision->SetWorld(nullptr);
    }

    world_.reset();

    Screen::OnExit();
}
