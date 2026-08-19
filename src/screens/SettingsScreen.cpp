#include "SettingsScreen.h"

#include "../core/Config.h"
#include "../core/Engine.h"
#include "../core/Window.h"
#include "../audio/AudioManager.h"
#include "../core/Logger.h"
#include "../networking/NetworkManager.h"
#include "../ui/UIButton.h"
#include "../ui/UIIcon.h"
#include "../ui/UILabel.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <string>

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    constexpr std::array<const char*, 6> kCategories = {
        "GENERAL", "GRAPHICS", "AUDIO", "CONTROLS", "ACCOUNT", "ACCESSIBILITY",
    };
}

SettingsScreen::SettingsScreen(Engine* engine)
    : Screen(engine)
{
}

void SettingsScreen::OnEnter()
{
    if (!uiManager_)
    {
        LOG_ERROR("SettingsScreen: UIManager not available");
        return;
    }

    CreateRoot();

    AddBackdrop(UITheme::ScreenBackground, UITheme::ScreenBackground, false);

    const float originX = DesignOriginX();
    const float originY = DesignOriginY();

    const float sidebarWidth = S(160.0f);

    auto sidebar = std::make_shared<UIPanel>();
    sidebar->setBackgroundColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    sidebar->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    sidebar->setBorderRadius(0.0f);
    sidebar->setPosition(originX, originY);
    sidebar->setSize(sidebarWidth, UIScale::kDesignHeight);
    root_->addChild(sidebar);

    auto rule = std::make_shared<UIPanel>();
    rule->setBackgroundColor(UITheme::WithAlpha(UITheme::Border, 0.20f));
    rule->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    rule->setBorderRadius(0.0f);
    rule->setPosition(sidebarWidth - UITheme::BorderThin, 0.0f);
    rule->setSize(UITheme::BorderThin, UIScale::kDesignHeight);
    sidebar->addChild(rule);

    titleLabel_ = std::make_shared<UILabel>();
    titleLabel_->setText("SETTINGS");
    titleLabel_->setFont(DisplayFont(UITheme::Display::Label));
    titleLabel_->setTextColor(UITheme::Muted);
    titleLabel_->setPosition(S(16.0f), S(16.0f));
    titleLabel_->setSize(sidebarWidth - S(32.0f), S(12.0f));
    sidebar->addChild(titleLabel_);

    float categoryY = S(40.0f);

    for (size_t i = 0; i < kCategories.size(); ++i)
    {
        const bool active = i == 0;

        auto entry = std::make_shared<UILabel>();
        entry->setText(kCategories[i]);
        entry->setFont(BodyFont(UITheme::Body::Regular));
        entry->setTextColor(active ? UITheme::Accent : UITheme::Subtext);
        entry->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
        entry->setPosition(S(16.0f), categoryY);
        entry->setSize(sidebarWidth - S(32.0f), S(20.0f));
        sidebar->addChild(entry);

        if (active)
        {
            auto marker = std::make_shared<UIPanel>();
            marker->setBackgroundColor(UITheme::Primary);
            marker->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
            marker->setBorderRadius(0.0f);
            marker->setPosition(0.0f, categoryY);
            marker->setSize(UITheme::BorderThick, S(20.0f));
            sidebar->addChild(marker);
        }

        categoryY += S(22.0f);
    }

    // Content area.
    auto heading = std::make_shared<UILabel>();
    heading->setText("GENERAL");
    heading->setFont(DisplayFont(UITheme::Display::Label));
    heading->setTextColor(UITheme::Accent);
    heading->setPosition(originX + sidebarWidth + S(20.0f), originY + S(20.0f));
    heading->setSize(S(300.0f), S(12.0f));
    root_->addChild(heading);

    BuildSettingsRows(originX + sidebarWidth + S(20.0f), originY + S(44.0f),
                      UIScale::kDesignWidth - sidebarWidth - S(60.0f));

    const float buttonHeight = S(26.0f);

    backButton_ = std::make_shared<UIButton>();
    backButton_->setText("BACK");
    backButton_->setFont(DisplayFont(UITheme::Display::Small));
    backButton_->setVariant(UIButton::Variant::Purple);
    backButton_->setPosition(originX + UIScale::kDesignWidth - S(120.0f),
                             originY + UIScale::kDesignHeight - S(50.0f));
    backButton_->setSize(S(100.0f), buttonHeight);
    backButton_->setOnClick([this]() { OnBackButtonClicked(); });
    root_->addChild(backButton_);

    auto backIcon = std::make_shared<UIIcon>(UIIcon::Shape::ArrowLeft);
    backIcon->setColor(UITheme::Text);
    backIcon->setPosition(S(8.0f), (buttonHeight - S(10.0f)) * 0.5f);
    backIcon->setSize(S(10.0f), S(10.0f));
    backButton_->addChild(backIcon);
    backButton_->setLabelInset(S(18.0f), 0.0f);

    // Only offered when Settings was opened from gameplay - there is no world
    // to leave when it was opened from the main menu.
    if (!engine_ || engine_->GetPreviousScreenId() != ScreenID::Game)
        return;

    // Leaving is destructive to the session, so it is styled as such and kept
    // clear of Back.
    leaveButton_ = std::make_shared<UIButton>();
    leaveButton_->setText("LEAVE WORLD");
    leaveButton_->setFont(DisplayFont(UITheme::Display::Small));
    leaveButton_->setVariant(UIButton::Variant::Danger);
    leaveButton_->setPosition(originX + UIScale::kDesignWidth - S(120.0f) - S(140.0f),
                              originY + UIScale::kDesignHeight - S(50.0f));
    leaveButton_->setSize(S(130.0f), buttonHeight);
    leaveButton_->setOnClick([this]() { OnLeaveWorldClicked(); });
    root_->addChild(leaveButton_);
}

void SettingsScreen::OnLeaveWorldClicked()
{
    if (engine_)
    {
        // Tell the server first: it despawns us for everyone else, and the
        // session stays open so we can pick another world.
        engine_->getNetworkManager().sendWorldLeave();

        // Forget the world. Leaving is a decision to stop playing there, so
        // there is nothing for Continue to offer next time.
        if (WorldManager* worlds = engine_->GetWorldManager())
            worlds->ClearLastWorld();

        engine_->SetSelectedWorldName(std::string());
    }

    LOG_INFO("SettingsScreen: left the world; returning to world selection");

    RequestScreenChange(ScreenID::WorldBrowser);
}

void SettingsScreen::OnKeyDown(int key, bool, bool)
{
    if (key == UIKey::Escape)
        OnBackButtonClicked();
}

void SettingsScreen::OnBackButtonClicked()
{
    // Settings is reachable from the main menu and from gameplay, so Back
    // returns to whichever it was. Anything else falls back to the menu.
    const ScreenID previous = engine_ ? engine_->GetPreviousScreenId() : ScreenID::MainMenu;

    RequestScreenChange(previous == ScreenID::Game ? ScreenID::Game : ScreenID::MainMenu);
}

namespace
{
    // Offered sizes, smallest first. All 16:9, because UIScale letterboxes
    // anything else and the design canvas is 1920x1080.
    struct Resolution { int width, height; };

    constexpr Resolution kResolutions[] = {
        {1280,  720}, {1366,  768}, {1600,  900},
        {1920, 1080}, {2560, 1440}, {3840, 2160},
    };

    constexpr int kVolumeStep = 10;
}

void SettingsScreen::BuildSettingsRows(float x, float y, float width)
{
    Config* config = engine_ ? engine_->GetConfig() : nullptr;
    if (!config)
    {
        auto missing = std::make_shared<UILabel>();
        missing->setText("Configuration unavailable; settings cannot be edited.");
        missing->setFont(BodyFont(UITheme::Body::Regular));
        missing->setTextColor(UITheme::Danger);
        missing->setPosition(x, y);
        missing->setSize(width, S(20.0f));
        root_->addChild(missing);
        return;
    }

    // Where the configured size sits in the offered list, so stepping starts
    // from the right place for a window that was sized by hand.
    resolutionIndex_ = -1;
    for (int i = 0; i < static_cast<int>(std::size(kResolutions)); ++i)
    {
        if (kResolutions[i].width == config->GetWidth() &&
            kResolutions[i].height == config->GetHeight())
        {
            resolutionIndex_ = i;
            break;
        }
    }

    float rowY = y;

    volumeValue_ = std::make_shared<UILabel>();
    rowY = BuildStepperRow(x, rowY, width, "MUSIC VOLUME", volumeValue_,
        [this]() {
            Config* c = engine_ ? engine_->GetConfig() : nullptr;
            if (!c) return;
            c->SetMusicVolume(std::max(0, c->GetMusicVolume() - kVolumeStep));
            ApplyAndSave();
        },
        [this]() {
            Config* c = engine_ ? engine_->GetConfig() : nullptr;
            if (!c) return;
            c->SetMusicVolume(std::min(100, c->GetMusicVolume() + kVolumeStep));
            ApplyAndSave();
        });

    resolutionValue_ = std::make_shared<UILabel>();
    rowY = BuildStepperRow(x, rowY, width, "RESOLUTION", resolutionValue_,
        [this]() { StepResolution(-1); },
        [this]() { StepResolution(1); });

    fullscreenToggle_ = std::make_shared<UIButton>();
    fullscreenToggle_->setOnClick([this]() {
        Config* c = engine_ ? engine_->GetConfig() : nullptr;
        if (!c) return;
        c->SetFullscreen(!c->IsFullscreen());
        ApplyAndSave();
    });
    rowY = BuildToggleRow(x, rowY, width, "FULLSCREEN", fullscreenToggle_);

    vsyncToggle_ = std::make_shared<UIButton>();
    vsyncToggle_->setOnClick([this]() {
        Config* c = engine_ ? engine_->GetConfig() : nullptr;
        if (!c) return;
        c->SetVSync(!c->IsVSyncEnabled());
        ApplyAndSave();
    });
    rowY = BuildToggleRow(x, rowY, width, "VERTICAL SYNC", vsyncToggle_);

    // Honest about what takes effect when. Volume and resolution are applied as
    // they change; the other two only reach the window at startup, and saying
    // so beats a setting that appears to do nothing.
    auto note = std::make_shared<UILabel>();
    note->setText("Volume and resolution apply immediately. "
                  "Fullscreen and vertical sync apply when the game restarts.");
    note->setFont(BodyFont(UITheme::Body::Caption));
    note->setTextColor(UITheme::Muted);
    note->setPosition(x, rowY + S(10.0f));
    note->setSize(width, S(18.0f));
    root_->addChild(note);

    RefreshSettingValues();
}

void SettingsScreen::StepResolution(int direction)
{
    Config* config = engine_ ? engine_->GetConfig() : nullptr;
    if (!config)
        return;

    const int count = static_cast<int>(std::size(kResolutions));

    // A size that is not in the list starts from the nearest entry rather than
    // jumping to one end.
    if (resolutionIndex_ < 0)
    {
        resolutionIndex_ = 0;
        for (int i = 0; i < count; ++i)
        {
            if (kResolutions[i].width <= config->GetWidth())
                resolutionIndex_ = i;
        }
    }
    else
    {
        resolutionIndex_ = std::clamp(resolutionIndex_ + direction, 0, count - 1);
    }

    config->SetWidth(kResolutions[resolutionIndex_].width);
    config->SetHeight(kResolutions[resolutionIndex_].height);

    if (Window* window = engine_ ? engine_->GetWindow() : nullptr)
        window->SetSize(kResolutions[resolutionIndex_].width,
                        kResolutions[resolutionIndex_].height);

    ApplyAndSave();
}

float SettingsScreen::BuildStepperRow(float x, float y, float width,
                                      const std::string& caption,
                                      const std::shared_ptr<UILabel>& value,
                                      const std::function<void()>& onDown,
                                      const std::function<void()>& onUp)
{
    const float rowHeight  = S(30.0f);
    const float stepWidth  = S(30.0f);
    const float valueWidth = S(120.0f);

    auto label = std::make_shared<UILabel>();
    label->setText(caption);
    label->setFont(BodyFont(UITheme::Body::Regular));
    label->setTextColor(UITheme::Subtext);
    label->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    label->setPosition(x, y);
    label->setSize(width - valueWidth - stepWidth * 2.0f - S(20.0f), rowHeight);
    root_->addChild(label);

    const float controlsX = x + width - valueWidth - stepWidth * 2.0f - S(10.0f);

    auto down = std::make_shared<UIButton>();
    down->setText("-");
    down->setFont(DisplayFont(UITheme::Display::Small));
    down->setVariant(UIButton::Variant::Purple);
    down->setPosition(controlsX, y);
    down->setSize(stepWidth, rowHeight - S(4.0f));
    down->setOnClick(onDown);
    root_->addChild(down);

    value->setFont(BodyFont(UITheme::Body::Medium));
    value->setTextColor(UITheme::Text);
    value->setAlignment(UILabel::Alignment::Center);
    value->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    value->setPosition(controlsX + stepWidth + S(5.0f), y);
    value->setSize(valueWidth, rowHeight);
    root_->addChild(value);

    auto up = std::make_shared<UIButton>();
    up->setText("+");
    up->setFont(DisplayFont(UITheme::Display::Small));
    up->setVariant(UIButton::Variant::Purple);
    up->setPosition(controlsX + stepWidth + valueWidth + S(10.0f), y);
    up->setSize(stepWidth, rowHeight - S(4.0f));
    up->setOnClick(onUp);
    root_->addChild(up);

    return y + rowHeight + S(8.0f);
}

float SettingsScreen::BuildToggleRow(float x, float y, float width,
                                     const std::string& caption,
                                     const std::shared_ptr<UIButton>& toggle)
{
    const float rowHeight   = S(30.0f);
    const float toggleWidth = S(90.0f);

    auto label = std::make_shared<UILabel>();
    label->setText(caption);
    label->setFont(BodyFont(UITheme::Body::Regular));
    label->setTextColor(UITheme::Subtext);
    label->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    label->setPosition(x, y);
    label->setSize(width - toggleWidth - S(10.0f), rowHeight);
    root_->addChild(label);

    toggle->setFont(DisplayFont(UITheme::Display::Small));
    toggle->setPosition(x + width - toggleWidth, y);
    toggle->setSize(toggleWidth, rowHeight - S(4.0f));
    root_->addChild(toggle);

    return y + rowHeight + S(8.0f);
}

void SettingsScreen::RefreshSettingValues()
{
    const Config* config = engine_ ? engine_->GetConfig() : nullptr;
    if (!config)
        return;

    if (volumeValue_)
        volumeValue_->setText(std::to_string(config->GetMusicVolume()) + "%");

    if (resolutionValue_)
        resolutionValue_->setText(std::to_string(config->GetWidth()) + " x " +
                                  std::to_string(config->GetHeight()));

    // A toggle carries its state as its own label and colour, so there is
    // nothing to read but the button.
    if (fullscreenToggle_)
    {
        const bool on = config->IsFullscreen();
        fullscreenToggle_->setText(on ? "ON" : "OFF");
        fullscreenToggle_->setVariant(on ? UIButton::Variant::Primary
                                         : UIButton::Variant::Purple);
    }

    if (vsyncToggle_)
    {
        const bool on = config->IsVSyncEnabled();
        vsyncToggle_->setText(on ? "ON" : "OFF");
        vsyncToggle_->setVariant(on ? UIButton::Variant::Primary
                                    : UIButton::Variant::Purple);
    }
}

void SettingsScreen::ApplyAndSave()
{
    Config* config = engine_ ? engine_->GetConfig() : nullptr;
    if (!config)
        return;

    // Music is the one setting that can be heard changing, so it is applied
    // straight away rather than on the next launch.
    if (engine_)
        engine_->GetAudio().SetMusicVolume(
            static_cast<float>(config->GetMusicVolume()) / 100.0f);

    RefreshSettingValues();

    // Written on every change: settings are few and small, and the alternative
    // is losing them when the client is closed from the window rather than
    // through this screen.
    config->Save();
}
