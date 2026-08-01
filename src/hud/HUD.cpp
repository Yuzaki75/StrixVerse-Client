#include "HUD.h"
#include "../core/Engine.h"
#include "../core/Window.h"
#include "../core/Logger.h"
#include "../core/ServiceLocator.h"
#include "../graphics/Color.h"
#include "../ui/UIManager.h"
#include "../ui/UILabel.h"
#include "../ui/UIImage.h"
#include "../ui/UIPanel.h"
#include "../ui/UIButton.h"

HUD::HUD(Engine* engine)
    : m_Engine(engine)
    , m_UIManager(nullptr)
    , m_HealthPanel(nullptr)
    , m_HealthLabel(nullptr)
    , m_HealthIcon(nullptr)
    , m_ManaPanel(nullptr)
    , m_ManaLabel(nullptr)
    , m_ManaIcon(nullptr)
    , m_LevelPanel(nullptr)
    , m_LevelLabel(nullptr)
    , m_ExperienceLabel(nullptr)
    , m_CoinPanel(nullptr)
    , m_CoinLabel(nullptr)
    , m_CoinIcon(nullptr)
    , m_GemPanel(nullptr)
    , m_GemLabel(nullptr)
    , m_GemIcon(nullptr)
    , m_ChatBackground(nullptr)
    , m_ChatText(nullptr)
    , m_NotificationPanel(nullptr)
    , m_NotificationLabel(nullptr)
    , m_NotificationTimer(0.0f)
    , m_NotificationActive(false)
{
}

HUD::~HUD()
{
    // Remove all HUD elements from the UIManager to avoid dangling pointers
    if (m_UIManager)
    {
        if (m_HealthPanel) m_UIManager->removeElement(m_HealthPanel);
        if (m_ManaPanel) m_UIManager->removeElement(m_ManaPanel);
        if (m_LevelPanel) m_UIManager->removeElement(m_LevelPanel);
        if (m_CoinPanel) m_UIManager->removeElement(m_CoinPanel);
        if (m_GemPanel) m_UIManager->removeElement(m_GemPanel);
        if (m_ChatBackground) m_UIManager->removeElement(m_ChatBackground);
        if (m_NotificationPanel) m_UIManager->removeElement(m_NotificationPanel);
    }
    // Note: We do not delete the UI elements here because they are managed by shared_ptr.
    // The UIManager also holds a shared_ptr, so removing from UIManager decreases the reference count.
    // When this HUD object is destroyed, our shared_ptr will go to zero and the objects will be deleted
    // if the UIManager no longer holds a reference (which we just removed).
}

void HUD::Initialize()
{
    if (!m_Engine)
    {
        LOG_ERROR("HUD: Engine is null");
        return;
    }

    m_UIManager = m_Engine->GetUIManager();
    if (!m_UIManager)
    {
        LOG_ERROR("HUD: UIManager not available");
        return;
    }

    // Create all HUD sections
    CreateHealthSection();
    CreateManaSection();
    CreateLevelSection();
    CreateCurrencySection();
    CreateChatSection();
    CreateNotificationSection();

    LOG_INFO("HUD initialized");
}

void HUD::Update(float deltaTime)
{
    // Update notification timer
    if (m_NotificationActive)
    {
        m_NotificationTimer -= deltaTime;
        if (m_NotificationTimer <= 0.0f)
        {
            m_NotificationActive = false;
            if (m_NotificationPanel)
            {
                m_NotificationPanel->setVisible(false);
            }
        }
    }
}

void HUD::Render()
{
    // The HUD is rendered by the UIManager, so we don't need to do anything here
    // unless we have custom rendering. We'll leave this empty for now.
}

void HUD::SetHealth(float current, float maximum)
{
    if (m_HealthLabel)
    {
        // Format: "HP: 100/100"
        m_HealthLabel->setText("HP: " + std::to_string((int)current) + "/" + std::to_string((int)maximum));
    }
}

void HUD::SetMana(float current, float maximum)
{
    if (m_ManaLabel)
    {
        // Format: "MP: 50/50"
        m_ManaLabel->setText("MP: " + std::to_string((int)current) + "/" + std::to_string((int)maximum));
    }
}

void HUD::SetExperience(int current, int requiredForNextLevel)
{
    if (m_ExperienceLabel)
    {
        m_ExperienceLabel->setText("EXP: " + std::to_string(current) + "/" + std::to_string(requiredForNextLevel));
    }
}

void HUD::SetLevel(int level)
{
    if (m_LevelLabel)
    {
        m_LevelLabel->setText("LVL: " + std::to_string(level));
    }
}

void HUD::SetCoins(int amount)
{
    if (m_CoinLabel)
    {
        m_CoinLabel->setText(std::to_string(amount));
    }
}

void HUD::SetGems(int amount)
{
    if (m_GemLabel)
    {
        m_GemLabel->setText(std::to_string(amount));
    }
}

void HUD::AddChatMessage(const std::string& message)
{
    m_ChatMessages.push_back(message);
    if (m_ChatMessages.size() > MAX_CHAT_MESSAGES)
    {
        m_ChatMessages.erase(m_ChatMessages.begin());
    }

    // Update the chat text display
    if (m_ChatText)
    {
        std::string combined;
        for (const auto& msg : m_ChatMessages)
        {
            if (!combined.empty())
                combined += "\n";
            combined += msg;
        }
        m_ChatText->setText(combined);
    }
}

void HUD::ShowNotification(const std::string& message, float duration)
{
    if (m_NotificationLabel)
    {
        m_NotificationLabel->setText(message);
    }
    m_NotificationMessage = message;
    m_NotificationTimer = duration;
    m_NotificationActive = true;
    if (m_NotificationPanel)
    {
        m_NotificationPanel->setVisible(true);
    }
}

void HUD::CreateHealthSection()
{
    // Create a panel for health
    m_HealthPanel = std::make_shared<UIPanel>();
    m_HealthPanel->setSize(200.0f, 30.0f);
    m_HealthPanel->setPosition(20.0f, 20.0f); // Top-left corner
    m_HealthPanel->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.5f}); // Semi-transparent black
    m_UIManager->addElement(m_HealthPanel);

    // Health icon (placeholder - we'd use a texture in reality)
    m_HealthIcon = std::make_shared<UIImage>();
    // In a real implementation, we'd set a texture here
    m_HealthIcon->setSize(24.0f, 24.0f);
    m_HealthIcon->setPosition(5.0f, 3.0f); // Relative to panel? We'll adjust later
    m_HealthIcon->setColor({1.0f, 0.0f, 0.0f, 1.0f}); // Red
    m_HealthPanel->addChild(m_HealthIcon);

    // Health label
    m_HealthLabel = std::make_shared<UILabel>();
    m_HealthLabel->setText("HP: 100/100");
    m_HealthLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f}); // White
    m_HealthLabel->setFontSize(18.0f);
    m_HealthLabel->setPosition(35.0f, 5.0f); // Adjust based on icon size
    m_HealthPanel->addChild(m_HealthLabel);
}

void HUD::CreateManaSection()
{
    // Similar to health but for mana
    m_ManaPanel = std::make_shared<UIPanel>();
    m_ManaPanel->setSize(200.0f, 30.0f);
    m_ManaPanel->setPosition(20.0f, 60.0f); // Below health
    m_ManaPanel->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.5f});
    m_UIManager->addElement(m_ManaPanel);

    m_ManaIcon = std::make_shared<UIImage>();
    m_ManaIcon->setSize(24.0f, 24.0f);
    m_ManaIcon->setPosition(5.0f, 3.0f);
    m_ManaIcon->setColor({0.0f, 0.0f, 1.0f, 1.0f}); // Blue
    m_ManaPanel->addChild(m_ManaIcon);

    m_ManaLabel = std::make_shared<UILabel>();
    m_ManaLabel->setText("MP: 50/50");
    m_ManaLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_ManaLabel->setFontSize(18.0f);
    m_ManaLabel->setPosition(35.0f, 5.0f);
    m_ManaPanel->addChild(m_ManaLabel);
}

void HUD::CreateLevelSection()
{
    m_LevelPanel = std::make_shared<UIPanel>();
    m_LevelPanel->setSize(150.0f, 30.0f);
    m_LevelPanel->setPosition(20.0f, 100.0f); // Below mana
    m_LevelPanel->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.5f});
    m_UIManager->addElement(m_LevelPanel);

    m_LevelLabel = std::make_shared<UILabel>();
    m_LevelLabel->setText("LVL: 1");
    m_LevelLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_LevelLabel->setFontSize(18.0f);
    m_LevelLabel->setPosition(5.0f, 5.0f);
    m_LevelPanel->addChild(m_LevelLabel);

    m_ExperienceLabel = std::make_shared<UILabel>();
    m_ExperienceLabel->setText("EXP: 0/100");
    m_ExperienceLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_ExperienceLabel->setFontSize(16.0f);
    m_ExperienceLabel->setPosition(5.0f, 20.0f); // Below level label
    m_LevelPanel->addChild(m_ExperienceLabel);
}

void HUD::CreateCurrencySection()
{
    // Coins
    m_CoinPanel = std::make_shared<UIPanel>();
    m_CoinPanel->setSize(150.0f, 30.0f);
    m_CoinPanel->setPosition(20.0f, 140.0f); // Below level
    m_CoinPanel->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.5f});
    m_UIManager->addElement(m_CoinPanel);

    m_CoinIcon = std::make_shared<UIImage>();
    m_CoinIcon->setSize(24.0f, 24.0f);
    m_CoinIcon->setPosition(5.0f, 3.0f);
    m_CoinIcon->setColor({1.0f, 0.84f, 0.0f, 1.0f}); // Gold
    m_CoinPanel->addChild(m_CoinIcon);

    m_CoinLabel = std::make_shared<UILabel>();
    m_CoinLabel->setText("0");
    m_CoinLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_CoinLabel->setFontSize(18.0f);
    m_CoinLabel->setPosition(35.0f, 5.0f);
    m_CoinPanel->addChild(m_CoinLabel);

    // Gems
    m_GemPanel = std::make_shared<UIPanel>();
    m_GemPanel->setSize(150.0f, 30.0f);
    m_GemPanel->setPosition(20.0f, 180.0f); // Below coins
    m_GemPanel->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.5f});
    m_UIManager->addElement(m_GemPanel);

    m_GemIcon = std::make_shared<UIImage>();
    m_GemIcon->setSize(24.0f, 24.0f);
    m_GemIcon->setPosition(5.0f, 3.0f);
    m_GemIcon->setColor({1.0f, 0.0f, 1.0f, 1.0f}); // Purple
    m_GemPanel->addChild(m_GemIcon);

    m_GemLabel = std::make_shared<UILabel>();
    m_GemLabel->setText("0");
    m_GemLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_GemLabel->setFontSize(18.0f);
    m_GemLabel->setPosition(35.0f, 5.0f);
    m_GemPanel->addChild(m_GemLabel);
}

void HUD::CreateChatSection()
{
    // Chat background (semi-transparent)
    m_ChatBackground = std::make_shared<UIPanel>();
    m_ChatBackground->setSize(300.0f, 150.0f);
    int winWidth = 800, winHeight = 600;
    if (m_Engine && m_Engine->GetWindow())
        m_Engine->GetWindow()->GetSize(winWidth, winHeight);
    m_ChatBackground->setPosition(
        static_cast<float>(winWidth) - 320.0f, // Right side, with padding
        20.0f); // Top
    m_ChatBackground->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.6f}); // Darker transparent
    m_UIManager->addElement(m_ChatBackground);

    m_ChatText = std::make_shared<UILabel>();
    m_ChatText->setText("");
    m_ChatText->setTextColor({1.0f, 1.0f, 1.0f, 1.0f}); // White
    m_ChatText->setFontSize(16.0f);
    m_ChatText->setPosition(10.0f, 10.0f); // Padding inside background
    m_ChatBackground->addChild(m_ChatText);
}

void HUD::CreateNotificationSection()
{
    // Notification panel (center top)
    m_NotificationPanel = std::make_shared<UIPanel>();
    m_NotificationPanel->setSize(400.0f, 50.0f);
    int winWidth = 800, winHeight = 600;
    if (m_Engine && m_Engine->GetWindow())
        m_Engine->GetWindow()->GetSize(winWidth, winHeight);
    m_NotificationPanel->setPosition(
        static_cast<float>(winWidth) / 2.0f - 200.0f, // Centered
        20.0f); // Top
    m_NotificationPanel->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.7f}); // Semi-transparent dark
    m_NotificationPanel->setVisible(false); // Start hidden
    m_UIManager->addElement(m_NotificationPanel);

    m_NotificationLabel = std::make_shared<UILabel>();
    m_NotificationLabel->setText("");
    m_NotificationLabel->setTextColor({1.0f, 1.0f, 0.0f, 1.0f}); // Yellow for notices
    m_NotificationLabel->setFontSize(20.0f);
    m_NotificationLabel->setPosition(10.0f, 10.0f); // Padding
    m_NotificationPanel->addChild(m_NotificationLabel);
}