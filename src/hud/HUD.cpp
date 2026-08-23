#include "HUD.h"
#include "../core/AssetManager.h"
#include "../core/Engine.h"
#include "../core/Window.h"
#include "../core/Logger.h"
#include "../core/ServiceLocator.h"
#include "../graphics/Color.h"
#include "../graphics/Font.h"
#include "../networking/NetworkManager.h"
#include "../networking/Protocol.h"
#include "../ui/UIButton.h"
#include "../ui/UIFonts.h"
#include "../ui/UIImage.h"
#include "../ui/UILabel.h"
#include "../ui/UIManager.h"
#include "../ui/UIPanel.h"
#include "../ui/UIProgressBar.h"
#include "../ui/UIScale.h"
#include "../ui/UITextBox.h"
#include "../ui/UITheme.h"

#include <algorithm>
#include <format>

HUD::HUD(Engine* engine)
    : m_Engine(engine)
    , m_UIManager(nullptr)
    , m_HealthPanel(nullptr)
    , m_HealthLabel(nullptr)
    , m_LevelPanel(nullptr)
    , m_LevelLabel(nullptr)
    , m_ExperienceLabel(nullptr)
    , m_ChatBackground(nullptr)
{
}

HUD::~HUD()
{
    // Stop the NetworkManager from delivering into a destroyed HUD; notices
    // that arrive before the next HUD binds itself queue there instead.
    if (m_NotificationBound && m_Engine)
        m_Engine->getNetworkManager().SetNotificationHandler(nullptr);

    // Remove all HUD elements from the UIManager to avoid dangling pointers
    if (m_UIManager)
    {
        if (m_HealthPanel) m_UIManager->removeElement(m_HealthPanel);
        if (m_LevelPanel) m_UIManager->removeElement(m_LevelPanel);
        if (m_ChatBackground) m_UIManager->removeElement(m_ChatBackground);
        if (m_InventoryBar) m_UIManager->removeElement(m_InventoryBar);

        for (const Notification& note : m_Notifications)
        {
            if (note.panel)
                m_UIManager->removeElement(note.panel);
        }
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
    CreateLevelSection();
    CreateChatSection();
    CreateInventorySection();

    LOG_INFO("HUD initialized");
}

void HUD::Update(float deltaTime)
{
    // Self-binding: GameScreen constructs the HUD with the engine, and the
    // engine owns the NetworkManager, so no screen wiring is needed. Done on
    // the first frame rather than Initialize so any notifications queued
    // before the HUD existed are drained into a fully built stack.
    if (!m_NotificationBound && m_Engine)
        BindNotificationSource(&m_Engine->getNetworkManager());

    // Tick the stack down, then repaint what survives at its current fade.
    // The last half second eases to invisible so an expiry never pops.
    constexpr float kFadeSeconds = 0.5f;

    for (std::size_t i = m_Notifications.size(); i > 0; --i)
    {
        Notification& note = m_Notifications[i - 1];
        note.remaining -= deltaTime;
        if (note.remaining <= 0.0f)
            CloseNotification(i - 1);
    }

    for (const Notification& note : m_Notifications)
    {
        const float fade = std::clamp(note.remaining / kFadeSeconds, 0.0f, 1.0f);

        if (note.panel)
            note.panel->setBackgroundColor(
                UITheme::WithAlpha(note.background, note.background.a * fade));

        if (note.accent)
            note.accent->setBackgroundColor(
                UITheme::WithAlpha(note.accentColor, note.accentColor.a * fade));

        if (note.label)
            note.label->setTextColor(UITheme::WithAlpha(UITheme::Text, fade));
    }

    LayoutNotifications();
}

void HUD::Render()
{
    // The HUD is rendered by the UIManager, so we don't need to do anything here
    // unless we have custom rendering. We'll leave this empty for now.
}

void HUD::SetStats(const Stats& stats)
{
    // Blank rather than zeroed: "0/0" would read as a real value.
    if (!stats.known)
    {
        if (m_HealthLabel)     m_HealthLabel->setText("");
        if (m_HealthBar)       m_HealthBar->setProgress(0.0f);
        if (m_LevelLabel)      m_LevelLabel->setText("");
        if (m_ExperienceLabel) m_ExperienceLabel->setText("");
        if (m_ExperienceBar)   m_ExperienceBar->setProgress(0.0f);
        return;
    }

    if (m_HealthLabel)
        m_HealthLabel->setText(std::format("{} / {}", stats.health, stats.maxHealth));

    if (m_HealthBar)
    {
        const float ratio = stats.maxHealth > 0
                                ? static_cast<float>(stats.health) /
                                  static_cast<float>(stats.maxHealth)
                                : 0.0f;
        m_HealthBar->setProgress(ratio);

        // Colour follows the same thresholds the world list uses.
        m_HealthBar->setFillColor(ratio <= 0.25f   ? UITheme::Danger
                                  : ratio <= 0.5f  ? UITheme::Warning
                                                   : UITheme::Success);
    }

    if (m_LevelLabel)
        m_LevelLabel->setText(std::format("LVL {}", stats.level));

    if (m_ExperienceLabel)
    {
        m_ExperienceLabel->setText(
            stats.experienceToNextLevel > 0
                ? std::format("{} / {}", stats.experience, stats.experienceToNextLevel)
                : std::format("{}", stats.experience));
    }

    if (m_ExperienceBar)
    {
        const float ratio = stats.experienceToNextLevel > 0
                                ? static_cast<float>(stats.experience) /
                                  static_cast<float>(stats.experienceToNextLevel)
                                : 0.0f;
        m_ExperienceBar->setProgress(ratio);
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

// The HUD is positioned in the same 1920x1080 design canvas as the screens, so
// it scales with the rest of the UI instead of drifting with the window size.
namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    // The notification channel's standard lifetime.
    constexpr float kNotificationSeconds = 5.0f;

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

void HUD::ShowNotification(const std::string& message, float duration)
{
    AddNotification(message, duration, 0);
}

void HUD::AddNotification(const std::string& message)
{
    AddNotification(message, kNotificationSeconds, 0);
}

void HUD::BindNotificationSource(NetworkManager* network)
{
    if (!network || m_NotificationBound)
        return;

    m_NotificationBound = true;

    // SetNotificationHandler first drains anything the NetworkManager queued
    // before this HUD existed, so early notices keep their order.
    network->SetNotificationHandler(
        [this](const std::string& message, int severity)
        {
            AddNotification(message, kNotificationSeconds, severity);
        });
}

void HUD::AddNotification(const std::string& message, float duration, int severity)
{
    if (!m_UIManager || message.empty())
        return;

    // Room for five; the oldest drops when a sixth arrives.
    while (m_Notifications.size() >= kMaxNotifications)
        CloseNotification(0);

    constexpr float kWidth  = S(420.0f);
    constexpr float kHeight = S(30.0f);

    Notification note;
    note.remaining   = duration;
    note.background  = UITheme::Hex(0x1E2230, 0.82f);

    note.panel = std::make_shared<UIPanel>();
    note.panel->setSize(kWidth, kHeight);
    note.panel->setBackgroundColor(note.background);
    note.panel->setBorderRadius(UITheme::RadiusChip);
    m_UIManager->addElement(note.panel);

    // Severity styling. Warnings read as gold and successes as green; plain
    // info keeps the alternating Aether-violet/-blue accent so a group of
    // notices reads as one family rather than one repeated card.
    switch (severity)
    {
    case 1:  note.accentColor = UITheme::Gold;    break;
    case 2:  note.accentColor = UITheme::Success; break;
    default: note.accentColor = (m_Notifications.size() % 2 == 0) ? UITheme::Secondary
                                                                  : UITheme::Accent;
             break;
    }

    if (severity == 1 || severity == 2)
    {
        note.panel->setBorder(UITheme::WithAlpha(note.accentColor, 0.65f),
                              UITheme::BorderThin);
    }

    note.accent = std::make_shared<UIPanel>();
    note.accent->setSize(S(3.0f), kHeight - S(10.0f));
    note.accent->setPosition(S(6.0f), S(5.0f));
    note.accent->setBackgroundColor(note.accentColor);
    note.accent->setBorderRadius(UITheme::RadiusBar);
    note.panel->addChild(note.accent);

    note.label = std::make_shared<UILabel>();
    note.label->setFont(HudFont(m_Engine, UIFonts::Typeface::Body, UITheme::Body::Caption));
    note.label->setTextColor(UITheme::Text);
    note.label->setAlignment(UILabel::Alignment::Left);
    note.label->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    note.label->setPosition(S(16.0f), 0.0f);
    note.label->setSize(kWidth - S(24.0f), kHeight);
    note.panel->addChild(note.label);

    note.label->setText(message);

    m_Notifications.push_back(std::move(note));
    LayoutNotifications();
}

void HUD::CloseNotification(std::size_t index)
{
    if (index >= m_Notifications.size())
        return;

    if (m_UIManager && m_Notifications[index].panel)
        m_UIManager->removeElement(m_Notifications[index].panel);

    m_Notifications.erase(m_Notifications.begin() +
                          static_cast<std::ptrdiff_t>(index));
}

void HUD::LayoutNotifications()
{
    // Top-centre stack, below the top edge and clear of the stat panels on
    // the left and the chat panel on the right.
    constexpr float kTopMargin = S(20.0f);
    constexpr float kGap       = S(6.0f);

    for (std::size_t i = 0; i < m_Notifications.size(); ++i)
    {
        const Notification& note = m_Notifications[i];
        if (!note.panel)
            continue;

        note.panel->setPosition(
            (UIScale::kDesignWidth - note.panel->getWidth()) * 0.5f,
            kTopMargin + static_cast<float>(i) *
                             (note.panel->getHeight() + kGap));
    }
}

void HUD::CreateHealthSection()
{
    // Caption on the left, figures on the right, bar underneath. The old
    // version placed a UIImage with no texture where an icon should be, which
    // simply drew nothing and left a gap.
    const float width   = S(200.0f);
    const float height  = S(40.0f);
    const float padding = S(9.0f);

    m_HealthPanel = std::make_shared<UIPanel>();
    m_HealthPanel->setSize(width, height);
    m_HealthPanel->setPosition(S(20.0f), S(20.0f));
    StyleStatPanel(m_HealthPanel);
    m_HealthPanel->setBlocksInput(true);
    m_UIManager->addElement(m_HealthPanel);

    auto caption = std::make_shared<UILabel>();
    caption->setText("HP");
    caption->setFont(HudFont(m_Engine, UIFonts::Typeface::Data, UITheme::Data::Small));
    caption->setTextColor(UITheme::Danger);
    caption->setPosition(padding, S(8.0f));
    caption->setSize(S(40.0f), S(10.0f));
    m_HealthPanel->addChild(caption);

    m_HealthLabel = std::make_shared<UILabel>();
    m_HealthLabel->setText("");
    m_HealthLabel->setFont(HudFont(m_Engine, UIFonts::Typeface::Data, UITheme::Data::Small));
    m_HealthLabel->setTextColor(UITheme::Subtext);
    m_HealthLabel->setAlignment(UILabel::Alignment::Right);
    m_HealthLabel->setPosition(padding, S(8.0f));
    m_HealthLabel->setSize(width - padding * 2.0f, S(10.0f));
    m_HealthPanel->addChild(m_HealthLabel);

    m_HealthBar = std::make_shared<UIProgressBar>();
    m_HealthBar->setProgress(0.0f);
    m_HealthBar->setFillColor(UITheme::Danger);
    m_HealthBar->setGlowColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    m_HealthBar->setBorderRadius(S(2.0f));
    m_HealthBar->setPosition(padding, S(24.0f));
    m_HealthBar->setSize(width - padding * 2.0f, S(6.0f));
    m_HealthPanel->addChild(m_HealthBar);
}

void HUD::CreateLevelSection()
{
    const float width   = S(200.0f);
    const float height  = S(40.0f);
    const float padding = S(9.0f);

    m_LevelPanel = std::make_shared<UIPanel>();
    m_LevelPanel->setSize(width, height);
    m_LevelPanel->setPosition(S(20.0f), S(66.0f));
    StyleStatPanel(m_LevelPanel);
    m_LevelPanel->setBlocksInput(true);
    m_UIManager->addElement(m_LevelPanel);

    m_LevelLabel = std::make_shared<UILabel>();
    m_LevelLabel->setText("");
    m_LevelLabel->setFont(HudFont(m_Engine, UIFonts::Typeface::Data, UITheme::Data::Small));
    m_LevelLabel->setTextColor(UITheme::Accent);
    m_LevelLabel->setPosition(padding, S(8.0f));
    m_LevelLabel->setSize(S(80.0f), S(10.0f));
    m_LevelPanel->addChild(m_LevelLabel);

    m_ExperienceLabel = std::make_shared<UILabel>();
    m_ExperienceLabel->setText("");
    m_ExperienceLabel->setFont(HudFont(m_Engine, UIFonts::Typeface::Data, UITheme::Data::Small));
    m_ExperienceLabel->setTextColor(UITheme::Subtext);
    m_ExperienceLabel->setAlignment(UILabel::Alignment::Right);
    m_ExperienceLabel->setPosition(padding, S(8.0f));
    m_ExperienceLabel->setSize(width - padding * 2.0f, S(10.0f));
    m_LevelPanel->addChild(m_ExperienceLabel);

    m_ExperienceBar = std::make_shared<UIProgressBar>();
    m_ExperienceBar->setProgress(0.0f);
    m_ExperienceBar->setFillColor(UITheme::Accent);
    m_ExperienceBar->setGlowColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    m_ExperienceBar->setBorderRadius(S(2.0f));
    m_ExperienceBar->setPosition(padding, S(24.0f));
    m_ExperienceBar->setSize(width - padding * 2.0f, S(6.0f));
    m_LevelPanel->addChild(m_ExperienceBar);
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
    m_ChatBackground->setBlocksInput(true);
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

void HUD::CreateInventorySection()
{
    // Ten slots along the bottom edge, clear of the world label. The design
    // set does not specify a hotbar, so this uses the shared panel and slot
    // tokens rather than inventing a look.
    constexpr size_t kSlotCount = 10;

    const float slotSize = S(34.0f);
    const float gap      = S(4.0f);
    const float padding  = S(6.0f);

    const float barWidth  = static_cast<float>(kSlotCount) * slotSize +
                            static_cast<float>(kSlotCount - 1) * gap + padding * 2.0f;
    const float barHeight = slotSize + padding * 2.0f;

    m_InventoryBar = std::make_shared<UIPanel>();
    m_InventoryBar->setSize(barWidth, barHeight);
    m_InventoryBar->setPosition((UIScale::kDesignWidth - barWidth) * 0.5f,
                                UIScale::kDesignHeight - barHeight - S(64.0f));
    m_InventoryBar->setBackgroundColor(UITheme::Hex(0x0E121E, 0.66f));
    m_InventoryBar->setBorder(UITheme::SubtleBorder, UITheme::BorderThin);
    m_InventoryBar->setBorderRadius(UITheme::RadiusPanel);
    m_InventoryBar->setBlocksInput(true);
    m_UIManager->addElement(m_InventoryBar);

    Font* slotFont = HudFont(m_Engine, UIFonts::Typeface::Data, UITheme::Data::Small);

    m_InventorySlots.clear();
    m_InventoryLabels.clear();
    m_InventorySlots.reserve(kSlotCount);
    m_InventoryLabels.reserve(kSlotCount);

    for (size_t i = 0; i < kSlotCount; ++i)
    {
        auto slot = std::make_shared<UIPanel>();
        slot->setSize(slotSize, slotSize);
        slot->setPosition(padding + static_cast<float>(i) * (slotSize + gap), padding);
        slot->setBackgroundColor(UITheme::RowBackground);
        slot->setBorder(UITheme::SubtleBorder, UITheme::BorderThin);
        slot->setBorderRadius(UITheme::RadiusCard);
        m_InventoryBar->addChild(slot);

        // Item identity, centred. Stands in for artwork until items have any.
        auto label = std::make_shared<UILabel>();
        label->setFont(slotFont);
        label->setTextColor(UITheme::Subtext);
        label->setAlignment(UILabel::Alignment::Center);
        label->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
        label->setPosition(0.0f, 0.0f);
        label->setSize(slotSize, slotSize);
        slot->addChild(label);

        // Stack count, bottom right, as a hotbar conventionally shows it.
        auto count = std::make_shared<UILabel>();
        count->setFont(slotFont);
        count->setTextColor(UITheme::Gold);
        count->setAlignment(UILabel::Alignment::Right);
        count->setVerticalAlignment(UILabel::VerticalAlignment::Bottom);
        count->setPosition(0.0f, 0.0f);
        count->setSize(slotSize - S(3.0f), slotSize - S(2.0f));
        slot->addChild(count);

        // Artwork sits above the slot background and below the click target.
        auto icon = std::make_shared<UIImage>();
        const float iconInset = S(4.0f);
        icon->setPosition(iconInset, iconInset);
        icon->setSize(slotSize - iconInset * 2.0f, slotSize - iconInset * 2.0f);
        icon->setVisible(false);
        slot->addChild(icon);

        auto hint = std::make_shared<UILabel>();
        hint->setFont(slotFont);
        hint->setText(i == 9 ? "0" : std::format("{}", i + 1));
        hint->setTextColor(UITheme::Muted);
        hint->setAlignment(UILabel::Alignment::Left);
        hint->setVerticalAlignment(UILabel::VerticalAlignment::Top);
        hint->setPosition(S(2.0f), S(1.0f));
        hint->setSize(slotSize, S(12.0f));
        slot->addChild(hint);

        // Click target last so it receives the press.
        auto hit = std::make_shared<UIButton>();
        hit->setPosition(0.0f, 0.0f);
        hit->setSize(slotSize, slotSize);
        hit->setText("");
        hit->setFocusable(false);
        const Color invisible(0.0f, 0.0f, 0.0f, 0.0f);
        hit->setNormalColors(invisible, invisible, invisible);
        hit->setHoverColors(UITheme::WithAlpha(UITheme::Accent, 0.12f),
                            UITheme::WithAlpha(UITheme::Accent, 0.12f),
                            UITheme::WithAlpha(UITheme::Accent, 0.35f));
        hit->setBorderRadius(UITheme::RadiusCard);

        const uint8_t slotIndex = static_cast<uint8_t>(i);
        hit->setOnClick([this, slotIndex]() { SetSelectedSlot(slotIndex); });
        slot->addChild(hit);

        m_InventorySlots.push_back(slot);
        m_InventoryLabels.push_back(label);
        m_InventoryCounts.push_back(count);
        m_InventoryIcons.push_back(icon);
        m_SlotButtons.push_back(hit);
    }

    // The two permanent tools. Set here rather than routed through
    // SetInventory, because they are not inventory: they are always present
    // and the server knows nothing about them.
    //
    // Drawn as artwork like every other slot, with the words kept only as the
    // fallback for a missing file. Two text labels among a row of pictures read
    // as a row that had not been finished.
    if (m_InventoryLabels.size() > kWrenchSlot)
    {
        SetToolSlot(kPunchSlot,  "assets/items/9001_punch.png",  "PUNCH",  UITheme::Text);
        SetToolSlot(kWrenchSlot, "assets/items/9002_wrench.png", "WRENCH", UITheme::Accent);
    }

    RefreshSlotHighlight();
}

void HUD::SetToolSlot(std::size_t barSlot, const std::string& iconPath,
                      const std::string& fallbackText, const Color& fallbackColor)
{
    if (barSlot >= m_InventorySlots.size())
        return;

    std::shared_ptr<Texture> icon;
    if (auto assets = ServiceLocator::Get<AssetManager>())
    {
        // Mipmaps off: 32x32 pixel art shown small goes soft with them.
        icon = assets->LoadTexture(iconPath, false, false);
    }

    if (icon && m_InventoryIcons[barSlot])
    {
        m_InventoryIcons[barSlot]->setTexture(std::move(icon));
        m_InventoryIcons[barSlot]->setVisible(true);

        if (auto& label = m_InventoryLabels[barSlot])
            label->setText("");
        return;
    }

    // No art: say what the slot is rather than leaving it blank.
    LOG_WARN("HUD: " + iconPath + " did not load; that tool keeps its label");

    if (auto& label = m_InventoryLabels[barSlot])
    {
        label->setText(fallbackText);
        label->setTextColor(fallbackColor);
    }
}

std::string HUD::IconPathForItem(uint16_t itemId)
{
    // Explicit table rather than arithmetic. Item ids and tile ids are related
    // by the item's placeBlockId, which lives in the server's items.json and
    // is not sent to the client -- and it is not a formula: 1000 places tile 1,
    // but 1004 places tile 6. Guessing would put the wrong picture on the slot.
    //
    // Seeds are named by their own item id, so those map directly.
    switch (itemId)
    {
    case 1000: return "assets/tiles/001_dirt.png";
    case 1002: return "assets/tiles/002_stone.png";
    case 1004: return "assets/tiles/006_bedrock.png";
    case 1006: return "assets/tiles/013_lava.png";
    case 1008: return "assets/tiles/011_copper_ore.png";
    case 1010: return "assets/tiles/012_lantern.png";

    case 1001: return "assets/items/1001_dirt_seed.png";
    case 1003: return "assets/items/1003_rock_seed.png";
    case 1009: return "assets/items/1009_copper_seed.png";
    case 1011: return "assets/items/1011_lantern_seed.png";

    case 3:  return "assets/tiles/003_grass.png";
    case 4:  return "assets/tiles/004_wood.png";
    case 5:  return "assets/tiles/005_leaves.png";
    case 7:  return "assets/tiles/007_water.png";
    case 8:  return "assets/tiles/008_torch.png";
    case 9:  return "assets/tiles/009_chest.png";
    case 10: return "assets/tiles/010_door.png";
    case 15: return "assets/tiles/015_coal_ore.png";
    case 16: return "assets/tiles/016_iron_ore.png";
    case 17: return "assets/tiles/017_gold_ore.png";
    case 18: return "assets/tiles/018_diamond_ore.png";
    case 19: return "assets/tiles/019_sapling.png";

    default:
        return std::format("assets/items/{}.png", itemId);
    }
}

void HUD::SetSelectedSlot(uint8_t slot)
{
    if (slot >= m_InventorySlots.size())
        return;

    m_SelectedSlot = slot;
    RefreshSlotHighlight();

    if (m_OnSlotSelected)
        m_OnSlotSelected(slot);
}

void HUD::CycleSelectedSlot(int delta)
{
    if (m_InventorySlots.empty() || delta == 0)
        return;

    const int count = static_cast<int>(m_InventorySlots.size());
    int slot = (static_cast<int>(m_SelectedSlot) + delta) % count;
    if (slot < 0)
        slot += count;

    SetSelectedSlot(static_cast<uint8_t>(slot));
}

void HUD::RefreshSlotHighlight()
{
    for (size_t i = 0; i < m_InventorySlots.size(); ++i)
    {
        if (!m_InventorySlots[i])
            continue;

        const bool selected = (i == m_SelectedSlot);
        m_InventorySlots[i]->setBorder(selected ? UITheme::Accent : UITheme::SubtleBorder,
                                       selected ? UITheme::BorderThick : UITheme::BorderThin);
    }
}

void HUD::SetInventory(const std::vector<InventoryEntry>& entries)
{
    // Clear every slot first, so a slot the server no longer reports goes
    // back to empty instead of keeping a stale item.
    for (size_t i = 0; i < m_InventorySlots.size(); ++i)
    {
        // The two tool slots are not inventory and must survive a sync;
        // clearing them here would blank PUNCH and WRENCH every time the
        // server sent an inventory update.
        if (i < kFirstItemSlot)
            continue;

        if (m_InventorySlots[i])
        {
            m_InventorySlots[i]->setBackgroundColor(UITheme::RowBackground);
            m_InventorySlots[i]->setBorder(UITheme::SubtleBorder, UITheme::BorderThin);
        }

        if (m_InventoryLabels[i])
            m_InventoryLabels[i]->setText("");

        if (m_InventoryCounts[i])
            m_InventoryCounts[i]->setText("");

        // Hidden rather than cleared: an icon left visible would advertise an
        // item the player has just spent.
        if (i < m_InventoryIcons.size() && m_InventoryIcons[i])
            m_InventoryIcons[i]->setVisible(false);
    }

    for (const InventoryEntry& entry : entries)
    {
        // The server's inventory is 0-based and the first two hotbar slots are
        // tools, so server slot 0 is drawn at hotbar slot 2. Without this shift
        // the first inventory item lands underneath PUNCH and is invisible.
        const size_t barSlot = static_cast<size_t>(entry.slot) + kFirstItemSlot;

        if (barSlot >= m_InventorySlots.size())
            continue;   // Beyond the visible bar.

        if (auto& slot = m_InventorySlots[barSlot])
        {
            slot->setBackgroundColor(UITheme::WithAlpha(UITheme::Primary, 0.16f));
            slot->setBorder(UITheme::WithAlpha(UITheme::Primary, 0.45f), UITheme::BorderThin);
        }

        // There is no item art or name table yet, so the slot shows the id and
        // the count, which is everything the server actually tells us.
        // Artwork where there is any, the id as a readable fallback where
        // there is not.
        const std::string iconPath = IconPathForItem(entry.itemId);
        std::shared_ptr<Texture> icon;

        if (!iconPath.empty())
        {
            if (auto assets = ServiceLocator::Get<AssetManager>())
            {
                // Mipmaps off: 32x32 pixel art shown small goes soft with them.
                icon = assets->LoadTexture(iconPath, false, false);
            }
        }

        if (icon && barSlot < m_InventoryIcons.size() && m_InventoryIcons[barSlot])
        {
            m_InventoryIcons[barSlot]->setTexture(std::move(icon));
            m_InventoryIcons[barSlot]->setVisible(true);

            if (auto& label = m_InventoryLabels[barSlot])
                label->setText("");
        }
        else if (auto& label = m_InventoryLabels[barSlot])
        {
            label->setText(std::format("{}", entry.itemId));
            label->setTextColor(UITheme::Text);
        }

        if (auto& count = m_InventoryCounts[barSlot])
            count->setText(entry.quantity > 1 ? std::format("{}", entry.quantity) : std::string());
    }

    // Repainted last: the loop above resets borders, which would otherwise
    // erase the selection ring.
    RefreshSlotHighlight();
}