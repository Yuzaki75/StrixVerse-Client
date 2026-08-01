#pragma once

#include "Screen.h"
#include "../ui/UIPanel.h"
#include "../ui/UILabel.h"
#include "../ui/UIButton.h"
#include "../ui/UITextBox.h"

class WorldBrowserScreen : public Screen
{
public:
    WorldBrowserScreen(Engine* engine);
    ~WorldBrowserScreen() override = default;

    // Screen overrides
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;
    void Render() const override;

private:
    // UI elements
    std::shared_ptr<UIPanel> m_Panel;
    std::shared_ptr<UILabel> m_TitleLabel;
    // We'll create a few world buttons dynamically
    std::vector<std::shared_ptr<UIButton>> m_WorldButtons;
    std::shared_ptr<UIButton> m_CreateButton;
    std::shared_ptr<UILabel> m_StatusLabel;

    // State
    std::string m_SelectedWorld;

    // Helper methods
    void CreateWorldButtons();
    void OnWorldButtonClicked(size_t index);
    void OnCreateButtonClicked();
};