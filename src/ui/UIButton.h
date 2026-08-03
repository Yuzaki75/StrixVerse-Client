#pragma once

#include "UIElement.h"
#include "../graphics/Color.h"

/**
 * Button UI element with multiple states
 */
class UIButton : public UIElement
{
public:
    enum class ButtonState
    {
        Normal,
        Hover,
        Pressed,
        Disabled
    };

    UIButton();
    ~UIButton() override = default;

    // Appearance
    void setNormalColors(const Color &bgColor, const Color &textColor);
    void setHoverColors(const Color &bgColor, const Color &textColor);
    void setPressedColors(const Color &bgColor, const Color &textColor);
    void setDisabledColors(const Color &bgColor, const Color &textColor);

    // Content
    void setText(const std::string &text);
    const std::string &getText() const { return text_; }

    void setIcon(unsigned int textureID);
    unsigned int getIcon() const { return iconTexture_; }

    void setNormalTexture(unsigned int textureID);
    void setHoverTexture(unsigned int textureID);
    void setPressedTexture(unsigned int textureID);
    void setDisabledTexture(unsigned int textureID);
    void setIconTexture(unsigned int textureID);

    // Behavior
    void setOnClick(std::function<void()> callback);
    void setOnClickCallback(std::function<void()> callback) { setOnClick(callback); }
    void triggerClick();
    void setEnabled(bool enabled);

    // Convenience methods for setting single-state colors
    void setBackgroundColor(const Color &color) { normalBgColor_ = color; }
    void setTextColor(const Color &color) { normalTextColor_ = color; }
    void setNormalColor(const Color &color);
    void setHoverColor(const Color &color);
    void setPressedColor(const Color &color);
    void setDisabledColor(const Color &color);
    void setFontSize(float) { /* No-op: font size handled by rendering */ }

    // UIElement overrides
    void update(float deltaTime) override;
    void renderSelf(SpriteBatch &spriteBatch, Font &font) const override;

protected:
    ButtonState getCurrentState() const;

private:
    std::string text_;
    unsigned int iconTexture_; // Texture ID for icon

    // Colors for each state
    Color normalBgColor_;
    Color normalTextColor_;
    Color hoverBgColor_;
    Color hoverTextColor_;
    Color pressedBgColor_;
    Color pressedTextColor_;
    Color disabledBgColor_;
    Color disabledTextColor_;

    std::function<void()> onClickCallback_;
    ButtonState currentState_;
    bool mousePressed_; // Track mouse button state for press detection
};