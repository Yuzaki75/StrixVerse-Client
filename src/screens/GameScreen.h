#pragma once

#include <memory>

#include <cstdint>
#include <string>
#include <unordered_map>

#include "Screen.h"
#include "../core/world/World.h"
#include "../ecs/Entity.h"
#include "../networking/PacketHandler.h"
#include "../ecs/TileRendererSystem.h"
#include "../hud/HUD.h"

class UIButton;
class UILabel;

/**
 * Gameplay screen.
 *
 * Owns the world instance, the tile renderer and the HUD. Its own chrome is
 * deliberately minimal - the gameplay HUD is a separate design surface - but it
 * has been ported onto the repaired UI system and shared theme.
 */
class GameScreen : public Screen
{
public:
    explicit GameScreen(Engine* engine);
    ~GameScreen() override = default;

    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;
    void OnKeyDown(int key, bool ctrl, bool shift) override;

private:
    void InitializeUI();
    void InitializeHUD();
    void InitializeWorld();

    // Fills world_ from the chunks NetworkManager collected during the loading
    // screen. Does nothing if none arrived.
    void BuildWorldFromServerTerrain();

    // Left click breaks, right click places. Both only ask -- the server
    // decides, and the change appears when its broadcast arrives.
    void OnMouseDown(float x, float y) override;
    void OnRightMouseDown(float x, float y) override;

    // True while a UI element holds focus or the game is paused, so world edits
    // are suppressed for the same reason movement already is.
    bool GameplayInputBlocked() const;

    // --- Pause -------------------------------------------------------------
    // An overlay, not a screen change. Escape used to call
    // RequestScreenChange(ScreenID::Settings), which runs OnExit and destroys
    // the HUD, the player entity, the camera and every remote player, then
    // rebuilds them all on the way back. The world stays live behind this.
    void BuildPausePanel();
    void SetPaused(bool paused);

    bool                       paused_ = false;
    std::shared_ptr<UIPanel>   pausePanel_;

    // Applies edits the server has accepted. Drained every frame so an edit
    // that lands mid-transition is not lost.
    void ApplyPendingTileEdits();

    // Wrench: reports whoever is standing on the given tile.
    void InspectPlayerAt(int32_t tileX, int32_t tileY);

    // Canvas point -> world tile, in the server's coordinate space. Returns
    // false if the click cannot be resolved (no player, no world).
    bool CanvasToServerTile(float canvasX, float canvasY,
                            int32_t& outTileX, int32_t& outTileY) const;

    // The server's tile Y runs upward (higher Y is closer to the sky); the
    // screen's runs downward. Every conversion between the two goes through
    // this pair, so terrain, the local player and remote players cannot end up
    // disagreeing about where the ground is. Applying the flip to only some of
    // them leaves the player standing in mid-air.
    float TileYToLocalY(float tileY) const;
    float LocalYToTileY(float localY) const;

    // --- Player anchor -----------------------------------------------------
    // The pair above converts a bare coordinate. These convert a *player*, and
    // they are not the same thing: the server models a player as the one tile
    // they occupy and looks at the tile directly below it for ground, while
    // the client's transform is the top-left corner of a sprite three quarters
    // of a tile wide and one and a half tall.
    //
    // Reporting that corner put a standing player a tile and a half above
    // their own feet. Everything downstream of the position agreed with the
    // server about the number and disagreed about what it meant, so the tile
    // below it was air and Player::IsAirborneMovementAllowed read someone
    // stood on the ground as hovering. That costs nothing while the player
    // flies, which is why it went unnoticed, and breaks the moment there is
    // gravity to be grounded against.
    //
    // Both directions are here so the round trip is exact.
    float PlayerLocalXToTileX(float localX) const;
    float PlayerLocalYToTileY(float localY) const;
    void  PlayerTileToLocal(float tileX, float tileY,
                            float& outLocalX, float& outLocalY) const;

    // Height of the built world in tiles, which is what the flip above pivots
    // around. Set from the server's chunks; the default matches the server's
    // 4-chunk-tall world so the conversion is still sane before terrain lands.
    int worldHeightInTiles_ = 64;

    // Spawns the local player and the camera that follows it.
    void InitializeActors();

    // True when the player box would overlap solid terrain here. Used to
    // report a disagreement with the server rather than act on one.
    bool collisionBlocksSpawn(float x, float y) const;

    // Nudges a spawn point to the nearest position the player fits in. Only
    // used for the fallback spawn - a position the server gave us is never
    // moved, because it is right and we are guessing.
    void FindFreeSpawn(float& x, float& y) const;

    void DestroyActors();

    void OnSettingsButtonClicked();

    // Sends a typed line and echoes it locally.
    void SubmitChat(const std::string& message);

    // Appends a line another player sent.
    void OnChatReceived(uint64_t senderId, const std::string& message);

    // --- Player replication ------------------------------------------------
    void RegisterNetworkHandlers();
    void UnregisterNetworkHandlers();

    // `announce` is false when replaying the roster of players who were
    // already in the world before this screen was built.
    void OnPlayerSpawn(uint64_t entityId, const std::string& username,
                       float tileX, float tileY, bool announce = true);
    void OnPlayerMove(uint64_t entityId, float tileX, float tileY);
    void OnPlayerRemove(uint64_t entityId);

    // Reports the local player's position, rate limited and only when it has
    // actually changed.
    void PublishLocalPosition(float deltaTime);

    // Watches the grounded flag CollisionSystem writes and logs one line per
    // airborne episode, on landing: how far the player climbed and how far
    // they came down, in tiles. One line per jump or fall rather than one per
    // frame, and it is the only way to see the arc a screenshot cannot show.
    void TrackAirborne();

    bool  wasGrounded_    = true;
    float groundedTileY_  = 0.0f;   // last server tile Y stood on
    float airborneFromY_  = 0.0f;   // server tile Y at take-off
    float airbornePeakY_  = 0.0f;   // highest server tile Y reached
    float airborneLowY_   = 0.0f;   // lowest server tile Y reached

    // Moves the local player to where the server says it is.
    void ApplyServerCorrection(float tileX, float tileY);

    // Name for a player id, falling back to the id when it is not known.
    std::string DisplayNameFor(uint64_t entityId) const;

    // Pushes NetworkManager's inventory into the HUD.
    void RefreshInventory();

    // Pushes NetworkManager's character stats into the HUD.
    void RefreshStats();

    std::shared_ptr<UILabel>  worldLabel_;
    std::shared_ptr<UIButton> settingsButton_;

    std::unique_ptr<HUD> hud_;

    std::unique_ptr<StrixVerse::World::World> world_;

    // Kept so the handlers can be removed again when the screen exits.
    std::shared_ptr<PacketHandler> chatHandler_;
    std::shared_ptr<PacketHandler> spawnHandler_;
    std::shared_ptr<PacketHandler> moveHandler_;
    std::shared_ptr<PacketHandler> removeHandler_;

    // Other players in the world, keyed by the server's entity id.
    std::unordered_map<uint64_t, StrixVerse::ECS::Entity> remotePlayers_;
    std::unordered_map<uint64_t, std::string>             remoteNames_;

    // Shared by every player sprite; kept alive for the screen's lifetime.
    std::shared_ptr<Texture> playerTexture_;

    // Last inventory revision drawn, so the hotbar is rebuilt only on change.
    uint32_t inventoryRevision_ = 0;
    uint32_t statsRevision_     = 0;

    float moveSendTimer_ = 0.0f;
    float lastSentTileX_ = 0.0f;
    float lastSentTileY_ = 0.0f;
    bool  hasSentMove_   = false;

    // The tile renderer is owned by the Engine's SystemManager and lives for
    // the whole session; this screen only lends it a world to draw.
    StrixVerse::ECS::Entity playerEntity_ = StrixVerse::ECS::NULL_ENTITY;
    StrixVerse::ECS::Entity cameraEntity_ = StrixVerse::ECS::NULL_ENTITY;
};
