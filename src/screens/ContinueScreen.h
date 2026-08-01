#pragma once

#include "Screen.h"
#include "../ui/UIPanel.h"
#include "../ui/UILabel.h"

/**
 * Continue screen: checks for a saved world and decides whether to auto-connect or go to world browser.
 */
class ContinueScreen : public Screen
{
public:
    ContinueScreen(Engine* engine);
    ~ContinueScreen() override = default;

    // Screen overrides
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;
    void Render() const override;

private:
    // UI elements
    std::shared_ptr<UIPanel> m_Panel;
    std::shared_ptr<UILabel> m_MessageLabel;
    std::shared_ptr<UILabel> m_StatusLabel;

    // State
    float m_Timer;
    bool m_CheckComplete;
    bool m_HasSavedWorld; // false means go to world browser, true means auto-connect

    // Helper methods
    void CheckForSavedWorld();
};