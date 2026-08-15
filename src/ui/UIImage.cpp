#include "UIImage.h"

#include "../graphics/Texture.h"
#include "../graphics/UIRenderer.h"

#include <algorithm>

UIImage::UIImage() = default;

void UIImage::setTexture(std::shared_ptr<Texture> texture)
{
    texture_ = std::move(texture);
}

void UIImage::renderSelf(UIRenderer& renderer) const
{
    if (!texture_ || texture_->GetRendererID() == 0 || color_.a <= 0.0f)
        return;

    const float boxX = getAbsoluteX();
    const float boxY = getAbsoluteY();

    float drawX = boxX;
    float drawY = boxY;
    float drawW = width_;
    float drawH = height_;

    if (scaleMode_ != ScaleMode::Stretch)
    {
        const float sourceW = static_cast<float>(texture_->GetWidth());
        const float sourceH = static_cast<float>(texture_->GetHeight());

        if (sourceW > 0.0f && sourceH > 0.0f && width_ > 0.0f && height_ > 0.0f)
        {
            const float scaleX = width_ / sourceW;
            const float scaleY = height_ / sourceH;

            // Fit contains the image; Fill covers the box.
            const float scale = scaleMode_ == ScaleMode::Fit
                                    ? std::min(scaleX, scaleY)
                                    : std::max(scaleX, scaleY);

            drawW = sourceW * scale;
            drawH = sourceH * scale;
            drawX = boxX + (width_ - drawW) * 0.5f;
            drawY = boxY + (height_ - drawH) * 0.5f;
        }
    }

    // Fill mode deliberately overflows, so clip it back to the element box.
    const bool needsClip = scaleMode_ == ScaleMode::Fill &&
                           (drawW > width_ || drawH > height_);

    if (needsClip)
        renderer.PushClip(boxX, boxY, width_, height_);

    renderer.DrawTexture(*texture_, drawX, drawY, drawW, drawH, color_, radius_);

    if (needsClip)
        renderer.PopClip();
}
