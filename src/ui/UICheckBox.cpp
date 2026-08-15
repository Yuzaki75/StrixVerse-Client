#include "UICheckBox.h"

#include "UILabel.h"
#include "UITheme.h"
#include "../graphics/Font.h"
#include "../graphics/UIRenderer.h"

#include <algorithm>

namespace
{
    constexpr float kLabelGap = UITheme::Scaled(8.0f);
}

UICheckBox::UICheckBox()
{
    labelColor_  = UITheme::Subtext;
    accentColor_ = UITheme::Primary;
    boxSize_     = UITheme::Scaled(14.0f);

    setSize(UITheme::Scaled(180.0f), UITheme::Scaled(20.0f));
}

void UICheckBox::setLabel(const std::string& label)
{
    label_ = label;
    labelLines_ = {label};
}

void UICheckBox::wrapLabel()
{
    if (!font_ || label_.empty())
        return;

    const float available = width_ - boxSize_ - kLabelGap;
    if (available <= 0.0f)
        return;

    labelLines_ = UILabel::WrapText(*font_, label_, available);
}

void UICheckBox::setChecked(bool checked)
{
    if (checked_ == checked)
        return;

    checked_ = checked;

    if (onChanged_)
        onChanged_(checked_);
}

void UICheckBox::onClick()
{
    if (enabled_)
        setChecked(!checked_);
}

void UICheckBox::onKeyDown(int key, bool, bool)
{
    // Enter toggles the focused checkbox, matching the button behaviour.
    if (key == UIKey::Enter)
        onClick();
}

void UICheckBox::renderSelf(UIRenderer& renderer) const
{
    const float x = getAbsoluteX();
    const float y = getAbsoluteY();

    const float boxY = y + (height_ - boxSize_) * 0.5f;

    UIQuadStyle box = UIQuadStyle::Solid(
        checked_ ? accentColor_ : UITheme::InputBackground,
        UITheme::Scaled(3.0f));

    const Color borderColor = checked_
                                  ? accentColor_
                                  : (hovered_ || focused_ ? UITheme::WithAlpha(UITheme::Accent, 0.7f)
                                                          : UITheme::InputBorder);

    box.WithBorder(borderColor, UITheme::BorderThin);

    if (checked_ || focused_)
        box.WithGlow(UITheme::WithAlpha(accentColor_, 0.45f), UITheme::Scaled(5.0f));

    renderer.DrawRect(x, boxY, boxSize_, boxSize_, box);

    // Check mark: two strokes rather than a glyph, so it renders identically
    // whichever typeface is in use.
    if (checked_)
    {
        const float inset = boxSize_ * 0.26f;
        const float thick = std::max(UITheme::Scaled(1.5f), boxSize_ * 0.14f);

        const UIQuadStyle tick = UIQuadStyle::Solid(UITheme::Text, thick * 0.5f);

        // Short arm, then long arm, meeting near the lower-left third.
        renderer.DrawRect(x + inset,
                          boxY + boxSize_ * 0.52f,
                          boxSize_ * 0.22f, thick, tick);

        renderer.DrawRect(x + inset + boxSize_ * 0.16f,
                          boxY + boxSize_ * 0.34f,
                          thick, boxSize_ * 0.34f, tick);
    }

    if (!labelLines_.empty() && font_ && font_->IsLoaded())
    {
        const float labelX     = x + boxSize_ + kLabelGap;
        const float lineHeight = font_->GetLineHeight();
        const float blockHeight = lineHeight * static_cast<float>(labelLines_.size());

        // Centre the whole block, so a wrapped label stays aligned with the box.
        float labelY = y + (height_ - blockHeight) * 0.5f;

        Color color = labelColor_;
        if (hovered_)
            color = UITheme::Text;
        if (!enabled_)
            color.a *= 0.5f;

        for (const std::string& line : labelLines_)
        {
            renderer.DrawText(*font_, line, labelX, labelY, color);
            labelY += lineHeight;
        }
    }
}
