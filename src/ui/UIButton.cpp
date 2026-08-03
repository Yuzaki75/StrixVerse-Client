#include "UIButton.h"
#include "../graphics/SpriteBatch.h"
#include "../graphics/Font.h"
#include "../core/AssetManager.h"
#include "../graphics/Shader.h"
#include "../graphics/Texture.h"
#include "../core/ServiceLocator.h"
#include "../core/Logger.h"

UIButton::UIButton()
    : normalBgColor_(0.2f, 0.3f, 0.5f, 1.0f) // Default blue background
      ,
      hoverBgColor_(0.3f, 0.4f, 0.6f, 1.0f), pressedBgColor_(0.1f, 0.2f, 0.4f, 1.0f), disabledBgColor_(0.5f, 0.5f, 0.5f, 0.5f), normalTextColor_(1.0f, 1.0f, 1.0f, 1.0f) // White text
      ,
      hoverTextColor_(1.0f, 1.0f, 1.0f, 1.0f), pressedTextColor_(1.0f, 1.0f, 1.0f, 1.0f), disabledTextColor_(1.0f, 1.0f, 1.0f, 0.5f), iconTexture_(0), text_(""), currentState_(ButtonState::Normal)
{
    // Set default size
    setSize(100.0f, 30.0f);
}

void UIButton::setNormalColors(const Color &bgColor, const Color &textColor)
{
    normalBgColor_ = bgColor;
    normalTextColor_ = textColor;
}

void UIButton::setHoverColors(const Color &bgColor, const Color &textColor)
{
    hoverBgColor_ = bgColor;
    hoverTextColor_ = textColor;
}

void UIButton::setPressedColors(const Color &bgColor, const Color &textColor)
{
    pressedBgColor_ = bgColor;
    pressedTextColor_ = textColor;
}

void UIButton::setDisabledColors(const Color &bgColor, const Color &textColor)
{
    disabledBgColor_ = bgColor;
    disabledTextColor_ = textColor;
}

void UIButton::setNormalColor(const Color &color)
{
    normalBgColor_ = color;
}

void UIButton::setHoverColor(const Color &color)
{
    hoverBgColor_ = color;
}

void UIButton::setPressedColor(const Color &color)
{
    pressedBgColor_ = color;
}

void UIButton::setDisabledColor(const Color &color)
{
    disabledBgColor_ = color;
}

void UIButton::setNormalTexture(unsigned int textureID)
{
    (void)textureID;
}

void UIButton::setHoverTexture(unsigned int textureID)
{
    (void)textureID;
}

void UIButton::setPressedTexture(unsigned int textureID)
{
    (void)textureID;
}

void UIButton::setDisabledTexture(unsigned int textureID)
{
    (void)textureID;
}

void UIButton::setText(const std::string &text)
{
    text_ = text;
}

void UIButton::setIconTexture(unsigned int textureID)
{
    iconTexture_ = textureID;
}

void UIButton::setOnClick(std::function<void()> callback)
{
    onClickCallback_ = callback;
}

UIButton::ButtonState UIButton::getCurrentState() const
{
    return currentState_;
}

void UIButton::setEnabled(bool enabled)
{
    UIElement::setEnabled(enabled);
    if (!enabled)
    {
        currentState_ = ButtonState::Disabled;
    }
    else
    {
        currentState_ = ButtonState::Normal;
    }
}

void UIButton::triggerClick()
{
    if (isEnabled() && onClickCallback_)
    {
        onClickCallback_();
    }
}

void UIButton::update(float deltaTime)
{
    // Update button state based on input would be handled by UIManager
    // For now, we'll just update children
    UIElement::update(deltaTime);
}

void UIButton::renderSelf(SpriteBatch &spriteBatch, Font &font) const
{
    // Get colors based on current state
    Color bgColor, textColor;

    switch (getCurrentState())
    {
    case ButtonState::Normal:
        bgColor = normalBgColor_;
        textColor = normalTextColor_;
        break;
    case ButtonState::Hover:
        bgColor = hoverBgColor_;
        textColor = hoverTextColor_;
        break;
    case ButtonState::Pressed:
        bgColor = pressedBgColor_;
        textColor = pressedTextColor_;
        break;
    case ButtonState::Disabled:
        bgColor = disabledBgColor_;
        textColor = disabledTextColor_;
        break;
    }

    // Calculate render position based on anchor
    float renderX, renderY;
    calculateRenderPosition(renderX, renderY);

    // Draw colored rectangle using a white texture tinted with our color.
    auto assetManager = ServiceLocator::Get<AssetManager>();
    if (assetManager)
    {
        std::shared_ptr<Texture> whiteTexture = assetManager->GetTexture("textures/white.png");
        if (!whiteTexture)
        {
            whiteTexture = assetManager->LoadTexture("textures/white.png");
        }
        if (whiteTexture)
        {
            spriteBatch.Draw(*whiteTexture, renderX, renderY, getWidth(), getHeight(),
                             bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        }
    }

    // Draw text if present
    if (!text_.empty())
    {
        float textX = renderX;
        float textY = renderY;

        // Center text vertically
        float textHeight = font.MeasureText(text_, 1.0f).y;
        textY += (getHeight() - textHeight) / 2.0f;

        // Get shader for text rendering
        auto textAssetManager = ServiceLocator::Get<AssetManager>();
        if (textAssetManager)
        {
            std::shared_ptr<Shader> shader = textAssetManager->GetShader("default.vert", "default.frag");
            if (!shader)
            {
                shader = textAssetManager->LoadShader("shaders/default.vert", "shaders/default.frag");
            }
            if (shader)
            {
                // Draw text
                font.DrawText(*shader, text_, textX, textY, 1.0f);
            }
        }
    }
}