#pragma once

#include "UIElement.h"
#include "../graphics/Color.h"

/**
 * Image UI element for displaying textures
 */
class UIImage : public UIElement {
public:
    UIImage();
    ~UIImage() override = default;

    // Image properties
    void setTexture(unsigned int textureID);
    unsigned int getTexture() const { return textureID_; }

    void setColor(const Color& color);
    const Color& getColor() const { return color_; }

    // UIElement overrides
    void renderSelf(SpriteBatch& spriteBatch, Font& font) const override;

private:
    unsigned int textureID_; // Texture ID (would map to actual Texture in AssetManager)
    Color color_;            // Tint color
};