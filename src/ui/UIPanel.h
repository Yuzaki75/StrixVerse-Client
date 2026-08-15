#pragma once

#include <memory>

#include "UIElement.h"
#include "../graphics/Color.h"
#include "../graphics/UIRenderer.h"

class Texture;

// -----------------------------------------------------------------------------
// UIPanel
//
// The design's ".sv-panel" primitive and the general-purpose container.
//
// Supports everything the crystal styling needs on a rectangle: a solid or
// vertical-gradient fill, a rounded corner radius, a coloured border and an
// outer glow - all drawn analytically by UIRenderer, so panels stay crisp at
// any UI scale without any exported nine-slice artwork.
//
// It can also render a texture (a background image or world artwork) clipped to
// the same rounded rectangle.
// -----------------------------------------------------------------------------
class UIPanel : public UIElement
{
public:
    UIPanel();
    ~UIPanel() override = default;

    // --- Fill ------------------------------------------------------------
    void setBackgroundColor(const Color& color);
    void setBackgroundColor(float r, float g, float b, float a = 1.0f);
    void setBackgroundGradient(const Color& top, const Color& bottom);

    const Color& getBackgroundColor() const { return style_.fillTop; }

    // --- Border ----------------------------------------------------------
    void setBorderColor(const Color& color);
    void setBorderColor(float r, float g, float b, float a = 1.0f);
    void setBorderWidth(float width);
    void setBorder(const Color& color, float width);

    float getBorderWidth() const { return style_.borderWidth; }

    // --- Shape and glow ---------------------------------------------------
    void setBorderRadius(float radius);
    float getBorderRadius() const { return style_.radius; }

    void setGlow(const Color& color, float size);

    // Rotation about the panel's centre, in radians. Used by the decorative
    // crystal shards on the splash and continue screens.
    void setRotation(float radians) { style_.rotation = radians; }

    const UIQuadStyle& getStyle() const { return style_; }
    void setStyle(const UIQuadStyle& style) { style_ = style; }

    // --- Image -----------------------------------------------------------
    // Drawn on top of the fill and clipped to the panel's rounded rectangle.
    void setBackgroundImage(std::shared_ptr<Texture> texture);
    void setBackgroundImageTint(const Color& tint) { imageTint_ = tint; }

    // Clips children to this panel's bounds. Off by default because most
    // panels are plain containers and scissoring costs a batch break.
    void setClipsChildren(bool clip) { clipsChildren_ = clip; }

protected:
    void renderSelf(UIRenderer& renderer) const override;
    void beginChildren(UIRenderer& renderer) const override;
    void endChildren(UIRenderer& renderer) const override;

    UIQuadStyle style_;

    std::shared_ptr<Texture> backgroundImage_;
    Color                    imageTint_{1.0f, 1.0f, 1.0f, 1.0f};

    bool clipsChildren_ = false;
};
