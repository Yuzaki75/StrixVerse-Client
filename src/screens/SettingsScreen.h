#pragma once

#include "Screen.h"
#include "../ui/UIPanel.h"
#include "../ui/UILabel.h"
#include "../ui/UIButton.h"
#include "../ui/UITextBox.h"
#include "../ui/UIImage.h"

/**
 * Settings screen for configuring game options.
 */
class SettingsScreen : public Screen
{
public:
    SettingsScreen(Engine* engine);
    ~SettingsScreen() override = default;

    // Screen overrides
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;
    void Render() const override;

private:
    // UI elements
    std::shared_ptr<UIPanel> m_Panel;
    std::shared_ptr<UILabel> m_TitleLabel;
    std::shared_ptr<UIButton> m_BackButton;

    // Settings categories (placeholders for future expansion)
    std::shared_ptr<UILabel> m_GraphicsLabel;
    std::shared_ptr<UILabel> m_AudioLabel;
    std::shared_ptr<UILabel> m_ControlsLabel;

    // Helper methods
    void OnBackButtonClicked();
};