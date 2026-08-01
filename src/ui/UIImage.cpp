#include "UIImage.h"
#include "../graphics/SpriteBatch.h"
#include "../graphics/Font.h"
#include "../core/AssetManager.h"
#include "../graphics/Texture.h"
#include "../core/ServiceLocator.h"
#include "../core/Logger.h"

UIImage::UIImage()
    : textureID_(0)
    , color_(1.0f, 1.0f, 1.0f, 1.0f)  // White (no tint) by default
{
    // Images typically size themselves to their texture
    // But we'll allow explicit sizing as well
}

void UIImage::setTexture(unsigned int textureID) {
    textureID_ = textureID;
    // Optionally auto-size to fit texture dimensions
    // In a real implementation, we'd get the texture size from AssetManager
}

void UIImage::setColor(const Color& color) {
    color_ = color;
}

void UIImage::renderSelf(SpriteBatch& spriteBatch, Font& font) const {
    if (textureID_ == 0) {
        // No texture set, maybe draw a placeholder or nothing
        return;
    }

    // Get the actual Texture object from AssetManager using textureID_
    auto assetManager = ServiceLocator::Get<AssetManager>();
    if (!assetManager) {
        Logger::Error("UIImage: AssetManager not available");
        return;
    }

    std::shared_ptr<Texture> texture = std::shared_ptr<Texture>(assetManager->GetTextureByRendererID(textureID_), [](Texture*) {});
    if (!texture) {
        // Texture not found, skip drawing
        return;
    }

    // Calculate render position based on anchor
    float renderX, renderY;
    calculateRenderPosition(renderX, renderY);

    // Draw the texture with tint color
    spriteBatch.Draw(*texture, renderX, renderY, getWidth(), getHeight(),
                    color_.r, color_.g, color_.b, color_.a);
}