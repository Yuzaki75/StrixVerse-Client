#pragma once

#include "UIElement.h"
#include "../graphics/Color.h"

/**
 * Text input UI element
 */
class UITextBox : public UIElement {
public:
    UITextBox();
    ~UITextBox() override = default;

    // Text properties
    void setText(const std::string& text);
    const std::string& getText() const { return text_; }

    void setPlaceholderText(const std::string& text);
    const std::string& getPlaceholderText() const { return placeholderText_; }

    void setFontSize(float size);
    float getFontSize() const { return fontSize_; }

    void setTextColor(const Color& color);
    const Color& getTextColor() const { return textColor_; }

    void setBackgroundColor(const Color& color);
    const Color& getBackgroundColor() const { return backgroundColor_; }

    void setBorderColor(const Color& color);
    const Color& getBorderColor() const { return borderColor_; }

    void setBorderWidth(float width);
    float getBorderWidth() const { return borderWidth_; }

    // Behavior
    void setPasswordMode(bool enabled);
    bool isPasswordMode() const { return passwordMode_; }

    void setMaxLength(int length);
    int getMaxLength() const { return maxLength_; }

    void setOnTextChanged(std::function<void(const std::string&)> callback);
    void setOnEnterPressed(std::function<void()> callback);

    // UIElement overrides
    void update(float deltaTime) override;
    void renderSelf(SpriteBatch& spriteBatch, Font& font) const override;
    void onFocusGained() override;
    void onFocusLost() override;

protected:
    void handleCharacterInput(char c);
    void handleBackspace();
    void handleEnter();

private:
    std::string text_;
    std::string placeholderText_;
    float fontSize_;
    Color textColor_;
    Color backgroundColor_;
    Color borderColor_;
    float borderWidth_;

    bool passwordMode_;
    int maxLength_; // -1 for unlimited

    std::function<void(const std::string&)> onTextChanged_;
    std::function<void()> onEnterPressed_;

    bool hasFocus_; // Track if this textbox has focus
    float cursorTimer_; // For cursor blinking
};