#pragma once

#include "Screen.h"
#include "../ui/UIPanel.h"
#include "../ui/UILabel.h"

/**
 * Loading screen: shows a loading progress bar and then transitions to the game.
 */
class LoadingScreen : public Screen
{
public:
    LoadingScreen(Engine* engine);
    ~LoadingScreen() override = default;

    // Screen overrides
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;
    void Render() const override;

private:
    // UI elements
    std::shared_ptr<UIPanel> m_Panel;
    std::shared_ptr<UILabel> m_TitleLabel;
    std::shared_ptr<UILabel> m_WorldLabel;
    std::shared_ptr<UIPanel> m_BackgroundBar; // background of the progress bar
    std::shared_ptr<UIPanel> m_ProgressBar;   // foreground progress bar

    // State
    float m_LoadProgress; // 0.0 to 1.0
    float m_LoadDelay;    // time to wait at 100% before switching to game
    bool m_ReadyToSwitch; // true when loading is complete and delay passed
};