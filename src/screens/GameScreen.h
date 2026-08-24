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
#include "../fx/ParticleSystem.h"
#include "../hud/HUD.h"
#include "../hud/WorldManagerPanel.h"
#include "../hud/PlayerListPanel.h"
#include "../hud/InventoryPanel.h"
#include "../hud/CharacterPanel.h"
#include "../hud/BuffDisplay.h"
#include "../hud/PauseOverlay.h"
#include "../hud/VaultPanel.h"
#include "../hud/GatePanel.h"
#include "../hud/StabilizerPanel.h"
#include "../hud/MemoryCrystalPanel.h"

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
    void OnMouseWheel(float x, float y, float delta) override;

    // World-space extras drawn after the systems' render pass: gameplay
    // particles. The camera projection is already on the SpriteBatch by the
    // time this runs, so a particle lands on the tile it was emitted over.
    void RenderGame() const override;

    // The build/break cursor: an outline on the tile under the mouse, bright
    // inside the 2x2x2 reach and dull red past it, so a refused click is
    // legible before it happens.
    void DrawHoverHighlight(SpriteBatch& batch) const;

    // Sky drawn before the systems' render pass, so tiles and players sit on
    // a gradient instead of the flat clear colour. Anchored to world height:
    // daylight Aether blue at the top of the map shading down to near-black
    // in the deep underground, whatever part of it the camera can see.
    void RenderBackground() const override;

private:
    void InitializeUI();
    void InitializeHUD();
    void InitializeWorld();

    // The UI overlays that are not part of the HUD proper: pause, player
    // list, inventory, character sheet and buff display. Built once in
    // OnEnter against the UIManager and shown or hidden from there.
    void InitializePanels();

    // Fills world_ from the chunks NetworkManager collected during the loading
    // screen. Does nothing if none arrived.
    void BuildWorldFromServerTerrain();

    // Left click is the only world action: the selected hotbar slot decides
    // whether it punches, wrenches or places. It only asks -- the server
    // decides, and the change appears when its broadcast arrives.
    //
    // OnRightMouseDown is deliberately not overridden. The base is a no-op, so
    // the right button does nothing in the world rather than doing a second,
    // redundant thing.
    void OnMouseDown(float x, float y) override;

    // The right button uses what is selected. Placing moved to the left button
    // with everything else, which left this one free; a potion had no way to
    // be drunk before it.
    void OnRightMouseDown(float x, float y) override;

    // True while a UI element holds focus or the game is paused, so world edits
    // are suppressed for the same reason movement already is.
    bool GameplayInputBlocked() const;

    // True when a HUD / chrome control is under the cursor, so a click on the
    // hotbar or chat does not also punch the tile behind it.
    bool UiConsumesPointer(float x, float y) const;

    // --- Pause -------------------------------------------------------------
    // An overlay, not a screen change. Escape used to call
    // RequestScreenChange(ScreenID::Settings), which runs OnExit and destroys
    // the HUD, the player entity, the camera and every remote player, then
    // rebuilds them all on the way back. The world stays live behind this.
    void SetPaused(bool paused);

    bool paused_ = false;

    // --- Panel overlays ----------------------------------------------------
    // One panel at a time: opening one closes the others. Escape closes the
    // topmost, with pause last so a stack unwinds one layer per press.
    void ClosePanelOverlays();

    // True while any overlay that covers live terrain is up. One list, so the
    // click gate, the key gate and the Escape ladder cannot drift apart.
    bool AnyPanelOpen() const;

    std::unique_ptr<PlayerListPanel> playerListPanel_;
    std::unique_ptr<InventoryPanel>  inventoryPanel_;
    std::unique_ptr<CharacterPanel>  characterPanel_;
    std::unique_ptr<BuffDisplay>     buffDisplay_;
    std::unique_ptr<PauseOverlay>    pauseOverlay_;

    // Lost Technology interfaces, opened from the world with E rather than a
    // key. Same lifecycle as the panels above.
    std::unique_ptr<VaultPanel>         vaultPanel_;
    std::unique_ptr<GatePanel>          gatePanel_;
    std::unique_ptr<StabilizerPanel>    stabilizerPanel_;
    std::unique_ptr<MemoryCrystalPanel> memoryCrystalPanel_;

    // Cosmetic effects pool. Updated every frame and drawn in RenderGame;
    // emission points land with Wave-2b.
    StrixVerse::FX::ParticleSystem particles_;

    // --- Gameplay keys -----------------------------------------------------
    // Engine::TranslateKey maps only the twelve editing keys, so letters and
    // Tab never reach OnKeyDown as anything but None. These bindings are
    // polled from the hardware keyboard instead - the same source
    // InputSystem reads - with edge detection here, which also makes them
    // immune to key-repeat events that the event path never filters.
    // Level-based hold-to-show falls out of the same read for free.
    void HandleGameplayKeys();

    bool prevPlayerListKey_ = false;  // Tab (hold to show; polled for key-up)

    // --- Interaction -------------------------------------------------------
    // Scans the tiles around the player for something interactable (a Strix
    // Core, the main door, or one of the Lost Technology devices) each frame,
    // shows a prompt naming what it found when one is in range, and keeps the
    // tile it found so pressing E acts on what the prompt is pointing at
    // rather than on whatever is under the cursor.
    void UpdateInteractPrompt();
    void InteractWithTarget();

    // What kind of thing the interact scan can act on.
    enum class InteractTarget
    {
        None,
        StrixCore,
        MainDoor,
        Vault,
        Gate,
        Stabilizer,
        MemoryCrystal
    };

    std::shared_ptr<UIPanel> interactPromptPanel_;
    std::shared_ptr<UILabel> interactPromptLabel_;

    int32_t interactTileX_ = 0;
    int32_t interactTileY_ = 0;

    // What was found at interactTileX_/Y_, so pressing E routes on what the
    // prompt is showing rather than re-reading the world.
    InteractTarget interactTarget_ = InteractTarget::None;

    // How far, in tiles, the scan reaches around the player. Doubles as the
    // break/place reach: one measure, so the prompt, the hover highlight and
    // the click gate can never disagree.
    static constexpr int kInteractRadius = 2;

    // True when the tile sits inside the 2x2x2 reach box around the player.
    bool TileWithinReach(int32_t tileX, int32_t tileY) const;

    // Applies edits the server has accepted. Drained every frame so an edit
    // that lands mid-transition is not lost.
    void ApplyPendingTileEdits();

    // True when a Strix Core stands on this tile, in the server's coordinate
    // space. Any of its five ids: unclaimed, or claimed at levels I to IV.
    // The server tile id stored at this position, or 0 for air or out of
    // bounds. One place does the coordinate flip, so callers cannot disagree.
    std::uint8_t ServerIdAt(int32_t tileX, int32_t tileY) const;

    bool IsStrixCoreAt(int32_t tileX, int32_t tileY) const;

    // What is interactable on this tile, if anything.
    InteractTarget InteractTargetAt(int32_t tileX, int32_t tileY) const;

    // Wrench: reports whoever is standing on the given tile.
    void InspectPlayerAt(int32_t tileX, int32_t tileY);

    // Canvas point -> world tile, in the server's coordinate space. Returns
    // false if the click cannot be resolved (no player, no world).
    bool CanvasToServerTile(float canvasX, float canvasY,
                            int32_t& outTileX, int32_t& outTileY) const;

    // --- Canvas <-> world pixels -------------------------------------------
    // Both directions go through the live camera - its position, its zoom and
    // its viewport - rather than assuming the player is in the middle of the
    // screen at zoom 1. That assumption was wrong twice over: the camera clamps
    // to the world bounds, so a player near an edge is off-centre, and the
    // wheel now changes the zoom.
    //
    // The pair is here so the two can be checked against each other; a click
    // that resolves to a tile and a bubble that floats over a head are the same
    // conversion read in opposite directions.
    bool CanvasToWorldPixel(float canvasX, float canvasY, float& outX, float& outY) const;
    bool WorldPixelToCanvas(float worldX, float worldY, float& outX, float& outY) const;

    // --- Zoom ---------------------------------------------------------------
    // Zoom rides on OnMouseWheel behind a shift/ctrl modifier; the wheel's
    // plain gesture belongs to hotbar cycling.

    // Multiplicative, so each notch feels the same at every zoom level; a fixed
    // step crawls when zoomed out and lurches when zoomed in.
    static constexpr float kZoomStep = 1.15f;
    static constexpr float kMinZoom  = 0.6f;
    static constexpr float kMaxZoom  = 2.5f;

    // --- Chat bubbles -------------------------------------------------------
    // What someone just said, over their head, for a few seconds. Held as UI
    // elements rather than drawn in RenderGame because text goes through
    // UIRenderer, which runs as its own pass over the design canvas - the world
    // pass has the camera's projection and no way to draw a glyph.
    struct ChatBubble
    {
        std::shared_ptr<UIPanel> panel;
        std::shared_ptr<UILabel> label;
        float remaining = 0.0f;
    };

    // Keyed by the speaker's network id, which is what chat carries. Entity is
    // a handle class with no std::hash, and resolving the id to an entity every
    // frame also handles a remote player who leaves mid-bubble.
    std::unordered_map<uint64_t, ChatBubble> chatBubbles_;

    // Our own messages are echoed locally and carry no sender id, so they need
    // a key that cannot collide with a real one. Server system messages already
    // arrive as id 0, which is why that is not it.
    static constexpr uint64_t kLocalSpeakerId = ~static_cast<uint64_t>(0);

    void ShowChatBubble(uint64_t speakerId, const std::string& message);
    void UpdateChatBubbles(float deltaTime);

    // --- Name tags ---------------------------------------------------------
    // Who each body on screen is, floating over their head. Same positioning
    // as a chat bubble and for the same reason - text goes through UIRenderer,
    // which is a separate pass over the design canvas - but permanent rather
    // than timed, and one per player rather than one per thing said.
    //
    // Keyed the same way as bubbles so both look a player up identically, and
    // so a player who leaves loses their tag by the same code path.
    std::unordered_map<uint64_t, std::shared_ptr<UILabel>> nameTags_;

    void UpdateNameTags();

    // The tag for a speaker, created on first use. Returns nullptr when the
    // speaker has no body in this world - a server system message, say.
    std::shared_ptr<UILabel> NameTagFor(uint64_t speakerId, const std::string& name);

    // How far above the player's own top edge the tag sits, in canvas units.
    // Bubbles clear this so the two never overlap.
    static float NameTagOffset();

    // The ECS entity a speaker id refers to, or NULL_ENTITY if they are gone.
    StrixVerse::ECS::Entity EntityForSpeaker(uint64_t speakerId) const;

    // How long a bubble stays up: a floor, plus reading time for its length.
    static float BubbleLifetimeFor(const std::string& message);

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

    // --- Feedback ----------------------------------------------------------
    // Sound and particle hooks. Every sfx call is safe by construction:
    // AudioManager::PlaySfx logs once and returns false for a missing asset,
    // so a hook costs nothing where the file does not exist.
    void PlaySfx(const std::string& baseName);

    // Picks a break sound from the broken tile's material id, falling back to
    // the generic name when no material take exists.
    void PlayBreakSfx(std::uint8_t brokenServerId);

    // The selected hotbar item's id, for the break packet's tool field.
    // Punch stays 0 (bare hands) and Wrench never reaches a break request.
    std::uint16_t SelectedToolItemId() const;

    // Violet/blue Aether burst over the Strix Core an interaction succeeded on.
    // Records the tile a Strix Core request named, for the burst that
    // follows the server's answer.
    void RememberCoreTarget(int32_t tileX, int32_t tileY);

    void EmitCoreBurst();

    // One ambient Aether mote every so often at a random point in view.
    void UpdateAmbientAether(float deltaTime);

    bool  wasConnected_ = false;    // for the connection-lost notification
    float aetherTimer_  = 0.0f;

    // Moves the local player to where the server says it is.
    void ApplyServerCorrection(float tileX, float tileY);

    // Name for a player id, falling back to the id when it is not known.
    std::string DisplayNameFor(uint64_t entityId) const;

    // Pushes NetworkManager's inventory into the HUD and the inventory panel.
    void RefreshInventory();

    // Pushes NetworkManager's character stats into the HUD.
    void RefreshStats();

    // Pushes the roster (everyone in the world, including us) into the
    // player list panel. Rebuilt per frame; the roster is small and has no
    // revision to key a change test on.
    // Re-pins the screen's own bottom-edge chrome - the world name and the
    // settings button - to the window. Both are laid out once in InitializeUI
    // against the visible canvas, which moves the moment the window changes
    // shape; without this they stayed where the window used to be.
    void LayoutScreenChrome();

    int chromeWidth_  = 0;
    int chromeHeight_ = 0;

    void RefreshRoster();

    // Pushes the server's buff set into the display. Called on the buff
    // revision only; the display animates between calls itself.
    void RefreshBuffs();

    // Pushes the character stats into the character panel, keyed on the same
    // stats revision RefreshStats follows.
    void RefreshCharacterPanel();
    uint32_t characterPanelRevision_ = 0;

    std::shared_ptr<UILabel>  worldLabel_;
    std::shared_ptr<UIButton> settingsButton_;

    std::unique_ptr<HUD> hud_;

    // The wrench panel. Built once with the rest of the UI and shown when the
    // server answers an InteractStrixCore with a WorldInfo - never on the
    // click, because whether this player may see anything is the server's call.
    std::unique_ptr<WorldManagerPanel> worldManagerPanel_;

    // Last world-info revision handed to the panel, so it repopulates on change
    // rather than every frame, matching how the HUD follows stats and inventory.
    uint32_t worldInfoRevision_ = 0;
    uint32_t worldLeftRevision_ = 0;

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

    // The world's sky: one of the loading-screen artworks, chosen by the same
    // name hash so a world loads into the scene its loading screen showed.
    // Drawn parallax-tiled behind the tiles in RenderBackground; null falls
    // back to the plain gradient.
    std::shared_ptr<Texture> worldBackdrop_;

    // Last inventory revision drawn, so the hotbar is rebuilt only on change.
    uint32_t inventoryRevision_ = 0;
    uint32_t statsRevision_     = 0;
    uint32_t buffRevision_      = 0;

    // The tile the last Strix Core request named, remembered at the point the
    // request is sent.
    //
    // EmitCoreBurst used to read interactTileX_/interactTileY_, which
    // UpdateInteractPrompt owns and clears to 0,0 at the top of every frame
    // before setting them to whatever the E prompt is nearest to. That is the
    // main door as often as it is a Core - so a burst fired for a Core claimed
    // by clicking it was drawn at the door instead, or at world tile 0,0 when
    // no prompt target was in range at all. Either way it was never where the
    // player was looking.
    int32_t coreBurstTileX_ = 0;
    int32_t coreBurstTileY_ = 0;
    bool    coreBurstArmed_ = false;

    // Counts down between the Strix Core burst and the panel that follows it.
    // Long enough for the puff to read and short enough not to feel like lag;
    // the burst itself lives 0.8-0.9s, so this shows roughly its first third.
    static constexpr float kCorePanelDelaySeconds = 0.28f;
    float corePanelDelay_ = 0.0f;

    float moveSendTimer_ = 0.0f;
    float lastSentTileX_ = 0.0f;
    float lastSentTileY_ = 0.0f;
    bool  hasSentMove_   = false;

    // The tile renderer is owned by the Engine's SystemManager and lives for
    // the whole session; this screen only lends it a world to draw.
    StrixVerse::ECS::Entity playerEntity_ = StrixVerse::ECS::NULL_ENTITY;
    StrixVerse::ECS::Entity cameraEntity_ = StrixVerse::ECS::NULL_ENTITY;
};
