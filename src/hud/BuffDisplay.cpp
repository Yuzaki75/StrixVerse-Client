#include "BuffDisplay.h"

#include "../core/Engine.h"
#include "../graphics/Color.h"
#include "../ui/UIFonts.h"
#include "../ui/UILabel.h"
#include "../ui/UIManager.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    Font* PanelFont(Engine* engine, UIFonts::Typeface face, unsigned int size)
    {
        UIFonts* fonts = engine ? engine->GetUIFonts() : nullptr;
        return fonts ? fonts->Get(face, size) : nullptr;
    }

    // Whole seconds left, as the countdown label shows them.
    int ShownSeconds(float remainingSeconds)
    {
        return static_cast<int>(std::ceil(std::max(remainingSeconds, 0.0f)));
    }

    // Geometry, in style-guide pixels. A narrow column pinned under the HUD's
    // health and level panels at the top-left; it must never reach far enough
    // right to meet the chat entry field.
    //
    // The Y was 64, which is the level panel's own top edge (HUD sets it at
    // S(66)), so the first buff row drew straight through it. Under the panels
    // means below the lower of the two: the level panel ends at 66 + 40, and
    // 114 leaves eight pixels of gap after it. Nothing showed here until buffs
    // were wired, which is why the overlap had never been seen.
    //
    // The X is 14 rather than the panels' 20 because a row is inset by
    // kPadding within the root; 14 + 6 puts the row's left edge on the same
    // line as the health and level panels above it.
    constexpr float kPanelX       = 14.0f;
    constexpr float kPanelY       = 114.0f;
    constexpr float kRowWidth     = 130.0f;
    constexpr float kPadding      = 6.0f;

    // 11 was sized for a 12px font. The type scale's smallest VT323 is 28, so
    // the row grew to hold it - a name nobody can read is not a smaller
    // version of the HUD, it is a different and worse one.
    constexpr float kNameHeight   = 14.0f;
    constexpr float kBarHeight    = 4.0f;
    constexpr float kBarGap       = 3.0f;

    // Between stacked rows, so two buffs do not read as one block.
    constexpr float kRowGap       = 4.0f;
}

BuffDisplay::BuffDisplay(Engine* engine, UIManager* uiManager)
    : engine_(engine), uiManager_(uiManager)
{
}

BuffDisplay::~BuffDisplay()
{
    // The UIManager outlives this element on a screen change, so the elements
    // have to be handed back or they keep drawing over whatever comes next.
    if (uiManager_ && root_)
        uiManager_->removeElement(root_);
}

void BuffDisplay::Build()
{
    if (built_ || !uiManager_)
        return;

    BuildFrame();

    built_ = true;

    // Nothing is active until the first packet says so.
    root_->setVisible(false);
}

void BuffDisplay::BuildFrame()
{
    root_ = std::make_shared<UIPanel>();
    root_->setSize(S(kRowWidth), 0.0f);   // Grown to fit in RebuildRows.
    root_->setPosition(S(kPanelX), S(kPanelY));

    // The same treatment HUD::StyleStatPanel gives the health and level
    // panels this column sits under. It was fully transparent, so the buff
    // rows floated over the terrain with nothing behind them while every
    // other HUD element sat on a panel.
    root_->setBackgroundColor(UITheme::Hex(0x0E121E, 0.62f));
    root_->setBorder(UITheme::SubtleBorder, UITheme::BorderThin);
    root_->setBorderRadius(UITheme::RadiusButton);

    // Added to the UIManager rather than to a screen's root so it draws above
    // the world; UIManager renders in insertion order.
    uiManager_->addElement(root_);
}

void BuffDisplay::SetBuffs(const std::vector<BuffEntry>& buffs)
{
    buffs_ = buffs;

    if (!built_)
        return;

    RebuildRows();
}

void BuffDisplay::Clear()
{
    buffs_.clear();

    if (!built_)
        return;

    RebuildRows();
}

void BuffDisplay::RebuildRows()
{
    rows_.clear();

    if (!root_)
        return;

    root_->clearChildren();

    const float width     = S(kRowWidth) - S(kPadding) * 2.0f;
    const float rowHeight = S(kNameHeight + kBarGap + kBarHeight);

    float y = S(kPadding);

    for (const auto& buff : buffs_)
    {
        Row row;

        row.backing = std::make_shared<UIPanel>();
        row.backing->setSize(width, rowHeight);
        row.backing->setPosition(S(kPadding), y);
        row.backing->setBackgroundColor(UITheme::Hex(0x000000, 0.0f));

        row.name = std::make_shared<UILabel>();
        // Body::Tiny, not Display::Micro. Micro is 12 and belongs to the
        // Display face; asking VT323 for 12 pixels produced a line of grey
        // mush. The Body scale bottoms out at 28 for a reason.
        row.name->setFont(PanelFont(engine_, UIFonts::Typeface::Body,
                                    UITheme::Body::Tiny));
        row.name->setTextColor(UITheme::Text);
        row.name->setText(buff.name.empty() ? buff.id : buff.name);
        row.name->setPosition(0.0f, 0.0f);
        // Narrowed to leave the right-hand column for the countdown.
        row.name->setSize(width - S(24.0f), S(kNameHeight));
        row.backing->addChild(row.name);

        // Seconds remaining, Share Tech Mono per the type scale's rule that
        // numbers are Data; at the name's pixel size so the row stays one
        // visual line.
        row.seconds = std::make_shared<UILabel>();
        row.seconds->setFont(PanelFont(engine_, UIFonts::Typeface::Data,
                                       UITheme::Data::Small));
        row.seconds->setTextColor(UITheme::Subtext);
        row.seconds->setAlignment(UILabel::Alignment::Right);
        row.seconds->setPosition(width - S(24.0f), 0.0f);
        row.seconds->setSize(S(24.0f), S(kNameHeight));
        row.shownSecond = ShownSeconds(buff.remainingSeconds);
        row.seconds->setText(std::format("{}", row.shownSecond));
        row.backing->addChild(row.seconds);

        row.barBackground = std::make_shared<UIPanel>();
        row.barBackground->setSize(width, S(kBarHeight));
        row.barBackground->setPosition(0.0f, S(kNameHeight + kBarGap));
        row.barBackground->setBackgroundColor(UITheme::Hex(0x141826, 0.85f));
        row.barBackground->setBorderRadius(UITheme::RadiusBar);
        row.backing->addChild(row.barBackground);

        // The fill carries the Aether palette as a gradient from crystal blue
        // down to violet, so a draining bar visibly cools as it empties.
        row.barFill = std::make_shared<UIPanel>();
        row.barFill->setSize(width, S(kBarHeight));
        row.barFill->setPosition(0.0f, S(kNameHeight + kBarGap));
        row.barFill->setBackgroundGradient(UITheme::Accent, UITheme::Secondary);
        row.barFill->setBorderRadius(UITheme::RadiusBar);
        row.backing->addChild(row.barFill);

        root_->addChild(row.backing);
        rows_.push_back(std::move(row));

        y += rowHeight + S(kRowGap);
    }

    // The column grows downward with its content and shrinks back when the
    // last buff expires. The trailing gap is subtracted so the bottom padding
    // matches the top rather than being one gap deeper.
    root_->setSize(S(kRowWidth),
                   std::max(y - S(kRowGap), S(kPadding)) + S(kPadding));

    ApplyBars();
    UpdateVisibility();
}

void BuffDisplay::ApplyBars() const
{
    for (std::size_t i = 0; i < rows_.size(); ++i)
    {
        const BuffEntry& buff = buffs_[i];
        const Row&       row  = rows_[i];

        const float total =
            buff.totalSeconds > 0.0f ? buff.totalSeconds : 1.0f;

        const float fraction = std::clamp(buff.remainingSeconds / total,
                                          0.0f, 1.0f);

        const float width = S(kRowWidth) - S(kPadding) * 2.0f;

        if (row.barFill)
            row.barFill->setSize(width * fraction, row.barFill->getHeight());
    }
}

void BuffDisplay::UpdateVisibility()
{
    if (!root_)
        return;

    // Hidden entirely when there is nothing to show: an empty frame floating
    // under the stats would read as a rendering bug, not an absence of buffs.
    bool anyVisible = false;

    for (const auto& buff : buffs_)
    {
        if (buff.remainingSeconds > 0.0f)
        {
            anyVisible = true;
            break;
        }
    }

    root_->setVisible(anyVisible);
}

void BuffDisplay::LayoutForCanvas()
{
    if (!root_ || !engine_)
        return;

    const UIScale& scale = engine_->GetUIScale();

    const int width  = scale.GetFramebufferWidth();
    const int height = scale.GetFramebufferHeight();
    if (width <= 0 || height <= 0)
        return;

    if (width == laidOutWidth_ && height == laidOutHeight_)
        return;

    laidOutWidth_  = width;
    laidOutHeight_ = height;

    root_->setPosition(scale.GetVisibleLeft() + S(kPanelX),
                       scale.GetVisibleTop() + S(kPanelY));
}

void BuffDisplay::Update(float dt)
{
    LayoutForCanvas();

    if (!built_ || dt <= 0.0f || buffs_.empty())
        return;

    // Count every bar down between packets. A bar that reaches zero stops
    // moving but keeps its row until the next SetBuffs removes it: the server
    // decides when a buff ends, so the client never drops one on its own guess
    // about network latency.
    for (auto& buff : buffs_)
        buff.remainingSeconds = std::max(buff.remainingSeconds - dt, 0.0f);

    ApplyBars();

    // The numeric countdown is only rewritten when the displayed second
    // flips over: setText copies its argument, so rewriting an unchanged
    // figure would allocate every frame for identical text.
    for (std::size_t i = 0; i < rows_.size(); ++i)
    {
        const int second = ShownSeconds(buffs_[i].remainingSeconds);
        if (second == rows_[i].shownSecond)
            continue;

        rows_[i].shownSecond = second;
        if (rows_[i].seconds)
            rows_[i].seconds->setText(std::format("{}", second));
    }

    UpdateVisibility();
}
