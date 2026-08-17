#include "MainMenuScreen.h"

#include "../core/AssetManager.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../core/Version.h"
#include "../graphics/Texture.h"
#include "../ui/UIButton.h"
#include "../ui/UIImage.h"
#include "../ui/UILabel.h"
#include "../ui/UIPanel.h"
#include "../ui/UIPatterns.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"
#include "../ui/UITiledImage.h"

#include <cmath>
#include <numbers>
#include <string>

namespace
{
    // The splash's own background, so dismissing it feels like the same scene
    // rather than a cut to a different screen.
    constexpr const char* kBackgroundArtwork = "assets/ui/world_loading/nature_3/origbig.png";

    constexpr float kArtworkAlpha = 0.30f;
    constexpr float kGrainAlpha   = 0.05f;

    // Menu geometry, in design pixels.
    constexpr float kButtonWidth  = 420.0f;
    constexpr float kButtonHeight = 74.0f;
    constexpr float kButtonGap    = 18.0f;

    constexpr float kMenuTop = 470.0f;

    struct Entry
    {
        const char*        label;
        UIButton::Variant  variant;
    };

    constexpr Entry kEntries[] = {
        {"PLAY ONLINE", UIButton::Variant::Primary},
        {"SETTINGS",    UIButton::Variant::Purple},
        {"CREDITS",     UIButton::Variant::Purple},
        {"EXIT",        UIButton::Variant::Danger},
    };
}

MainMenuScreen::MainMenuScreen(Engine* engine)
    : Screen(engine)
{
}

void MainMenuScreen::OnEnter()
{
    if (!uiManager_)
    {
        LOG_ERROR("MainMenuScreen: UIManager not available");
        return;
    }

    elapsed_ = 0.0f;

    CreateRoot();

    BuildBackground();
    BuildTitle();
    BuildMenu();
    BuildFooter();
}

void MainMenuScreen::BuildBackground()
{
    const UIScale* scale = Scale();

    const float width  = scale ? scale->GetVisibleWidth()  : UIScale::kDesignWidth;
    const float height = scale ? scale->GetVisibleHeight() : UIScale::kDesignHeight;

    auto background = std::make_shared<UIPanel>();
    background->setBackgroundColor(UITheme::Background);
    background->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    background->setBorderRadius(0.0f);
    background->setPosition(0.0f, 0.0f);
    background->setSize(width, height);
    root_->addChild(background);

    if (AssetManager* assets = Assets())
    {
        if (std::shared_ptr<Texture> artwork = assets->LoadTexture(kBackgroundArtwork))
        {
            auto image = std::make_shared<UIImage>();
            image->setTexture(std::move(artwork));
            image->setScaleMode(UIImage::ScaleMode::Fill);
            image->setColor(Color(1.0f, 1.0f, 1.0f, kArtworkAlpha));
            image->setPosition(0.0f, 0.0f);
            image->setSize(width, height);
            root_->addChild(image);
        }

        // Slightly heavier than the splash: the menu has more text over it.
        auto scrim = std::make_shared<UIPanel>();
        scrim->setBackgroundGradient(UITheme::Hex(0x090E1A, 0.62f), UITheme::Hex(0x0F1828, 0.86f));
        scrim->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
        scrim->setBorderRadius(0.0f);
        scrim->setPosition(0.0f, 0.0f);
        scrim->setSize(width, height);
        root_->addChild(scrim);

        if (auto grainTexture = UIPatterns::GetGrain(*assets))
        {
            auto grain = std::make_shared<UITiledImage>();
            grain->setTexture(std::move(grainTexture));
            grain->setTileSize(UIPatterns::kGrainTileSize);
            grain->setColor(Color(1.0f, 1.0f, 1.0f, kGrainAlpha));
            grain->setPosition(0.0f, 0.0f);
            grain->setSize(width, height);
            root_->addChild(grain);
        }
    }
}

void MainMenuScreen::BuildTitle()
{
    const float originX = DesignOriginX();
    const float originY = DesignOriginY();

    // Same wordmark as the splash, smaller, so the eye stays on the menu.
    auto title = std::make_shared<UILabel>();
    title->setText("StrixVerse");
    title->setFont(DisplayFont(UITheme::Display::Hero));
    title->setTextColor(UITheme::Text);
    title->setAlignment(UILabel::Alignment::Center);
    title->setLetterSpacing(4.0f);
    title->setGlow(UITheme::WithAlpha(UITheme::Accent, 0.55f), 34.0f);
    title->setPosition(originX, originY + 250.0f);
    title->setSize(UIScale::kDesignWidth, 80.0f);
    root_->addChild(title);

    auto tagline = std::make_shared<UILabel>();
    tagline->setText("CRYSTAL TECHNOLOGY - FANTASY WORLD");
    tagline->setFont(DisplayFont(UITheme::Display::Label));
    tagline->setTextColor(UITheme::Accent);
    tagline->setAlignment(UILabel::Alignment::Center);
    tagline->setLetterSpacing(3.68f);
    tagline->setPosition(originX, originY + 360.0f);
    tagline->setSize(UIScale::kDesignWidth, 30.0f);
    root_->addChild(tagline);
}

void MainMenuScreen::BuildMenu()
{
    const float originX = DesignOriginX();
    const float originY = DesignOriginY();

    const float x = originX + (UIScale::kDesignWidth - kButtonWidth) * 0.5f;

    for (size_t i = 0; i < std::size(kEntries); ++i)
    {
        auto button = std::make_shared<UIButton>();
        button->setText(kEntries[i].label);
        button->setFont(DisplayFont(UITheme::Display::Button));
        button->setVariant(kEntries[i].variant);
        button->setSize(kButtonWidth, kButtonHeight);
        button->setPosition(x, originY + kMenuTop +
                                static_cast<float>(i) * (kButtonHeight + kButtonGap));
        root_->addChild(button);

        buttons_[i] = button;
    }

    buttons_[0]->setOnClick([this]() { OnPlayOnline(); });
    buttons_[1]->setOnClick([this]() { RequestScreenChange(ScreenID::Settings); });
    buttons_[2]->setOnClick([this]() { RequestScreenChange(ScreenID::Credits); });
    buttons_[3]->setOnClick([this]() { OnQuitGame(); });
}

void MainMenuScreen::BuildFooter()
{
    const float originX = DesignOriginX();
    const float originY = DesignOriginY();

    statusLabel_ = std::make_shared<UILabel>();
    statusLabel_->setText(std::string("v") + Version::GetClientVersion() + " - Alpha Build");
    statusLabel_->setFont(DataFont(UITheme::Data::Small));
    statusLabel_->setTextColor(UITheme::WithAlpha(UITheme::Subtext, 0.30f));
    statusLabel_->setLetterSpacing(3.36f);
    statusLabel_->setPosition(originX + 30.0f, originY + 1033.0f);
    statusLabel_->setSize(528.0f, 24.0f);
    root_->addChild(statusLabel_);

    auto copyright = std::make_shared<UILabel>();
    copyright->setText("StrixVerse Studios (c) 2026");
    copyright->setFont(DataFont(UITheme::Data::Small));
    copyright->setTextColor(UITheme::WithAlpha(UITheme::Subtext, 0.30f));
    copyright->setAlignment(UILabel::Alignment::Right);
    copyright->setLetterSpacing(3.36f);
    copyright->setPosition(originX + UIScale::kDesignWidth - 30.0f - 528.0f,
                           originY + 1033.0f);
    copyright->setSize(528.0f, 24.0f);
    root_->addChild(copyright);
}

void MainMenuScreen::OnPlayOnline()
{
    // Straight to sign-in. Whether that leads to Continue or World Selection is
    // decided later, by whether this account has a saved session.
    RequestScreenChange(ScreenID::Login);
}

void MainMenuScreen::OnQuitGame()
{
    LOG_INFO("MainMenuScreen: exit requested");

    if (engine_)
        engine_->Stop();
}

void MainMenuScreen::Update(float deltaTime)
{
    elapsed_ += deltaTime;
}

void MainMenuScreen::OnKeyDown(int key, bool, bool)
{
    // Escape deliberately does nothing here. This is the top level, so there
    // is nowhere to go back to, and quitting the game on a stray Escape is a
    // harsh thing to do when Exit is right there on screen.
    (void)key;
}
