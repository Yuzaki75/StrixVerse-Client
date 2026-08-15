#pragma once

#include <glm/glm.hpp>

// -----------------------------------------------------------------------------
// UIScale
//
// Maps the fixed 1920x1080 design canvas onto whatever framebuffer the window
// currently has.
//
// A single uniform scale factor - min(width/1920, height/1080) - is used for
// both axes so the design never stretches, and the canvas is centred so that
// the 1920x1080 safe area is always fully on screen. The axis with spare room
// simply sees a little more canvas, which lets backgrounds run edge to edge
// instead of leaving letterbox bars.
//
// Screens are authored purely in canvas coordinates; ToCanvas() converts mouse
// input back so hit testing always agrees with what was drawn.
// -----------------------------------------------------------------------------
class UIScale
{
public:
    static constexpr float kDesignWidth  = 1920.0f;
    static constexpr float kDesignHeight = 1080.0f;

    UIScale() { Update(static_cast<int>(kDesignWidth), static_cast<int>(kDesignHeight)); }

    void Update(int framebufferWidth, int framebufferHeight);

    float GetScale() const { return m_Scale; }

    int GetFramebufferWidth() const { return m_FramebufferWidth; }
    int GetFramebufferHeight() const { return m_FramebufferHeight; }

    // (left, top, right, bottom) of the canvas region the framebuffer covers.
    // Always contains 0,0 -> 1920,1080 and extends past it on one axis.
    const glm::vec4& GetVisibleCanvas() const { return m_VisibleCanvas; }

    float GetVisibleLeft() const { return m_VisibleCanvas.x; }
    float GetVisibleTop() const { return m_VisibleCanvas.y; }
    float GetVisibleWidth() const { return m_VisibleCanvas.z - m_VisibleCanvas.x; }
    float GetVisibleHeight() const { return m_VisibleCanvas.w - m_VisibleCanvas.y; }

    // Framebuffer pixels -> canvas coordinates (for mouse input).
    glm::vec2 ToCanvas(float screenX, float screenY) const;

    // Canvas coordinates -> framebuffer pixels (for scissor rectangles).
    glm::vec2 ToScreen(float canvasX, float canvasY) const;

private:
    int   m_FramebufferWidth  = 0;
    int   m_FramebufferHeight = 0;
    float m_Scale             = 1.0f;

    glm::vec2 m_Offset{0.0f, 0.0f};          // Framebuffer pixels.
    glm::vec4 m_VisibleCanvas{0.0f, 0.0f, kDesignWidth, kDesignHeight};
};
