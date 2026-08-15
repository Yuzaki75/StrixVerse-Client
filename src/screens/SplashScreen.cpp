#include "SplashScreen.h"

#include "../core/AssetManager.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../core/Version.h"
#include "../graphics/Texture.h"
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
#include <vector>

namespace
{
    // ---------------------------------------------------------------------
    // Every value below is taken directly from the Figma frame "Desktop - 1"
    // (file 0hsZzh0jsqtbAJyb9wDkAu, node 15:5), which is authored at the
    // 1920x1080 design resolution. No conversion is applied.
    // ---------------------------------------------------------------------

    constexpr float kWelcomeTop  = 333.0f;
    constexpr float kTitleTop    = 392.0f;
    constexpr float kTaglineTop  = 544.0f;
    constexpr float kPromptTop   = 685.0f;
    constexpr float kFooterTop   = 1033.0f;
    constexpr float kFooterWidth = 528.0f;

    constexpr float kFooterLeftCentre  = 220.0f;    // Centre of the left footer box.
    constexpr float kFooterRightCentre = 1656.0f;   // Centre of the right footer box.

    // "Crystal Shards": five tilted slivers. Each entry is the centre of the
    // Figma container plus the sliver's own width and height.
    struct Shard
    {
        float centreX;
        float centreY;
        float width;
        float height;
    };

    constexpr Shard kShards[] = {
        {137.55f + 94.612f * 0.5f,  130.0f + 78.873f * 0.5f,  42.0f, 85.0f},
        {1706.45f + 73.658f * 0.5f,  86.0f + 61.579f * 0.5f,  33.0f, 66.0f},
        {102.11f + 62.497f * 0.5f,  778.0f + 52.249f * 0.5f,  28.0f, 56.0f},
        {1755.66f + 83.952f * 0.5f, 756.0f + 70.409f * 0.5f,  38.0f, 75.0f},
        {992.91f + 52.703f * 0.5f,  950.0f + 44.285f * 0.5f,  24.0f, 47.0f},
    };

    constexpr float kShardRotation = 60.0f * std::numbers::pi_v<float> / 180.0f;

    // The pulsing prompt cycles between these alphas; Figma layers a 0.6-alpha
    // copy under a solid one to show both ends of the animation.
    constexpr float kPromptMinAlpha = 0.60f;
    constexpr float kPromptMaxAlpha = 1.00f;
    constexpr float kPromptPeriod   = 1.6f;

    constexpr float kIntroDuration = 0.9f;

    // The design's grain sits at ~16% effective opacity in screen blend mode;
    // a low-alpha white noise over the dark background reads the same way in a
    // single pass.
    constexpr float kGrainAlpha = 0.05f;

    // Background artwork. The Figma frame is a flat dark field, so this is an
    // addition rather than something transcribed from it. It is held at low
    // alpha under a gradient scrim for two reasons: the title carries a 50/100px
    // cyan glow that needs a dark ground to read against, and the footer type is
    // drawn at 30% alpha, which disappears over anything bright.
    constexpr const char* kBackgroundArtwork = "assets/ui/world_loading/nature_3/origbig.png";

    constexpr float kArtworkAlpha = 0.30f;
}

SplashScreen::SplashScreen(Engine* engine)
    : Screen(engine)
{
}

void SplashScreen::OnEnter()
{
    if (!uiManager_)
    {
        LOG_ERROR("SplashScreen: UIManager not available");
        return;
    }

    elapsed_  = 0.0f;
    advanced_ = false;

    CreateRoot();

    BuildBackground();
    BuildBrand();
    BuildFooter();
}

void SplashScreen::BuildBackground()
{
    const UIScale* scale = Scale();

    // Full-bleed background. It covers the whole window, not just the design
    // area, so a non-16:9 window shows no bars.
    auto background = std::make_shared<UIPanel>();
    background->setBackgroundColor(UITheme::Background);
    background->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    background->setBorderRadius(0.0f);

    if (scale)
    {
        background->setPosition(0.0f, 0.0f);
        background->setSize(scale->GetVisibleWidth(), scale->GetVisibleHeight());
    }
    else
    {
        background->setPosition(0.0f, 0.0f);
        background->setSize(UIScale::kDesignWidth, UIScale::kDesignHeight);
    }

    root_->addChild(background);

    BuildArtwork(background->getWidth(), background->getHeight());

    // Crystal shards, positioned in design space (the root is offset to the
    // visible canvas origin, so shift them back onto the design grid).
    const float originX = scale ? -scale->GetVisibleLeft() : 0.0f;
    const float originY = scale ? -scale->GetVisibleTop() : 0.0f;

    for (const Shard& shard : kShards)
    {
        auto sliver = std::make_shared<UIPanel>();
        sliver->setBackgroundColor(UITheme::WithAlpha(UITheme::Accent, 0.05f));
        sliver->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.10f), 1.0f);
        sliver->setBorderRadius(2.0f);
        sliver->setRotation(kShardRotation);
        sliver->setPosition(originX + shard.centreX - shard.width * 0.5f,
                            originY + shard.centreY - shard.height * 0.5f);
        sliver->setSize(shard.width, shard.height);

        root_->addChild(sliver);
    }

    // Film grain over the whole background ("Noise & Texture" in the frame).
    if (AssetManager* assets = Assets())
    {
        if (auto grainTexture = UIPatterns::GetGrain(*assets))
        {
            auto grain = std::make_shared<UITiledImage>();
            grain->setTexture(std::move(grainTexture));
            grain->setTileSize(UIPatterns::kGrainTileSize);
            grain->setColor(Color(1.0f, 1.0f, 1.0f, kGrainAlpha));
            grain->setPosition(0.0f, 0.0f);
            grain->setSize(background->getWidth(), background->getHeight());

            root_->addChild(grain);
        }
    }
}

void SplashScreen::BuildArtwork(float width, float height)
{
    AssetManager* assets = Assets();
    if (!assets)
        return;

    // The AssetManager caches by path, so this texture is shared with the world
    // loading screen rather than decoded a second time.
    std::shared_ptr<Texture> artwork = assets->LoadTexture(kBackgroundArtwork);

    if (!artwork)
    {
        // Missing artwork is not fatal - the flat background underneath is the
        // design's own, so the splash degrades to exactly the Figma frame.
        LOG_WARN(std::string("SplashScreen: background artwork '") + kBackgroundArtwork +
                 "' unavailable; falling back to the flat background");
        return;
    }

    // Cover the visible canvas: the art is 16:9, so on a 16:9 window this is an
    // exact fit and on any other shape the overflow is cropped rather than
    // letterboxed.
    auto image = std::make_shared<UIImage>();
    image->setTexture(std::move(artwork));
    image->setScaleMode(UIImage::ScaleMode::Fill);
    image->setColor(Color(1.0f, 1.0f, 1.0f, kArtworkAlpha));
    image->setPosition(0.0f, 0.0f);
    image->setSize(width, height);
    root_->addChild(image);

    // Darkening scrim, heavier at the bottom where the prompt and the footers
    // sit over the artwork's brighter foreground.
    auto scrim = std::make_shared<UIPanel>();
    scrim->setBackgroundGradient(UITheme::Hex(0x090E1A, 0.52f), UITheme::Hex(0x0F1828, 0.80f));
    scrim->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    scrim->setBorderRadius(0.0f);
    scrim->setPosition(0.0f, 0.0f);
    scrim->setSize(width, height);
    root_->addChild(scrim);
}

void SplashScreen::BuildBrand()
{
    const UIScale* scale = Scale();
    const float originX = scale ? -scale->GetVisibleLeft() : 0.0f;
    const float originY = scale ? -scale->GetVisibleTop() : 0.0f;

    // "WELCOME TO" - VT323 33px, #6C5CE7, 9.9px tracking.
    auto welcome = std::make_shared<UILabel>();
    welcome->setText("WELCOME TO");
    welcome->setFont(BodyFont(UITheme::Body::Welcome));
    welcome->setTextColor(UITheme::Secondary);
    welcome->setLetterSpacing(UITheme::Tracking::SplashWelcome);
    welcome->setAlignment(UILabel::Alignment::Center);
    welcome->setPosition(originX, originY + kWelcomeTop);
    welcome->setSize(UIScale::kDesignWidth, 40.0f);
    root_->addChild(welcome);

    // "StrixVerse" - Press Start 2P 103px, white, 6.18px tracking,
    // 4px hard shadow plus the layered cyan/blue glow.
    auto title = std::make_shared<UILabel>();
    title->setText("StrixVerse");
    title->setFont(DisplayFont(UITheme::Display::Splash));
    title->setTextColor(UITheme::Text);
    title->setLetterSpacing(UITheme::Tracking::SplashTitle);
    title->setAlignment(UILabel::Alignment::Center);
    title->setShadow(Color(0.0f, 0.0f, 0.0f, 1.0f), 4.0f, 4.0f);
    title->setGlow(UITheme::WithAlpha(UITheme::Accent, 0.95f), 22.0f);
    title->setPosition(originX, originY + kTitleTop);
    title->setSize(UIScale::kDesignWidth, 120.0f);
    root_->addChild(title);

    // Tagline - Press Start 2P 23px, #4DE1FF, 3.68px tracking, cyan glow.
    auto tagline = std::make_shared<UILabel>();
    tagline->setText("CRYSTAL TECHNOLOGY \xE2\x80\xA2 FANTASY WORLD");
    tagline->setFont(DisplayFont(UITheme::Display::Tagline));
    tagline->setTextColor(UITheme::Accent);
    tagline->setLetterSpacing(UITheme::Tracking::SplashTagline);
    tagline->setAlignment(UILabel::Alignment::Center);
    tagline->setGlow(UITheme::WithAlpha(UITheme::Accent, 0.75f), 8.0f);
    tagline->setPosition(originX, originY + kTaglineTop);
    tagline->setSize(UIScale::kDesignWidth, 40.0f);
    root_->addChild(tagline);

    // Pulsing prompt - VT323 52px, #C7D0E0, 8.32px tracking.
    promptLabel_ = std::make_shared<UILabel>();
    promptLabel_->setText("PRESS ANY KEY TO CONTINUE");
    promptLabel_->setFont(BodyFont(UITheme::Body::Splash));
    promptLabel_->setTextColor(UITheme::Subtext);
    promptLabel_->setLetterSpacing(UITheme::Tracking::SplashPrompt);
    promptLabel_->setAlignment(UILabel::Alignment::Center);
    promptLabel_->setPosition(originX, originY + kPromptTop);
    promptLabel_->setSize(UIScale::kDesignWidth, 60.0f);
    root_->addChild(promptLabel_);
}

void SplashScreen::BuildFooter()
{
    const UIScale* scale = Scale();
    const float originX = scale ? -scale->GetVisibleLeft() : 0.0f;
    const float originY = scale ? -scale->GetVisibleTop() : 0.0f;

    const Color footerColor = UITheme::WithAlpha(UITheme::Subtext, 0.30f);

    auto build = std::make_shared<UILabel>();
    build->setText(std::string("v") + Version::GetClientVersion() + " - Alpha Build");
    build->setFont(DataFont(UITheme::Data::Small));
    build->setTextColor(footerColor);
    build->setLetterSpacing(UITheme::Tracking::SplashFooter);
    build->setAlignment(UILabel::Alignment::Center);
    build->setPosition(originX + kFooterLeftCentre - kFooterWidth * 0.5f, originY + kFooterTop);
    build->setSize(kFooterWidth, 29.0f);
    root_->addChild(build);

    auto studio = std::make_shared<UILabel>();
    studio->setText("StrixVerse Studios \xC2\xA9 2026");
    studio->setFont(DataFont(UITheme::Data::Small));
    studio->setTextColor(footerColor);
    studio->setLetterSpacing(UITheme::Tracking::SplashFooter);
    studio->setAlignment(UILabel::Alignment::Center);
    studio->setPosition(originX + kFooterRightCentre - kFooterWidth * 0.5f, originY + kFooterTop);
    studio->setSize(kFooterWidth, 29.0f);
    root_->addChild(studio);
}

void SplashScreen::Update(float deltaTime)
{
    elapsed_ += deltaTime;

    // Entry fade so the lockup resolves rather than popping in.
    if (root_ && elapsed_ < kIntroDuration)
        root_->setOpacity(elapsed_ / kIntroDuration);
    else if (root_)
        root_->setOpacity(1.0f);

    // glowPulse: a smooth 1.6s cycle between the two alphas Figma layers.
    if (promptLabel_)
    {
        const float phase = std::sin(elapsed_ * 2.0f * std::numbers::pi_v<float> / kPromptPeriod);
        const float t     = 0.5f + 0.5f * phase;
        const float alpha = kPromptMinAlpha + (kPromptMaxAlpha - kPromptMinAlpha) * t;

        promptLabel_->setTextColor(UITheme::WithAlpha(UITheme::Subtext, alpha));
    }
}

void SplashScreen::Advance()
{
    if (advanced_)
        return;

    // Ignore input during the intro fade, so a stray keypress at launch does
    // not skip the screen before it is even visible.
    if (elapsed_ < 0.35f)
        return;

    advanced_ = true;
    RequestScreenChange(ScreenID::Login);
}

void SplashScreen::OnKeyDown(int, bool, bool)
{
    Advance();
}

void SplashScreen::OnMouseDown(float, float)
{
    Advance();
}
