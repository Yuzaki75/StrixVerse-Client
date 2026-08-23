#include "GatePanel.h"

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

    // Card geometry, in style-guide pixels. Smaller than the vault: a gate
    // reports two facts and offers one action.
    constexpr float kCardWidth  = 220.0f;
    constexpr float kCardHeight = 150.0f;
    constexpr float kPadding    = 10.0f;
}

GatePanel::GatePanel(Engine* engine, UIManager* uiManager)
    : engine_(engine), uiManager_(uiManager)
{
}

GatePanel::~GatePanel()
{
    // The UIManager outlives this panel on a screen change, so the elements
    // have to be handed back or they keep drawing over whatever comes next.
    if (uiManager_ && root_)
        uiManager_->removeElement(root_);
}

void GatePanel::Build()
{
    if (root_ || !uiManager_)
        return;

    BuildFrame();

    root_->setVisible(false);
}

void GatePanel::BuildFrame()
{
    const float width  = S(kCardWidth);
    const float height = S(kCardHeight);

    root_ = std::make_shared<UIPanel>();
    root_->setSize(width, height);
    root_->setPosition((UIScale::kDesignWidth  - width)  * 0.5f,
                       (UIScale::kDesignHeight - height) * 0.5f);
    root_->setBackgroundColor(UITheme::Hex(0x1E2230, 0.94f));
    root_->setBorder(UITheme::WithAlpha(UITheme::Secondary, 0.45f),
                     UITheme::BorderThin);
    root_->setBorderRadius(UITheme::RadiusPanel);

    // A panel over live terrain swallows clicks: a bare UIPanel reports
    // wantsInput() false, which would let every input fall through to the world.
    root_->setBlocksInput(true);

    // Added to the UIManager rather than to a screen's root so it draws above
    // the HUD; UIManager renders in insertion order.
    uiManager_->addElement(root_);

    // Thin violet accent bar along the top edge - the gate's own colour among
    // the Lost Technology devices.
    auto topBar = std::make_shared<UIPanel>();
    topBar->setSize(width, S(3.0f));
    topBar->setPosition(0.0f, 0.0f);
    topBar->setBackgroundGradient(UITheme::Secondary,
                                  UITheme::WithAlpha(UITheme::Secondary, 0.35f));
    topBar->setBorderRadius(UITheme::RadiusBar);
    root_->addChild(topBar);

    auto title = std::make_shared<UILabel>();
    title->setText("AETHER GATE");
    title->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                             UITheme::Display::Section));
    title->setTextColor(UITheme::Text);
    title->setAlignment(UILabel::Alignment::Center);
    title->setPosition(0.0f, S(9.0f));
    title->setSize(width, S(13.0f));
    root_->addChild(title);

    statusLabel_ = std::make_shared<UILabel>();
    statusLabel_->setFont(PanelFont(engine_, UIFonts::Typeface::Data,
                                    UITheme::Data::Large));
    statusLabel_->setTextColor(UITheme::Muted);
    statusLabel_->setAlignment(UILabel::Alignment::Center);
    statusLabel_->setPosition(S(kPadding), S(30.0f));
    statusLabel_->setSize(width - S(kPadding) * 2.0f, S(14.0f));
    root_->addChild(statusLabel_);

    destinationLabel_ = std::make_shared<UILabel>();
    destinationLabel_->setFont(PanelFont(engine_, UIFonts::Typeface::Body,
                                         UITheme::Display::Label));
    destinationLabel_->setTextColor(UITheme::Subtext);
    destinationLabel_->setAlignment(UILabel::Alignment::Center);
    destinationLabel_->setPosition(S(kPadding), S(52.0f));
    destinationLabel_->setSize(width - S(kPadding) * 2.0f, S(11.0f));
    root_->addChild(destinationLabel_);

    auto activate = std::make_shared<UIButton>();
    activate->setText("ACTIVATE");
    activate->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                                UITheme::Display::Button));
    activate->setVariant(UIButton::Variant::Primary);
    activate->setSize(width - S(kPadding) * 2.0f, S(20.0f));
    activate->setPosition(S(kPadding),
                          height - S(20.0f) - S(kPadding));
    activate->setOnClick([this]() {
        if (onActivate)
            onActivate();
    });
    root_->addChild(activate);

    RefreshStatus();
}

void GatePanel::SetStatus(bool active)
{
    active_ = active;

    if (open_)
        RefreshStatus();
}

void GatePanel::SetDestination(const std::string& destination)
{
    destination_ = destination;

    if (open_)
        RefreshStatus();
}

void GatePanel::Open()
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

    // Status changes while closed were stashed rather than rendered; apply
    // them now so the panel never opens showing a stale gate.
    RefreshStatus();

    root_->setVisible(true);
}

void GatePanel::Close()
{
    if (!root_)
        return;

    open_ = false;
    root_->setVisible(false);
}

void GatePanel::Toggle()
{
    if (open_)
        Close();
    else
        Open();
}

void GatePanel::RefreshStatus()
{
    if (!statusLabel_ || !destinationLabel_)
        return;

    statusLabel_->setText(active_ ? "ACTIVE" : "DORMANT");
    statusLabel_->setTextColor(active_ ? UITheme::Success : UITheme::Muted);

    destinationLabel_->setText(destination_.empty()
                                   ? std::string("No destination linked")
                                   : "Destination: " + destination_);
}
