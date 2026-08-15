#pragma once

#include <functional>
#include <memory>
#include <string>

#include "UIElement.h"
#include "../graphics/Color.h"
#include "../graphics/UIRenderer.h"

class Font;
class Texture;

// -----------------------------------------------------------------------------
// UIButton
//
// The design's ".sv-btn" primitive, including its variants.
//
// Each variant carries its own gradient, border and hover glow straight from
// the style guide. State is driven by the real mouse events routed through
// UIManager (enter/leave/down/up), and the pressed state nudges the button down
// by a pixel exactly as the CSS does.
// -----------------------------------------------------------------------------
class UIButton : public UIElement
{
public:
    enum class Variant
    {
        Primary,   // Crystal blue
        Purple,    // Secondary / create account
        Success,   // Confirm
        Danger,    // Destructive
        Ghost      // Transparent, used for filter pills and text links
    };

    enum class ButtonState
    {
        Normal,
        Hover,
        Pressed,
        Disabled
    };

    UIButton();
    ~UIButton() override = default;

    // --- Content ---------------------------------------------------------
    void setText(const std::string& text) { text_ = text; }
    const std::string& getText() const { return text_; }

    void setFont(Font* font) { font_ = font; }
    Font* getFont() const { return font_; }

    void setLetterSpacing(float spacing) { letterSpacing_ = spacing; }

    // Optional icon drawn to the left of the label.
    void setIcon(std::shared_ptr<Texture> icon, float size);

    // Reserves space at the edges of the button so the label centres within
    // what is left. Use this when a UIIcon child is placed inside the button,
    // otherwise the centred label runs underneath it.
    void setLabelInset(float left, float right);

    // --- Appearance -------------------------------------------------------
    void setVariant(Variant variant);
    Variant getVariant() const { return variant_; }

    void setBorderRadius(float radius) { radius_ = radius; }
    void setTextColor(const Color& color) { textColor_ = color; }

    // Overrides for the cases where the design deviates from a stock variant
    // (the "selected" filter pill, for example).
    void setNormalColors(const Color& top, const Color& bottom, const Color& border);
    void setHoverColors(const Color& top, const Color& bottom, const Color& border);
    void setGlowColor(const Color& color) { glowColor_ = color; }

    // --- Behaviour --------------------------------------------------------
    void setOnClick(std::function<void()> callback) { onClick_ = std::move(callback); }
    void setOnClickCallback(std::function<void()> callback) { setOnClick(std::move(callback)); }

    // Fires the callback as if the button had been clicked (used for the
    // keyboard path: Enter on a focused button).
    void triggerClick();

    void setEnabled(bool enabled) override;

    ButtonState getState() const;

    // --- UIElement --------------------------------------------------------
    bool wantsInput() const override { return true; }
    bool isFocusable() const override { return enabled_; }

    void onMouseEnter() override;
    void onMouseLeave() override;
    void onMouseDown(float x, float y) override;
    void onMouseUp(float x, float y) override;
    void onClick() override;
    void onKeyDown(int key, bool ctrl, bool shift) override;
    void onFocusGained() override;
    void onFocusLost() override;

protected:
    void renderSelf(UIRenderer& renderer) const override;

private:
    void applyVariant(Variant variant);

    std::string text_;
    Font*       font_ = nullptr;
    float       letterSpacing_ = 0.0f;

    std::shared_ptr<Texture> icon_;
    float                    iconSize_ = 0.0f;

    float labelInsetLeft_  = 0.0f;
    float labelInsetRight_ = 0.0f;

    Variant variant_ = Variant::Primary;
    float   radius_  = 0.0f;

    Color normalTop_,  normalBottom_,  normalBorder_;
    Color hoverTop_,   hoverBottom_,   hoverBorder_;
    Color pressedTop_, pressedBottom_;
    Color disabledFill_, disabledBorder_;

    Color textColor_{1.0f, 1.0f, 1.0f, 1.0f};
    Color glowColor_{0.0f, 0.0f, 0.0f, 0.0f};

    bool hovered_ = false;
    bool pressed_ = false;
    bool focused_ = false;

    std::function<void()> onClick_;
};
