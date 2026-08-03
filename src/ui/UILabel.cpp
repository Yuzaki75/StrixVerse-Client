#include "UILabel.h"
#include "../graphics/SpriteBatch.h"
#include "../graphics/Font.h"
#include "../core/ServiceLocator.h"
#include "../core/AssetManager.h"
#include "../graphics/Shader.h"
#include "../core/Logger.h"

UILabel::UILabel()
    : fontSize_(16.0f) // Default font size
      ,
      textColor_(1.0f, 1.0f, 1.0f, 1.0f) // White text
      ,
      hAlign_(0) // Left aligned
      ,
      vAlign_(0) // Top aligned
{
    // Labels typically size themselves to their text
    // But we'll allow explicit sizing as well
}

void UILabel::setText(const std::string &text)
{
    text_ = text;
    // Optionally auto-size to fit text
    // setSize(font.MeasureText(text_).x, font.MeasureText(text_).y);
}

void UILabel::setFontSize(float size)
{
    fontSize_ = size;
}

void UILabel::setTextColor(const Color &color)
{
    textColor_ = color;
}

void UILabel::setHorizontalAlignment(int alignment)
{
    hAlign_ = alignment;
}

void UILabel::setVerticalAlignment(int alignment)
{
    vAlign_ = alignment;
}

void UILabel::renderSelf(SpriteBatch &spriteBatch, Font &font) const
{
    if (text_.empty())
    {
        return;
    }

    // Calculate text position based on alignment
    float textWidth = font.MeasureText(text_, 1.0f).x;
    float textHeight = font.MeasureText(text_, 1.0f).y;

    float textX = getX();
    float textY = getY();

    // Apply horizontal alignment
    switch (hAlign_)
    {
    case 1: // Center
        textX = getX() + (getWidth() - textWidth) / 2.0f;
        break;
    case 2: // Right
        textX = getX() + getWidth() - textWidth;
        break;
    default: // Left (0)
        textX = getX();
        break;
    }

    // Apply vertical alignment
    switch (vAlign_)
    {
    case 1: // Middle
        textY = getY() + (getHeight() - textHeight) / 2.0f;
        break;
    case 2: // Bottom
        textY = getY() + getHeight() - textHeight;
        break;
    default: // Top (0)
        textY = getY();
        break;
    }

    // Calculate render position based on anchor
    float renderX, renderY;
    calculateRenderPosition(renderX, renderY);

    // Adjust text position by the render offset
    textX += renderX - getX();
    textY += renderY - getY();

    // Get shader from AssetManager for font rendering
    auto assetManager = ServiceLocator::Get<AssetManager>();
    if (!assetManager)
    {
        Logger::Error("UILabel: AssetManager not available");
        return;
    }

    // Get a default shader for text rendering
    // In a real implementation, we might have a specific shader for fonts
    // For now, we'll try to get a default shader or create a simple one
    std::shared_ptr<Shader> shader = assetManager->GetShader("default.vert", "default.frag");
    if (!shader)
    {
        // If we can't get a shader, try to load a basic one
        shader = assetManager->LoadShader("shaders/default.vert", "shaders/default.frag");
        if (!shader)
        {
            Logger::Error("UILabel: Failed to get or create shader for text rendering");
            return;
        }
    }

    // Draw the text
    font.DrawText(*shader, text_, textX, textY, 1.0f);
}