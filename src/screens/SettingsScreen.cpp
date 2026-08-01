#include "SettingsScreen.h"
#include "../core/Logger.h"
#include "../core/Engine.h"
#include "../graphics/Color.h"
#include "../core/Window.h"

SettingsScreen::SettingsScreen(Engine* engine)
    : Screen(engine)
    , m_Panel(nullptr)
    , m_TitleLabel(nullptr)
    , m_BackButton(nullptr)
    , m_GraphicsLabel(nullptr)
    , m_AudioLabel(nullptr)
    , m_ControlsLabel(nullptr)
{
}

void SettingsScreen::OnEnter()
{
    if (!engine_)
    {
        LOG_ERROR("SettingsScreen: Engine is null");
        return;
    }

    if (!uiManager_)
    {
        LOG_ERROR("SettingsScreen: UIManager not available");
        return;
    }

    // Create a panel to hold the settings UI
    m_Panel = std::make_shared<UIPanel>();
    int width, height;
    engine_->GetWindow()->GetSize(width, height);
    m_Panel->setSize(static_cast<float>(width), static_cast<float>(height));
    m_Panel->setPosition(0.0f, 0.0f);
    m_Panel->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.8f}); // Semi-transparent dark
    uiManager_->addElement(m_Panel);

    float panelX = m_Panel->getPosition().x;
    float panelY = m_Panel->getPosition().y;
    float windowWidth = static_cast<float>(width);
    float windowHeight = static_cast<float>(height);

    // Title label
    m_TitleLabel = std::make_shared<UILabel>();
    m_TitleLabel->setText("Settings");
    m_TitleLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f}); // White
    m_TitleLabel->setFontSize(48.0f);
    m_TitleLabel->setPosition(panelX + windowWidth / 2.0f, panelY + 80.0f);
    m_TitleLabel->setAlignment(UILabel::Alignment::Center);
    m_Panel->addChild(m_TitleLabel);

    // Back button
    m_BackButton = std::make_shared<UIButton>();
    m_BackButton->setSize(120.0f, 50.0f);
    m_BackButton->setPosition(panelX + 50.0f, panelY + 50.0f);
    m_BackButton->setText("Back");
    m_BackButton->setBackgroundColor({0.6f, 0.0f, 0.0f, 1.0f}); // Red
    m_BackButton->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_BackButton->setOnClickCallback([this]() { this->OnBackButtonClicked(); });
    m_Panel->addChild(m_BackButton);

    // Settings categories (placeholders)
    m_GraphicsLabel = std::make_shared<UILabel>();
    m_GraphicsLabel->setText("Graphics Settings");
    m_GraphicsLabel->setTextColor({0.8f, 0.8f, 1.0f, 1.0f}); // Light blue
    m_GraphicsLabel->setFontSize(24.0f);
    m_GraphicsLabel->setPosition(panelX + windowWidth / 2.0f, panelY + 180.0f);
    m_GraphicsLabel->setAlignment(UILabel::Alignment::Center);
    m_Panel->addChild(m_GraphicsLabel);

    m_AudioLabel = std::make_shared<UILabel>();
    m_AudioLabel->setText("Audio Settings");
    m_AudioLabel->setTextColor({0.8f, 0.8f, 1.0f, 1.0f});
    m_AudioLabel->setFontSize(24.0f);
    m_AudioLabel->setPosition(panelX + windowWidth / 2.0f, panelY + 240.0f);
    m_AudioLabel->setAlignment(UILabel::Alignment::Center);
    m_Panel->addChild(m_AudioLabel);

    m_ControlsLabel = std::make_shared<UILabel>();
    m_ControlsLabel->setText("Controls Settings");
    m_ControlsLabel->setTextColor({0.8f, 0.8f, 1.0f, 1.0f});
    m_ControlsLabel->setFontSize(24.0f);
    m_ControlsLabel->setPosition(panelX + windowWidth / 2.0f, panelY + 300.0f);
    m_ControlsLabel->setAlignment(UILabel::Alignment::Center);
    m_Panel->addChild(m_ControlsLabel);
}

void SettingsScreen::OnExit()
{
    if (uiManager_ && m_Panel)
    {
        uiManager_->removeElement(m_Panel);
        m_Panel.reset();
        m_TitleLabel.reset();
        m_BackButton.reset();
        m_GraphicsLabel.reset();
        m_AudioLabel.reset();
        m_ControlsLabel.reset();
    }
}

void SettingsScreen::Update(float deltaTime)
{
    // No periodic updates needed for settings screen
}

void SettingsScreen::Render() const
{
    // UI is rendered by UIManager
}

void SettingsScreen::OnBackButtonClicked()
{
    RequestScreenChange(ScreenID::WorldBrowser); // Go back to world browser for now
    // In a real implementation, we might want to go back to the previous screen
    // which would require a screen stack or tracking previous screen
}