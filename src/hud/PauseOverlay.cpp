#include "PauseOverlay.h"

#include "../core/Engine.h"
#include "../ui/UIButton.h"
#include "../ui/UIFonts.h"
#include "../ui/UILabel.h"
#include "../ui/UIManager.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    Font* PanelFont(Engine* engine, UIFonts::Typeface face, unsigned int size)
    {
        UIFonts* fonts = engine ? engine->GetUIFonts() : nullptr;
        return fonts ? fonts->Get(face, size) : nullptr;
    }

    // The centred box, in style-guide pixels. Sized against the old in-screen
    // pause panel's 260x170, with room for three stacked buttons.
    constexpr float kBoxWidth  = 260.0f;
    constexpr float kBoxHeight = 210.0f;
}

PauseOverlay::PauseOverlay(Engine* engine, UIManager* uiManager)
    : engine_(engine), uiManager_(uiManager)
{
}

PauseOverlay::~PauseOverlay()
{
    // The UIManager outlives this panel on a screen change, so the elements
    // have to be handed back or they keep drawing over whatever comes next.
    if (uiManager_ && root_)
        uiManager_->removeElement(root_);
}

void PauseOverlay::Build()
{
    if (root_ || !uiManager_)
        return;

    BuildFrame();

    root_->setVisible(false);
}

void PauseOverlay::BuildFrame()
{
    // Full-canvas dim. It is the root and it blocks input: while paused, a
    // click belongs to the overlay or to nothing - the same modal rule the
    // world manager panel plays over live terrain.
    root_ = std::make_shared<UIPanel>();
    root_->setSize(UIScale::kDesignWidth, UIScale::kDesignHeight);
    root_->setPosition(0.0f, 0.0f);
    root_->setBackgroundColor(UITheme::Hex(0x000000, 0.55f));
    root_->setBlocksInput(true);

    // Added to the UIManager rather than to a screen's root so it draws above
    // the HUD; UIManager renders in insertion order.
    uiManager_->addElement(root_);

    const float boxWidth  = S(kBoxWidth);
    const float boxHeight = S(kBoxHeight);

    box_ = std::make_shared<UIPanel>();
    box_->setSize(boxWidth, boxHeight);
    box_->setPosition((UIScale::kDesignWidth  - boxWidth)  * 0.5f,
                      (UIScale::kDesignHeight - boxHeight) * 0.5f);
    box_->setBackgroundColor(UITheme::Hex(0x1E2230, 0.97f));
    box_->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.45f), UITheme::BorderThin);
    box_->setBorderRadius(UITheme::RadiusPanel);
    box_->setBlocksInput(true);
    root_->addChild(box_);

    auto title = std::make_shared<UILabel>();
    title->setText("PAUSED");
    title->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                             UITheme::Display::Heading));
    title->setTextColor(UITheme::Text);
    title->setAlignment(UILabel::Alignment::Center);
    title->setPosition(0.0f, S(18.0f));
    title->setSize(boxWidth, S(24.0f));
    box_->addChild(title);

    const float buttonWidth  = boxWidth - S(40.0f);
    const float buttonHeight = S(34.0f);
    float y = S(60.0f);

    const auto button = [&](const char* text, UIButton::Variant variant) {
        auto b = std::make_shared<UIButton>();
        b->setText(text);
        b->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                             UITheme::Display::Button));
        b->setVariant(variant);
        b->setSize(buttonWidth, buttonHeight);
        b->setPosition(S(20.0f), y);
        box_->addChild(b);

        y += buttonHeight + S(12.0f);
        return b;
    };

    auto resume = button("RESUME", UIButton::Variant::Primary);
    resume->setOnClick([this]() {
        if (onResume)
            onResume();
    });

    // Settings stays a real screen change: leaving gameplay for it is the
    // intended behaviour, unlike pausing.
    auto settings = button("SETTINGS", UIButton::Variant::Ghost);
    settings->setOnClick([this]() {
        if (onSettings)
            onSettings();
    });

    auto exit = button("EXIT WORLD", UIButton::Variant::Danger);
    exit->setOnClick([this]() {
        if (onExitWorld)
            onExitWorld();
    });
}

void PauseOverlay::Open()
{
    if (!root_)
        return;

    open_ = true;

    // UIManager renders in insertion order, so re-inserting keeps the overlay
    // above anything added after it was constructed.
    if (uiManager_)
    {
        uiManager_->removeElement(root_);
        uiManager_->addElement(root_);
    }

    root_->setVisible(true);
}

void PauseOverlay::Close()
{
    if (!root_)
        return;

    open_ = false;
    root_->setVisible(false);
}

void PauseOverlay::RaiseToFront()
{
    if (uiManager_ && root_)
    {
        uiManager_->bringToFront(root_);
    }
}
