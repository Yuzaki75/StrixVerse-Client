#include "GameScreen.h"

#include "../core/AssetManager.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../core/ServiceLocator.h"
#include "../ecs/Camera2DComponent.h"
#include "../ecs/Camera2DSystem.h"
#include "../ecs/ColliderComponent.h"
#include "../ecs/CollisionSystem.h"
#include "../ecs/ComponentManager.h"
#include "../ecs/EntityManager.h"
#include "../ecs/InputComponent.h"
#include "../ecs/InputSystem.h"
#include "../ecs/PlayerComponent.h"
#include "../ecs/PlayerSystem.h"
#include "../ecs/SpriteComponent.h"
#include "../ecs/CharacterComponent.h"
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
#include "../ui/UIElement.h"
#include "../ui/UILabel.h"
#include "../ui/UIManager.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

#include <SDL3/SDL.h>

#include <algorithm>
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

    // No gameplay track exists yet, so the menu music stops here rather than
    // looping underneath the game.
    if (engine_)
        engine_->GetAudio().StopMusic();

    CreateRoot();

    InitializeUI();
    InitializeHUD();

    // After the HUD, so the overlay draws above it.
    BuildPausePanel();
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

        // Ask for the inventory, and show whatever is already known in case
        // the reply landed while the loading screen was up.
        engine_->getNetworkManager().sendInventoryRequest();
        RefreshInventory();

        // Stats arrive with the world join, so they are usually already here.
        RefreshStats();
    }
}

void GameScreen::RefreshStats()
{
    if (!hud_ || !engine_)
        return;

    const NetworkManager::CharacterStats& source =
        engine_->getNetworkManager().getCharacterStats();

    HUD::Stats stats;
    stats.known                 = source.known;
    stats.level                 = source.level;
    stats.experience            = source.experience;
    stats.experienceToNextLevel = source.experienceToNextLevel;
    stats.health                = source.health;
    stats.maxHealth             = source.maxHealth;

    hud_->SetStats(stats);

    statsRevision_ = engine_->getNetworkManager().getStatsRevision();
}

void GameScreen::RefreshInventory()
{
    if (!hud_ || !engine_)
        return;

    const NetworkManager& network = engine_->getNetworkManager();

    std::vector<HUD::InventoryEntry> entries;
    entries.reserve(network.getInventory().size());

    for (const auto& [slot, item] : network.getInventory())
    {
        if (item.IsEmpty())
            continue;

        HUD::InventoryEntry entry;
        entry.slot     = slot;
        entry.itemId   = item.itemId;
        entry.quantity = item.quantity;
        entries.push_back(entry);
    }

    hud_->SetInventory(entries);

    inventoryRevision_ = network.getInventoryRevision();
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

    hud_->AddChatMessage("Welcome to StrixVerse!");
}

namespace
{
    // Server foreground tile id -> the client's five-way Tile::Type.
    //
    // The ids are WorldGenerator.cpp's. This mapping is no longer what decides
    // how a tile looks - the id is carried onto the Tile and TileRendererSystem
    // picks the sprite from it - so what remains here is the coarse grouping
    // used for the flat-colour fallback when an id has no art, and for
    // anything that reasons about a tile's kind rather than its identity.
    // Air is the one id with no tile at all: a null tile is
    // skipped by TileRendererSystem and treated as passable by
    // CollisionSystem, which is exactly what air should be.
    //
    // Returns false for air.
    bool ServerTileToClientType(std::uint8_t id, StrixVerse::World::Tile::Type& outType)
    {
        using Type = StrixVerse::World::Tile::Type;

        switch (id)
        {
        case 0:  return false;                      // air
        case 1:  outType = Type::Dirt;   return true;
        case 2:  outType = Type::Stone;  return true;
        case 3:  outType = Type::Grass;  return true;
        case 4:  outType = Type::Dirt;   return true;  // wood
        case 5:  outType = Type::Grass;  return true;  // leaves
        case 6:  outType = Type::Stone;  return true;  // bedrock
        case 13: outType = Type::Water;  return true;  // lava

        // A planted seed while it grows. Without this it fell through to the
        // default and every sapling rendered as grey stone, which makes
        // planting look broken even though the server accepted it.
        case 19: outType = Type::Grass;  return true;  // sapling

        default: outType = Type::Stone;  return true;  // ores and anything new
        }
    }
} // namespace

bool GameScreen::CanvasToServerTile(float canvasX, float canvasY,
                                    int32_t& outTileX, int32_t& outTileY) const
{
    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();
    if (!componentManager || !world_)
        return false;

    const auto* transform =
        componentManager->getComponent<StrixVerse::ECS::Transform>(playerEntity_);
    if (!transform)
        return false;

    // The camera follows the player at zoom 1, so the centre of the design
    // canvas is the player and the mapping is one canvas pixel to one world
    // pixel. If zoom ever stops being 1 this has to divide by it.
    const float worldPixelX =
        transform->position.x + (canvasX - UIScale::kDesignWidth  * 0.5f);
    const float worldPixelY =
        transform->position.y + (canvasY - UIScale::kDesignHeight * 0.5f);

    const int localRowX = static_cast<int>(std::floor(worldPixelX / kTileSize));
    const int localRowY = static_cast<int>(std::floor(worldPixelY / kTileSize));

    // Back into the server's Y-up space, the inverse of TileYToLocalY.
    outTileX = static_cast<int32_t>(localRowX);
    outTileY = static_cast<int32_t>((worldHeightInTiles_ - 1) - localRowY);
    return true;
}

void GameScreen::BuildPausePanel()
{
    if (pausePanel_ || !uiManager_)
        return;

    // Built once and hidden, not created per Escape. Added to the UIManager
    // after the HUD so it draws above it; UIManager renders in insertion order.
    const float width  = S(260.0f);
    const float height = S(170.0f);

    pausePanel_ = std::make_shared<UIPanel>();
    pausePanel_->setSize(width, height);
    pausePanel_->setPosition((UIScale::kDesignWidth - width) * 0.5f,
                             (UIScale::kDesignHeight - height) * 0.5f);
    pausePanel_->setBackgroundColor(UITheme::Hex(0x1E2230, 0.97f));
    pausePanel_->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.45f), UITheme::BorderThin);
    pausePanel_->setBorderRadius(UITheme::RadiusPanel);
    pausePanel_->setVisible(false);
    uiManager_->addElement(pausePanel_);

    auto title = std::make_shared<UILabel>();
    title->setText("PAUSED");
    title->setFont(DisplayFont(UITheme::Display::Heading));
    title->setTextColor(UITheme::Text);
    title->setAlignment(UILabel::Alignment::Center);
    title->setPosition(0.0f, S(18.0f));
    title->setSize(width, S(24.0f));
    pausePanel_->addChild(title);

    const float buttonWidth  = width - S(40.0f);
    const float buttonHeight = S(34.0f);
    float y = S(60.0f);

    auto resume = std::make_shared<UIButton>();
    resume->setText("RESUME");
    resume->setFont(DisplayFont(UITheme::Display::Button));
    resume->setPosition(S(20.0f), y);
    resume->setSize(buttonWidth, buttonHeight);
    resume->setOnClick([this]() { SetPaused(false); });
    pausePanel_->addChild(resume);

    y += buttonHeight + S(12.0f);

    // Settings remains a real screen change: leaving gameplay for it is the
    // intended behaviour, unlike pausing.
    auto settings = std::make_shared<UIButton>();
    settings->setText("SETTINGS");
    settings->setFont(DisplayFont(UITheme::Display::Button));
    settings->setPosition(S(20.0f), y);
    settings->setSize(buttonWidth, buttonHeight);
    settings->setOnClick([this]() {
        SetPaused(false);
        OnSettingsButtonClicked();
    });
    pausePanel_->addChild(settings);
}

void GameScreen::SetPaused(bool paused)
{
    if (paused_ == paused)
        return;

    paused_ = paused;

    if (pausePanel_)
        pausePanel_->setVisible(paused_);

    // Movement is stopped at the source. InputSystem samples the hardware
    // keyboard every frame, so anything cleared here would be rewritten before
    // MovementSystem ran; the system itself has to know.
    if (auto systemManager = ServiceLocator::Get<StrixVerse::ECS::SystemManager>())
    {
        if (auto input = systemManager->getSystem<StrixVerse::ECS::InputSystem>())
            input->SetGameplayPaused(paused_);
    }

    // The position on both edges of the pause. "Movement stops while paused" is
    // not provable from a screenshot: the camera keeps lerping toward the
    // player while the overlay is up, so the view drifts whether or not the
    // player actually moved. Comparing these two lines settles it.
    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();
    const auto* transform =
        componentManager
            ? componentManager->getComponent<StrixVerse::ECS::Transform>(playerEntity_)
            : nullptr;

    if (transform)
    {
        // The camera goes on the same line as the player. Away from an edge the
        // two track exactly; near one they must diverge, and that difference is
        // the only direct evidence that the world clamp is doing anything.
        const glm::vec2 eye = engine_ ? engine_->GetCamera().GetPosition() : glm::vec2{0.0f, 0.0f};

        LOG_INFO(std::format("GameScreen: {} at tile {:.1f},{:.1f} - player px {:.0f},{:.0f} "
                             "camera px {:.0f},{:.0f}",
                             paused_ ? "paused" : "resumed",
                             PlayerLocalXToTileX(transform->position.x),
                             PlayerLocalYToTileY(transform->position.y),
                             transform->position.x, transform->position.y,
                             eye.x, eye.y));
    }
    else
    {
        LOG_INFO(paused_ ? "GameScreen: paused" : "GameScreen: resumed");
    }
}

bool GameScreen::GameplayInputBlocked() const
{
    if (paused_)
        return true;

    // The same test InputSystem uses to stop the player walking while a field
    // has focus. Movement was already suppressed there, but clicks reached the
    // screen unconditionally, so a player could mine and build in the middle of
    // typing a chat message.
    auto uiManager = ServiceLocator::Get<UIManager>();
    return uiManager && uiManager->getFocusedElement() != nullptr;
}

bool GameScreen::UiConsumesPointer(float x, float y) const
{
    auto uiManager = ServiceLocator::Get<UIManager>();
    return uiManager && uiManager->getElementAt(x, y) != nullptr;
}

void GameScreen::OnMouseDown(float x, float y)
{
    if (!engine_ || GameplayInputBlocked() || UiConsumesPointer(x, y))
        return;

    int32_t tileX = 0;
    int32_t tileY = 0;
    if (!CanvasToServerTile(x, y, tileX, tileY))
        return;

    // The wrench inspects rather than edits, so it takes the click before any
    // world edit is considered -- otherwise inspecting someone standing on
    // ground would also dig it out from under them.
    if (hud_ && hud_->GetSelectedTool() == HUD::Tool::Wrench)
    {
        InspectPlayerAt(tileX, tileY);
        return;
    }

    engine_->getNetworkManager().sendBlockBreak(tileX, tileY);
}

void GameScreen::OnRightMouseDown(float x, float y)
{
    if (!engine_ || GameplayInputBlocked() || UiConsumesPointer(x, y))
        return;

    int32_t tileX = 0;
    int32_t tileY = 0;
    if (!CanvasToServerTile(x, y, tileX, tileY))
        return;

    if (hud_ && hud_->GetSelectedTool() == HUD::Tool::Wrench)
    {
        InspectPlayerAt(tileX, tileY);
        return;
    }

    auto& network = engine_->getNetworkManager();

    // Prefer whatever the hotbar has selected. Hotbar slots 0 and 1 are the
    // tools, so server inventory slot N sits at hotbar slot N + 2.
    uint16_t itemId = 0;
    if (hud_ && hud_->GetSelectedTool() == HUD::Tool::Item)
    {
        const uint8_t inventorySlot =
            static_cast<uint8_t>(hud_->GetSelectedSlot() - HUD::kFirstItemSlot);

        const auto it = network.getInventory().find(inventorySlot);
        if (it != network.getInventory().end() && !it->second.IsEmpty())
            itemId = it->second.itemId;
    }

    // Nothing selected, or an empty slot: fall back to the first thing held,
    // so right-click still does something sensible before anyone has chosen.
    if (itemId == 0)
    {
        for (const auto& [slotIndex, slot] : network.getInventory())
        {
            (void)slotIndex;
            if (!slot.IsEmpty())
            {
                itemId = slot.itemId;
                break;
            }
        }
    }

    if (itemId == 0)
    {
        LOG_INFO("GameScreen: nothing in the inventory to place");
        return;
    }

    network.sendBlockPlace(tileX, tileY, itemId);
}

void GameScreen::InspectPlayerAt(int32_t tileX, int32_t tileY)
{
    if (!engine_ || !hud_)
        return;

    // Nearest player within a tile of the click. The roster is in server tile
    // space, which is what CanvasToServerTile already produced, so no
    // conversion is needed here.
    const NetworkManager::RemotePlayer* found = nullptr;
    float bestDistance = 1.5f;   // tiles; a little forgiveness on the click

    for (const auto& [id, player] : engine_->getNetworkManager().getRemotePlayers())
    {
        (void)id;
        const float dx = player.tileX - static_cast<float>(tileX);
        const float dy = player.tileY - static_cast<float>(tileY);
        const float distance = std::sqrt(dx * dx + dy * dy);

        if (distance < bestDistance)
        {
            bestDistance = distance;
            found = &player;
        }
    }

    if (!found)
    {
        hud_->ShowNotification("No player there.");
        return;
    }

    // Name and position are everything the server actually tells us about
    // another player -- level and health arrive only for ourselves, via
    // CharacterData. Showing anything more would mean inventing it.
    hud_->ShowNotification(std::format("{}  -  tile {:.0f}, {:.0f}",
                                       found->username, found->tileX, found->tileY),
                           4.0f);
}

void GameScreen::ApplyPendingTileEdits()
{
    if (!engine_ || !world_)
        return;

    const auto edits = engine_->getNetworkManager().takePendingTileEdits();
    if (edits.empty())
        return;

    for (const auto& edit : edits)
    {
        // Same flip the terrain builder uses, so an edit lands on the row the
        // player is looking at.
        const int localRowY = (worldHeightInTiles_ - 1) - static_cast<int>(edit.tileY);
        if (localRowY < 0)
            continue;

        StrixVerse::World::Tile::Type type{};
        if (!ServerTileToClientType(edit.tileId, type))
        {
            world_->SetTileAt(static_cast<int>(edit.tileX), localRowY, 0, nullptr);
            continue;
        }

        world_->SetTileAt(static_cast<int>(edit.tileX), localRowY, 0,
                          std::make_shared<StrixVerse::World::Tile>(type, edit.tileId));
    }
}

float GameScreen::TileYToLocalY(float tileY) const
{
    return (static_cast<float>(worldHeightInTiles_ - 1) - tileY) * kTileSize;
}

float GameScreen::LocalYToTileY(float localY) const
{
    return static_cast<float>(worldHeightInTiles_ - 1) - localY / kTileSize;
}

float GameScreen::PlayerLocalXToTileX(float localX) const
{
    // The column the player's middle is in, not the one their left edge
    // touches. The server rounds what it is sent, and a three-quarter-tile
    // sprite whose left edge is reported can round into the column beside the
    // one it is actually standing in.
    return std::floor((localX + kPlayerWidth * 0.5f) / kTileSize);
}

float GameScreen::PlayerLocalYToTileY(float localY) const
{
    // The row the player's feet are in. The epsilon keeps feet resting exactly
    // on a tile boundary in the tile above the floor rather than inside it -
    // the same edge case CollisionSystem::kEdgeEpsilon exists for.
    const float feet = localY + kPlayerHeight - 0.01f;
    const float row  = std::floor(feet / kTileSize);

    return static_cast<float>(worldHeightInTiles_ - 1) - row;
}

void GameScreen::PlayerTileToLocal(float tileX, float tileY,
                                   float& outLocalX, float& outLocalY) const
{
    // The exact inverse of the two above: centred in the column, feet on the
    // floor of the row.
    outLocalX = tileX * kTileSize + (kTileSize - kPlayerWidth) * 0.5f;

    const float row = static_cast<float>(worldHeightInTiles_ - 1) - tileY;
    outLocalY = (row + 1.0f) * kTileSize - kPlayerHeight;
}

void GameScreen::BuildWorldFromServerTerrain()
{
    if (!world_ || !engine_)
        return;

    const auto& terrain = engine_->getNetworkManager().getTerrain();
    if (terrain.empty())
    {
        LOG_WARN("GameScreen: no terrain arrived from the server; the world will be empty");
        return;
    }

    // Size the world to the chunks actually received rather than to a constant,
    // so a server with different world limits still renders correctly.
    std::int32_t maxChunkX = 0;
    std::int32_t maxChunkY = 0;
    for (const auto& [key, chunk] : terrain)
    {
        (void)key;
        maxChunkX = std::max(maxChunkX, chunk.chunkX);
        maxChunkY = std::max(maxChunkY, chunk.chunkY);
    }

    const int widthInChunks  = static_cast<int>(maxChunkX) + 1;
    const int heightInChunks = static_cast<int>(maxChunkY) + 1;

    // Fixed before any tile is placed: TileYToLocalY pivots around it, and the
    // rows written below use the same value.
    worldHeightInTiles_ = heightInChunks * 16;

    // Allocates the chunk grid. It also generates placeholder terrain, but
    // every tile below is overwritten from server data, so none of it survives.
    world_->GenerateNewWorld(widthInChunks, heightInChunks, 1);

    constexpr int kChunkSize = 16;   // matches Server/src/world/Chunk.h
    std::size_t solid = 0;

    for (const auto& [key, chunk] : terrain)
    {
        (void)key;
        for (int localY = 0; localY < kChunkSize; ++localY)
        {
            for (int localX = 0; localX < kChunkSize; ++localX)
            {
                const std::size_t index =
                    static_cast<std::size_t>(localY) * kChunkSize + static_cast<std::size_t>(localX);
                if (index >= chunk.tiles.size())
                    continue;

                const int worldX = chunk.chunkX * kChunkSize + localX;

                // Flip onto screen rows. The server's Y climbs toward the sky,
                // so writing it straight through put the ground at the top of
                // the screen with the trees hanging downward. This is the same
                // conversion TileYToLocalY applies to the player, expressed in
                // whole rows.
                const int serverY = chunk.chunkY * kChunkSize + localY;
                const int worldY  = (worldHeightInTiles_ - 1) - serverY;
                if (worldY < 0)
                    continue;

                // A client chunk is 16x16xCHUNK_HEIGHT and TileRendererSystem
                // draws every layer, but the server sends a single foreground
                // plane. Clear the upper layers explicitly: GenerateNewWorld
                // above fills the whole grid with random placeholder terrain,
                // and anything left behind on layers 1+ is drawn straight over
                // the server's world.
                for (int z = 1; z < StrixVerse::World::Chunk::GetDepth(); ++z)
                {
                    world_->SetTileAt(worldX, worldY, z, nullptr);
                }

                // Layer 0 is written for every cell, air included, for the same
                // reason -- skipping air would leave the placeholder in place
                // and the world would be solid wherever the server said sky.
                StrixVerse::World::Tile::Type type{};
                if (!ServerTileToClientType(chunk.tiles[index], type))
                {
                    world_->SetTileAt(worldX, worldY, 0, nullptr);
                    continue;
                }

                world_->SetTileAt(worldX, worldY, 0,
                                  std::make_shared<StrixVerse::World::Tile>(
                                      type, chunk.tiles[index]));
                ++solid;
            }
        }
    }

    LOG_INFO(std::format("GameScreen: built world from {} server chunk(s), {} solid tile(s)",
                         terrain.size(), solid));
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

    // Terrain comes from the server. Nothing is invented locally: if the
    // chunks did not arrive the world stays empty, which is visible rather
    // than papered over with sample tiles.
    world_ = std::make_unique<StrixVerse::World::World>();
    BuildWorldFromServerTerrain();

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

    // Gravity is switched on with the world and off with it. CollisionSystem
    // does nothing while it holds no world, so gravity without one would
    // accelerate the player downward past every tile there is.
    if (auto playerSystem = systemManager->getSystem<StrixVerse::ECS::PlayerSystem>())
        playerSystem->SetGravityEnabled(true);

    // The camera may not leave the world. It takes the size in pixels rather
    // than a world pointer: it never reads a tile, only the extent, and a raw
    // pointer would be a third system to remember to null on the way out.
    if (auto cameraSystem = systemManager->getSystem<StrixVerse::ECS::Camera2DSystem>())
    {
        cameraSystem->SetWorldBounds(static_cast<float>(world_->GetWidthInTiles())  * kTileSize,
                                     static_cast<float>(world_->GetHeightInTiles()) * kTileSize);
    }

    LOG_INFO(std::format("GameScreen: entered '{}' ({} x {} tiles)",
                         engine_ ? engine_->GetSelectedWorldName() : std::string(),
                         world_->GetWidthInTiles(),
                         world_->GetHeightInTiles()));
}

bool GameScreen::collisionBlocksSpawn(float x, float y) const
{
    auto systemManager = ServiceLocator::Get<StrixVerse::ECS::SystemManager>();
    if (!systemManager)
        return false;

    auto collision = systemManager->getSystem<StrixVerse::ECS::CollisionSystem>();
    if (!collision)
        return false;

    return collision->IsAreaBlocked(x, y, kPlayerWidth, kPlayerHeight);
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

    // Search outwards in rings for somewhere the player actually fits.
    //
    // "Fits" is not enough on its own. A gap exactly one tile wide passes that
    // test and is a cell: the player stands in it and cannot walk out in
    // either direction, which is precisely what happened between a tree trunk
    // and a raised stone platform. So the first pass also requires room to
    // step sideways, and only if no such place exists in range does a second
    // pass accept a bare fit - standing somewhere cramped still beats standing
    // inside a rock.
    constexpr int kMaxRings = 64;

    const float startX = x;
    const float startY = y;

    const auto stepAside = [&](float cx, float cy) {
        return !collision->IsAreaBlocked(cx - kTileSize, cy, kPlayerWidth, kPlayerHeight) ||
               !collision->IsAreaBlocked(cx + kTileSize, cy, kPlayerWidth, kPlayerHeight);
    };

    float fallbackX = 0.0f;
    float fallbackY = 0.0f;
    bool  haveFallback = false;

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

                if (collision->IsAreaBlocked(candidateX, candidateY, kPlayerWidth, kPlayerHeight))
                    continue;

                if (stepAside(candidateX, candidateY))
                {
                    x = candidateX;
                    y = candidateY;
                    return;
                }

                if (!haveFallback)
                {
                    fallbackX    = candidateX;
                    fallbackY    = candidateY;
                    haveFallback = true;
                }
            }
        }
    }

    if (haveFallback)
    {
        LOG_WARN("GameScreen: spawn has no room to either side; the player starts boxed in");
        x = fallbackX;
        y = fallbackY;
        return;
    }

    LOG_WARN("GameScreen: no clear spawn found near the requested position");
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

    // Where to put the player.
    //
    // The server is the only thing that knows this, and it now says so in
    // PlayerData. Previously it did not, so this guessed the middle of the
    // world and searched outward for the nearest gap it fit in - which put a
    // returning player nowhere near where they left, sometimes wedged in a
    // one-tile slot between a tree and a cliff, and left the client reporting
    // a position the server had never agreed to.
    //
    // The guess survives only as a fallback for a world entered before the
    // stats packet arrived. It is a worse answer, so it says so in the log.
    float spawnX = 0.0f;
    float spawnY = 0.0f;

    const bool haveWorld = world_ && world_->GetWidthInTiles() > 0 &&
                           world_->GetHeightInTiles() > 0;

    const NetworkManager::CharacterStats& stats =
        engine_->getNetworkManager().getCharacterStats();

    if (stats.hasPosition)
    {
        PlayerTileToLocal(stats.tileX, stats.tileY, spawnX, spawnY);

        LOG_INFO(std::format("GameScreen: spawning at the server's tile {:.0f},{:.0f}",
                             stats.tileX, stats.tileY));

        // Deliberately not nudged.
        //
        // This did call FindFreeSpawn, on the theory that the client might
        // disagree about the terrain. Entering a freshly generated world showed
        // why that is wrong: the search moved the player three tiles to a spot
        // the server considered unreachable, the very first position report was
        // rejected as movement through solid terrain, and the server put them
        // back where it had said all along. The server is authoritative about
        // where a player is. If its answer looks blocked from here, the honest
        // response is to stand there and let collision and gravity resolve it,
        // not to pick somewhere else and argue.
        if (haveWorld && collisionBlocksSpawn(spawnX, spawnY))
        {
            LOG_WARN(std::format("GameScreen: the server's spawn tile {:.0f},{:.0f} reads as "
                                 "solid here; standing there anyway",
                                 stats.tileX, stats.tileY));
        }
    }
    else if (haveWorld)
    {
        LOG_WARN("GameScreen: no position from the server; falling back to the world centre");

        // Tile-aligned, so the ring search steps whole tiles and the player
        // lands flush on what it finds rather than a fraction inside it.
        const float centreColumn = std::floor(static_cast<float>(world_->GetWidthInTiles()) * 0.5f);
        const float centreRow    = std::floor(static_cast<float>(world_->GetHeightInTiles()) * 0.5f);

        spawnX = centreColumn * kTileSize + (kTileSize - kPlayerWidth) * 0.5f;
        spawnY = (centreRow + 1.0f) * kTileSize - kPlayerHeight;

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

    // A character, not a coloured box. The six indices come from PlayerData,
    // which the server sends on world join; until it arrives every index is 0,
    // which is a valid look rather than an absent one, so the player is never
    // invisible while waiting.
    StrixVerse::ECS::CharacterComponent look;
    look.hair     = stats.look.hair;
    look.skin     = stats.look.skin;
    look.eyes     = stats.look.eyes;
    look.shirt    = stats.look.shirt;
    look.trousers = stats.look.trousers;
    look.boots    = stats.look.boots;
    look.layer    = 10;  // Above the tiles.
    componentManager->addComponent<StrixVerse::ECS::CharacterComponent>(playerEntity_, look);

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
    // Server-accepted world edits, applied before anything reads the world.
    ApplyPendingTileEdits();

    TrackAirborne();

    if (hud_)
        hud_->Update(deltaTime);

    PublishLocalPosition(deltaTime);

    // Redraw the hotbar only when the inventory actually changed, rather than
    // rebuilding it every frame.
    if (engine_ && engine_->getNetworkManager().getInventoryRevision() != inventoryRevision_)
        RefreshInventory();

    if (engine_ && engine_->getNetworkManager().getStatsRevision() != statsRevision_)
        RefreshStats();
}

void GameScreen::TrackAirborne()
{
    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();
    if (!componentManager || playerEntity_ == StrixVerse::ECS::NULL_ENTITY)
        return;

    const auto* collider =
        componentManager->getComponent<StrixVerse::ECS::ColliderComponent>(playerEntity_);
    const auto* transform =
        componentManager->getComponent<StrixVerse::ECS::Transform>(playerEntity_);

    if (!collider || !transform)
        return;

    const float tileY    = PlayerLocalYToTileY(transform->position.y);
    const bool  grounded = collider->grounded;

    if (!grounded)
    {
        if (wasGrounded_)
        {
            // The take-off tile is the last one we were *standing* on, not the
            // first one we were seen falling from. CollisionSystem writes the
            // grounded flag on the fixed simulation step while this runs every
            // rendered frame, so by the time the flag flips the player has
            // already left, and a short drop was measured entirely after it
            // had happened - the log read "fell 0" for a fall that plainly
            // ended a tile lower than it began.
            airborneFromY_ = groundedTileY_;
            airbornePeakY_ = std::max(groundedTileY_, tileY);
            airborneLowY_  = std::min(groundedTileY_, tileY);
        }
        else
        {
            airbornePeakY_ = std::max(airbornePeakY_, tileY);
            airborneLowY_  = std::min(airborneLowY_,  tileY);
        }
    }
    else if (!wasGrounded_)
    {
        // Fold the landing tile into the range before measuring. A one-tile
        // drop is over in a couple of frames, and sampling only the airborne
        // frames missed the end of it entirely - the log read "fell 0" for a
        // fall that plainly landed a tile lower than it started.
        airbornePeakY_ = std::max(airbornePeakY_, tileY);
        airborneLowY_  = std::min(airborneLowY_,  tileY);

        // Server Y runs upward, so a climb is a rise in tile Y and a fall is a
        // drop. Both are reported because a jump off a ledge is both.
        LOG_INFO(std::format("GameScreen: landed at tile Y {:.0f} - left ground at {:.0f}, "
                             "rose {:.0f}, fell {:.0f}",
                             tileY, airborneFromY_,
                             airbornePeakY_ - airborneFromY_,
                             airbornePeakY_ - airborneLowY_));
    }

    if (grounded)
        groundedTileY_ = tileY;

    wasGrounded_ = grounded;
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

    // Escape toggles the overlay. This only runs once UIManager has already
    // consumed an Escape to clear field focus, so the two-stage behaviour is
    // preserved: Escape while typing abandons the message, Escape again pauses.
    if (key == UIKey::Escape)
    {
        SetPaused(!paused_);
        return;
    }

    if (!hud_ || paused_)
        return;

    // 1-9 select slots 0-8; 0 selects the last slot, same as a typical hotbar.
    if (key == UIKey::Digit0)
    {
        hud_->SetSelectedSlot(9);
        return;
    }

    if (key >= UIKey::Digit1 && key <= UIKey::Digit9)
        hud_->SetSelectedSlot(static_cast<uint8_t>(key - UIKey::Digit1));
}

void GameScreen::OnMouseWheel(float, float, float delta)
{
    if (!hud_ || paused_ || GameplayInputBlocked())
        return;

    if (delta > 0.0f)
        hud_->CycleSelectedSlot(-1);
    else if (delta < 0.0f)
        hud_->CycleSelectedSlot(1);
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

    float x = 0.0f;
    float y = 0.0f;
    PlayerTileToLocal(tileX, tileY, x, y);

    StrixVerse::ECS::Entity entity = entityManager->createEntity();

    StrixVerse::ECS::Transform transform;
    transform.position.x = x;
    transform.position.y = y;
    transform.scale      = {kPlayerWidth, kPlayerHeight};
    componentManager->addComponent<StrixVerse::ECS::Transform>(entity, transform);

    // Their look, from the PlayerSpawn that announced them. Other players are
    // no longer told apart by being a different colour of rectangle -- they are
    // told apart by looking like themselves.
    StrixVerse::ECS::CharacterComponent look;
    for (const auto& [id, remote] : engine_->getNetworkManager().getRemotePlayers())
    {
        if (id != entityId)
            continue;
        look.hair     = remote.look.hair;
        look.skin     = remote.look.skin;
        look.eyes     = remote.look.eyes;
        look.shirt    = remote.look.shirt;
        look.trousers = remote.look.trousers;
        look.boots    = remote.look.boots;
        break;
    }
    look.layer = 9;   // Just under the local player.
    componentManager->addComponent<StrixVerse::ECS::CharacterComponent>(entity, look);

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

    // A move addressed to us is the server correcting a position it rejected.
    //
    // This used to be inferred rather than asked: any id not in the roster was
    // assumed to be ours, on the reasoning that the server excludes the sender
    // from its broadcasts. That happens to be true, but it makes every
    // correction depend on the roster being complete and on a server send rule
    // staying as it is - and it silently turns a spawn that has not arrived yet
    // into a teleport. LoginSuccess tells us our entity id, so ask directly.
    if (engine_ && engine_->getNetworkManager().isSelf(entityId))
    {
        ApplyServerCorrection(tileX, tileY);
        return;
    }

    const auto it = remotePlayers_.find(entityId);

    if (it == remotePlayers_.end())
    {
        // Someone else, whose spawn has not been processed yet. Their position
        // is already recorded in NetworkManager's roster and will be applied
        // when the entity is created, so there is nothing to do here.
        return;
    }

    if (auto* network = componentManager->getComponent<StrixVerse::ECS::NetworkComponent>(it->second))
    {
        PlayerTileToLocal(tileX, tileY, network->x, network->y);
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

    PlayerTileToLocal(tileX, tileY, transform->position.x, transform->position.y);

    // A correction is the server disagreeing about where we are. Keeping the
    // velocity would carry the player straight back into whatever was
    // rejected, so the fall or jump in progress ends here and gravity starts
    // again from rest.
    if (auto* velocity =
            componentManager->getComponent<StrixVerse::ECS::VelocityComponent>(playerEntity_))
    {
        velocity->vx = 0.0f;
        velocity->vy = 0.0f;
    }

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
    // the conversion has to happen here rather than on the wire. It is the tile
    // the player *occupies* that is reported - whole numbers, because the
    // server rounds to a tile anyway and sending a fraction only invites the
    // two sides to round it differently.
    const float tileX = PlayerLocalXToTileX(transform->position.x);
    const float tileY = PlayerLocalYToTileY(transform->position.y);

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

        // Nothing left to fall onto.
        if (auto playerSystem = systemManager->getSystem<StrixVerse::ECS::PlayerSystem>())
            playerSystem->SetGravityEnabled(false);

        // No world, no bounds. The menu screens have no camera entity, but
        // leaving stale bounds behind would clamp the first frame of the next
        // world against the previous one's size.
        if (auto cameraSystem = systemManager->getSystem<StrixVerse::ECS::Camera2DSystem>())
            cameraSystem->ClearWorldBounds();
    }

    world_.reset();

    Screen::OnExit();
}
