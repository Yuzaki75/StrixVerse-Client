#pragma once

#include "UIElement.h"
#include "../graphics/Color.h"

/**
 * A panel UI element with background styling
 */
class UIPanel : public UIElement {
public:
    UIPanel();
    ~UIPanel() override = default;

    // Background styling
    void setBackgroundColor(const Color& color);
    void setBackgroundColor(float r, float g, float b, float a = 1.0f);
    const Color& getBackgroundColor() const { return backgroundColor_; }

    void setBorderWidth(float width);
    float getBorderWidth() const { return borderWidth_; }

    void setBorderColor(const Color& color);
    void setBorderColor(float r, float g, float b, float a = 1.0f);
    const Color& getBorderColor() const { return borderColor_; }

    void setBorderRadius(float radius);
    float getBorderRadius() const { return borderRadius_; }

    void setBackgroundImage(unsigned int textureID);
    unsigned int getBackgroundImage() const { return backgroundImage_; }

protected:
    void renderSelf(SpriteBatch& spriteBatch, Font& font) const override;

private:
    Color backgroundColor_;
    Color borderColor_;
    float borderWidth_;
    float borderRadius_;
    unsigned int backgroundImage_; // Texture ID for background image
};