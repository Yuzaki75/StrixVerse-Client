#include "UIRenderer.h"

#include "Font.h"
#include "Shader.h"
#include "Texture.h"
#include "../core/AssetManager.h"
#include "../core/Logger.h"

#include <glad/glad.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
    // Must match the flag constants in shaders/ui.frag.
    constexpr float kFlagNone    = 0.0f;
    constexpr float kFlagTexture = 1.0f;
    constexpr float kFlagGlyph   = 2.0f;
    constexpr float kFlagNoShape = 4.0f;

    constexpr size_t kInitialQuadCapacity = 1024;
    constexpr size_t kVerticesPerQuad     = 6;

    void WriteColor(float (&dst)[4], const Color& color, float opacity)
    {
        dst[0] = color.r;
        dst[1] = color.g;
        dst[2] = color.b;
        dst[3] = color.a * opacity;
    }
}

UIRenderer::UIRenderer() = default;

UIRenderer::~UIRenderer()
{
    Shutdown();
}

bool UIRenderer::Initialize(AssetManager& assets)
{
    m_Shader = assets.LoadShader("shaders/ui.vert", "shaders/ui.frag");
    if (!m_Shader)
    {
        Logger::Error("UIRenderer: failed to load shaders/ui.vert + shaders/ui.frag.");
        return false;
    }

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    m_VBOCapacityBytes = kInitialQuadCapacity * kVerticesPerQuad * sizeof(Vertex);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_VBOCapacityBytes), nullptr, GL_STREAM_DRAW);

    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(Vertex));
    auto attribute = [stride](GLuint index, GLint components, size_t offset)
    {
        glEnableVertexAttribArray(index);
        glVertexAttribPointer(index, components, GL_FLOAT, GL_FALSE, stride,
                              reinterpret_cast<const void*>(offset));
    };

    attribute(0, 2, offsetof(Vertex, x));
    attribute(1, 2, offsetof(Vertex, u));
    attribute(2, 4, offsetof(Vertex, fillTop));
    attribute(3, 4, offsetof(Vertex, fillBottom));
    attribute(4, 4, offsetof(Vertex, border));
    attribute(5, 4, offsetof(Vertex, glow));
    attribute(6, 4, offsetof(Vertex, rect));
    attribute(7, 4, offsetof(Vertex, style));
    attribute(8, 2, offsetof(Vertex, localX));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    m_Vertices.reserve(kInitialQuadCapacity * kVerticesPerQuad);

    Logger::Info("UIRenderer: initialised.");
    return true;
}

void UIRenderer::Shutdown()
{
    if (m_VBO != 0)
    {
        glDeleteBuffers(1, &m_VBO);
        m_VBO = 0;
    }

    if (m_VAO != 0)
    {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }

    m_Shader.reset();
    m_Vertices.clear();
    m_Vertices.shrink_to_fit();
    m_Batches.clear();
    m_ClipStack.clear();
    m_VBOCapacityBytes = 0;
}

void UIRenderer::Begin(const glm::vec4& visibleCanvas, int framebufferWidth, int framebufferHeight)
{
    m_VisibleCanvas      = visibleCanvas;
    m_FramebufferWidth   = framebufferWidth;
    m_FramebufferHeight  = framebufferHeight;
    m_GlobalOpacity      = 1.0f;
    m_DrawCallCount      = 0;
    m_QuadCount          = 0;
    m_InFrame            = true;

    const float canvasWidth = visibleCanvas.z - visibleCanvas.x;
    m_Scale = canvasWidth > 0.0f ? static_cast<float>(framebufferWidth) / canvasWidth : 1.0f;

    m_Vertices.clear();
    m_Batches.clear();
    m_ClipStack.clear();
}

void UIRenderer::End()
{
    if (!m_InFrame)
        return;

    Flush();
    m_InFrame = false;
}

glm::vec4 UIRenderer::CanvasToFramebuffer(float x, float y, float width, float height) const
{
    const float screenX = (x - m_VisibleCanvas.x) * m_Scale;
    const float screenY = (y - m_VisibleCanvas.y) * m_Scale;

    return {screenX, screenY, width * m_Scale, height * m_Scale};
}

void UIRenderer::PushClip(float x, float y, float width, float height)
{
    glm::vec4 rect = CanvasToFramebuffer(x, y, width, height);

    if (!m_ClipStack.empty())
    {
        // Nested clips intersect with their parent.
        const glm::vec4& parent = m_ClipStack.back();

        const float left   = std::max(rect.x, parent.x);
        const float top    = std::max(rect.y, parent.y);
        const float right  = std::min(rect.x + rect.z, parent.x + parent.z);
        const float bottom = std::min(rect.y + rect.w, parent.y + parent.w);

        rect = {left, top, std::max(0.0f, right - left), std::max(0.0f, bottom - top)};
    }

    m_ClipStack.push_back(rect);
}

void UIRenderer::PopClip()
{
    if (!m_ClipStack.empty())
        m_ClipStack.pop_back();
}

void UIRenderer::SetGlobalOpacity(float opacity)
{
    m_GlobalOpacity = std::clamp(opacity, 0.0f, 1.0f);
}

UIRenderer::Batch& UIRenderer::CurrentBatch(unsigned int textureID)
{
    const bool      clipped = !m_ClipStack.empty();
    const glm::vec4 clip    = clipped ? m_ClipStack.back() : glm::vec4(0.0f);

    if (!m_Batches.empty())
    {
        Batch& back = m_Batches.back();

        const bool sameTexture = back.textureID == textureID;
        const bool sameClip    = back.clipped == clipped &&
                                 (!clipped || (back.clip == clip));

        if (sameTexture && sameClip)
            return back;
    }

    Batch batch;
    batch.textureID   = textureID;
    batch.firstVertex = m_Vertices.size();
    batch.vertexCount = 0;
    batch.clipped     = clipped;
    batch.clip        = clip;

    m_Batches.push_back(batch);
    return m_Batches.back();
}

void UIRenderer::PushQuad(float x, float y, float width, float height,
                          const glm::vec4& uv,
                          const UIQuadStyle& style,
                          unsigned int textureID,
                          float flags,
                          float expand)
{
    if (width <= 0.0f || height <= 0.0f)
        return;

    Batch& batch = CurrentBatch(textureID);

    // The geometry is grown outwards so an outer glow has somewhere to land.
    // aRect still describes the logical box, keeping the distance field honest.
    const float x0 = x - expand;
    const float y0 = y - expand;
    const float x1 = x + width + expand;
    const float y1 = y + height + expand;

    Vertex vertex{};
    WriteColor(vertex.fillTop,    style.fillTop,    m_GlobalOpacity);
    WriteColor(vertex.fillBottom, style.fillBottom, m_GlobalOpacity);
    WriteColor(vertex.border,     style.border,     m_GlobalOpacity);
    WriteColor(vertex.glow,       style.glow,       m_GlobalOpacity);

    vertex.rect[0] = x;
    vertex.rect[1] = y;
    vertex.rect[2] = width;
    vertex.rect[3] = height;

    vertex.style[0] = style.radius;
    vertex.style[1] = style.borderWidth;
    vertex.style[2] = style.glowSize;
    vertex.style[3] = flags;

    // UVs are only meaningful over the logical box; the expanded skirt samples
    // outside it, which is harmless because glow pixels ignore the texture.
    const float uSpan = uv.z - uv.x;
    const float vSpan = uv.w - uv.y;
    const float uPerPixel = width > 0.0f ? uSpan / width : 0.0f;
    const float vPerPixel = height > 0.0f ? vSpan / height : 0.0f;

    // Rotation is applied to the geometry only; the shader keeps evaluating
    // the distance field in the quad's own unrotated space.
    const bool  rotated  = style.rotation != 0.0f;
    const float cosTheta = rotated ? std::cos(style.rotation) : 1.0f;
    const float sinTheta = rotated ? std::sin(style.rotation) : 0.0f;
    const float pivotX   = x + width * 0.5f;
    const float pivotY   = y + height * 0.5f;

    auto emit = [&](float px, float py)
    {
        Vertex out = vertex;

        out.localX = px;
        out.localY = py;

        if (rotated)
        {
            const float dx = px - pivotX;
            const float dy = py - pivotY;
            out.x = pivotX + dx * cosTheta - dy * sinTheta;
            out.y = pivotY + dx * sinTheta + dy * cosTheta;
        }
        else
        {
            out.x = px;
            out.y = py;
        }

        out.u = uv.x + (px - x) * uPerPixel;
        out.v = uv.y + (py - y) * vPerPixel;

        m_Vertices.push_back(out);
    };

    emit(x0, y0);
    emit(x1, y0);
    emit(x0, y1);

    emit(x1, y0);
    emit(x1, y1);
    emit(x0, y1);

    batch.vertexCount += kVerticesPerQuad;
    ++m_QuadCount;
}

void UIRenderer::DrawRect(float x, float y, float width, float height, const UIQuadStyle& style)
{
    const bool hasFill   = style.fillTop.a > 0.0f || style.fillBottom.a > 0.0f;
    const bool hasBorder = style.borderWidth > 0.0f && style.border.a > 0.0f;
    const bool hasGlow   = style.glowSize > 0.0f && style.glow.a > 0.0f;

    if (!hasFill && !hasBorder && !hasGlow)
        return;

    PushQuad(x, y, width, height, {0.0f, 0.0f, 1.0f, 1.0f}, style, 0, kFlagNone,
             hasGlow ? style.glowSize : 0.0f);
}

void UIRenderer::DrawTexture(const Texture& texture,
                             float x, float y, float width, float height,
                             const Color& tint,
                             float radius)
{
    if (texture.GetRendererID() == 0 || tint.a <= 0.0f)
        return;

    UIQuadStyle style = UIQuadStyle::Solid(tint, radius);

    PushQuad(x, y, width, height, {0.0f, 0.0f, 1.0f, 1.0f}, style,
             texture.GetRendererID(), kFlagTexture, 0.0f);
}

void UIRenderer::PushFlatVertex(float x, float y, const Color& color)
{
    Vertex vertex{};

    WriteColor(vertex.fillTop,    color, m_GlobalOpacity);
    WriteColor(vertex.fillBottom, color, m_GlobalOpacity);

    vertex.x      = x;
    vertex.y      = y;
    vertex.localX = x;
    vertex.localY = y;

    // A zero-height logical rect makes the gradient constant, so the flat
    // colour comes straight from fillTop.
    vertex.style[3] = kFlagNoShape;

    m_Vertices.push_back(vertex);
}

void UIRenderer::DrawTriangle(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c,
                              const Color& color)
{
    if (color.a <= 0.0f)
        return;

    Batch& batch = CurrentBatch(0);

    PushFlatVertex(a.x, a.y, color);
    PushFlatVertex(b.x, b.y, color);
    PushFlatVertex(c.x, c.y, color);

    batch.vertexCount += 3;
}

void UIRenderer::DrawPolygon(const glm::vec2* points, size_t count, const Color& color)
{
    if (!points || count < 3 || color.a <= 0.0f)
        return;

    for (size_t i = 1; i + 1 < count; ++i)
        DrawTriangle(points[0], points[i], points[i + 1], color);
}

void UIRenderer::DrawTextureTiled(const Texture& texture,
                                  float x, float y, float width, float height,
                                  float tileSize,
                                  const Color& tint)
{
    if (texture.GetRendererID() == 0 || tint.a <= 0.0f || tileSize <= 0.0f)
        return;

    const glm::vec4 uv{0.0f, 0.0f, width / tileSize, height / tileSize};

    UIQuadStyle style = UIQuadStyle::Solid(tint);

    PushQuad(x, y, width, height, uv, style,
             texture.GetRendererID(), kFlagTexture + kFlagNoShape, 0.0f);
}

void UIRenderer::DrawText(const Font& font,
                          const std::string& utf8,
                          float x, float y,
                          const Color& color,
                          float letterSpacing)
{
    if (!font.IsLoaded() || utf8.empty() || color.a <= 0.0f)
        return;

    const unsigned int atlas = font.GetAtlasTexture();
    if (atlas == 0)
        return;

    const std::u32string codePoints = Font::DecodeUtf8(utf8);

    // (x, y) is the top-left of the line box; glyphs hang off the baseline.
    const float baselineY = y + font.GetAscent();

    float penX = x;
    bool  first = true;

    for (char32_t codePoint : codePoints)
    {
        const Glyph* glyph = font.GetGlyph(codePoint);
        if (!glyph)
            continue;

        if (!first)
            penX += letterSpacing;
        first = false;

        if (glyph->size.x > 0 && glyph->size.y > 0)
        {
            const float gx = penX + static_cast<float>(glyph->bearing.x);
            const float gy = baselineY - static_cast<float>(glyph->bearing.y);

            UIQuadStyle style = UIQuadStyle::Solid(color);

            PushQuad(gx, gy,
                     static_cast<float>(glyph->size.x),
                     static_cast<float>(glyph->size.y),
                     {glyph->uvMin.x, glyph->uvMin.y, glyph->uvMax.x, glyph->uvMax.y},
                     style, atlas, kFlagGlyph + kFlagNoShape, 0.0f);
        }

        penX += glyph->advance;
    }
}

void UIRenderer::DrawTextAligned(const Font& font,
                                 const std::string& utf8,
                                 float x, float y, float width,
                                 TextAlign align,
                                 const Color& color,
                                 float letterSpacing)
{
    if (!font.IsLoaded() || utf8.empty())
        return;

    float drawX = x;

    if (align != TextAlign::Left)
    {
        const float textWidth = font.MeasureWidth(utf8, letterSpacing);
        drawX = align == TextAlign::Center
                    ? x + (width - textWidth) * 0.5f
                    : x + width - textWidth;
    }

    DrawText(font, utf8, drawX, y, color, letterSpacing);
}

void UIRenderer::DrawTextGlow(const Font& font,
                              const std::string& utf8,
                              float x, float y,
                              const Color& color,
                              const Color& glowColor,
                              float glowRadius,
                              float letterSpacing)
{
    if (glowRadius > 0.0f && glowColor.a > 0.0f)
    {
        // Approximates a radial blur without a second render target: the run is
        // stamped around several concentric rings at low alpha. Rings rather
        // than a single offset ring, and enough samples per ring, are what stop
        // the halo reading as discrete ghost copies of the text.
        constexpr int   kRings          = 3;
        constexpr int   kSamplesPerRing = 12;
        constexpr float kRingWeights[kRings] = {0.055f, 0.035f, 0.020f};
        constexpr float kRingRadii[kRings]   = {0.38f, 0.72f, 1.0f};

        constexpr float kTwoPi = 6.28318530718f;

        for (int ring = 0; ring < kRings; ++ring)
        {
            Color halo = glowColor;
            halo.a = glowColor.a * kRingWeights[ring];

            if (halo.a <= 0.0f)
                continue;

            const float radius = glowRadius * kRingRadii[ring];

            // Offset every other ring by half a step so the samples interleave.
            const float phase = (ring % 2 == 0) ? 0.0f
                                                : kTwoPi / (kSamplesPerRing * 2.0f);

            for (int sample = 0; sample < kSamplesPerRing; ++sample)
            {
                const float angle = phase + kTwoPi * static_cast<float>(sample) /
                                                static_cast<float>(kSamplesPerRing);

                DrawText(font, utf8,
                         x + std::cos(angle) * radius,
                         y + std::sin(angle) * radius,
                         halo, letterSpacing);
            }
        }
    }

    DrawText(font, utf8, x, y, color, letterSpacing);
}

void UIRenderer::Flush()
{
    if (m_Vertices.empty() || !m_Shader)
        return;

    // Orthographic projection over the visible slice of the design canvas.
    const float left   = m_VisibleCanvas.x;
    const float top    = m_VisibleCanvas.y;
    const float right  = m_VisibleCanvas.z;
    const float bottom = m_VisibleCanvas.w;

    glm::mat4 projection(1.0f);
    projection[0][0] =  2.0f / (right - left);
    projection[1][1] =  2.0f / (top - bottom);
    projection[2][2] = -1.0f;
    projection[3][0] = -(right + left) / (right - left);
    projection[3][1] = -(top + bottom) / (top - bottom);

    // The shader emits premultiplied alpha so translucent panels, glows and
    // glyphs can overlap without the seams darkening.
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    m_Shader->Bind();
    m_Shader->SetMat4("uProjection", projection);
    m_Shader->SetInt("uTexture", 0);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    const size_t requiredBytes = m_Vertices.size() * sizeof(Vertex);
    if (requiredBytes > m_VBOCapacityBytes)
    {
        // Grow once and keep the larger buffer; the UI reaches a steady state
        // after the first frame of the busiest screen.
        m_VBOCapacityBytes = requiredBytes * 2;
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_VBOCapacityBytes), nullptr, GL_STREAM_DRAW);
    }
    else
    {
        // Orphan the previous contents so the upload never waits on the GPU.
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_VBOCapacityBytes), nullptr, GL_STREAM_DRAW);
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(requiredBytes), m_Vertices.data());

    glActiveTexture(GL_TEXTURE0);

    bool scissorEnabled = false;

    for (const Batch& batch : m_Batches)
    {
        if (batch.vertexCount == 0)
            continue;

        if (batch.clipped)
        {
            if (!scissorEnabled)
            {
                glEnable(GL_SCISSOR_TEST);
                scissorEnabled = true;
            }

            // glScissor's origin is bottom-left; the canvas grows downwards.
            const GLint   sx = static_cast<GLint>(batch.clip.x);
            const GLint   sy = static_cast<GLint>(static_cast<float>(m_FramebufferHeight) - (batch.clip.y + batch.clip.w));
            const GLsizei sw = static_cast<GLsizei>(std::max(0.0f, batch.clip.z));
            const GLsizei sh = static_cast<GLsizei>(std::max(0.0f, batch.clip.w));

            glScissor(sx, sy, sw, sh);
        }
        else if (scissorEnabled)
        {
            glDisable(GL_SCISSOR_TEST);
            scissorEnabled = false;
        }

        glBindTexture(GL_TEXTURE_2D, batch.textureID);

        glDrawArrays(GL_TRIANGLES,
                     static_cast<GLint>(batch.firstVertex),
                     static_cast<GLsizei>(batch.vertexCount));

        ++m_DrawCallCount;
    }

    if (scissorEnabled)
        glDisable(GL_SCISSOR_TEST);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    m_Shader->Unbind();

    // Leave the pipeline the way the rest of the renderer expects to find it.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (depthWasEnabled)
        glEnable(GL_DEPTH_TEST);

    m_Vertices.clear();
    m_Batches.clear();
}
