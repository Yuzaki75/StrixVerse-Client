#include "MemoryCrystalPanel.h"

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

    // Card geometry, in style-guide pixels. Taller than the vault because the
    // log is the whole point of the device.
    constexpr float kCardWidth   = 260.0f;
    constexpr float kCardHeight  = 210.0f;
    constexpr float kPadding     = 10.0f;
    constexpr float kRowHeight   = 14.0f;

    // Simple scrollable feel for now: show at most this many of the most
    // recent entries. A UIScrollPanel swap can come later without changing
    // the public surface.
    constexpr std::size_t kMaxRows = 9;

    // The crystal's own colour - a cyan-teal the theme palette does not carry,
    // so it is defined locally like every other Lost Technology accent.
    constexpr Color CrystalTeal = UITheme::Hex(0x3EE8C8);
}

MemoryCrystalPanel::MemoryCrystalPanel(Engine* engine, UIManager* uiManager)
    : engine_(engine), uiManager_(uiManager)
{
}

MemoryCrystalPanel::~MemoryCrystalPanel()
{
    // The UIManager outlives this panel on a screen change, so the elements
    // have to be handed back or they keep drawing over whatever comes next.
    if (uiManager_ && root_)
        uiManager_->removeElement(root_);
}

void MemoryCrystalPanel::Build()
{
    if (root_ || !uiManager_)
        return;

    BuildFrame();

    root_->setVisible(false);
}

void MemoryCrystalPanel::BuildFrame()
{
    const float width  = S(kCardWidth);
    const float height = S(kCardHeight);

    root_ = std::make_shared<UIPanel>();
    root_->setSize(width, height);
    root_->setPosition((UIScale::kDesignWidth  - width)  * 0.5f,
                       (UIScale::kDesignHeight - height) * 0.5f);
    root_->setBackgroundColor(UITheme::Hex(0x1E2230, 0.94f));
    root_->setBorder(UITheme::WithAlpha(CrystalTeal, 0.45f), UITheme::BorderThin);
    root_->setBorderRadius(UITheme::RadiusPanel);

    // A panel over live terrain swallows clicks: a bare UIPanel reports
    // wantsInput() false, which would let every input fall through to the world.
    root_->setBlocksInput(true);

    // Added to the UIManager rather than to a screen's root so it draws above
    // the HUD; UIManager renders in insertion order.
    uiManager_->addElement(root_);

    // Thin teal accent bar along the top edge - the crystal's own colour among
    // the Lost Technology devices.
    auto topBar = std::make_shared<UIPanel>();
    topBar->setSize(width, S(3.0f));
    topBar->setPosition(0.0f, 0.0f);
    topBar->setBackgroundGradient(CrystalTeal,
                                  UITheme::WithAlpha(CrystalTeal, 0.35f));
    topBar->setBorderRadius(UITheme::RadiusBar);
    root_->addChild(topBar);

    auto title = std::make_shared<UILabel>();
    title->setText("MEMORY CRYSTAL");
    title->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                             UITheme::Display::Section));
    title->setTextColor(UITheme::Text);
    title->setAlignment(UILabel::Alignment::Center);
    title->setPosition(0.0f, S(9.0f));
    title->setSize(width, S(13.0f));
    root_->addChild(title);

    list_ = std::make_shared<UIPanel>();
    list_->setSize(width - S(kPadding) * 2.0f, height - S(56.0f));
    list_->setPosition(S(kPadding), S(28.0f));
    list_->setBackgroundColor(UITheme::RowBackground);
    list_->setBorder(UITheme::SubtleBorder, UITheme::BorderThin);
    list_->setBorderRadius(UITheme::RadiusInput);
    root_->addChild(list_);

    const float buttonWidth  = (width - S(kPadding) * 2.0f - S(12.0f)) / 2.0f;
    const float buttonHeight = S(18.0f);
    const float buttonY      = height - buttonHeight - S(kPadding);

    auto extract = std::make_shared<UIButton>();
    extract->setText("EXTRACT");
    extract->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                               UITheme::Display::Small));
    extract->setVariant(UIButton::Variant::Primary);
    extract->setSize(buttonWidth, buttonHeight);
    extract->setPosition(S(kPadding), buttonY);
    extract->setOnClick([this]() {
        if (onExtract)
            onExtract();
    });
    root_->addChild(extract);

    auto close = std::make_shared<UIButton>();
    close->setText("CLOSE");
    close->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                             UITheme::Display::Small));
    close->setVariant(UIButton::Variant::Ghost);
    close->setSize(buttonWidth, buttonHeight);
    close->setPosition(S(kPadding) + buttonWidth + S(6.0f), buttonY);
    close->setOnClick([this]() { Close(); });
    root_->addChild(close);

    RebuildRows();
}

void MemoryCrystalPanel::SetEntries(const std::vector<std::string>& lines)
{
    entries_ = lines;

    if (open_)
        RebuildRows();
}

void MemoryCrystalPanel::Open()
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

    // Entries added while closed were stashed rather than rendered; apply
    // them now so the panel never opens showing a stale log.
    RebuildRows();

    root_->setVisible(true);
}

void MemoryCrystalPanel::Close()
{
    if (!root_)
        return;

    open_ = false;
    root_->setVisible(false);
}

void MemoryCrystalPanel::Toggle()
{
    if (open_)
        Close();
    else
        Open();
}

void MemoryCrystalPanel::RebuildRows()
{
    if (!list_)
        return;

    list_->clearChildren();

    const float width     = list_->getWidth();
    const float rowHeight = S(kRowHeight);

    // Show the most recent entries; older ones are simply beyond the window
    // until a real scrolling list replaces this one.
    const std::size_t count = entries_.size();
    const std::size_t first = count > kMaxRows ? count - kMaxRows : 0;

    float y = S(4.0f);

    for (std::size_t i = first; i < count; ++i)
    {
        auto line = std::make_shared<UILabel>();
        line->setFont(PanelFont(engine_, UIFonts::Typeface::Data,
                                UITheme::Data::Small));
        line->setTextColor(i % 2 == 0 ? UITheme::Subtext : UITheme::Text);
        line->setText(entries_[i]);
        line->setPosition(S(5.0f), y);
        line->setSize(width - S(10.0f), rowHeight);
        list_->addChild(line);

        y += rowHeight + S(2.0f);
    }
}
