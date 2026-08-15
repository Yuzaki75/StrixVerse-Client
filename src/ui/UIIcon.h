#pragma once

#include "UIElement.h"
#include "../graphics/Color.h"

// -----------------------------------------------------------------------------
// UIIcon
//
// The small vector glyphs the designed screens use, drawn from primitives
// rather than from a font or a sprite sheet.
//
// The three design typefaces are pixel fonts with no symbol coverage (and the
// style guide's mock-ups lean on emoji, which no game font ships), so the
// icons that carry meaning - progress checks, the play arrow, the favourite
// star - are drawn instead. They stay crisp at every UI scale and need no
// asset pipeline.
// -----------------------------------------------------------------------------
class UIIcon : public UIElement
{
public:
    enum class Shape
    {
        Check,        // Completed step
        Ring,         // Pending step
        Dot,          // Filled status dot / loading pip
        Play,         // "Continue" triangle
        Star,         // Favourite world
        ArrowLeft,    // Back
        Refresh,      // Reload the world list
        Diamond,      // Crystal motif
        Search,       // Empty-results state
        Cross         // Close / failure
    };

    UIIcon();
    explicit UIIcon(Shape shape);
    ~UIIcon() override = default;

    void setShape(Shape shape) { shape_ = shape; }
    Shape getShape() const { return shape_; }

    void setColor(const Color& color) { color_ = color; }
    const Color& getColor() const { return color_; }

    // Stroke weight for the outline shapes (Ring, Check, Cross, Refresh).
    // Defaults to a proportion of the element size.
    void setStrokeWidth(float width) { strokeWidth_ = width; }

protected:
    void renderSelf(UIRenderer& renderer) const override;

private:
    float stroke() const;

    void drawCheck(UIRenderer& renderer, float x, float y, float size) const;
    void drawRing(UIRenderer& renderer, float x, float y, float size) const;
    void drawPlay(UIRenderer& renderer, float x, float y, float size) const;
    void drawStar(UIRenderer& renderer, float x, float y, float size) const;
    void drawArrowLeft(UIRenderer& renderer, float x, float y, float size) const;
    void drawRefresh(UIRenderer& renderer, float x, float y, float size) const;
    void drawDiamond(UIRenderer& renderer, float x, float y, float size) const;
    void drawSearch(UIRenderer& renderer, float x, float y, float size) const;
    void drawCross(UIRenderer& renderer, float x, float y, float size) const;

    Shape shape_ = Shape::Dot;
    Color color_{1.0f, 1.0f, 1.0f, 1.0f};
    float strokeWidth_ = 0.0f;
};
