#include "UIProgressBar.h"

#include "UITheme.h"
#include "../graphics/Font.h"
#include "../graphics/UIRenderer.h"

#include <algorithm>

UIProgressBar::UIProgressBar()
{
    trackColor_  = Color(0.0f, 0.0f, 0.0f, 0.45f);
    trackBorder_ = UITheme::WithAlpha(UITheme::Border, 0.25f);
    fillStart_   = UITheme::Primary;
    fillEnd_     = UITheme::Accent;
    glowColor_   = UITheme::WithAlpha(UITheme::Accent, 0.65f);
    labelColor_  = UITheme::Text;

    setSize(UITheme::Scaled(360.0f), UITheme::Scaled(14.0f));
    radius_ = UITheme::Scaled(7.0f);
}

void UIProgressBar::setProgress(float progress)
{
    progress_ = std::clamp(progress, 0.0f, 1.0f);
}

void UIProgressBar::setFillColor(const Color& color)
{
    fillStart_ = color;
    fillEnd_   = color;
}

void UIProgressBar::setFillGradient(const Color& start, const Color& end)
{
    fillStart_ = start;
    fillEnd_   = end;
}

void UIProgressBar::renderSelf(UIRenderer& renderer) const
{
    const float x = getAbsoluteX();
    const float y = getAbsoluteY();

    // Track.
    UIQuadStyle track = UIQuadStyle::Solid(trackColor_, radius_);
    track.WithBorder(trackBorder_, UITheme::BorderThin);
    renderer.DrawRect(x, y, width_, height_, track);

    // Fill. The gradient in the design runs left to right, but the shader's
    // gradient axis is vertical, so the horizontal ramp is approximated by
    // blending the two stops by how much of the bar is filled.
    const float fillWidth = width_ * progress_;

    if (fillWidth > 0.0f)
    {
        const Color blended(
            fillStart_.r + (fillEnd_.r - fillStart_.r) * progress_,
            fillStart_.g + (fillEnd_.g - fillStart_.g) * progress_,
            fillStart_.b + (fillEnd_.b - fillStart_.b) * progress_,
            fillStart_.a);

        UIQuadStyle fill = UIQuadStyle::Solid(blended, radius_);

        if (glowColor_.a > 0.0f)
            fill.WithGlow(glowColor_, UITheme::Scaled(6.0f));

        renderer.DrawRect(x, y, fillWidth, height_, fill);
    }

    if (!label_.empty() && font_ && font_->IsLoaded())
    {
        const float textY = y + (height_ - font_->GetLineHeight()) * 0.5f;
        renderer.DrawTextAligned(*font_, label_, x, textY, width_,
                                 TextAlign::Center, labelColor_);
    }
}
