#include "LoadingScreen.h"
#include "../core/Logger.h"
#include "../core/Engine.h"
#include "../graphics/Color.h"
#include "../core/Window.h"

LoadingScreen::LoadingScreen(Engine* engine)
    : Screen(engine)
    , m_Panel(nullptr)
    , m_TitleLabel(nullptr)
    , m_WorldLabel(nullptr)
    , m_BackgroundBar(nullptr)
    , m_ProgressBar(nullptr)
    , m_LoadProgress(0.0f)
    , m_LoadDelay(0.0f)
    , m_ReadyToSwitch(false)
{
}

void LoadingScreen::OnEnter()
{
    if (!engine_)
    {
        LOG_ERROR("LoadingScreen: Engine is null");
        return;
    }

    if (!uiManager_)
    {
        LOG_ERROR("LoadingScreen: UIManager not available");
        return;
    }

    // Create a panel to hold the loading UI
    m_Panel = std::make_shared<UIPanel>();
    int width, height;
    engine_->GetWindow()->GetSize(width, height);
    m_Panel->setSize(static_cast<float>(width), static_cast<float>(height));
    m_Panel->setPosition(0.0f, 0.0f);
    m_Panel->setBackgroundColor({0.0f, 0.0f, 0.0f, 1.0f}); // Opaque black
    uiManager_->addElement(m_Panel);

    float windowWidth = static_cast<float>(width);
    float windowHeight = static_cast<float>(height);

    // Title label
    m_TitleLabel = std::make_shared<UILabel>();
    m_TitleLabel->setText("Loading World...");
    m_TitleLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_TitleLabel->setFontSize(32.0f);
    m_TitleLabel->setPosition(windowWidth / 2.0f, windowHeight / 2.0f - 60.0f);
    m_TitleLabel->setAlignment(UILabel::Alignment::Center);
    uiManager_->addElement(m_TitleLabel);

    // World name label
    m_WorldLabel = std::make_shared<UILabel>();
    std::string worldName = engine_->GetSelectedWorldName();
    if (!worldName.empty())
    {
        m_WorldLabel->setText("World: " + worldName);
    }
    else
    {
        m_WorldLabel->setText("Preparing world...");
    }
    m_WorldLabel->setTextColor({0.8f, 0.8f, 0.8f, 1.0f});
    m_WorldLabel->setFontSize(18.0f);
    m_WorldLabel->setPosition(windowWidth / 2.0f, windowHeight / 2.0f - 20.0f);
    m_WorldLabel->setAlignment(UILabel::Alignment::Center);
    uiManager_->addElement(m_WorldLabel);

    // Progress bar background
    m_BackgroundBar = std::make_shared<UIPanel>();
    m_BackgroundBar->setSize(400.0f, 20.0f);
    m_BackgroundBar->setPosition(windowWidth / 2.0f - 200.0f, windowHeight / 2.0f + 20.0f);
    m_BackgroundBar->setBackgroundColor({0.3f, 0.3f, 0.3f, 1.0f});
    uiManager_->addElement(m_BackgroundBar);

    // Progress bar foreground
    m_ProgressBar = std::make_shared<UIPanel>();
    m_ProgressBar->setSize(0.0f, 20.0f);
    m_ProgressBar->setPosition(windowWidth / 2.0f - 200.0f, windowHeight / 2.0f + 20.0f);
    m_ProgressBar->setBackgroundColor({0.0f, 0.6f, 0.0f, 1.0f});
    uiManager_->addElement(m_ProgressBar);

    m_LoadProgress = 0.0f;
    m_LoadDelay = 0.0f;
    m_ReadyToSwitch = false;

    LOG_INFO("LoadingScreen: Entered");
}

void LoadingScreen::OnExit()
{
    if (uiManager_)
    {
        if (m_Panel) uiManager_->removeElement(m_Panel);
        if (m_TitleLabel) uiManager_->removeElement(m_TitleLabel);
        if (m_WorldLabel) uiManager_->removeElement(m_WorldLabel);
        if (m_BackgroundBar) uiManager_->removeElement(m_BackgroundBar);
        if (m_ProgressBar) uiManager_->removeElement(m_ProgressBar);
    }

    m_Panel.reset();
    m_TitleLabel.reset();
    m_WorldLabel.reset();
    m_BackgroundBar.reset();
    m_ProgressBar.reset();
}

void LoadingScreen::Update(float deltaTime)
{
    if (!m_ReadyToSwitch)
    {
        // Simulate loading progress
        m_LoadProgress += deltaTime * 0.5f; // 2 seconds to fill
        if (m_LoadProgress >= 1.0f)
        {
            m_LoadProgress = 1.0f;
            m_LoadDelay += deltaTime;
            if (m_LoadDelay >= 0.5f) // Half second delay at 100%
            {
                m_ReadyToSwitch = true;
                RequestScreenChange(ScreenID::Game);
            }
        }

        // Update progress bar width
        if (m_ProgressBar)
        {
            float barWidth = 400.0f * m_LoadProgress;
            m_ProgressBar->setSize(barWidth, 20.0f);
        }

        // Update title text based on progress
        if (m_TitleLabel)
        {
            if (m_LoadProgress < 0.33f)
                m_TitleLabel->setText("Loading World...");
            else if (m_LoadProgress < 0.66f)
                m_TitleLabel->setText("Generating Terrain...");
            else if (m_LoadProgress < 1.0f)
                m_TitleLabel->setText("Preparing Resources...");
            else
                m_TitleLabel->setText("Ready!");
        }
    }
}

void LoadingScreen::Render() const
{
    // UI is rendered by UIManager
}