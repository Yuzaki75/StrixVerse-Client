#include "UIPanel.h"
#include "../graphics/SpriteBatch.h"
#include "../graphics/Font.h"
#include "../core/ServiceLocator.h"
#include "../core/AssetManager.h"
#include "../core/Logger.h"

UIPanel::UIPanel()
    : backgroundColor_(0.0f, 0.0f, 0.0f, 0.5f), // Semi-transparent black by default
      borderColor_(1.0f, 1.0f, 1.0f, 1.0f),      // White border
      borderWidth_(1.0f),
      borderRadius_(0.0f),
      backgroundImage_(0) {}

void UIPanel::setBackgroundColor(const Color& color) {
    backgroundColor_ = color;
}

void UIPanel::setBackgroundColor(float r, float g, float b, float a) {
    backgroundColor_ = Color(r, g, b, a);
}

void UIPanel::setBorderWidth(float width) {
    borderWidth_ = width;
}

void UIPanel::setBorderColor(const Color& color) {
    borderColor_ = color;
}

void UIPanel::setBorderColor(float r, float g, float b, float a) {
    borderColor_ = Color(r, g, b, a);
}

void UIPanel::setBorderRadius(float radius) {
    borderRadius_ = radius;
}

void UIPanel::setBackgroundImage(unsigned int textureID) {
    backgroundImage_ = textureID;
}

void UIPanel::renderSelf(SpriteBatch& spriteBatch, Font& font) const {
    // Calculate render position based on anchor
    float renderX, renderY;
    calculateRenderPosition(renderX, renderY);

    // Get AssetManager for textures and shaders
    auto assetManager = ServiceLocator::Get<AssetManager>();
    if (!assetManager) {
        Logger::Error("UIPanel: AssetManager not available");
        return;
    }

    // Get a white texture for drawing rectangles
    std::shared_ptr<Texture> whiteTexture = assetManager->GetTexture("textures/white.png");
    if (!whiteTexture) {
        whiteTexture = assetManager->LoadTexture("textures/white.png");
    }
    if (!whiteTexture) {
        Logger::Error("UIPanel: Failed to get or load white texture");
        return;
    }

    // Draw background color
    spriteBatch.Draw(*whiteTexture, renderX, renderY, getWidth(), getHeight(),
                    backgroundColor_.r, backgroundColor_.g, backgroundColor_.b, backgroundColor_.a);

    // Draw border if width > 0
    if (borderWidth_ > 0.0f) {
        // Draw border by drawing four rectangles (top, bottom, left, right)
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
    }
    // Note: Border radius is not implemented in this simple version
    // A more advanced implementation would use a shader or more complex geometry
}