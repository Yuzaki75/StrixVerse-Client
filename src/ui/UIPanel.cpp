#include "UIPanel.h"

#include "UITheme.h"
#include "../graphics/Texture.h"

namespace
{
    // Hover blend strength fallback when the tint carries no alpha hint.
    constexpr float kHoverBlend = 0.35f;
}

UIPanel::UIPanel()
{
    style_ = UIQuadStyle::Solid(UITheme::Panel, UITheme::RadiusPanel);
    style_.WithBorder(UITheme::PanelBorder, UITheme::BorderThin);
}

void UIPanel::setHoverable(bool hoverable)
{
    hoverable_ = hoverable;

    if (!hoverable)
        hovered_ = false;
}

void UIPanel::setEnabled(bool enabled)
{
    UIElement::setEnabled(enabled);

    // Drop the transient state so a panel disabled under the mouse never
    // keeps a stale highlight, matching UIButton.
    if (!enabled)
        hovered_ = false;
}

void UIPanel::onMouseEnter()
{
    if (enabled_ && hoverable_)
        hovered_ = true;
}

void UIPanel::onMouseLeave()
{
    hovered_ = false;
}

void UIPanel::setBackgroundColor(const Color& color)
{
    style_.fillTop    = color;
    style_.fillBottom = color;
}

void UIPanel::setBackgroundColor(float r, float g, float b, float a)
{
    setBackgroundColor(Color(r, g, b, a));
}

void UIPanel::setBackgroundGradient(const Color& top, const Color& bottom)
{
    style_.fillTop    = top;
    style_.fillBottom = bottom;
}

void UIPanel::setBorderColor(const Color& color)
{
    style_.border = color;
}

void UIPanel::setBorderColor(float r, float g, float b, float a)
{
    style_.border = Color(r, g, b, a);
}

void UIPanel::setBorderWidth(float width)
{
    style_.borderWidth = width;
}

void UIPanel::setBorder(const Color& color, float width)
{
    style_.border      = color;
    style_.borderWidth = width;
}

void UIPanel::setBorderRadius(float radius)
{
    style_.radius = radius;
}

void UIPanel::setGlow(const Color& color, float size)
{
    style_.glow     = color;
    style_.glowSize = size;
}

void UIPanel::setBackgroundImage(std::shared_ptr<Texture> texture)
{
    backgroundImage_ = std::move(texture);
}

void UIPanel::renderSelf(UIRenderer& renderer) const
{
    const float x = getAbsoluteX();
    const float y = getAbsoluteY();

    UIQuadStyle drawStyle = style_;

    // Hover tint: blend the fill toward the tint while highlighted. The
    // tint's alpha is the blend strength; a fully opaque tint falls back to
    // a gentle blend so a default-constructed colour cannot wash the fill out.
    if (hoverable_ && hovered_ && enabled_)
    {
        const float t = hoverTint_.a > 0.0f && hoverTint_.a < 1.0f ? hoverTint_.a : kHoverBlend;

        drawStyle.fillTop    = Color::Lerp(style_.fillTop, hoverTint_, t);
        drawStyle.fillBottom = Color::Lerp(style_.fillBottom, hoverTint_, t);
    }

    renderer.DrawRect(x, y, width_, height_, drawStyle);

    if (backgroundImage_)
    {
        renderer.DrawTexture(*backgroundImage_, x, y, width_, height_,
                             imageTint_, style_.radius);
    }
}

void UIPanel::beginChildren(UIRenderer& renderer) const
{
    if (clipsChildren_)
        renderer.PushClip(getAbsoluteX(), getAbsoluteY(), width_, height_);
}

void UIPanel::endChildren(UIRenderer& renderer) const
{
    if (clipsChildren_)
        renderer.PopClip();
}
