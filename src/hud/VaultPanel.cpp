#include "VaultPanel.h"

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

    // Card geometry, in style-guide pixels. Sized against the pause overlay's
    // S(260) x S(210) box; wider because the rows carry a name and a count.
    constexpr float kCardWidth   = 240.0f;
    constexpr float kCardHeight  = 200.0f;
    constexpr float kPadding     = 10.0f;
    constexpr float kRowHeight   = 15.0f;

    // Rows beyond this simply are not shown until the list becomes scrollable.
    constexpr std::size_t kMaxRows = 8;

    // The vault's accent. UITheme has Gold for exactly this - coins and rare
    // stores - so no local colour is needed.
    const Color& AccentColor() { return UITheme::Gold; }

    // Thousands separators, matching how other numeric displays read.
    std::string FormatQuantity(int quantity)
    {
        std::string digits = std::to_string(quantity);

        std::string grouped;
        grouped.reserve(digits.size() + digits.size() / 3);

        for (std::size_t i = 0; i < digits.size(); ++i)
        {
            if (i > 0 && (digits.size() - i) % 3 == 0)
                grouped += ',';

            grouped += digits[i];
        }

        return grouped;
    }
}

VaultPanel::VaultPanel(Engine* engine, UIManager* uiManager)
    : engine_(engine), uiManager_(uiManager)
{
}

VaultPanel::~VaultPanel()
{
    // The UIManager outlives this panel on a screen change, so the elements
    // have to be handed back or they keep drawing over whatever comes next.
    if (uiManager_ && root_)
        uiManager_->removeElement(root_);
}

void VaultPanel::Build()
{
    if (root_ || !uiManager_)
        return;

    BuildFrame();

    root_->setVisible(false);
}

void VaultPanel::BuildFrame()
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

    // Thin accent bar along the top edge - each Lost Technology device wears
    // its own colour so they read as different machines at a glance.
    auto topBar = std::make_shared<UIPanel>();
    topBar->setSize(width, S(3.0f));
    topBar->setPosition(0.0f, 0.0f);
    topBar->setBackgroundGradient(AccentColor(),
                                  UITheme::WithAlpha(AccentColor(), 0.35f));
    topBar->setBorderRadius(UITheme::RadiusBar);
    root_->addChild(topBar);

    auto title = std::make_shared<UILabel>();
    title->setText("AETHER VAULT");
    title->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                             UITheme::Display::Section));
    title->setTextColor(UITheme::Text);
    title->setAlignment(UILabel::Alignment::Center);
    title->setPosition(0.0f, S(9.0f));
    title->setSize(width, S(13.0f));
    root_->addChild(title);

    list_ = std::make_shared<UIPanel>();
    list_->setSize(width - S(kPadding) * 2.0f, height - S(58.0f));
    list_->setPosition(S(kPadding), S(28.0f));
    list_->setBackgroundColor(UITheme::Hex(0x000000, 0.0f));
    root_->addChild(list_);

    // Shown only when the vault holds nothing at all.
    emptyLabel_ = std::make_shared<UILabel>();
    emptyLabel_->setText("Vault is empty");
    emptyLabel_->setFont(PanelFont(engine_, UIFonts::Typeface::Body,
                                   UITheme::Display::Label));
    emptyLabel_->setTextColor(UITheme::Muted);
    emptyLabel_->setAlignment(UILabel::Alignment::Center);
    emptyLabel_->setPosition(S(kPadding), S(60.0f));
    emptyLabel_->setSize(width - S(kPadding) * 2.0f, S(12.0f));
    emptyLabel_->setVisible(false);
    root_->addChild(emptyLabel_);

    const float buttonWidth  = (width - S(kPadding) * 2.0f - S(12.0f)) / 3.0f;
    const float buttonHeight = S(18.0f);
    const float buttonY      = height - buttonHeight - S(kPadding);

    const auto action = [&](const char* text, UIButton::Variant variant, float x,
                            std::function<void()> VaultPanel::* callbackSlot) {
        auto b = std::make_shared<UIButton>();
        b->setText(text);
        b->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                             UITheme::Display::Small));
        b->setVariant(variant);
        b->setSize(buttonWidth, buttonHeight);
        b->setPosition(x, buttonY);
        b->setOnClick([this, callbackSlot]() {
            if (this->*callbackSlot)
                (this->*callbackSlot)();
        });
        root_->addChild(b);
    };

    action("WITHDRAW", UIButton::Variant::Ghost, S(kPadding),
           &VaultPanel::onWithdraw);
    action("DEPOSIT", UIButton::Variant::Primary, S(kPadding) + buttonWidth + S(6.0f),
           &VaultPanel::onDeposit);
    action("MANAGE", UIButton::Variant::Purple, width - S(kPadding) - buttonWidth,
           &VaultPanel::onManage);

    RebuildRows();
}

void VaultPanel::SetResources(const std::vector<ResourceEntry>& resources)
{
    resources_ = resources;

    if (open_)
        RebuildRows();
}

void VaultPanel::Open()
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

    // Contents changed while closed were stashed rather than rendered; apply
    // them now so the panel never opens showing a stale vault.
    RebuildRows();

    root_->setVisible(true);
}

void VaultPanel::Close()
{
    if (!root_)
        return;

    open_ = false;
    root_->setVisible(false);
}

void VaultPanel::Toggle()
{
    if (open_)
        Close();
    else
        Open();
}

void VaultPanel::RebuildRows()
{
    if (!list_)
        return;

    list_->clearChildren();
    list_->setSize(S(kCardWidth) - S(kPadding) * 2.0f, S(kCardHeight) - S(58.0f));

    if (emptyLabel_)
        emptyLabel_->setVisible(resources_.empty());

    const float width     = list_->getWidth();
    const float rowHeight = S(kRowHeight);

    const std::size_t shown = std::min(resources_.size(), kMaxRows);

    float y = 0.0f;

    for (std::size_t i = 0; i < shown; ++i)
    {
        const ResourceEntry& entry = resources_[i];

        auto row = std::make_shared<UIPanel>();
        row->setSize(width, rowHeight);
        row->setPosition(0.0f, y);
        row->setBackgroundColor(UITheme::RowBackground);
        row->setBorderRadius(UITheme::RadiusChip);
        list_->addChild(row);

        auto name = std::make_shared<UILabel>();
        name->setFont(PanelFont(engine_, UIFonts::Typeface::Data,
                                UITheme::Data::Regular));
        name->setTextColor(UITheme::Subtext);
        name->setText(entry.name);
        name->setPosition(S(5.0f), S(3.0f));
        name->setSize(width * 0.6f, S(10.0f));
        row->addChild(name);

        auto quantity = std::make_shared<UILabel>();
        quantity->setFont(PanelFont(engine_, UIFonts::Typeface::Data,
                                    UITheme::Data::Regular));
        quantity->setTextColor(UITheme::Text);
        quantity->setText(FormatQuantity(entry.quantity));
        quantity->setAlignment(UILabel::Alignment::Right);
        quantity->setPosition(width * 0.55f, S(3.0f));
        quantity->setSize(width * 0.45f - S(5.0f), S(10.0f));
        row->addChild(quantity);

        y += rowHeight + S(3.0f);
    }
}
