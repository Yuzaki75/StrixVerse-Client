#include "UIButton.h"

#include "UITheme.h"
#include "../graphics/Font.h"
#include "../graphics/Texture.h"

namespace
{
    // The pressed state darkens the variant's gradient rather than defining a
    // fourth palette, matching ".sv-btn:active" in the style guide.
    Color Darken(const Color& color, float amount)
    {
        return Color(color.r * amount, color.g * amount, color.b * amount, color.a);
    }

    constexpr float kPressOffset = UITheme::Scaled(1.0f);
    constexpr float kIconGap     = UITheme::Scaled(6.0f);
}

UIButton::UIButton()
{
    radius_ = UITheme::RadiusButton;
    setSize(UITheme::Scaled(100.0f), UITheme::Scaled(30.0f));
    applyVariant(Variant::Primary);
}

void UIButton::applyVariant(Variant variant)
{
    variant_ = variant;

    disabledFill_   = UITheme::DisabledFill;
    disabledBorder_ = UITheme::DisabledBorder;

    switch (variant)
    {
    case Variant::Primary:
        normalTop_    = UITheme::ButtonTop;
        normalBottom_ = UITheme::ButtonBottom;
        normalBorder_ = UITheme::ButtonBorder;
        hoverTop_     = UITheme::ButtonHoverTop;
        hoverBottom_  = UITheme::ButtonHoverBot;
        hoverBorder_  = UITheme::WithAlpha(UITheme::Accent, 0.80f);
        glowColor_    = UITheme::WithAlpha(UITheme::Accent, 0.50f);
        break;

    case Variant::Purple:
        normalTop_    = UITheme::PurpleTop;
        normalBottom_ = UITheme::PurpleBottom;
        normalBorder_ = UITheme::PurpleBorder;
        hoverTop_     = UITheme::PurpleTop;
        hoverBottom_  = UITheme::PurpleBottom;
        hoverBorder_  = UITheme::WithAlpha(UITheme::Secondary, 0.95f);
        glowColor_    = UITheme::WithAlpha(UITheme::Secondary, 0.55f);
        break;

    case Variant::Success:
        normalTop_    = UITheme::SuccessTop;
        normalBottom_ = UITheme::SuccessBottom;
        normalBorder_ = UITheme::SuccessBorder;
        hoverTop_     = UITheme::SuccessTop;
        hoverBottom_  = UITheme::SuccessBottom;
        hoverBorder_  = UITheme::WithAlpha(UITheme::Success, 0.95f);
        glowColor_    = UITheme::WithAlpha(UITheme::Success, 0.55f);
        break;

    case Variant::Danger:
        normalTop_    = UITheme::DangerTop;
        normalBottom_ = UITheme::DangerBottom;
        normalBorder_ = UITheme::DangerBorder;
        hoverTop_     = UITheme::DangerTop;
        hoverBottom_  = UITheme::DangerBottom;
        hoverBorder_  = UITheme::WithAlpha(UITheme::Danger, 0.95f);
        glowColor_    = UITheme::WithAlpha(UITheme::Danger, 0.55f);
        break;

    case Variant::Ghost:
        normalTop_    = Color(0.0f, 0.0f, 0.0f, 0.0f);
        normalBottom_ = Color(0.0f, 0.0f, 0.0f, 0.0f);
        normalBorder_ = UITheme::SubtleBorder;
        hoverTop_     = UITheme::WithAlpha(UITheme::Primary, 0.10f);
        hoverBottom_  = UITheme::WithAlpha(UITheme::Primary, 0.10f);
        hoverBorder_  = UITheme::WithAlpha(UITheme::Primary, 0.50f);
        glowColor_    = Color(0.0f, 0.0f, 0.0f, 0.0f);
        textColor_    = UITheme::Subtext;
        break;
    }

    pressedTop_    = Darken(normalTop_, 0.82f);
    pressedBottom_ = Darken(normalBottom_, 0.82f);
}

void UIButton::setVariant(Variant variant)
{
    applyVariant(variant);
}

void UIButton::setNormalColors(const Color& top, const Color& bottom, const Color& border)
{
    normalTop_     = top;
    normalBottom_  = bottom;
    normalBorder_  = border;
    pressedTop_    = Darken(top, 0.82f);
    pressedBottom_ = Darken(bottom, 0.82f);
}

void UIButton::setHoverColors(const Color& top, const Color& bottom, const Color& border)
{
    hoverTop_    = top;
    hoverBottom_ = bottom;
    hoverBorder_ = border;
}

void UIButton::setIcon(std::shared_ptr<Texture> icon, float size)
{
    icon_     = std::move(icon);
    iconSize_ = size;
}

void UIButton::setLabelInset(float left, float right)
{
    labelInsetLeft_  = left;
    labelInsetRight_ = right;
}

void UIButton::setEnabled(bool enabled)
{
    UIElement::setEnabled(enabled);

    if (!enabled)
    {
        // Drop any transient state so a disabled button never keeps a stale
        // hover highlight.
        hovered_ = false;
        pressed_ = false;
        focused_ = false;
    }
}

UIButton::ButtonState UIButton::getState() const
{
    if (!enabled_)
        return ButtonState::Disabled;
    if (pressed_)
        return ButtonState::Pressed;
    if (hovered_ || focused_)
        return ButtonState::Hover;

    return ButtonState::Normal;
}

void UIButton::onMouseEnter()
{
    if (enabled_)
        hovered_ = true;
}

void UIButton::onMouseLeave()
{
    hovered_ = false;
    pressed_ = false;
}

void UIButton::onMouseDown(float, float)
{
    if (enabled_)
        pressed_ = true;
}

void UIButton::onMouseUp(float, float)
{
    pressed_ = false;
}

void UIButton::onClick()
{
    triggerClick();
}

void UIButton::onKeyDown(int key, bool, bool)
{
    // Keyboard activation for the focused button.
    if (key == UIKey::Enter)
        triggerClick();
}

void UIButton::onFocusGained()
{
    focused_ = true;
}

void UIButton::onFocusLost()
{
    focused_ = false;
    pressed_ = false;
}

void UIButton::triggerClick()
{
    if (enabled_ && onClick_)
        onClick_();
}

void UIButton::renderSelf(UIRenderer& renderer) const
{
    const ButtonState state = getState();

    UIQuadStyle style;
    style.radius      = radius_;
    style.borderWidth = UITheme::BorderThin;

    switch (state)
    {
    case ButtonState::Normal:
        style.fillTop    = normalTop_;
        style.fillBottom = normalBottom_;
        style.border     = normalBorder_;
        if (glowColor_.a > 0.0f)
            style.WithGlow(UITheme::WithAlpha(glowColor_, glowColor_.a * 0.35f), UITheme::Scaled(8.0f));
        break;

    case ButtonState::Hover:
        style.fillTop    = hoverTop_;
        style.fillBottom = hoverBottom_;
        style.border     = hoverBorder_;
        if (glowColor_.a > 0.0f)
            style.WithGlow(glowColor_, UITheme::Scaled(16.0f));
        break;

    case ButtonState::Pressed:
        style.fillTop    = pressedTop_;
        style.fillBottom = pressedBottom_;
        style.border     = hoverBorder_;
        if (glowColor_.a > 0.0f)
            style.WithGlow(UITheme::WithAlpha(glowColor_, glowColor_.a * 0.4f), UITheme::Scaled(6.0f));
        break;

    case ButtonState::Disabled:
        style.fillTop    = disabledFill_;
        style.fillBottom = disabledFill_;
        style.border     = disabledBorder_;
        break;
    }

    const float x = getAbsoluteX();
    // Pressed buttons sink by a pixel, as ".sv-btn:active" does.
    const float y = getAbsoluteY() + (state == ButtonState::Pressed ? kPressOffset : 0.0f);

    renderer.DrawRect(x, y, width_, height_, style);

    Color labelColor = textColor_;
    if (state == ButtonState::Disabled)
        labelColor.a *= 0.40f;

    // Icon and label are centred together as one group.
    float contentWidth = 0.0f;
    float textWidth    = 0.0f;

    if (font_ && font_->IsLoaded() && !text_.empty())
    {
        textWidth = font_->MeasureWidth(text_, letterSpacing_);
        contentWidth += textWidth;
    }

    if (icon_ && iconSize_ > 0.0f)
        contentWidth += iconSize_ + (textWidth > 0.0f ? kIconGap : 0.0f);

    // Centre within the area left over after any reserved insets, so a child
    // icon placed at one edge does not sit underneath the label.
    const float contentAreaX     = x + labelInsetLeft_;
    const float contentAreaWidth = width_ - labelInsetLeft_ - labelInsetRight_;

    float cursorX = contentAreaX + (contentAreaWidth - contentWidth) * 0.5f;

    if (icon_ && iconSize_ > 0.0f)
    {
        const float iconY = y + (height_ - iconSize_) * 0.5f;
        renderer.DrawTexture(*icon_, cursorX, iconY, iconSize_, iconSize_, labelColor);
        cursorX += iconSize_ + (textWidth > 0.0f ? kIconGap : 0.0f);
    }

    if (textWidth > 0.0f && font_)
    {
        const float textY = y + (height_ - font_->GetLineHeight()) * 0.5f;

        // The style guide gives every button label a hard drop shadow.
        renderer.DrawText(*font_, text_,
                          cursorX + UITheme::Scaled(1.0f), textY + UITheme::Scaled(2.0f),
                          Color(0.0f, 0.0f, 0.0f, 0.8f * labelColor.a), letterSpacing_);

        renderer.DrawText(*font_, text_, cursorX, textY, labelColor, letterSpacing_);
    }
}
