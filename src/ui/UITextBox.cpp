#include "UITextBox.h"
#include "../graphics/SpriteBatch.h"
#include "../graphics/Font.h"
#include "../core/AssetManager.h"
#include "../graphics/Shader.h"
#include "../graphics/Texture.h"
#include "../core/ServiceLocator.h"
#include "../core/Logger.h"

UITextBox::UITextBox()
    : fontSize_(16.0f)
    , textColor_(1.0f, 1.0f, 1.0f, 1.0f)  // White text
    , backgroundColor_(0.2f, 0.2f, 0.2f, 0.8f)  // Semi-transparent dark background
    , borderColor_(0.5f, 0.5f, 0.5f, 1.0f)  // Gray border
    , borderWidth_(1.0f)
    , passwordMode_(false)
    , maxLength_(-1)  // Unlimited by default
    , hasFocus_(false)
    , cursorTimer_(0.0f) {
    // Set reasonable default size
    setSize(200.0f, 30.0f);
}

void UITextBox::setText(const std::string& text) {
    if (maxLength_ > 0 && text.length() > static_cast<size_t>(maxLength_)) {
        text_ = text.substr(0, maxLength_);
    } else {
        text_ = text;
    }

    if (onTextChanged_) {
        onTextChanged_(text_);
    }
}

void UITextBox::setPlaceholderText(const std::string& text) {
    placeholderText_ = text;
}

void UITextBox::setFontSize(float size) {
    fontSize_ = size;
}

void UITextBox::setTextColor(const Color& color) {
    textColor_ = color;
}

void UITextBox::setBackgroundColor(const Color& color) {
    backgroundColor_ = color;
}

void UITextBox::setBorderColor(const Color& color) {
    borderColor_ = color;
}

void UITextBox::setBorderWidth(float width) {
    borderWidth_ = width;
}

void UITextBox::setPasswordMode(bool enabled) {
    passwordMode_ = enabled;
}

void UITextBox::setMaxLength(int length) {
    maxLength_ = length;
    // Ensure current text doesn't exceed new max length
    if (maxLength_ > 0 && text_.length() > static_cast<size_t>(maxLength_)) {
        text_ = text_.substr(0, maxLength_);
        if (onTextChanged_) {
            onTextChanged_(text_);
        }
    }
}

void UITextBox::setOnTextChanged(std::function<void(const std::string&)> callback) {
    onTextChanged_ = callback;
}

void UITextBox::setOnEnterPressed(std::function<void()> callback) {
    onEnterPressed_ = callback;
}

void UITextBox::update(float deltaTime) {
    // Update focus state would be handled by UIManager
    UIElement::update(deltaTime);

    // Update cursor timer for blinking effect
    if (hasFocus_) {
        cursorTimer_ += deltaTime;
        if (cursorTimer_ > 0.5f) { // Blink every 0.5 seconds
            cursorTimer_ = 0.0f;
        }
    }
}

void UITextBox::renderSelf(SpriteBatch& spriteBatch, Font& font) const {
    // Calculate render position based on anchor
    float renderX, renderY;
    calculateRenderPosition(renderX, renderY);

    // Get AssetManager for textures and shaders
    auto assetManager = ServiceLocator::Get<AssetManager>();
    if (!assetManager) {
        Logger::Error("UITextBox: AssetManager not available");
        return;
    }

    // Get a white texture for drawing rectangles
    // In a real engine, we'd have a default white texture
    std::shared_ptr<Texture> whiteTexture = assetManager->GetTexture(0); // Assuming 0 is white texture
    if (!whiteTexture) {
        // Try to load a white pixel texture
        whiteTexture = assetManager->LoadTexture("textures/white.png");
    }
    if (!whiteTexture) {
        Logger::Error("UITextBox: Failed to get or load white texture");
        return;
    }

    // Get shader for text rendering
    std::shared_ptr<Shader> shader = assetManager->GetShader("default.vert", "default.frag");
    if (!shader) {
        shader = assetManager->LoadShader("shaders/default.vert", "shaders/default.frag");
    }
    if (!shader) {
        Logger::Error("UITextBox: Failed to get or create shader for text rendering");
        // We can still draw the background and border without text
    }

    // Draw background
    spriteBatch.Draw(*whiteTexture, renderX, renderY, getWidth(), getHeight(),
                    backgroundColor_.r, backgroundColor_.g, backgroundColor_.b, backgroundColor_.a);

    // Draw border (by drawing four rectangles for each side)
    // Top border
    spriteBatch.Draw(*whiteTexture, renderX, renderY, getWidth(), borderWidth_,
                    borderColor_.r, borderColor_.g, borderColor_.b, borderColor_.a);
    // Bottom border
    spriteBatch.Draw(*whiteTexture, renderX, renderY + getHeight() - borderWidth_, getWidth(), borderWidth_,
                    borderColor_.r, borderColor_.g, borderColor_.b, borderColor_.a);
    // Left border
    spriteBatch.Draw(*whiteTexture, renderX, renderY, borderWidth_, getHeight(),
                    borderColor_.r, borderColor_.g, borderColor_.b, borderColor_.a);
    // Right border
    spriteBatch.Draw(*whiteTexture, renderX + getWidth() - borderWidth_, renderY, borderWidth_, getHeight(),
                    borderColor_.r, borderColor_.g, borderColor_.b, borderColor_.a);

    // Determine what text to display
    std::string displayText;
    if (text_.empty() && !hasFocus_) {
        displayText = placeholderText_;
    } else if (passwordMode_) {
        displayText = std::string(text_.length(), '*');
    } else {
        displayText = text_;
    }

    // Calculate text position (with padding)
    float textX = renderX + 5.0f;  // 5px padding from left
    float textY = renderY + (getHeight() - font.MeasureText(displayText, 1.0f).y) / 2.0f;

    // Draw text with appropriate color
    Color textColorToUse = hasFocus_ ? textColor_ : Color(textColor_.r, textColor_.g, textColor_.b, textColor_.a * 0.7f);
    if (shader) {
        font.DrawText(*shader, displayText, textX, textY, 1.0f);
    }

    // Draw cursor if focused
    if (hasFocus_ && shader) {
        // Only draw cursor if in the visible part of the blink cycle
        if (cursorTimer_ < 0.25f) {
            // Calculate cursor position
            float cursorX = textX + font.MeasureText(displayText, 1.0f).x;
            float cursorY = textY;
            float cursorWidth = 2.0f;
            float cursorHeight = font.MeasureText("|", 1.0f).y; // Approximate height

            // Draw cursor as a thin rectangle
            spriteBatch.Draw(*whiteTexture, cursorX, cursorY, cursorWidth, cursorHeight,
                            textColorToUse.r, textColorToUse.g, textColorToUse.b, textColorToUse.a);
        }
    }
}

void UITextBox::handleCharacterInput(char c) {
    if (maxLength_ > 0 && text_.length() >= static_cast<size_t>(maxLength_)) {
        return; // Don't add more characters if at max length
    }

    text_ += c;

    if (onTextChanged_) {
        onTextChanged_(text_);
    }
}

void UITextBox::handleBackspace() {
    if (!text_.empty()) {
        text_.pop_back();

        if (onTextChanged_) {
            onTextChanged_(text_);
        }
    }
}

void UITextBox::handleEnter() {
    if (onEnterPressed_) {
        onEnterPressed_();
    }
}

void UITextBox::onFocusGained() {
    hasFocus_ = true;
}

void UITextBox::onFocusLost() {
    hasFocus_ = false;
}