#include "UIIcon.h"

#include "../graphics/UIRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace
{
    glm::vec2 Rotate(const glm::vec2& point, const glm::vec2& pivot, float radians)
    {
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        const glm::vec2 d = point - pivot;
        return {pivot.x + d.x * c - d.y * s,
                pivot.y + d.x * s + d.y * c};
    }

    // Emits a rectangle of the given thickness between two points, so a stroke
    // can run at any angle.
    void DrawStroke(UIRenderer& renderer,
                    const glm::vec2& from, const glm::vec2& to,
                    float thickness, const Color& color)
    {
        const glm::vec2 delta = to - from;
        const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        if (length <= 0.0f)
            return;

        const float nx = -delta.y / length * thickness * 0.5f;
        const float ny =  delta.x / length * thickness * 0.5f;

        const std::array<glm::vec2, 4> quad = {
            glm::vec2{from.x + nx, from.y + ny},
            glm::vec2{to.x + nx,   to.y + ny},
            glm::vec2{to.x - nx,   to.y - ny},
            glm::vec2{from.x - nx, from.y - ny},
        };

        renderer.DrawPolygon(quad.data(), quad.size(), color);
    }
}

UIIcon::UIIcon() = default;

UIIcon::UIIcon(Shape shape)
    : shape_(shape)
{
}

float UIIcon::stroke() const
{
    if (strokeWidth_ > 0.0f)
        return strokeWidth_;

    // A sensible default weight relative to the icon box.
    return std::max(1.0f, std::min(width_, height_) * 0.12f);
}

void UIIcon::renderSelf(UIRenderer& renderer) const
{
    if (color_.a <= 0.0f || width_ <= 0.0f || height_ <= 0.0f)
        return;

    // Icons are square; centre the shape inside a non-square box.
    const float size = std::min(width_, height_);
    const float x = getAbsoluteX() + (width_ - size) * 0.5f;
    const float y = getAbsoluteY() + (height_ - size) * 0.5f;

    switch (shape_)
    {
    case Shape::Check:     drawCheck(renderer, x, y, size); break;
    case Shape::Ring:      drawRing(renderer, x, y, size); break;
    case Shape::Play:      drawPlay(renderer, x, y, size); break;
    case Shape::Star:      drawStar(renderer, x, y, size); break;
    case Shape::ArrowLeft: drawArrowLeft(renderer, x, y, size); break;
    case Shape::Refresh:   drawRefresh(renderer, x, y, size); break;
    case Shape::Diamond:   drawDiamond(renderer, x, y, size); break;
    case Shape::Search:    drawSearch(renderer, x, y, size); break;
    case Shape::Cross:     drawCross(renderer, x, y, size); break;

    case Shape::Dot:
        renderer.DrawRect(x, y, size, size, UIQuadStyle::Solid(color_, size * 0.5f));
        break;
    }
}

void UIIcon::drawCheck(UIRenderer& renderer, float x, float y, float size) const
{
    const float thickness = stroke();

    // Classic tick: a short down-right stroke meeting a long up-right one.
    const glm::vec2 start{x + size * 0.20f, y + size * 0.52f};
    const glm::vec2 knee {x + size * 0.42f, y + size * 0.74f};
    const glm::vec2 end  {x + size * 0.82f, y + size * 0.26f};

    DrawStroke(renderer, start, knee, thickness, color_);
    DrawStroke(renderer, knee, end, thickness, color_);
}

void UIIcon::drawRing(UIRenderer& renderer, float x, float y, float size) const
{
    UIQuadStyle style;
    style.radius      = size * 0.5f;
    style.border      = color_;
    style.borderWidth = stroke();

    renderer.DrawRect(x, y, size, size, style);
}

void UIIcon::drawPlay(UIRenderer& renderer, float x, float y, float size) const
{
    const glm::vec2 apex  {x + size * 0.82f, y + size * 0.50f};
    const glm::vec2 top   {x + size * 0.24f, y + size * 0.18f};
    const glm::vec2 bottom{x + size * 0.24f, y + size * 0.82f};

    renderer.DrawTriangle(top, apex, bottom, color_);
}

void UIIcon::drawStar(UIRenderer& renderer, float x, float y, float size) const
{
    // Five-pointed star as a ten-vertex polygon, alternating outer and inner
    // radii. Drawn as a fan, which is valid because the shape is star-convex
    // about its centre.
    const glm::vec2 centre{x + size * 0.5f, y + size * 0.5f};
    const float outer = size * 0.5f;
    const float inner = outer * 0.42f;

    std::array<glm::vec2, 11> points{};
    points[0] = centre;

    constexpr float kPi = std::numbers::pi_v<float>;

    for (size_t i = 0; i < 10; ++i)
    {
        const float radius = (i % 2 == 0) ? outer : inner;
        const float angle  = -kPi * 0.5f + static_cast<float>(i) * kPi / 5.0f;

        points[i + 1] = {centre.x + std::cos(angle) * radius,
                         centre.y + std::sin(angle) * radius};
    }

    // Fan from the centre so every wedge is covered, including the last.
    for (size_t i = 1; i <= 10; ++i)
    {
        const glm::vec2& a = points[i];
        const glm::vec2& b = points[i == 10 ? 1 : i + 1];
        renderer.DrawTriangle(centre, a, b, color_);
    }
}

void UIIcon::drawArrowLeft(UIRenderer& renderer, float x, float y, float size) const
{
    const glm::vec2 tip   {x + size * 0.22f, y + size * 0.50f};
    const glm::vec2 top   {x + size * 0.58f, y + size * 0.20f};
    const glm::vec2 bottom{x + size * 0.58f, y + size * 0.80f};

    renderer.DrawTriangle(tip, top, bottom, color_);

    // Shaft.
    renderer.DrawRect(x + size * 0.52f, y + size * 0.42f,
                      size * 0.30f, size * 0.16f,
                      UIQuadStyle::Solid(color_, size * 0.05f));
}

void UIIcon::drawRefresh(UIRenderer& renderer, float x, float y, float size) const
{
    // An open ring drawn as a stroked circle with a wedge masked off by the
    // arrow head, which reads as the design's rotate glyph.
    const float thickness = stroke();
    const float inset     = thickness * 0.5f;

    UIQuadStyle ring;
    ring.radius      = size * 0.5f;
    ring.border      = color_;
    ring.borderWidth = thickness;

    renderer.DrawRect(x + inset, y + inset,
                      size - inset * 2.0f, size - inset * 2.0f, ring);

    // Arrow head at the top right of the ring.
    const glm::vec2 centre{x + size * 0.5f, y + size * 0.5f};
    const glm::vec2 tip = Rotate({x + size * 0.5f, y}, centre, 0.6f);
    const glm::vec2 a   = Rotate({x + size * 0.5f - size * 0.16f, y + size * 0.14f}, centre, 0.6f);
    const glm::vec2 b   = Rotate({x + size * 0.5f + size * 0.16f, y + size * 0.14f}, centre, 0.6f);

    renderer.DrawTriangle(tip, a, b, color_);
}

void UIIcon::drawDiamond(UIRenderer& renderer, float x, float y, float size) const
{
    const std::array<glm::vec2, 4> points = {
        glm::vec2{x + size * 0.5f,  y + size * 0.06f},
        glm::vec2{x + size * 0.94f, y + size * 0.42f},
        glm::vec2{x + size * 0.5f,  y + size * 0.94f},
        glm::vec2{x + size * 0.06f, y + size * 0.42f},
    };

    renderer.DrawPolygon(points.data(), points.size(), color_);
}

void UIIcon::drawSearch(UIRenderer& renderer, float x, float y, float size) const
{
    const float thickness = stroke();
    const float lensSize  = size * 0.68f;

    UIQuadStyle lens;
    lens.radius      = lensSize * 0.5f;
    lens.border      = color_;
    lens.borderWidth = thickness;

    renderer.DrawRect(x, y, lensSize, lensSize, lens);

    DrawStroke(renderer,
               {x + lensSize * 0.85f, y + lensSize * 0.85f},
               {x + size * 0.96f, y + size * 0.96f},
               thickness, color_);
}

void UIIcon::drawCross(UIRenderer& renderer, float x, float y, float size) const
{
    const float thickness = stroke();
    const float inset = size * 0.22f;

    DrawStroke(renderer,
               {x + inset, y + inset},
               {x + size - inset, y + size - inset},
               thickness, color_);

    DrawStroke(renderer,
               {x + size - inset, y + inset},
               {x + inset, y + size - inset},
               thickness, color_);
}
