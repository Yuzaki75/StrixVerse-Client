#include "SettingsScreen.h"

#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../networking/NetworkManager.h"
#include "../ui/UIButton.h"
#include "../ui/UIIcon.h"
#include "../ui/UILabel.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

#include <array>

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

    auto note = std::make_shared<UILabel>();
    note->setText("Settings are not part of the delivered design set yet.");
    note->setFont(BodyFont(UITheme::Body::Regular));
    note->setTextColor(UITheme::Muted);
    note->setPosition(originX + sidebarWidth + S(20.0f), originY + S(44.0f));
    note->setSize(S(400.0f), S(20.0f));
    root_->addChild(note);

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
