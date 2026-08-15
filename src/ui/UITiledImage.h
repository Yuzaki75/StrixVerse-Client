#pragma once

#include <memory>

#include "UIElement.h"
#include "../graphics/Color.h"

class Texture;

// -----------------------------------------------------------------------------
// UITiledImage
//
// Repeats a texture across the element instead of stretching it.
//
// Used for the design's full-bleed overlays: the splash screen's film grain and
// the "sv-pixel-grid" dot pattern that sits behind the login, register,
// connecting, continue and loading screens.
// -----------------------------------------------------------------------------
class UITiledImage : public UIElement
{
public:
    UITiledImage();
    ~UITiledImage() override = default;

    void setTexture(std::shared_ptr<Texture> texture);

    // Size of one repeat, in canvas pixels.
    void setTileSize(float tileSize) { tileSize_ = tileSize; }

    void setColor(const Color& color) { color_ = color; }

protected:
    void renderSelf(UIRenderer& renderer) const override;

private:
    std::shared_ptr<Texture> texture_;
    float                    tileSize_ = 32.0f;
    Color                    color_{1.0f, 1.0f, 1.0f, 1.0f};
};
