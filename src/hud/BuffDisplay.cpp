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

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    Font* PanelFont(Engine* engine, UIFonts::Typeface face, unsigned int size)
    {
        UIFonts* fonts = engine ? engine->GetUIFonts() : nullptr;
        return fonts ? fonts->Get(face, size) : nullptr;
    }

    // Geometry, in style-guide pixels. A narrow column pinned under the HUD's
    // health and level panels at the top-left; it must never reach far enough
    // right to meet the chat entry field.
    constexpr float kPanelX       = 12.0f;
    constexpr float kPanelY       = 64.0f;
    constexpr float kRowWidth     = 130.0f;
    constexpr float kPadding      = 6.0f;
    constexpr float kNameHeight   = 11.0f;
    constexpr float kBarHeight    = 4.0f;
    constexpr float kBarGap       = 2.0f;
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
    root_->setBackgroundColor(UITheme::Hex(0x000000, 0.0f));

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
        row.name->setFont(PanelFont(engine_, UIFonts::Typeface::Body,
                                    UITheme::Display::Micro));
        row.name->setTextColor(UITheme::Subtext);
        row.name->setText(buff.name.empty() ? buff.id : buff.name);
        row.name->setPosition(0.0f, 0.0f);
        row.name->setSize(width, S(kNameHeight - 1.0f));
        row.backing->addChild(row.name);

        row.barBackground = std::make_shared<UIPanel>();
        row.barBackground->setSize(width, S(kBarHeight));
        row.barBackground->setPosition(0.0f, S(kNameHeight));
        row.barBackground->setBackgroundColor(UITheme::Hex(0x141826, 0.85f));
        row.barBackground->setBorderRadius(UITheme::RadiusBar);
        row.backing->addChild(row.barBackground);

        // The fill carries the Aether palette as a gradient from crystal blue
        // down to violet, so a draining bar visibly cools as it empties.
        row.barFill = std::make_shared<UIPanel>();
        row.barFill->setSize(width, S(kBarHeight));
        row.barFill->setPosition(0.0f, S(kNameHeight));
        row.barFill->setBackgroundGradient(UITheme::Accent, UITheme::Secondary);
        row.barFill->setBorderRadius(UITheme::RadiusBar);
        row.backing->addChild(row.barFill);

        root_->addChild(row.backing);
        rows_.push_back(std::move(row));

        y += rowHeight + S(3.0f);
    }

    // The column grows downward with its content and shrinks back when the
    // last buff expires.
    root_->setSize(S(kRowWidth),
                   std::max(y - S(3.0f), S(kPadding)) + S(kPadding));

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

void BuffDisplay::Update(float dt)
{
    if (!built_ || dt <= 0.0f || buffs_.empty())
        return;

    // Count every bar down between packets. A bar that reaches zero stops
    // moving but keeps its row until the next SetBuffs removes it: the server
    // decides when a buff ends, so the client never drops one on its own guess
    // about network latency.
    for (auto& buff : buffs_)
        buff.remainingSeconds = std::max(buff.remainingSeconds - dt, 0.0f);

    ApplyBars();
    UpdateVisibility();
}
