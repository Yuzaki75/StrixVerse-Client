#include "UIPanel.h"

#include "UITheme.h"
#include "../graphics/Texture.h"

UIPanel::UIPanel()
{
    style_ = UIQuadStyle::Solid(UITheme::Panel, UITheme::RadiusPanel);
    style_.WithBorder(UITheme::PanelBorder, UITheme::BorderThin);
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

    renderer.DrawRect(x, y, width_, height_, style_);

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
