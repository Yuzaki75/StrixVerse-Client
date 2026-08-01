#pragma once

#include "Screen.h"
#include "../hud/HUD.h"
#include "../ui/UIPanel.h"
#include "../ui/UILabel.h"
#include "../ui/UIButton.h"
#include "../core/world/World.h"
#include "../core/Engine.h"
#include "../ecs/TileRendererSystem.h"

/**
 * Gameplay screen - placeholder for the actual game.
 */
class GameScreen : public Screen
{
public:
    GameScreen(Engine* engine);
    ~GameScreen() override = default;

    // Screen overrides
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;
    void Render() const override;

private:
    // UI elements
    std::shared_ptr<UIPanel> m_Panel;
    std::shared_ptr<UILabel> m_TitleLabel;
    std::shared_ptr<UIButton> m_SettingsButton;

    // HUD
    std::unique_ptr<HUD> m_HUD;

    // World system
    std::unique_ptr<StrixVerse::World::World> m_World;

    // Tile renderer system
    std::unique_ptr<StrixVerse::ECS::TileRendererSystem> m_TileRenderer;

    // Helper methods
    void OnSettingsButtonClicked();
    void InitializeWorld();
    void InitializeUI();
    void InitializeHUD();
};