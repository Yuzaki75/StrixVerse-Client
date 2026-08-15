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

    // Spawns the local player and the camera that follows it.
    void InitializeActors();

    // Nudges a spawn point to the nearest position the player fits in.
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

    // Moves the local player to where the server says it is.
    void ApplyServerCorrection(float tileX, float tileY);

    // Name for a player id, falling back to the id when it is not known.
    std::string DisplayNameFor(uint64_t entityId) const;

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

    float moveSendTimer_ = 0.0f;
    float lastSentTileX_ = 0.0f;
    float lastSentTileY_ = 0.0f;
    bool  hasSentMove_   = false;

    // The tile renderer is owned by the Engine's SystemManager and lives for
    // the whole session; this screen only lends it a world to draw.
    StrixVerse::ECS::Entity playerEntity_ = StrixVerse::ECS::NULL_ENTITY;
    StrixVerse::ECS::Entity cameraEntity_ = StrixVerse::ECS::NULL_ENTITY;
};
