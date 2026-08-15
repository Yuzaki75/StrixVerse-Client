#include "HUD.h"
#include "../core/Engine.h"
#include "../core/Window.h"
#include "../core/Logger.h"
#include "../core/ServiceLocator.h"
#include "../graphics/Color.h"
#include "../graphics/Font.h"
#include "../networking/Protocol.h"
#include "../ui/UIButton.h"
#include "../ui/UIFonts.h"
#include "../ui/UIImage.h"
#include "../ui/UILabel.h"
#include "../ui/UIManager.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITextBox.h"
#include "../ui/UITheme.h"

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

    // UILabel draws a single line, so the log is rendered as one label per
    // line, newest at the bottom.
    const size_t lineCount = m_ChatLines.size();
    const size_t visible   = std::min(lineCount, m_ChatMessages.size());

    for (size_t i = 0; i < lineCount; ++i)
    {
        if (!m_ChatLines[i])
            continue;

        // Bottom-align the log: empty slots stay at the top.
        const size_t slotFromBottom = lineCount - 1 - i;

        if (slotFromBottom < visible)
        {
            const size_t index = m_ChatMessages.size() - 1 - slotFromBottom;
            m_ChatLines[i]->setText(m_ChatMessages[index]);
        }
        else
        {
            m_ChatLines[i]->setText("");
        }
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

// The HUD is positioned in the same 1920x1080 design canvas as the screens, so
// it scales with the rest of the UI instead of drifting with the window size.
namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    Font* HudFont(Engine* engine, UIFonts::Typeface face, unsigned int size)
    {
        UIFonts* fonts = engine ? engine->GetUIFonts() : nullptr;
        return fonts ? fonts->Get(face, size) : nullptr;
    }

    // Shared look for the HUD's stat pills.
    void StyleStatPanel(const std::shared_ptr<UIPanel>& panel)
    {
        panel->setBackgroundColor(UITheme::Hex(0x0E121E, 0.62f));
        panel->setBorder(UITheme::SubtleBorder, UITheme::BorderThin);
        panel->setBorderRadius(UITheme::RadiusButton);
    }
}

void HUD::CreateHealthSection()
{
    m_HealthPanel = std::make_shared<UIPanel>();
    m_HealthPanel->setSize(S(200.0f), S(30.0f));
    m_HealthPanel->setPosition(S(20.0f), S(20.0f));
    StyleStatPanel(m_HealthPanel);
    m_UIManager->addElement(m_HealthPanel);

    m_HealthIcon = std::make_shared<UIImage>();
    m_HealthIcon->setSize(S(18.0f), S(18.0f));
    m_HealthIcon->setPosition(S(7.0f), S(6.0f));
    m_HealthIcon->setColor(UITheme::Danger);
    m_HealthPanel->addChild(m_HealthIcon);

    m_HealthLabel = std::make_shared<UILabel>();
    m_HealthLabel->setText("HP: 100/100");
    m_HealthLabel->setFont(HudFont(m_Engine, UIFonts::Typeface::Data, UITheme::Data::Regular));
    m_HealthLabel->setTextColor(UITheme::Text);
    m_HealthLabel->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    m_HealthLabel->setPosition(S(32.0f), 0.0f);
    m_HealthLabel->setSize(S(160.0f), S(30.0f));
    m_HealthPanel->addChild(m_HealthLabel);
}

void HUD::CreateManaSection()
{
    m_ManaPanel = std::make_shared<UIPanel>();
    m_ManaPanel->setSize(S(200.0f), S(30.0f));
    m_ManaPanel->setPosition(S(20.0f), S(56.0f));
    StyleStatPanel(m_ManaPanel);
    m_UIManager->addElement(m_ManaPanel);

    m_ManaIcon = std::make_shared<UIImage>();
    m_ManaIcon->setSize(S(18.0f), S(18.0f));
    m_ManaIcon->setPosition(S(7.0f), S(6.0f));
    m_ManaIcon->setColor(UITheme::Primary);
    m_ManaPanel->addChild(m_ManaIcon);

    m_ManaLabel = std::make_shared<UILabel>();
    m_ManaLabel->setText("MP: 50/50");
    m_ManaLabel->setFont(HudFont(m_Engine, UIFonts::Typeface::Data, UITheme::Data::Regular));
    m_ManaLabel->setTextColor(UITheme::Text);
    m_ManaLabel->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    m_ManaLabel->setPosition(S(32.0f), 0.0f);
    m_ManaLabel->setSize(S(160.0f), S(30.0f));
    m_ManaPanel->addChild(m_ManaLabel);
}

void HUD::CreateLevelSection()
{
    m_LevelPanel = std::make_shared<UIPanel>();
    m_LevelPanel->setSize(S(150.0f), S(38.0f));
    m_LevelPanel->setPosition(S(20.0f), S(92.0f));
    StyleStatPanel(m_LevelPanel);
    m_UIManager->addElement(m_LevelPanel);

    m_LevelLabel = std::make_shared<UILabel>();
    m_LevelLabel->setText("LVL: 1");
    m_LevelLabel->setFont(HudFont(m_Engine, UIFonts::Typeface::Display, UITheme::Display::Small));
    m_LevelLabel->setTextColor(UITheme::Text);
    m_LevelLabel->setPosition(S(9.0f), S(7.0f));
    m_LevelLabel->setSize(S(132.0f), S(10.0f));
    m_LevelPanel->addChild(m_LevelLabel);

    m_ExperienceLabel = std::make_shared<UILabel>();
    m_ExperienceLabel->setText("EXP: 0/100");
    m_ExperienceLabel->setFont(HudFont(m_Engine, UIFonts::Typeface::Data, UITheme::Data::Small));
    m_ExperienceLabel->setTextColor(UITheme::Subtext);
    m_ExperienceLabel->setPosition(S(9.0f), S(21.0f));
    m_ExperienceLabel->setSize(S(132.0f), S(12.0f));
    m_LevelPanel->addChild(m_ExperienceLabel);
}

void HUD::CreateCurrencySection()
{
    m_CoinPanel = std::make_shared<UIPanel>();
    m_CoinPanel->setSize(S(150.0f), S(30.0f));
    m_CoinPanel->setPosition(S(20.0f), S(138.0f));
    StyleStatPanel(m_CoinPanel);
    m_UIManager->addElement(m_CoinPanel);

    m_CoinIcon = std::make_shared<UIImage>();
    m_CoinIcon->setSize(S(18.0f), S(18.0f));
    m_CoinIcon->setPosition(S(7.0f), S(6.0f));
    m_CoinIcon->setColor(UITheme::Gold);
    m_CoinPanel->addChild(m_CoinIcon);

    m_CoinLabel = std::make_shared<UILabel>();
    m_CoinLabel->setText("0");
    m_CoinLabel->setFont(HudFont(m_Engine, UIFonts::Typeface::Data, UITheme::Data::Regular));
    m_CoinLabel->setTextColor(UITheme::Gold);
    m_CoinLabel->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    m_CoinLabel->setPosition(S(32.0f), 0.0f);
    m_CoinLabel->setSize(S(110.0f), S(30.0f));
    m_CoinPanel->addChild(m_CoinLabel);

    m_GemPanel = std::make_shared<UIPanel>();
    m_GemPanel->setSize(S(150.0f), S(30.0f));
    m_GemPanel->setPosition(S(20.0f), S(174.0f));
    StyleStatPanel(m_GemPanel);
    m_UIManager->addElement(m_GemPanel);

    m_GemIcon = std::make_shared<UIImage>();
    m_GemIcon->setSize(S(18.0f), S(18.0f));
    m_GemIcon->setPosition(S(7.0f), S(6.0f));
    m_GemIcon->setColor(UITheme::Accent);
    m_GemPanel->addChild(m_GemIcon);

    m_GemLabel = std::make_shared<UILabel>();
    m_GemLabel->setText("0");
    m_GemLabel->setFont(HudFont(m_Engine, UIFonts::Typeface::Data, UITheme::Data::Regular));
    m_GemLabel->setTextColor(UITheme::Accent);
    m_GemLabel->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    m_GemLabel->setPosition(S(32.0f), 0.0f);
    m_GemLabel->setSize(S(110.0f), S(30.0f));
    m_GemPanel->addChild(m_GemLabel);
}

void HUD::CreateChatSection()
{
    const float width       = S(300.0f);
    const float height      = S(150.0f);
    const float padding     = S(10.0f);
    const float inputHeight = S(22.0f);
    const float inputGap    = S(6.0f);

    m_ChatBackground = std::make_shared<UIPanel>();
    m_ChatBackground->setSize(width, height);
    m_ChatBackground->setPosition(UIScale::kDesignWidth - width - S(20.0f), S(20.0f));
    m_ChatBackground->setBackgroundColor(UITheme::Hex(0x0E121E, 0.66f));
    m_ChatBackground->setBorder(UITheme::SubtleBorder, UITheme::BorderThin);
    m_ChatBackground->setBorderRadius(UITheme::RadiusPanel);
    m_UIManager->addElement(m_ChatBackground);

    Font* chatFont = HudFont(m_Engine, UIFonts::Typeface::Body, UITheme::Body::Caption);
    const float lineHeight = chatFont ? chatFont->GetLineHeight() : S(14.0f);

    // The input row sits along the bottom, so the log gets what is left.
    const float logHeight = height - padding * 2.0f - inputHeight - inputGap;

    // One label per visible line; AddChatMessage fills them bottom-up.
    const size_t lines = logHeight > 0.0f
                             ? static_cast<size_t>(logHeight / lineHeight)
                             : 0;

    m_ChatLines.clear();
    m_ChatLines.reserve(lines);

    for (size_t i = 0; i < lines; ++i)
    {
        auto line = std::make_shared<UILabel>();
        line->setFont(chatFont);
        line->setTextColor(UITheme::Subtext);
        line->setPosition(padding, padding + lineHeight * static_cast<float>(i));
        line->setSize(width - padding * 2.0f, lineHeight);
        m_ChatBackground->addChild(line);

        m_ChatLines.push_back(line);
    }

    m_ChatInput = std::make_shared<UITextBox>();
    m_ChatInput->setFont(chatFont);
    m_ChatInput->setPlaceholderText("Press Enter to chat");
    m_ChatInput->setMaxLength(static_cast<int>(ProtocolLimits::MaxChatMessageLength));
    m_ChatInput->setPadding(S(8.0f));
    m_ChatInput->setSize(width - padding * 2.0f, inputHeight);
    m_ChatInput->setPosition(padding, height - padding - inputHeight);
    m_ChatInput->setOnEnterPressed(
        [this]()
        {
            if (!m_ChatInput)
                return;

            const std::string text = m_ChatInput->getText();

            // Clear before dispatching: the handler may add a line to the log,
            // and the field should already be empty when it does.
            m_ChatInput->setText("");

            if (!text.empty() && m_OnChatSubmit)
                m_OnChatSubmit(text);

            if (m_UIManager)
                m_UIManager->clearFocus();
        });
    m_ChatBackground->addChild(m_ChatInput);
}

void HUD::SetChatSubmitHandler(std::function<void(const std::string&)> handler)
{
    m_OnChatSubmit = std::move(handler);
}

void HUD::FocusChatInput()
{
    if (m_UIManager && m_ChatInput)
        m_UIManager->setFocusedElement(m_ChatInput);
}

bool HUD::IsChatInputFocused() const
{
    return m_UIManager && m_ChatInput &&
           m_UIManager->getFocusedElement() == m_ChatInput;
}

void HUD::CreateNotificationSection()
{
    const float width  = S(400.0f);
    const float height = S(50.0f);

    m_NotificationPanel = std::make_shared<UIPanel>();
    m_NotificationPanel->setSize(width, height);
    m_NotificationPanel->setPosition((UIScale::kDesignWidth - width) * 0.5f, S(20.0f));
    m_NotificationPanel->setBackgroundColor(UITheme::Hex(0x1E2230, 0.96f));
    m_NotificationPanel->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.35f), UITheme::BorderThin);
    m_NotificationPanel->setBorderRadius(UITheme::RadiusPanel);
    m_NotificationPanel->setVisible(false);
    m_UIManager->addElement(m_NotificationPanel);

    m_NotificationLabel = std::make_shared<UILabel>();
    m_NotificationLabel->setText("");
    m_NotificationLabel->setFont(HudFont(m_Engine, UIFonts::Typeface::Body, UITheme::Body::Regular));
    m_NotificationLabel->setTextColor(UITheme::Warning);
    m_NotificationLabel->setAlignment(UILabel::Alignment::Center);
    m_NotificationLabel->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    m_NotificationLabel->setPosition(S(10.0f), 0.0f);
    m_NotificationLabel->setSize(width - S(20.0f), height);
    m_NotificationPanel->addChild(m_NotificationLabel);
}