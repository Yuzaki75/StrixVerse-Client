#include "ContinueScreen.h"
#include "../core/Logger.h"
#include "../core/Engine.h"
#include "../graphics/Color.h"
#include "../core/Window.h"

ContinueScreen::ContinueScreen(Engine* engine)
    : Screen(engine)
    , m_Panel(nullptr)
    , m_MessageLabel(nullptr)
    , m_StatusLabel(nullptr)
    , m_Timer(0.0f)
    , m_CheckComplete(false)
    , m_HasSavedWorld(false) // Will be updated by CheckForSavedWorld()
{
}

void ContinueScreen::OnEnter()
{
    if (!engine_)
    {
        LOG_ERROR("ContinueScreen: Engine is null");
        return;
    }

    if (!uiManager_)
    {
        LOG_ERROR("ContinueScreen: UIManager not available");
        return;
    }

    // Create a panel to hold the message
    m_Panel = std::make_shared<UIPanel>();
    int width, height;
    engine_->GetWindow()->GetSize(width, height);
    m_Panel->setSize(400.0f, 200.0f);
    m_Panel->setPosition(
        (static_cast<float>(width) - 400.0f) / 2.0f,
        (static_cast<float>(height) - 200.0f) / 2.0f);
    m_Panel->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.7f}); // Semi-transparent dark
    uiManager_->addElement(m_Panel);

    // Message label
    m_MessageLabel = std::make_shared<UILabel>();
    m_MessageLabel->setText("Checking for saved world...");
    m_MessageLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f}); // White
    m_MessageLabel->setFontSize(24.0f);
    m_MessageLabel->setPosition(200.0f, 50.0f); // Centered in panel (x=200 for width 400)
    m_MessageLabel->setAlignment(UILabel::Alignment::Center);
    m_Panel->addChild(m_MessageLabel);

    // Status label (for result)
    m_StatusLabel = std::make_shared<UILabel>();
    m_StatusLabel->setText("");
    m_StatusLabel->setTextColor({1.0f, 1.0f, 0.0f, 1.0f}); // Yellow
    m_StatusLabel->setFontSize(18.0f);
    m_StatusLabel->setPosition(200.0f, 100.0f);
    m_StatusLabel->setAlignment(UILabel::Alignment::Center);
    m_Panel->addChild(m_StatusLabel);

    // Start the check
    m_Timer = 0.0f;
    m_CheckComplete = false;
    CheckForSavedWorld();
}

void ContinueScreen::OnExit()
{
    if (uiManager_ && m_Panel)
    {
        uiManager_->removeElement(m_Panel);
        m_Panel.reset();
        m_MessageLabel.reset();
        m_StatusLabel.reset();
    }
}

void ContinueScreen::Update(float deltaTime)
{
    m_Timer += deltaTime;

    // Simulate a check taking some time
    if (!m_CheckComplete && m_Timer >= 1.0f) // 1 second delay for check
    {
        m_CheckComplete = true;
        m_Timer = 0.0f; // reset for next phase

        // Update UI based on result
        if (m_HasSavedWorld)
        {
            m_MessageLabel->setText("Found saved world! Connecting...");
            m_StatusLabel->setText("");
        }
        else
        {
            m_MessageLabel->setText("No saved world found.");
            m_StatusLabel->setText("Opening world browser...");
        }
    }

    // After displaying result, wait a bit then transition
    if (m_CheckComplete && m_Timer >= 2.0f) // 2 seconds after check completes
    {
        if (m_HasSavedWorld)
        {
            // Auto-connect to last world -> go to LoadingScreen
            RequestScreenChange(ScreenID::Loading);
        }
        else
        {
            // No saved world -> go to WorldBrowserScreen
            RequestScreenChange(ScreenID::WorldBrowser);
        }
    }
}

void ContinueScreen::Render() const
{
    // UI is rendered by UIManager
}

void ContinueScreen::CheckForSavedWorld()
{
    // Check if we have a saved world using the WorldManager
    if (engine_ && engine_->GetWorldManager())
    {
        m_HasSavedWorld = engine_->GetWorldManager()->HasSavedWorld();
        LOG_INFO("ContinueScreen: Saved world check result: " + std::string(m_HasSavedWorld ? "Yes" : "No"));
    }
    else
    {
        LOG_WARN("ContinueScreen: Engine or WorldManager is null");
        m_HasSavedWorld = false;
    }
}