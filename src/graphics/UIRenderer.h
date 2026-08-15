#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Color.h"

class AssetManager;
class Font;
class Shader;
class Texture;

// -----------------------------------------------------------------------------
// UIQuadStyle
//
// Everything the Figma "crystal" look needs from one primitive: a vertical
// gradient fill, a rounded corner radius, an inner border and an outer glow.
// Reproducing these analytically in the shader is what lets the screens be
// built from real components instead of exported bitmaps.
// -----------------------------------------------------------------------------
struct UIQuadStyle
{
    // Fully transparent by default. Spelled out rather than using
    // Color::Transparent so the factories below can stay constexpr.
    Color fillTop{0.0f, 0.0f, 0.0f, 0.0f};
    Color fillBottom{0.0f, 0.0f, 0.0f, 0.0f};
    Color border{0.0f, 0.0f, 0.0f, 0.0f};
    Color glow{0.0f, 0.0f, 0.0f, 0.0f};

    float radius      = 0.0f;
    float borderWidth = 0.0f;
    float glowSize    = 0.0f;

    // Rotation about the quad's centre, in radians. The design's decorative
    // crystal shards are tilted; everything else leaves this at zero.
    float rotation = 0.0f;

    static constexpr UIQuadStyle Solid(const Color& fill, float radius = 0.0f)
    {
        UIQuadStyle style;
        style.fillTop    = fill;
        style.fillBottom = fill;
        style.radius     = radius;
        return style;
    }

    static constexpr UIQuadStyle Gradient(const Color& top, const Color& bottom, float radius = 0.0f)
    {
        UIQuadStyle style;
        style.fillTop    = top;
        style.fillBottom = bottom;
        style.radius     = radius;
        return style;
    }

    static constexpr UIQuadStyle None()
    {
        return UIQuadStyle{};
    }

    constexpr UIQuadStyle& WithBorder(const Color& color, float width)
    {
        border      = color;
        borderWidth = width;
        return *this;
    }

    constexpr UIQuadStyle& WithGlow(const Color& color, float size)
    {
        glow     = color;
        glowSize = size;
        return *this;
    }

    constexpr UIQuadStyle& WithRadius(float value)
    {
        radius = value;
        return *this;
    }

    constexpr UIQuadStyle& WithRotation(float radians)
    {
        rotation = radians;
        return *this;
    }
};

enum class TextAlign
{
    Left,
    Center,
    Right
};

// -----------------------------------------------------------------------------
// UIRenderer
//
// Immediate-mode batching renderer for the UI layer.
//
// All coordinates are in the 1920x1080 virtual design canvas; Begin() installs
// a projection that maps that canvas onto the framebuffer with a uniform scale,
// so screens are authored once at the design resolution and stay proportional
// everywhere else.
//
// Draw order is insertion order - the batch is only broken when the bound
// texture or the clip rectangle changes - so panels, images and text composite
// exactly as they were issued.
// -----------------------------------------------------------------------------
class UIRenderer
{
public:
    UIRenderer();
    ~UIRenderer();

    UIRenderer(const UIRenderer&) = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;

    bool Initialize(AssetManager& assets);
    void Shutdown();

    bool IsReady() const { return m_Shader != nullptr; }

    // visibleCanvas is (left, top, right, bottom) of the design canvas region
    // covered by the framebuffer. It extends past 0..1920 / 0..1080 on the axis
    // with spare room, which lets backgrounds fill the window edge to edge.
    void Begin(const glm::vec4& visibleCanvas, int framebufferWidth, int framebufferHeight);
    void End();

    void DrawRect(float x, float y, float width, float height, const UIQuadStyle& style);

    void DrawTexture(const Texture& texture,
                     float x, float y, float width, float height,
                     const Color& tint = Color::White,
                     float radius = 0.0f);

    // Repeats the texture every tileSize canvas pixels instead of stretching
    // it, for grain and pattern overlays. The texture must use GL_REPEAT wrap,
    // which is the AssetManager default.
    void DrawTextureTiled(const Texture& texture,
                          float x, float y, float width, float height,
                          float tileSize,
                          const Color& tint = Color::White);

    // Draws a UTF-8 run with its top-left corner at (x, y). Glyphs the face
    // lacks are skipped rather than drawn as boxes.
    void DrawText(const Font& font,
                  const std::string& utf8,
                  float x, float y,
                  const Color& color,
                  float letterSpacing = 0.0f);

    // Convenience wrapper that aligns inside [x, x + width].
    void DrawTextAligned(const Font& font,
                         const std::string& utf8,
                         float x, float y, float width,
                         TextAlign align,
                         const Color& color,
                         float letterSpacing = 0.0f);

    // Cheap approximation of the design's text glow: the run is drawn several
    // times at low alpha with a small offset before the solid pass.
    void DrawTextGlow(const Font& font,
                      const std::string& utf8,
                      float x, float y,
                      const Color& color,
                      const Color& glowColor,
                      float glowRadius,
                      float letterSpacing = 0.0f);

    // Flat-shaded geometry, for the design's small vector icons (check marks,
    // play arrows, stars). Points are in canvas coordinates.
    void DrawTriangle(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c,
                      const Color& color);

    // Convex polygon, emitted as a triangle fan from the first point.
    void DrawPolygon(const glm::vec2* points, size_t count, const Color& color);

    // Clip rectangles are given in canvas coordinates and nest by intersection.
    void PushClip(float x, float y, float width, float height);
    void PopClip();

    // Multiplies the alpha of everything drawn afterwards. Used by screen fades.
    void SetGlobalOpacity(float opacity);
    float GetGlobalOpacity() const { return m_GlobalOpacity; }

    // Statistics for the frame that just ended, for performance checks.
    size_t GetDrawCallCount() const { return m_DrawCallCount; }
    size_t GetQuadCount() const { return m_QuadCount; }

private:
    struct Vertex
    {
        float x, y;
        float u, v;
        float fillTop[4];
        float fillBottom[4];
        float border[4];
        float glow[4];
        float rect[4];
        float style[4];
        float localX, localY;   // Pre-rotation position, for the distance field.
    };

    struct Batch
    {
        unsigned int textureID   = 0;
        size_t       firstVertex = 0;
        size_t       vertexCount = 0;
        glm::vec4    clip{0.0f, 0.0f, 0.0f, 0.0f};   // Framebuffer pixels.
        bool         clipped     = false;
    };

    void PushQuad(float x, float y, float width, float height,
                  const glm::vec4& uv,
                  const UIQuadStyle& style,
                  unsigned int textureID,
                  float flags,
                  float expand);

    void PushFlatVertex(float x, float y, const Color& color);

    Batch& CurrentBatch(unsigned int textureID);
    void   Flush();
    glm::vec4 CanvasToFramebuffer(float x, float y, float width, float height) const;

    std::shared_ptr<Shader> m_Shader;

    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    size_t       m_VBOCapacityBytes = 0;

    std::vector<Vertex> m_Vertices;
    std::vector<Batch>  m_Batches;

    std::vector<glm::vec4> m_ClipStack;   // Framebuffer pixels.

    glm::vec4 m_VisibleCanvas{0.0f, 0.0f, 0.0f, 0.0f};
    int       m_FramebufferWidth  = 0;
    int       m_FramebufferHeight = 0;
    float     m_Scale             = 1.0f;

    float m_GlobalOpacity = 1.0f;

    size_t m_DrawCallCount = 0;
    size_t m_QuadCount     = 0;

    bool m_InFrame = false;
};
