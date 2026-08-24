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
#include "../graphics/SpriteBatch.h"
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
#include <array>
#include <cmath>
#include <cstdlib>
#include <format>
#include <vector>

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    // Mirrors Server/src/world/WorldRole.h. The client is told a role and
    // renders it; it never decides one, so this is a display table and not a
    // permission table.
    const char* WorldRoleName(std::uint8_t role)
    {
        switch (role)
        {
        case 1:  return "Member";
        case 2:  return "Builder";
        case 3:  return "Co-Owner";
        case 4:  return "Owner";
        default: return "Visitor";
        }
    }

    // Tile 24 is an unclaimed Strix Core; 25-28 are levels I-IV. Mirrors
    // Server/src/item/ItemDefinition.h.
    constexpr std::uint8_t kStrixCoreUnclaimedTile = 24;

    // The world's entrance and its only exit. Mirrors the tile the server's
    // generator builds at every world's spawn.
    constexpr std::uint8_t kMainDoorTile = 20;

    // TODO(server): the Lost Technology devices have no ids in the server's
    // block registry yet - Tile.h carries only the server id the chunk sent,
    // and nothing today places these tiles. These placeholders follow the
    // Strix Core block (24-28) and MUST be reconciled with the server's
    // registry once it defines them; until then a world containing the real
    // ids will not open these panels.
    constexpr std::uint8_t kAetherVaultTile       = 29;
    constexpr std::uint8_t kAetherGateTile        = 30;
    constexpr std::uint8_t kStabilizerTile        = 31;
    constexpr std::uint8_t kMemoryCrystalTile     = 32;

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

    // The display colour of a tile type, the same table TileRendererSystem
    // paints its flat-colour fallback textures with, so break debris matches
    // what the eye had just been looking at.
    Color TileDisplayColor(StrixVerse::World::Tile::Type type)
    {
        switch (type)
        {
        case StrixVerse::World::Tile::Type::Grass: return Color(34 / 255.0f, 139 / 255.0f, 34 / 255.0f, 1.0f);
        case StrixVerse::World::Tile::Type::Dirt:  return Color(139 / 255.0f, 69 / 255.0f, 19 / 255.0f, 1.0f);
        case StrixVerse::World::Tile::Type::Stone: return Color(112 / 255.0f, 128 / 255.0f, 144 / 255.0f, 1.0f);
        case StrixVerse::World::Tile::Type::Water: return Color(30 / 255.0f, 110 / 255.0f, 190 / 255.0f, 1.0f);
        case StrixVerse::World::Tile::Type::Sand:  return Color(237 / 255.0f, 201 / 255.0f, 145 / 255.0f, 1.0f);
        default:                                   return Color(1.0f, 0.0f, 1.0f, 1.0f);
        }
    }

    // Break sound per broken tile's server id, README spec section 36 names.
    // Soft ground takes one take, worked wood another, gem ore the crystal
    // one; everything else is stone-adjacent.
    const char* BreakSoundForServerId(std::uint8_t id)
    {
        switch (id)
        {
        case 4:                              return "break_wood";     // wood
        case 18:                             return "break_crystal";  // diamond ore
        case 1: case 3: case 5: case 19:     return "break_dirt";     // dirt/grass/leaves/sapling
        default:                             return "break_stone";
        }
    }
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

    // After the HUD, so the overlays draw above it.
    InitializePanels();
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

void GameScreen::RefreshCharacterPanel()
{
    if (!characterPanel_ || !engine_)
        return;

    const NetworkManager& network = engine_->getNetworkManager();

    // Both revisions, because this panel reads from both sources.
    //
    // It followed only getStatsRevision while drawing the player's world role
    // out of getWorldManageState(), which a different counter drives. Claiming
    // a world bumped the world-info revision and left the stats revision
    // untouched, so the sheet went on reporting the role the player had before
    // they owned the place - until some unrelated stat happened to move, which
    // in a quiet session is never.
    // Summing them is safe precisely because both only ever increment: the sum
    // cannot stay put while either moves. It would not be safe for counters
    // that could go down.
    const uint32_t revision = network.getStatsRevision() +
                              network.getWorldInfoRevision();
    if (revision == characterPanelRevision_)
        return;

    characterPanelRevision_ = revision;

    const NetworkManager::CharacterStats& source = network.getCharacterStats();

    CharacterPanel::CharacterInfo info;

    // The signed-in name is what the server knows us by; PlayerData carries
    // no separate display name.
    info.name = engine_->GetSignedInUser();
    if (info.name.empty())
        info.name = network.getUsername();

    // The roster carries no role for anyone - including us. The only role
    // the server ever sends is our own WorldRole in WorldInfo, so that is
    // what is shown when it exists and everyone else reads as a player.
    const NetworkManager::WorldManageState& world = network.getWorldManageState();
    if (world.valid)
    {
        switch (world.viewerRole)
        {
        case 4:  info.role = "Owner";     break;
        case 3:  info.role = "Co-Owner";  break;
        case 2:  info.role = "Builder";   break;
        case 1:  info.role = "Member";    break;
        default: info.role = "Visitor";   break;
        }
    }
    else
    {
        info.role = "Player";
    }

    // Nothing invented: until PlayerData arrives there are no numbers to
    // show, so the level falls back to 1 and the stats map stays empty -
    // which renders as a panel with no stat rows rather than made-up ones.
    if (source.known)
    {
        info.level = source.level > 0 ? source.level : 1;
        info.stats["Health"]                  = static_cast<int>(source.health);
        info.stats["Max Health"]              = static_cast<int>(source.maxHealth);
        info.stats["Experience"]              = static_cast<int>(source.experience);
        info.stats["Experience To Next Level"] =
            static_cast<int>(source.experienceToNextLevel);
    }
    else
    {
        info.level = 1;
    }

    characterPanel_->SetCharacter(info);
}

void GameScreen::RefreshRoster()
{
    if (!playerListPanel_ || !engine_)
        return;

    std::vector<PlayerListPanel::Entry> players;
    players.reserve(engine_->getNetworkManager().getRemotePlayers().size() + 1);

    // Us first, then everyone else in id order-ish map order; the panel only
    // lists rows, it does not rank them.
    //
    // Our own standing comes from WorldInfo, which is the same answer the
    // management panel is drawn from; everyone else's rides in with their
    // spawn. Both were previously the literal "Player", which is why
    // PlayerListPanel::RoleColor could never reach its Developer or Moderator
    // branches - the data existed on the wire and nothing read it.
    const NetworkManager& network = engine_->getNetworkManager();

    const std::string own = engine_->GetSignedInUser();
    if (!own.empty())
        players.push_back({own, WorldRoleName(network.getWorldManageState().viewerRole)});

    for (const auto& [id, remote] : network.getRemotePlayers())
    {
        (void)id;
        players.push_back({remote.username.empty()
                               ? std::format("Player {}", id)
                               : remote.username,
                           WorldRoleName(remote.worldRole)});
    }

    playerListPanel_->SetPlayers(players);
}

void GameScreen::RefreshInventory()
{
    if (!engine_)
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

    if (hud_)
        hud_->SetInventory(entries);

    // The full-inventory overlay reads the same source, mapped into its own
    // slot shape. The server sends no item names and the client has no
    // catalogue, so the id doubles as the display string; iconPath stays
    // empty and the panel falls back to its letter glyph.
    if (inventoryPanel_)
    {
        std::vector<InventoryPanel::Slot> slots;
        slots.reserve(entries.size());

        const uint8_t selectedSlot = hud_ ? hud_->GetSelectedSlot() : 0;

        for (const auto& entry : entries)
        {
            InventoryPanel::Slot slot;
            slot.itemId   = std::to_string(entry.itemId);
            slot.name     = slot.itemId;
            slot.quantity = entry.quantity;
            // Hotbar slots 0 and 1 are the tools; an inventory slot N sits at
            // hotbar slot N + 2, so only a tool-free selection highlights a
            // grid row.
            slot.selected = selectedSlot >= HUD::kFirstItemSlot &&
                            entry.slot == static_cast<uint8_t>(selectedSlot - HUD::kFirstItemSlot);
            slots.push_back(slot);
        }

        inventoryPanel_->SetSlots(slots);
    }

    inventoryRevision_ = network.getInventoryRevision();
}

void GameScreen::InitializeUI()
{
    const float originX = DesignOriginX();
    const float originY = DesignOriginY();

    // The world the server put us in, not the one we asked for.
    //
    // These are usually the same and are not always: a join can be refused -
    // banned, or the world is closed to visitors - and the server answers by
    // confirming whichever world we are actually standing in. Labelling the
    // screen from our own selection then named a world the player had just been
    // refused, over that other world's terrain.
    const std::string confirmed =
        engine_ ? engine_->getNetworkManager().getCurrentWorld() : std::string();

    const std::string worldName =
        !confirmed.empty() ? confirmed
                           : (engine_ ? engine_->GetSelectedWorldName() : std::string());

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
    settingsButton_->setOnClick([this]()
    {
        PlaySfx("ui_click");
        OnSettingsButtonClicked();
    });
    root_->addChild(settingsButton_);

    // Built here and hidden, not created per open: creating it on each wrench
    // click would throw away scroll position, field focus and every live
    // callback, which is the mistake the pause overlay was rewritten to avoid.
    worldManagerPanel_ = std::make_unique<WorldManagerPanel>(engine_, uiManager_);
    worldManagerPanel_->Build();
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

    // Server ids a body walks through. Every tile defaults to solid, which
    // turned the spawn doorway into a wall: the generator raises the main door
    // as "a walkable door flanked by cut stone", and a player born behind it
    // could not reach the [ E ] Leave prompt standing in their own exit. Torch
    // and sapling are the same class of thing - props that occupy a tile for
    // drawing but not for collision. Water stays solid deliberately: lava is
    // something you stand on, per Tile.cpp's note.
    // The gameplay backdrop pool. The same eight scenes the loading screen
    // offers, so the picture that loaded is the sky you play under.
    constexpr std::array<const char*, 8> kBackdropArtwork = {
        "assets/ui/world_loading/nature_1/origbig.png",
        "assets/ui/world_loading/nature_2/origbig.png",
        "assets/ui/world_loading/nature_3/origbig.png",
        "assets/ui/world_loading/nature_4/origbig.png",
        "assets/ui/world_loading/nature_5/origbig.png",
        "assets/ui/world_loading/nature_6/origbig.png",
        "assets/ui/world_loading/nature_7/origbig.png",
        "assets/ui/world_loading/nature_8/origbig.png",
    };

    bool ServerTileIsWalkable(std::uint8_t id)
    {
        switch (id)
        {
        case 8:   // torch
        case 19:  // sapling
        case 20:  // main door
            return true;
        default:
            return false;
        }
    }
} // namespace

bool GameScreen::CanvasToWorldPixel(float canvasX, float canvasY,
                                    float& outX, float& outY) const
{
    if (!engine_)
        return false;

    const Camera2D& camera = engine_->GetCamera();
    const glm::vec2 viewport = camera.GetViewport();
    if (viewport.x <= 0.0f || viewport.y <= 0.0f)
        return false;

    const float zoom = camera.GetZoom() > 0.0f ? camera.GetZoom() : 1.0f;

    // Canvas -> window pixels is the inverse of what UIScale did on the way in,
    // and window pixels are what the camera's projection is built over.
    const glm::vec4& visible = engine_->GetUIScale().GetVisibleCanvas();
    if (visible.z <= 0.0f || visible.w <= 0.0f)
        return false;

    const float screenX = (canvasX - visible.x) / visible.z * viewport.x;
    const float screenY = (canvasY - visible.y) / visible.w * viewport.y;

    // Camera2D::GetViewMatrix is  screen = (world - position) * zoom + viewport/2,
    // so this is that read backwards.
    const glm::vec2 centre = camera.GetPosition();
    outX = (screenX - viewport.x * 0.5f) / zoom + centre.x;
    outY = (screenY - viewport.y * 0.5f) / zoom + centre.y;
    return true;
}

bool GameScreen::WorldPixelToCanvas(float worldX, float worldY,
                                    float& outX, float& outY) const
{
    if (!engine_)
        return false;

    const Camera2D& camera = engine_->GetCamera();
    const glm::vec2 viewport = camera.GetViewport();
    if (viewport.x <= 0.0f || viewport.y <= 0.0f)
        return false;

    const float zoom = camera.GetZoom() > 0.0f ? camera.GetZoom() : 1.0f;

    const glm::vec4& visible = engine_->GetUIScale().GetVisibleCanvas();
    if (visible.z <= 0.0f || visible.w <= 0.0f)
        return false;

    const glm::vec2 centre = camera.GetPosition();
    const float screenX = (worldX - centre.x) * zoom + viewport.x * 0.5f;
    const float screenY = (worldY - centre.y) * zoom + viewport.y * 0.5f;

    outX = screenX / viewport.x * visible.z + visible.x;
    outY = screenY / viewport.y * visible.w + visible.y;
    return true;
}

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

    (void)transform;

    float worldPixelX = 0.0f;
    float worldPixelY = 0.0f;
    if (!CanvasToWorldPixel(canvasX, canvasY, worldPixelX, worldPixelY))
        return false;

    const int localRowX = static_cast<int>(std::floor(worldPixelX / kTileSize));
    const int localRowY = static_cast<int>(std::floor(worldPixelY / kTileSize));

    // Back into the server's Y-up space, the inverse of TileYToLocalY.
    outTileX = static_cast<int32_t>(localRowX);
    outTileY = static_cast<int32_t>((worldHeightInTiles_ - 1) - localRowY);
    return true;
}

float GameScreen::BubbleLifetimeFor(const std::string& message)
{
    // A floor so a one-word reply is still readable, plus reading time. Roughly
    // fifteen characters a second, which is slow enough to be comfortable for
    // someone who was not watching when it appeared.
    constexpr float kFloor      = 3.0f;
    constexpr float kCeiling    = 9.0f;
    constexpr float kPerCharSec = 1.0f / 15.0f;

    const float earned = kFloor + static_cast<float>(message.size()) * kPerCharSec;
    return earned > kCeiling ? kCeiling : earned;
}

StrixVerse::ECS::Entity GameScreen::EntityForSpeaker(uint64_t speakerId) const
{
    if (speakerId == kLocalSpeakerId)
        return playerEntity_;

    const auto it = remotePlayers_.find(speakerId);
    return it != remotePlayers_.end() ? it->second : StrixVerse::ECS::NULL_ENTITY;
}

void GameScreen::ShowChatBubble(uint64_t speakerId, const std::string& message)
{
    if (!uiManager_ || message.empty())
        return;

    // Nothing to hang a bubble on. Server system messages arrive as id 0 and
    // have no body in the world, so they are chat-log-only by nature.
    if (EntityForSpeaker(speakerId) == StrixVerse::ECS::NULL_ENTITY)
        return;

    // A long line would draw a bubble wider than the screen, and the point of
    // the bubble is a glance, not a transcript - the chat log keeps the whole
    // message either way.
    // Three ASCII dots rather than U+2026: the label drew the UTF-8 ellipsis as
    // two mojibake glyphs, so whatever the text path does it is not decoding
    // multi-byte sequences here. Not worth chasing for a truncation marker.
    constexpr std::size_t kMaxShown = 40;
    std::string shown = message.size() > kMaxShown
                            ? message.substr(0, kMaxShown - 3) + "..."
                            : message;

    ChatBubble& bubble = chatBubbles_[speakerId];
    bubble.remaining = BubbleLifetimeFor(shown);

    if (!bubble.panel)
    {
        bubble.panel = std::make_shared<UIPanel>();
        bubble.panel->setBackgroundColor(UITheme::Hex(0x1E2230, 0.92f));
        bubble.panel->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.40f),
                                UITheme::BorderThin);
        bubble.panel->setBorderRadius(UITheme::RadiusPanel);

        // Deliberately not blocking input: a bubble drifts over the world and
        // must not carve a hole in it that swallows clicks.
        bubble.label = std::make_shared<UILabel>();
        bubble.label->setFont(DisplayFont(UITheme::Display::Small));
        bubble.label->setTextColor(UITheme::Text);
        bubble.label->setAlignment(UILabel::Alignment::Center);
        bubble.panel->addChild(bubble.label);

        // Added last so bubbles draw over the HUD panels rather than under them.
        uiManager_->addElement(bubble.panel);
    }

    bubble.label->setText(shown);

    // Width follows the text, measured rather than estimated. A per-character
    // guess is wrong in both directions on a proportional face - it gives a
    // short line a bubble with a gap at each end, and lets a long one run out
    // past the border it is supposed to sit inside.
    const float padding = S(14.0f);
    const float width   = bubble.label->measureTextWidth() + padding * 2.0f;
    const float height  = S(26.0f);

    bubble.panel->setSize(width, height);
    bubble.label->setPosition(0.0f, S(7.0f));
    bubble.label->setSize(width, S(14.0f));
    bubble.panel->setVisible(true);
}

float GameScreen::NameTagOffset()
{
    // Clear of the sprite's head, and far enough that a chat bubble stacked
    // above it still reads as belonging to the same player.
    return S(14.0f);
}

std::shared_ptr<UILabel> GameScreen::NameTagFor(uint64_t speakerId, const std::string& name)
{
    if (!uiManager_ || name.empty())
        return nullptr;

    auto& tag = nameTags_[speakerId];
    if (!tag)
    {
        tag = std::make_shared<UILabel>();
        tag->setFont(DisplayFont(UITheme::Display::Small));
        tag->setAlignment(UILabel::Alignment::Center);

        // A hard black shadow rather than a panel behind the text. A tag is on
        // screen for every player all of the time, and a filled box per player
        // would compete with the world in a way a transient chat bubble does
        // not - but plain text disappeared against bright terrain, and the
        // first version was unreadable sitting over a lit neon strip. One
        // offset pixel of black is enough to carry it over anything.
        tag->setShadow(UITheme::Hex(0x000000, 1.0f), S(1.5f), S(1.5f));

        // White for our own, accent for everyone else. The first version used
        // Subtext grey for the local player and it was illegible sitting over
        // the spawn gate's lit neon strip - grey and cyan are close enough in
        // luminance that the shadow alone could not separate them. White beats
        // any terrain in this palette.
        tag->setTextColor(speakerId == kLocalSpeakerId ? UITheme::Text
                                                       : UITheme::Accent);
        uiManager_->addElement(tag);
    }

    tag->setText(name);
    return tag;
}

void GameScreen::UpdateNameTags()
{
    if (!uiManager_ || !engine_)
        return;

    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();
    if (!componentManager)
        return;

    // Who should have a tag this frame: us, and everyone we can see. Built
    // fresh each frame rather than maintained on spawn and remove, because the
    // roster already changes through three different paths and a fourth place
    // to keep in sync is a fourth place to forget.
    std::vector<std::pair<uint64_t, std::string>> wanted;

    if (playerEntity_ != StrixVerse::ECS::NULL_ENTITY)
    {
        const std::string own = engine_->GetSignedInUser();
        if (!own.empty())
            wanted.emplace_back(kLocalSpeakerId, own);
    }

    for (const auto& [entityId, entity] : remotePlayers_)
    {
        (void)entity;
        wanted.emplace_back(entityId, DisplayNameFor(entityId));
    }

    // Drop tags for anyone no longer here, before placing the rest.
    for (auto it = nameTags_.begin(); it != nameTags_.end();)
    {
        const bool stillHere =
            std::any_of(wanted.begin(), wanted.end(),
                        [&](const auto& entry) { return entry.first == it->first; });

        if (!stillHere)
        {
            if (it->second)
                uiManager_->removeElement(it->second);
            it = nameTags_.erase(it);
            continue;
        }
        ++it;
    }

    const glm::vec4& visible = engine_->GetUIScale().GetVisibleCanvas();

    for (const auto& [speakerId, name] : wanted)
    {
        const StrixVerse::ECS::Entity entity = EntityForSpeaker(speakerId);
        if (entity == StrixVerse::ECS::NULL_ENTITY)
            continue;

        const auto* transform =
            componentManager->getComponent<StrixVerse::ECS::Transform>(entity);
        if (!transform)
            continue;

        auto tag = NameTagFor(speakerId, name);
        if (!tag)
            continue;

        const float headX = transform->position.x + kPlayerWidth * 0.5f;
        const float headY = transform->position.y;

        float canvasX = 0.0f;
        float canvasY = 0.0f;
        if (!WorldPixelToCanvas(headX, headY, canvasX, canvasY))
            continue;

        const float width  = S(160.0f);
        const float height = S(12.0f);

        tag->setSize(width, height);
        tag->setPosition(canvasX - width * 0.5f, canvasY - NameTagOffset() - height);

        // Off-screen players keep their tag but stop drawing it, so tags do not
        // pile up against the edge of the view.
        const bool onScreen = canvasX >= visible.x - width &&
                              canvasX <= visible.x + visible.z + width &&
                              canvasY >= visible.y - height &&
                              canvasY <= visible.y + visible.w + height;
        tag->setVisible(onScreen);
    }
}

void GameScreen::UpdateChatBubbles(float deltaTime)
{
    if (chatBubbles_.empty())
        return;

    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();

    for (auto it = chatBubbles_.begin(); it != chatBubbles_.end();)
    {
        ChatBubble& bubble = it->second;
        bubble.remaining -= deltaTime;

        const StrixVerse::ECS::Entity speaker = EntityForSpeaker(it->first);

        // Expired, or the speaker left the world while their bubble was up.
        if (bubble.remaining <= 0.0f || speaker == StrixVerse::ECS::NULL_ENTITY ||
            !componentManager)
        {
            if (bubble.panel)
            {
                bubble.panel->setVisible(false);
                uiManager_->removeElement(bubble.panel);
            }
            it = chatBubbles_.erase(it);
            continue;
        }

        const auto* transform =
            componentManager->getComponent<StrixVerse::ECS::Transform>(speaker);
        if (!transform || !bubble.panel)
        {
            ++it;
            continue;
        }

        // Anchor to the middle of the sprite's top edge, then lift the bubble
        // clear of the head. The player transform is a top-left corner, which
        // is why the half-width is added rather than the position used raw.
        const float headX = transform->position.x + kPlayerWidth * 0.5f;
        const float headY = transform->position.y;

        float canvasX = 0.0f;
        float canvasY = 0.0f;
        if (!WorldPixelToCanvas(headX, headY, canvasX, canvasY))
        {
            ++it;
            continue;
        }

        const float sizeX = bubble.panel->getWidth();
        const float sizeY = bubble.panel->getHeight();
        const float gap = NameTagOffset() + S(12.0f);   // clear of the name tag

        bubble.panel->setPosition(canvasX - sizeX * 0.5f, canvasY - sizeY - gap);

        // Off-screen speakers keep their bubble alive but stop drawing it, so a
        // bubble does not stack up against the edge of the view.
        const glm::vec4& visible = engine_->GetUIScale().GetVisibleCanvas();
        const bool onScreen = canvasX >= visible.x - sizeX &&
                              canvasX <= visible.x + visible.z + sizeX &&
                              canvasY >= visible.y - sizeY &&
                              canvasY <= visible.y + visible.w + sizeY;
        bubble.panel->setVisible(onScreen);

        ++it;
    }
}

void GameScreen::InitializePanels()
{
    if (!uiManager_)
        return;

    // Built here and hidden, not created per open: creating one on each
    // toggle would throw away scroll position, field focus and every live
    // callback, which is the mistake the pause overlay was rewritten to
    // avoid. All of them add themselves to the UIManager directly so they
    // draw above the HUD.
    pauseOverlay_ = std::make_unique<PauseOverlay>(engine_, uiManager_);
    pauseOverlay_->Build();

    // The overlay decides nothing about navigation; the screen owns the
    // session, so the buttons come back here.
    pauseOverlay_->onResume = [this]() {
        PlaySfx("ui_click");
        SetPaused(false);
    };
    pauseOverlay_->onSettings = [this]() {
        PlaySfx("ui_click");
        SetPaused(false);
        OnSettingsButtonClicked();
    };
    pauseOverlay_->onExitWorld = [this]() {
        PlaySfx("ui_click");
        if (engine_)
        {
            // Same path as SettingsScreen's LEAVE WORLD: the server despawns
            // us for everyone else and the session stays open, but leaving is
            // a decision to stop playing this world, so Continue forgets it.
            engine_->getNetworkManager().sendWorldLeave();

            if (WorldManager* worlds = engine_->GetWorldManager())
                worlds->ClearLastWorld();

            engine_->SetSelectedWorldName(std::string());
        }

        LOG_INFO("GameScreen: left the world from the pause menu");
        SetPaused(false);
        RequestScreenChange(ScreenID::WorldBrowser);
    };

    playerListPanel_ = std::make_unique<PlayerListPanel>(engine_, uiManager_);
    playerListPanel_->Build();

    inventoryPanel_ = std::make_unique<InventoryPanel>(engine_, uiManager_);
    inventoryPanel_->Build();

    characterPanel_ = std::make_unique<CharacterPanel>(engine_, uiManager_);
    characterPanel_->Build();

    buffDisplay_ = std::make_unique<BuffDisplay>(engine_, uiManager_);
    buffDisplay_->Build();

    // Lost Technology interfaces, same lifecycle as the panels above.
    vaultPanel_ = std::make_unique<VaultPanel>(engine_, uiManager_);
    vaultPanel_->Build();

    gatePanel_ = std::make_unique<GatePanel>(engine_, uiManager_);
    gatePanel_->Build();

    stabilizerPanel_ = std::make_unique<StabilizerPanel>(engine_, uiManager_);
    stabilizerPanel_->Build();

    memoryCrystalPanel_ = std::make_unique<MemoryCrystalPanel>(engine_, uiManager_);
    memoryCrystalPanel_->Build();

    // The panels only raise intents; nothing here sends protocol, because no
    // vault/gate/stabiliser/crystal senders exist in NetworkManager yet.
    vaultPanel_->onWithdraw = [this]() {
        PlaySfx("ui_click");
        LOG_INFO("GameScreen: vault withdraw - not yet implemented");
    };
    vaultPanel_->onDeposit = [this]() {
        PlaySfx("ui_click");
        LOG_INFO("GameScreen: vault deposit - not yet implemented");
    };
    vaultPanel_->onManage = [this]() {
        PlaySfx("ui_click");
        LOG_INFO("GameScreen: vault manage - not yet implemented");
    };

    gatePanel_->onActivate = [this]() {
        PlaySfx("ui_click");
        LOG_INFO("GameScreen: gate activate - not yet implemented");
    };

    stabilizerPanel_->onStabilize = [this]() {
        PlaySfx("ui_click");
        LOG_INFO("GameScreen: stabilizer stabilize - not yet implemented");
    };

    memoryCrystalPanel_->onExtract = [this]() {
        PlaySfx("ui_click");
        LOG_INFO("GameScreen: memory crystal extract - not yet implemented");
    };

    // TODO(server): populate the Lost Technology panels from packets. Nothing
    // in WorldManageState or anywhere else on the wire carries this data
    // today, so every panel opens with its empty default. Once the server
    // defines them, these handlers must call:
    //   - VaultPanel::SetResources()      from the vault contents packet handler
    //   - GatePanel::SetStatus()/SetDestination() from the gate state handler
    //   - StabilizerPanel::SetStability()/SetUpgrades() from the stabilizer state handler
    //   - MemoryCrystalPanel::SetEntries() from the crystal log handler
    // following the revision-keyed refresh idiom RefreshInventory uses.

    // The interact prompt is a standing element like a chat bubble: scenery
    // over the world that must not carve clicks out of it, built once and
    // toggled by visibility rather than created per frame.
    constexpr float kPromptWidth  = 120.0f;
    constexpr float kPromptHeight = 22.0f;

    interactPromptPanel_ = std::make_shared<UIPanel>();
    interactPromptPanel_->setSize(S(kPromptWidth), S(kPromptHeight));
    interactPromptPanel_->setBackgroundColor(UITheme::Hex(0x1E2230, 0.92f));
    interactPromptPanel_->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.40f),
                                    UITheme::BorderThin);
    interactPromptPanel_->setBorderRadius(UITheme::RadiusChip);

    interactPromptLabel_ = std::make_shared<UILabel>();
    interactPromptLabel_->setText("[ E ] Interact");
    interactPromptLabel_->setFont(DisplayFont(UITheme::Display::Small));
    interactPromptLabel_->setTextColor(UITheme::Text);
    interactPromptLabel_->setAlignment(UILabel::Alignment::Center);
    interactPromptLabel_->setPosition(0.0f, S(5.0f));
    interactPromptLabel_->setSize(S(kPromptWidth), S(12.0f));
    interactPromptPanel_->addChild(interactPromptLabel_);

    interactPromptPanel_->setVisible(false);
    uiManager_->addElement(interactPromptPanel_);

    // Whatever stats arrived while the loading screen was up should be on
    // the panels immediately rather than waiting for a revision change.
    characterPanelRevision_ = 0;
    RefreshCharacterPanel();
    RefreshRoster();
}

void GameScreen::ClosePanelOverlays()
{
    if (inventoryPanel_)      inventoryPanel_->Close();
    if (characterPanel_)      characterPanel_->Close();
    if (playerListPanel_)     playerListPanel_->Close();
    if (vaultPanel_)          vaultPanel_->Close();
    if (gatePanel_)           gatePanel_->Close();
    if (stabilizerPanel_)     stabilizerPanel_->Close();
    if (memoryCrystalPanel_)  memoryCrystalPanel_->Close();
}

bool GameScreen::AnyPanelOpen() const
{
    // One list, consulted by everything that needs to know whether a panel is
    // up: the click gate, the key gate, and the Escape ladder. They had drifted
    // apart - the click gate knew only about the management panel and the key
    // gate knew only about pause - which is how a keypress could act on the
    // world from behind a modal that was already swallowing clicks.
    return (worldManagerPanel_   && worldManagerPanel_->IsOpen())   ||
           (inventoryPanel_      && inventoryPanel_->IsOpen())      ||
           (characterPanel_      && characterPanel_->IsOpen())      ||
           (playerListPanel_     && playerListPanel_->IsOpen())     ||
           (vaultPanel_          && vaultPanel_->IsOpen())          ||
           (gatePanel_           && gatePanel_->IsOpen())           ||
           (stabilizerPanel_     && stabilizerPanel_->IsOpen())     ||
           (memoryCrystalPanel_  && memoryCrystalPanel_->IsOpen());
}

void GameScreen::SetPaused(bool paused)
{
    if (paused_ == paused)
        return;

    paused_ = paused;

    // The overlay is the only visual; the flag is the only authority. Keeping
    // them in one method means Escape and both buttons cannot disagree.
    if (pauseOverlay_)
    {
        if (paused_)
            pauseOverlay_->Open();
        else
            pauseOverlay_->Close();
    }

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

    // Every panel that covers live terrain, not just the management one.
    //
    // Each panel does set blocksInput on its root, so a click *inside* its
    // rectangle is swallowed - but those rectangles cover under a tenth of the
    // screen. With the inventory open, a click anywhere in the remaining nine
    // tenths still reached OnMouseDown and broke a block the player could not
    // see, which is the hotbar click-through bug over again at a much larger
    // scale. A panel is either modal or it is not; being modal only within its
    // own borders is not a thing.
    if (AnyPanelOpen())
        return true;

    return uiManager && uiManager->isTextInputFocused();
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
        // The wrench is the "interact with things" tool, so it is what claims a
        // Strix Core. The [ E ] Interact prompt reaches the same fork from the
        // keyboard, targeting the tile the prompt found rather than this one.
        // Says what the wrench found. A tool that silently does nothing is the
        // hardest kind of thing to diagnose, and this is the one interaction
        // where "nothing happened" and "that is not a Core" look identical.
        LOG_INFO(std::format("GameScreen: wrench on tile {},{} (server id {})",
                             tileX, tileY, static_cast<int>(ServerIdAt(tileX, tileY))));

        // The Core is found the way the E prompt finds it - nearest within a
        // tile of the click, not under the pixel. The server refuses a claim
        // whose coordinates are not exactly the world's recorded Core and says
        // nothing when it does, so an off-by-one click used to vanish without
        // a trace instead of claiming anything.
        int32_t coreX = tileX;
        int32_t coreY = tileY;
        bool    foundCore = false;

        for (int dy = -1; dy <= 1 && !foundCore; ++dy)
        {
            for (int dx = -1; dx <= 1 && !foundCore; ++dx)
            {
                if (IsStrixCoreAt(tileX + dx, tileY + dy))
                {
                    coreX     = tileX + dx;
                    coreY     = tileY + dy;
                    foundCore = true;
                }
            }
        }

        if (foundCore)
        {
            // An unclaimed Core is claimed; a claimed one opens management.
            // Which of the two this is comes from the tile the server sent us,
            // and asking the wrong one is harmless: the server answers a claim
            // on an owned world with "already belongs to someone else" and an
            // interact on an unowned one with "no Core here".
            if (ServerIdAt(coreX, coreY) == kStrixCoreUnclaimedTile)
                engine_->getNetworkManager().sendClaimStrixCore(coreX, coreY);
            else
                engine_->getNetworkManager().sendInteractStrixCore(coreX, coreY);

            return;
        }

        InspectPlayerAt(tileX, tileY);
        return;
    }

    // One button, and what you are holding decides what it does.
    //
    // Left click used to always break, with placing on the right button, so the
    // selected slot only mattered for the wrench. Now the fist breaks, the
    // wrench interacts (above), and an item places - which makes the hotbar
    // selection the whole interface, with one thing to learn instead of two
    // buttons whose meanings depended on the slot anyway.
    auto& network = engine_->getNetworkManager();

    if (!hud_ || hud_->GetSelectedTool() == HUD::Tool::Punch)
    {
        // Fired on the swing rather than the confirm - a punch that hits
        // nothing (or is refused) should still sound like one.
        PlaySfx("punch");
        network.sendBlockBreak(tileX, tileY, SelectedToolItemId());
        return;
    }

    // Holding an item: place exactly what is selected, and nothing if that slot
    // is empty.
    //
    // There is deliberately no fall back to "the first thing in the bag". The
    // old right-click path had one, which was harmless while placing lived on
    // its own button; here it would mean a click placing a block the player
    // never chose, and the selection would stop meaning anything.
    const uint8_t inventorySlot =
        static_cast<uint8_t>(hud_->GetSelectedSlot() - HUD::kFirstItemSlot);

    const auto held = network.getInventory().find(inventorySlot);
    if (held == network.getInventory().end() || held->second.IsEmpty())
    {
        LOG_INFO("GameScreen: the selected slot is empty; nothing to place");
        return;
    }

    network.sendBlockPlace(tileX, tileY, held->second.itemId);
}


std::uint8_t GameScreen::ServerIdAt(int32_t tileX, int32_t tileY) const
{
    if (!world_)
        return 0;

    // Server tile space to the client's row order - the same flip
    // BuildWorldFromServerTerrain applied on the way in, so a tile is looked
    // for where it was written.
    const int localRowY = (worldHeightInTiles_ - 1) - tileY;
    if (tileX < 0 || localRowY < 0 ||
        tileX >= world_->GetWidthInTiles() || localRowY >= world_->GetHeightInTiles())
    {
        return 0;
    }

    const auto tile = world_->GetTileAt(tileX, localRowY, 0);
    return tile ? tile->GetServerId() : 0;
}

bool GameScreen::IsStrixCoreAt(int32_t tileX, int32_t tileY) const
{
    // Ids 24 through 28: unclaimed, then one per Core level. The level is the
    // id because tile ids are the only per-tile thing the chunk format keeps.
    const std::uint8_t id = ServerIdAt(tileX, tileY);
    return id >= 24 && id <= 28;
}

GameScreen::InteractTarget GameScreen::InteractTargetAt(int32_t tileX, int32_t tileY) const
{
    const std::uint8_t id = ServerIdAt(tileX, tileY);

    if (id >= 24 && id <= 28)
        return InteractTarget::StrixCore;

    switch (id)
    {
    case kMainDoorTile:      return InteractTarget::MainDoor;
    case kAetherVaultTile:   return InteractTarget::Vault;
    case kAetherGateTile:    return InteractTarget::Gate;
    case kStabilizerTile:    return InteractTarget::Stabilizer;
    case kMemoryCrystalTile: return InteractTarget::MemoryCrystal;
    default:                 return InteractTarget::None;
    }
}

void GameScreen::UpdateInteractPrompt()
{
    if (!interactPromptPanel_ || !uiManager_)
        return;

    interactTileX_   = 0;
    interactTileY_   = 0;
    interactTarget_  = InteractTarget::None;

    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();
    if (!componentManager || playerEntity_ == StrixVerse::ECS::NULL_ENTITY ||
        !engine_)
    {
        interactPromptPanel_->setVisible(false);
        return;
    }

    const auto* transform =
        componentManager->getComponent<StrixVerse::ECS::Transform>(playerEntity_);
    if (!transform)
    {
        interactPromptPanel_->setVisible(false);
        return;
    }

    // The tile the player occupies, in server coordinates - the same
    // conversion PublishLocalPosition reports with, so what E targets and
    // where the server thinks we stand cannot disagree about the space.
    const int32_t playerX =
        static_cast<int32_t>(PlayerLocalXToTileX(transform->position.x));
    const int32_t playerY =
        static_cast<int32_t>(PlayerLocalYToTileY(transform->position.y));

    // Nearest interactable within reach. Chebyshev distance, so the scan is a
    // square; ties go to whichever came first, which is fine for a prompt.
    bool found  = false;
    int  bestD2 = kInteractRadius * kInteractRadius + 1;

    for (int dy = -kInteractRadius; dy <= kInteractRadius; ++dy)
    {
        for (int dx = -kInteractRadius; dx <= kInteractRadius; ++dx)
        {
            const InteractTarget target =
                InteractTargetAt(playerX + dx, playerY + dy);

            // A Core, the main door, or one of the Lost Technology devices --
            // anything E acts on. Whichever is nearest wins, so standing in
            // the doorway of a world whose Core sits beside it still offers
            // the door.
            if (target == InteractTarget::None)
                continue;

            const int d2 = dx * dx + dy * dy;
            if (d2 < bestD2)
            {
                bestD2          = d2;
                found           = true;
                interactTileX_  = playerX + dx;
                interactTileY_  = playerY + dy;
                interactTarget_ = target;
            }
        }
    }

    if (!found)
    {
        interactPromptPanel_->setVisible(false);
        return;
    }

    // Say which of them it is. A prompt that reads "Interact" while pointing
    // at the way out is worse than no prompt: the player has to press it to
    // find out what it does.
    const char* promptText = "[ E ] Interact";
    switch (interactTarget_)
    {
    case InteractTarget::MainDoor:      promptText = "[ E ] Leave world";    break;
    case InteractTarget::Vault:         promptText = "[ E ] Aether Vault";   break;
    case InteractTarget::Gate:          promptText = "[ E ] Aether Gate";    break;
    case InteractTarget::Stabilizer:    promptText = "[ E ] Stabilizer";     break;
    case InteractTarget::MemoryCrystal: promptText = "[ E ] Memory Crystal"; break;
    case InteractTarget::StrixCore:
    case InteractTarget::None:          break;
    }

    if (interactPromptLabel_)
    {
        interactPromptLabel_->setText(promptText);

        // Width follows the text, as the chat bubbles do: the fixed width was
        // sized for "[ E ] Interact" and the longer device names would run
        // past their own border.
        const float padding = S(14.0f);
        const float width   =
            interactPromptLabel_->measureTextWidth() + padding * 2.0f;

        interactPromptLabel_->setSize(width, S(12.0f));
        interactPromptPanel_->setSize(width, interactPromptPanel_->getHeight());
    }

    // Centred along the bottom edge of the design canvas, clear of the world
    // label that shares that edge.
    const float width  = interactPromptPanel_->getWidth();
    const float height = interactPromptPanel_->getHeight();

    // Above the hotbar, not on top of it. The hotbar's top edge sits at
    // kDesignHeight - S(110) -- its own S(46) height plus the S(64) it is
    // lifted off the bottom, in HUD::CreateInventorySection -- and the prompt
    // used to be placed at S(70), which put it 18 units inside the bar and
    // covered four slots and their icons.
    //
    // It went unnoticed because the only prompt was a Strix Core, which a
    // player stands next to rarely. The main door is at every spawn, so the
    // prompt is now on screen the moment anyone arrives.
    constexpr float kHotbarTopGap = 110.0f + 12.0f;
    interactPromptPanel_->setPosition(
        DesignOriginX() + (UIScale::kDesignWidth - width) * 0.5f,
        DesignOriginY() + UIScale::kDesignHeight - height - S(kHotbarTopGap));
    interactPromptPanel_->setVisible(true);
}

void GameScreen::InteractWithTarget()
{
    // The prompt's tile, not whatever is under the cursor: pressing E acts on
    // exactly what the prompt is pointing at.
    if (!engine_ || !interactPromptPanel_ || !interactPromptPanel_->isVisible())
        return;

    switch (interactTarget_)
    {
    case InteractTarget::None:
        return;

    case InteractTarget::MainDoor:
        // The server answers only on success and is silent on refusal -- it
        // refuses when the player is not actually at the door -- so nothing is
        // assumed here. OnWorldLeft, driven by the reply, is what changes
        // screen.
        engine_->getNetworkManager().sendWorldLeave();
        LOG_INFO(std::format("GameScreen: asked to leave through the door at {},{}",
                             interactTileX_, interactTileY_));
        return;

    case InteractTarget::Vault:
        if (vaultPanel_)
        {
            ClosePanelOverlays();
            vaultPanel_->Open();
        }
        return;

    case InteractTarget::Gate:
        if (gatePanel_)
        {
            ClosePanelOverlays();
            gatePanel_->Open();
        }
        return;

    case InteractTarget::Stabilizer:
        if (stabilizerPanel_)
        {
            ClosePanelOverlays();
            stabilizerPanel_->Open();
        }
        return;

    case InteractTarget::MemoryCrystal:
        if (memoryCrystalPanel_)
        {
            ClosePanelOverlays();
            memoryCrystalPanel_->Open();
        }
        return;

    case InteractTarget::StrixCore:
        break;
    }

    // The same fork the wrench uses on a Core click: an unclaimed Core is
    // claimed, a claimed one opens management. Which of the two this is comes
    // from the tile, and asking the wrong one is harmless - the server
    // answers each with its own refusal.
    if (ServerIdAt(interactTileX_, interactTileY_) == kStrixCoreUnclaimedTile)
        engine_->getNetworkManager().sendClaimStrixCore(interactTileX_, interactTileY_);
    else
        engine_->getNetworkManager().sendInteractStrixCore(interactTileX_, interactTileY_);

    LOG_INFO(std::format("GameScreen: interact key on tile {},{}",
                         interactTileX_, interactTileY_));
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

        // What stood here before the edit landed. A break needs it for the
        // debris colour and the material-appropriate sound; by the time this
        // runs it is still in the world, because nothing else moves tiles.
        const auto previous =
            world_->GetTileAt(static_cast<int>(edit.tileX), localRowY, 0);

        StrixVerse::World::Tile::Type type{};
        if (!ServerTileToClientType(edit.tileId, type))
        {
            // Confirmed break. TileEdit carries no actor id, so remote
            // players' confirmed breaks play and burst here too - the apply
            // path is shared and distinguishing them would mean a protocol
            // change, which is not worth losing the feedback over.
            if (previous)
            {
                particles_.EmitBlockBreak(static_cast<float>(edit.tileX),
                                          static_cast<float>(localRowY),
                                          TileDisplayColor(previous->GetType()));
            }

            PlayBreakSfx(previous ? previous->GetServerId()
                                  : static_cast<std::uint8_t>(2));

            world_->SetTileAt(static_cast<int>(edit.tileX), localRowY, 0, nullptr);
            continue;
        }

        world_->SetTileAt(static_cast<int>(edit.tileX), localRowY, 0,
                          [&]() {
                              auto tile = std::make_shared<StrixVerse::World::Tile>(
                                  type, edit.tileId);
                              tile->SetSolid(!ServerTileIsWalkable(edit.tileId));
                              return tile;
                          }());

        // Confirmed placement, ours or anyone's.
        PlaySfx("place");
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
                                  [&]() {
                                      auto tile = std::make_shared<StrixVerse::World::Tile>(
                                          type, chunk.tiles[index]);
                                      tile->SetSolid(!ServerTileIsWalkable(chunk.tiles[index]));
                                      return tile;
                                  }());
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

    // The server's answer, for the same reason the label uses it: this line is
    // the first thing read when diagnosing where a player ended up, and naming
    // the world we asked for rather than the one we got is exactly the wrong
    // answer to that question.
    LOG_INFO(std::format("GameScreen: entered '{}' ({} x {} tiles)",
                         engine_ ? engine_->getNetworkManager().getCurrentWorld()
                                 : std::string(),
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

    // The gameplay backdrop, picked exactly the way the loading screen picks
    // its artwork - same pool, same name hash - so joining a world continues
    // into the scene that just loaded. Missing art is not fatal: RenderBackground
    // falls back to the plain gradient.
    const std::string worldName =
        engine_ ? engine_->GetSelectedWorldName() : std::string();

    std::size_t artworkIndex = 0;
    for (char c : worldName)
        artworkIndex = artworkIndex * 31 + static_cast<std::size_t>(static_cast<unsigned char>(c));
    artworkIndex %= kBackdropArtwork.size();

    worldBackdrop_ =
        assets->LoadTexture(kBackdropArtwork[artworkIndex]);
    if (!worldBackdrop_)
        LOG_WARN(std::format("GameScreen: backdrop '{}' unavailable; the sky is the plain gradient",
                             kBackdropArtwork[artworkIndex]));

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
    worldBackdrop_.reset();
}

void GameScreen::Update(float deltaTime)
{
    // Server-accepted world edits, applied before anything reads the world.
    ApplyPendingTileEdits();

    // Jump sounds. Engine::Update runs the ECS systems before the screen, so
    // PlayerSystem has already counted this frame's jumps by the time we get
    // here; drain the counter and take one per jump. ConsumeJumpCount returns
    // zero once drained, so a frame with no jump costs one call.
    if (auto systemManager = ServiceLocator::Get<StrixVerse::ECS::SystemManager>())
    {
        if (auto playerSystem = systemManager->getSystem<StrixVerse::ECS::PlayerSystem>())
        {
            while (playerSystem->ConsumeJumpCount() > 0)
                PlaySfx("jump");
        }
    }

    TrackAirborne();

    // The drop is announced once, on the transition - NetworkManager keeps
    // the failure description, but the stack only needs to know it happened.
    if (engine_)
    {
        const bool connected = engine_->getNetworkManager().isConnected();
        if (wasConnected_ && !connected && hud_)
            hud_->AddNotification("Connection lost.");
        wasConnected_ = connected;
    }

    if (hud_)
        hud_->Update(deltaTime);

    PublishLocalPosition(deltaTime);

    // Redraw the hotbar only when the inventory actually changed, rather than
    // rebuilding it every frame.
    if (engine_ && engine_->getNetworkManager().getInventoryRevision() != inventoryRevision_)
        RefreshInventory();

    if (engine_ && engine_->getNetworkManager().getStatsRevision() != statsRevision_)
        RefreshStats();

    // The character panel follows the same stats revision; both calls are
    // no-ops until it moves.
    RefreshCharacterPanel();

    // The roster has no revision to key on and is small, so it is pushed
    // every frame and the panel decides what to do with it.
    RefreshRoster();

    // Buffs stay empty until buff packets exist; Update keeps an empty
    // display hidden at no cost.
    if (buffDisplay_)
        buffDisplay_->Update(deltaTime);

    // Gameplay keys are polled rather than event-driven - Engine translates
    // only the editing keys, so I/C/E/Tab never arrive as anything usable.
    HandleGameplayKeys();

    particles_.Update(deltaTime);
    UpdateAmbientAether(deltaTime);

    // After the camera has been moved for this frame by Camera2DSystem, so a
    // bubble is placed against where its speaker is now rather than trailing
    // them by a frame.
    UpdateChatBubbles(deltaTime);
    UpdateNameTags();

    // Both of the calls above can insert a top-level element, and so can
    // HUD::AddNotification at any moment. UIManager draws in insertion order,
    // so each of them lands *over* whatever overlay is open - a name tag and a
    // notification card floating on top of the pause dim. Re-asserting the open
    // overlay afterwards is one list operation on a handful of elements, and
    // only while something is actually open.
    if (paused_ && pauseOverlay_)
        pauseOverlay_->RaiseToFront();
    else if (worldManagerPanel_ && worldManagerPanel_->IsOpen())
        worldManagerPanel_->RaiseToFront();
    else if (inventoryPanel_ && inventoryPanel_->IsOpen())
        inventoryPanel_->RaiseToFront();
    else if (characterPanel_ && characterPanel_->IsOpen())
        characterPanel_->RaiseToFront();
    else if (playerListPanel_ && playerListPanel_->IsOpen())
        playerListPanel_->RaiseToFront();
    UpdateInteractPrompt();

    // Leaving is confirmed by the server, never assumed here. It answers
    // WorldLeave on success and says nothing on refusal -- it refuses when the
    // player is not really standing at the door -- so a press that changes
    // nothing correctly leaves the player where they are.
    //
    // The session stays authenticated across this, so the browser's ordinary
    // join path works on the way back in without another login.
    if (engine_)
    {
        const uint32_t left = engine_->getNetworkManager().getWorldLeftRevision();
        if (left != worldLeftRevision_)
        {
            worldLeftRevision_ = left;
            LOG_INFO("GameScreen: the server confirmed the world was left");
            RequestScreenChange(ScreenID::WorldBrowser);
            return;
        }
    }

    // WorldInfo arriving is what opens the wrench panel. The click only asked;
    // the server decides whether this player sees anything at all, so a refusal
    // simply never produces one of these and the panel never appears.
    if (engine_ && worldManagerPanel_)
    {
        const uint32_t revision = engine_->getNetworkManager().getWorldInfoRevision();
        if (revision != worldInfoRevision_)
        {
            worldInfoRevision_ = revision;
            if (!worldManagerPanel_->IsOpen())
            {
                // The server answering WorldInfo *is* the interaction
                // succeeding, so this is where the burst belongs - a claim or
                // an open-management both land here.
                EmitCoreBurst();
                worldManagerPanel_->Show();
            }
        }

        worldManagerPanel_->Refresh();
    }
}

void GameScreen::RenderBackground() const
{
    auto batch = ServiceLocator::Get<SpriteBatch>();
    if (!batch || !playerTexture_ || !engine_)
        return;

    // The same visible-rect math TileRendererSystem uses, so the sky exactly
    // underwrites the tiles no matter where the camera is or how far out it
    // is zoomed - plus a tile of margin on every side, for the same reason.
    const Camera2D& camera   = engine_->GetCamera();
    const glm::vec2 viewport = camera.GetViewport();
    const glm::vec2 centre   = camera.GetPosition();
    const float     zoom     = camera.GetZoom() > 0.0f ? camera.GetZoom() : 1.0f;

    const float halfWidth  = (viewport.x * 0.5f) / zoom + kTileSize;
    const float halfHeight = (viewport.y * 0.5f) / zoom + kTileSize;

    const float left  = centre.x - halfWidth;
    const float right  = centre.x + halfWidth;
    const float top    = centre.y - halfHeight;
    const float bottom = centre.y + halfHeight;

    const float worldHeightPx =
        static_cast<float>(worldHeightInTiles_) * static_cast<float>(kTileSize);
    if (right <= left || bottom <= top || worldHeightPx <= 0.0f)
        return;

    batch->Begin();

    if (worldBackdrop_)
    {
        // One copy of the artwork, stretched over the world's full height.
        //
        // It used to tile vertically like it tiles horizontally, and that read
        // as a bug: the seam between two stacked copies puts the bottom of one
        // picture - its grass - directly above the sky of the next, so a
        // yellow-green band floated at the top of the screen behind the HUD.
        // Anchored to the world instead, the picture's own sky sits over the
        // game's sky and its grass lands deep underground, where the depth
        // darkening below buries it.
        const float imgW = static_cast<float>(worldBackdrop_->GetWidth());
        const float imgH = static_cast<float>(worldBackdrop_->GetHeight());
        if (imgW > 0.0f && imgH > 0.0f)
        {
            constexpr float kParallax = 0.25f;    // moves at a quarter of the world
            constexpr float kMargin   = 4.0f;     // tiles of overscan, top and bottom

            const float drawTop    = -kMargin * kTileSize;
            const float drawHeight = worldHeightPx + 2.0f * kMargin * kTileSize;
            const float drawWidth  = drawHeight * (imgW / imgH);

            // Horizontal parallax only: the layer slides at kParallax of the
            // camera's sideways speed. Shifting the tile grid by
            // centre.x * (1 - kParallax) is what makes that read - as the
            // camera moves d, the grid moves d*(1-p) the other way, and the
            // picture crosses the screen at d*p.
            const float offsetX = centre.x * (1.0f - kParallax);

            auto firstBefore = [](float viewEdge, float size, float shift) {
                // Largest grid line at or before viewEdge, for tiles spaced
                // `size` apart on an origin shifted by `shift`.
                float x = viewEdge - shift;
                x = std::floor(x / size) * size;
                return x + shift;
            };

            // Pulled toward dusk so foreground tiles keep their contrast; the
            // loading screen scrims its copy of the same art for the identical
            // reason.
            const Color tint(0.62f, 0.64f, 0.74f, 1.0f);

            const float startX = firstBefore(left - drawWidth, drawWidth, offsetX);

            for (float x = startX; x < right; x += drawWidth)
            {
                batch->Draw(*worldBackdrop_, x, drawTop, drawWidth, drawHeight,
                            tint.r, tint.g, tint.b, tint.a);
            }

            // Depth darkening over the art. The surface sits in the upper
            // third of most worlds, so from mid-height down the picture fades
            // out and digging reads as getting darker rather than as the sky
            // following you down.
            constexpr int   kStripCount = 48;
            const float     stripHeight = (bottom - top) / static_cast<float>(kStripCount);

            for (int i = 0; i < kStripCount; ++i)
            {
                const float y     = top + stripHeight * static_cast<float>(i);
                const float t     = std::clamp((y + stripHeight * 0.5f) / worldHeightPx,
                                               0.0f, 1.0f);

                float fade = (t - 0.40f) / (0.90f - 0.40f);
                fade       = std::clamp(fade, 0.0f, 1.0f);
                fade *= fade;   // ease-in, so the surface band stays clear

                if (fade <= 0.0f)
                    continue;

                batch->Draw(*playerTexture_, left, y, right - left, stripHeight + 0.5f,
                            0x07 / 255.0f, 0x06 / 255.0f, 0x0D / 255.0f, fade);
            }

            batch->End();
            return;
        }
    }

    // No artwork: the plain gradient stands alone, anchored to world height.
    static const Color kSkyStops[] = {
        UITheme::Hex(0x6FB6E8),   // open Aether sky
        UITheme::Hex(0x3D5C94),   // thinning toward the horizon
        UITheme::Hex(0x2A2350),   // dusk band of violet over the surface
        UITheme::Hex(0x151024),   // shallow underground
        UITheme::Hex(0x07060D),   // deep underground
    };

    auto skyAt = [&](float worldY) {
        const float t       = std::clamp(worldY / worldHeightPx, 0.0f, 1.0f);
        const float scaled  = t * static_cast<float>(std::size(kSkyStops) - 1);
        const auto  index   = static_cast<std::size_t>(scaled);
        const auto  next    = static_cast<std::size_t>(
            std::min<float>(scaled + 1.0f, static_cast<float>(std::size(kSkyStops) - 1)));

        return Color::Lerp(kSkyStops[index], kSkyStops[next],
                           scaled - static_cast<float>(index));
    };

    constexpr int   kStripCount = 64;
    const float     stripHeight = (bottom - top) / static_cast<float>(kStripCount);

    for (int i = 0; i < kStripCount; ++i)
    {
        const float y     = top + stripHeight * static_cast<float>(i);
        const Color color = skyAt(y + stripHeight * 0.5f);

        // The half-pixel of overlap hides the seam a hard strip edge would
        // leave at some zooms.
        batch->Draw(*playerTexture_, left, y, right - left, stripHeight + 0.5f,
                    color.r, color.g, color.b, color.a);
    }

    batch->End();
}

void GameScreen::RenderGame() const
{
    // Runs right after SystemManager::render in Engine::Render, so particles
    // draw over tiles and players but under the UI, through the same camera
    // projection and white texture everything else in the world uses.
    auto batch = ServiceLocator::Get<SpriteBatch>();
    if (!batch || !playerTexture_)
        return;

    particles_.Render(*batch, *playerTexture_);
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

void GameScreen::PlaySfx(const std::string& baseName)
{
    if (engine_)
        engine_->GetAudio().PlaySfx(baseName);
}

void GameScreen::PlayBreakSfx(std::uint8_t brokenServerId)
{
    if (!engine_)
        return;

    // The material take first, the generic name when only that exists - a
    // false return means "nothing by that name", which is harmless.
    if (engine_->GetAudio().PlaySfx(BreakSoundForServerId(brokenServerId)))
        return;

    engine_->GetAudio().PlaySfx("break");
}

std::uint16_t GameScreen::SelectedToolItemId() const
{
    // Hotbar slots 0 and 1 are the client-side PUNCH/WRENCH tools, so server
    // inventory slot N sits at hotbar slot N + 2 - the same mapping
    // OnRightMouseDown reads for placing.
    if (!engine_ || !hud_ || hud_->GetSelectedTool() != HUD::Tool::Item)
        return 0;

    const uint8_t inventorySlot =
        static_cast<uint8_t>(hud_->GetSelectedSlot() - HUD::kFirstItemSlot);

    const auto& inventory = engine_->getNetworkManager().getInventory();
    const auto it = inventory.find(inventorySlot);
    if (it == inventory.end() || it->second.IsEmpty())
        return 0;

    return it->second.itemId;
}

void GameScreen::EmitCoreBurst()
{
    // Centred on the tile the interaction targeted, in local world pixels -
    // TileYToLocalY already does the server-Y flip, so +half a tile is the
    // middle of that tile's square.
    const float cx = static_cast<float>(interactTileX_) * kTileSize + kTileSize * 0.5f;
    const float cy = TileYToLocalY(static_cast<float>(interactTileY_) + 0.5f);

    // Two interleaved bursts, one end of the Aether palette each, so the puff
    // reads violet-blue rather than either alone.
    particles_.EmitBurst(cx, cy, 20, 40.0f, 150.0f, UITheme::Secondary, 60.0f, 0.8f);
    particles_.EmitBurst(cx, cy, 20, 40.0f, 150.0f, UITheme::Accent,   60.0f, 0.9f);
}

void GameScreen::UpdateAmbientAether(float deltaTime)
{
    if (!engine_)
        return;

    constexpr float kAetherInterval = 0.5f;

    aetherTimer_ += deltaTime;
    if (aetherTimer_ < kAetherInterval)
        return;

    aetherTimer_ = 0.0f;

    // The visible bounds exactly as TileRendererSystem derives them: camera
    // centre plus half the viewport over zoom, floored to tiles. One mote per
    // tick anywhere inside costs a couple of comparisons and one particle.
    const Camera2D& camera = engine_->GetCamera();
    const glm::vec2 viewport = camera.GetViewport();
    if (viewport.x <= 0.0f || viewport.y <= 0.0f)
        return;

    const float zoom   = camera.GetZoom() > 0.0f ? camera.GetZoom() : 1.0f;
    const glm::vec2 centre = camera.GetPosition();

    const float halfWidth  = (viewport.x * 0.5f) / zoom;
    const float halfHeight = (viewport.y * 0.5f) / zoom;

    const float x = centre.x + (std::rand() / static_cast<float>(RAND_MAX) * 2.0f - 1.0f) * halfWidth;
    const float y = centre.y + (std::rand() / static_cast<float>(RAND_MAX) * 2.0f - 1.0f) * halfHeight;

    particles_.EmitAether(x, y);
}

void GameScreen::OnKeyDown(int key, bool, bool)
{
    // This only runs while nothing holds keyboard focus, so Enter here always
    // means "start typing" and never "send".
    if (key == UIKey::Enter)
    {
        PlaySfx("ui_click");
        if (hud_)
            hud_->FocusChatInput();
        return;
    }

    // Escape unwinds one layer per press: the topmost overlay first, with
    // pause last so a stack closes in the order it was opened. This only
    // runs once UIManager has already consumed an Escape to clear field
    // focus, so Escape while typing abandons the message and nothing here.
    if (key == UIKey::Escape)
    {
        if (pauseOverlay_ && pauseOverlay_->IsOpen())
            SetPaused(false);
        else if (worldManagerPanel_ && worldManagerPanel_->IsOpen())
            worldManagerPanel_->Hide();
        else if (inventoryPanel_ && inventoryPanel_->IsOpen())
            inventoryPanel_->Close();
        else if (characterPanel_ && characterPanel_->IsOpen())
            characterPanel_->Close();
        else if (playerListPanel_ && playerListPanel_->IsOpen())
            playerListPanel_->Close();
        else if (vaultPanel_ && vaultPanel_->IsOpen())
            vaultPanel_->Close();
        else if (gatePanel_ && gatePanel_->IsOpen())
            gatePanel_->Close();
        else if (stabilizerPanel_ && stabilizerPanel_->IsOpen())
            stabilizerPanel_->Close();
        else if (memoryCrystalPanel_ && memoryCrystalPanel_->IsOpen())
            memoryCrystalPanel_->Close();
        else
            SetPaused(true);
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

    // Panel and interaction keys. The event path is the right one for these -
    // they are presses, not held states - and key repeat is suppressed at the
    // source, so a held I toggles exactly once. Tab stays polled in
    // HandleGameplayKeys: hold-to-show needs the release, and the event path
    // carries no key-up. The same gate as clicks applies, so E behind an open
    // vault does not act on the terrain behind the panel.
    if (GameplayInputBlocked())
        return;

    switch (key)
    {
    case UIKey::LetterI:
        if (inventoryPanel_ && !inventoryPanel_->IsOpen())
            ClosePanelOverlays();
        if (inventoryPanel_)
            inventoryPanel_->Toggle();
        break;

    case UIKey::LetterC:
        if (characterPanel_ && !characterPanel_->IsOpen())
            ClosePanelOverlays();
        if (characterPanel_)
            characterPanel_->Toggle();
        break;

    case UIKey::LetterE:
        InteractWithTarget();
        break;

    default:
        break;
    }
}

void GameScreen::HandleGameplayKeys()
{
    // Tab is the one gameplay key still read as a held state: hold-to-show
    // needs the release, and the event path carries no key-up. Everything
    // press-shaped moved to OnKeyDown now that the engine translates the
    // whole keyboard. Blocked frames still refresh the previous state, so
    // releasing Tab behind a focused chat field does not queue a phantom
    // open for when focus goes away.
    const bool* state = SDL_GetKeyboardState(nullptr);
    const bool  tab   = state[SDL_SCANCODE_TAB] != 0;

    // The management panel and the Lost Technology devices count as blocking
    // here, exactly as they do for clicks. The toggle panels deliberately do
    // NOT block: Tab over an open inventory still shows the player list, and
    // AnyPanelOpen is the click gate's test and would trap everything shut.
    const bool blocked = !uiManager_ || uiManager_->isTextInputFocused() || paused_ ||
                         (worldManagerPanel_ && worldManagerPanel_->IsOpen()) ||
                         (vaultPanel_         && vaultPanel_->IsOpen()) ||
                         (gatePanel_          && gatePanel_->IsOpen()) ||
                         (stabilizerPanel_    && stabilizerPanel_->IsOpen()) ||
                         (memoryCrystalPanel_ && memoryCrystalPanel_->IsOpen());

    if (!blocked && tab && !prevPlayerListKey_)
    {
        ClosePanelOverlays();
        if (playerListPanel_)
            playerListPanel_->Open();
    }

    // Tab's release is handled whether or not the frame was blocked.
    //
    // The open edge used to sit inside the blocked gate while the
    // previous-state below was updated unconditionally, so a release that
    // happened during a blocked frame was swallowed and then forgotten: hold
    // Tab, press Escape, let go, and the player list stayed up for good with no
    // key held. Closing is the safe half of a hold-to-show binding and nothing
    // is gained by suppressing it.
    if (!tab && prevPlayerListKey_ && playerListPanel_ && playerListPanel_->IsOpen())
    {
        playerListPanel_->Close();
    }

    prevPlayerListKey_ = tab;
}

void GameScreen::OnMouseWheel(float, float, float delta)
{
    // GameplayInputBlocked already covers paused_; it is the single answer to
    // "is gameplay input live", and restating one of its terms here invites the
    // next reader to think it is not.
    if (!hud_ || GameplayInputBlocked())
        return;

    // Shift or ctrl held: the wheel zooms the camera instead of cycling the
    // hotbar. Both features wanted the wheel; this is how both get it without
    // a notch doing two things at once.
    const bool* keys    = SDL_GetKeyboardState(nullptr);
    const bool  zoomHeld = keys[SDL_SCANCODE_LSHIFT] != 0 || keys[SDL_SCANCODE_RSHIFT] != 0 ||
                           keys[SDL_SCANCODE_LCTRL]  != 0 || keys[SDL_SCANCODE_RCTRL]  != 0;

    if (zoomHeld && delta != 0.0f)
    {
        auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();
        if (!componentManager || cameraEntity_ == StrixVerse::ECS::NULL_ENTITY)
            return;

        auto* cameraComp =
            componentManager->getComponent<StrixVerse::ECS::Camera2DComponent>(cameraEntity_);
        if (!cameraComp)
            return;

        // The zoom lives on the component rather than on Camera2D directly,
        // because Camera2DSystem writes the camera from the component every
        // frame and would overwrite anything set on the camera itself.
        const float previous = cameraComp->zoom > 0.0f ? cameraComp->zoom : 1.0f;

        float next = delta > 0.0f ? previous * kZoomStep : previous / kZoomStep;
        next       = std::clamp(next, kMinZoom, kMaxZoom);

        if (next != previous)
            cameraComp->zoom = next;

        return;
    }

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
    // echo is what puts our own line in the log - and, for the same reason,
    // what raises our own bubble. Nothing comes back to trigger it.
    if (hud_)
        hud_->AddChatMessage(author + ": " + message);

    ShowChatBubble(kLocalSpeakerId, message);
}

void GameScreen::OnChatReceived(uint64_t senderId, const std::string& message)
{
    if (!hud_ || message.empty())
        return;

    hud_->AddChatMessage(DisplayNameFor(senderId) + ": " + message);

    // And over their head, if they have one in this world.
    ShowChatBubble(senderId, message);
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
        hud_->AddNotification(username + " joined.");

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
        hud_->AddNotification(DisplayNameFor(entityId) + " left.");

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

    // The gameplay-pause gate lives on InputSystem, which outlives this
    // screen. Leaving it set would freeze the next world's player.
    SetPaused(false);

    // Bubbles hold UIManager elements and are keyed on entities DestroyActors
    // is about to invalidate, so they go first.
    for (auto& [speakerId, bubble] : chatBubbles_)
    {
        (void)speakerId;
        if (bubble.panel && uiManager_)
            uiManager_->removeElement(bubble.panel);
    }
    chatBubbles_.clear();

    for (auto& [speakerId, tag] : nameTags_)
    {
        (void)speakerId;
        if (tag && uiManager_)
            uiManager_->removeElement(tag);
    }
    nameTags_.clear();

    hud_.reset();

    // Panel overlays hold UIManager elements; each destructor hands its root
    // back to the UIManager, which outlives the screen. Reset before the
    // prompt element below, which nothing else owns.
    pauseOverlay_.reset();
    playerListPanel_.reset();
    inventoryPanel_.reset();
    characterPanel_.reset();
    buffDisplay_.reset();

    vaultPanel_.reset();
    gatePanel_.reset();
    stabilizerPanel_.reset();
    memoryCrystalPanel_.reset();

    if (interactPromptPanel_ && uiManager_)
        uiManager_->removeElement(interactPromptPanel_);
    interactPromptPanel_.reset();
    interactPromptLabel_.reset();

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
