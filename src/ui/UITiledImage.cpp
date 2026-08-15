#include "UITiledImage.h"

#include "../graphics/Texture.h"
#include "../graphics/UIRenderer.h"

UITiledImage::UITiledImage() = default;

void UITiledImage::setTexture(std::shared_ptr<Texture> texture)
{
    texture_ = std::move(texture);
}

void UITiledImage::renderSelf(UIRenderer& renderer) const
{
    if (!texture_ || tileSize_ <= 0.0f || color_.a <= 0.0f)
        return;

    renderer.DrawTextureTiled(*texture_,
                              getAbsoluteX(), getAbsoluteY(),
                              width_, height_,
                              tileSize_,
                              color_);
}
