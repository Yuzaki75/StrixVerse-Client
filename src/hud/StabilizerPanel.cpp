#include "StabilizerPanel.h"

#include "../core/Engine.h"
#include "../ui/UIButton.h"
#include "../ui/UIFonts.h"
#include "../ui/UILabel.h"
#include "../ui/UIManager.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

#include <algorithm>

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    Font* PanelFont(Engine* engine, UIFonts::Typeface face, unsigned int size)
    {
        UIFonts* fonts = engine ? engine->GetUIFonts() : nullptr;
        return fonts ? fonts->Get(face, size) : nullptr;
    }

    // Card geometry, in style-guide pixels. Between the gate and the vault in
    // size: a reading, a bar and a count.
    constexpr float kCardWidth  = 230.0f;
    constexpr float kCardHeight = 160.0f;
    constexpr float kPadding    = 10.0f;
    constexpr float kBarHeight  = 8.0f;

    // The stabiliser's accent is the theme's crystal blue itself.
    const Color& AccentColor() { return UITheme::Accent; }
}

StabilizerPanel::StabilizerPanel(Engine* engine, UIManager* uiManager)
    : engine_(engine), uiManager_(uiManager)
{
}

StabilizerPanel::~StabilizerPanel()
{
    // The UIManager outlives this panel on a screen change, so the elements
    // have to be handed back or they keep drawing over whatever comes next.
    if (uiManager_ && root_)
        uiManager_->removeElement(root_);
}

void StabilizerPanel::Build()
{
    if (root_ || !uiManager_)
        return;

    BuildFrame();

    root_->setVisible(false);
}

void StabilizerPanel::BuildFrame()
{
    const float width  = S(kCardWidth);
    const float height = S(kCardHeight);

    root_ = std::make_shared<UIPanel>();
    root_->setSize(width, height);
    root_->setPosition((UIScale::kDesignWidth  - width)  * 0.5f,
                       (UIScale::kDesignHeight - height) * 0.5f);
    root_->setBackgroundColor(UITheme::Hex(0x1E2230, 0.94f));
    root_->setBorder(UITheme::WithAlpha(AccentColor(), 0.45f), UITheme::BorderThin);
    root_->setBorderRadius(UITheme::RadiusPanel);

    // A panel over live terrain swallows clicks: a bare UIPanel reports
    // wantsInput() false, which would let every input fall through to the world.
    root_->setBlocksInput(true);

    // Added to the UIManager rather than to a screen's root so it draws above
    // the HUD; UIManager renders in insertion order.
    uiManager_->addElement(root_);

    // Thin blue accent bar along the top edge - the stabiliser's own colour
    // among the Lost Technology devices.
    auto topBar = std::make_shared<UIPanel>();
    topBar->setSize(width, S(3.0f));
    topBar->setPosition(0.0f, 0.0f);
    topBar->setBackgroundGradient(AccentColor(),
                                  UITheme::WithAlpha(AccentColor(), 0.35f));
    topBar->setBorderRadius(UITheme::RadiusBar);
    root_->addChild(topBar);

    auto title = std::make_shared<UILabel>();
    title->setText("AETHER STABILIZER");
    title->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                             UITheme::Display::Section));
    title->setTextColor(UITheme::Text);
    title->setAlignment(UILabel::Alignment::Center);
    title->setPosition(0.0f, S(9.0f));
    title->setSize(width, S(13.0f));
    root_->addChild(title);

    auto caption = std::make_shared<UILabel>();
    caption->setText("STABILITY");
    caption->setFont(PanelFont(engine_, UIFonts::Typeface::Body,
                               UITheme::Display::Small));
    caption->setTextColor(UITheme::Muted);
    caption->setPosition(S(kPadding), S(30.0f));
    caption->setSize(width - S(kPadding) * 2.0f, S(9.0f));
    root_->addChild(caption);

    const float barWidth = width - S(kPadding) * 2.0f;

    auto barBackground = std::make_shared<UIPanel>();
    barBackground->setSize(barWidth, S(kBarHeight));
    barBackground->setPosition(S(kPadding), S(42.0f));
    barBackground->setBackgroundColor(UITheme::InputBackground);
    barBackground->setBorderRadius(UITheme::RadiusBar);
    root_->addChild(barBackground);

    // The fill carries the Aether palette as a gradient from crystal blue down
    // to violet - the same draining-bar treatment BuffDisplay uses.
    barFill_ = std::make_shared<UIPanel>();
    barFill_->setSize(barWidth, S(kBarHeight));
    barFill_->setPosition(S(kPadding), S(42.0f));
    barFill_->setBackgroundGradient(AccentColor(), UITheme::Secondary);
    barFill_->setBorderRadius(UITheme::RadiusBar);
    root_->addChild(barFill_);

    upgradesLabel_ = std::make_shared<UILabel>();
    upgradesLabel_->setFont(PanelFont(engine_, UIFonts::Typeface::Data,
                                      UITheme::Data::Regular));
    upgradesLabel_->setTextColor(UITheme::Subtext);
    upgradesLabel_->setPosition(S(kPadding), S(58.0f));
    upgradesLabel_->setSize(barWidth, S(10.0f));
    root_->addChild(upgradesLabel_);

    auto stabilize = std::make_shared<UIButton>();
    stabilize->setText("STABILIZE");
    stabilize->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                                 UITheme::Display::Button));
    stabilize->setVariant(UIButton::Variant::Primary);
    stabilize->setSize(width - S(kPadding) * 2.0f, S(20.0f));
    stabilize->setPosition(S(kPadding),
                           height - S(20.0f) - S(kPadding));
    stabilize->setOnClick([this]() {
        if (onStabilize)
            onStabilize();
    });
    root_->addChild(stabilize);

    ApplyBar();
    RefreshLabels();
}

void StabilizerPanel::SetStability(float v01)
{
    stability01_ = std::clamp(v01, 0.0f, 1.0f);

    if (open_)
        ApplyBar();
}

void StabilizerPanel::SetUpgrades(int current, int max)
{
    upgradesCurrent_ = current;
    upgradesMax_     = max;

    if (open_)
        RefreshLabels();
}

void StabilizerPanel::Open()
{
    if (!root_)
        return;

    open_ = true;

    // UIManager renders in insertion order, so re-inserting keeps the panel
    // above anything added after it was constructed.
    if (uiManager_)
    {
        uiManager_->removeElement(root_);
        uiManager_->addElement(root_);
    }

    // Readings taken while closed were stashed rather than rendered; apply
    // them now so the panel never opens showing a stale device.
    ApplyBar();
    RefreshLabels();

    root_->setVisible(true);
}

void StabilizerPanel::Close()
{
    if (!root_)
        return;

    open_ = false;
    root_->setVisible(false);
}

void StabilizerPanel::Toggle()
{
    if (open_)
        Close();
    else
        Open();
}

void StabilizerPanel::ApplyBar()
{
    if (!barFill_)
        return;

    const float fullWidth = S(kCardWidth) - S(kPadding) * 2.0f;

    // Same fill-resizing move as BuffDisplay: shrink the fill's width inside a
    // fixed background track.
    barFill_->setSize(fullWidth * stability01_, barFill_->getHeight());
}

void StabilizerPanel::RefreshLabels()
{
    if (!upgradesLabel_)
        return;

    upgradesLabel_->setText("Upgrades: " + std::to_string(upgradesCurrent_) +
                            " / " + std::to_string(upgradesMax_));
}
