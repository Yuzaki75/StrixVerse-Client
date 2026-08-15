#pragma once

#include <memory>

#include "UIElement.h"
#include "../graphics/Color.h"

class Texture;

// -----------------------------------------------------------------------------
// UIImage
//
// Displays a texture: world artwork, the logo, an icon.
//
// Holds the Texture by shared_ptr taken from the AssetManager cache, so drawing
// never searches for it and the texture cannot be freed while still on screen.
// -----------------------------------------------------------------------------
class UIImage : public UIElement
{
public:
    enum class ScaleMode
    {
        Stretch,   // Fill the box exactly, ignoring the source aspect ratio.
        Fit,       // Contain: whole image visible, letterboxed inside the box.
        Fill       // Cover: box fully painted, overflow cropped by the clip.
    };

    UIImage();
    ~UIImage() override = default;

    void setTexture(std::shared_ptr<Texture> texture);
    const std::shared_ptr<Texture>& getTexture() const { return texture_; }

    void setColor(const Color& color) { color_ = color; }
    const Color& getColor() const { return color_; }

    void setScaleMode(ScaleMode mode) { scaleMode_ = mode; }
    void setBorderRadius(float radius) { radius_ = radius; }

protected:
    void renderSelf(UIRenderer& renderer) const override;

private:
    std::shared_ptr<Texture> texture_;
    Color                    color_{1.0f, 1.0f, 1.0f, 1.0f};
    ScaleMode                scaleMode_ = ScaleMode::Stretch;
    float                    radius_    = 0.0f;
};
