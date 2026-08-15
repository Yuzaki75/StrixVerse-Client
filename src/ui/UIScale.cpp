#include "UIScale.h"

#include <algorithm>

void UIScale::Update(int framebufferWidth, int framebufferHeight)
{
    // A minimised window reports a zero-sized framebuffer; keep the previous
    // mapping rather than dividing by zero.
    if (framebufferWidth <= 0 || framebufferHeight <= 0)
        return;

    m_FramebufferWidth  = framebufferWidth;
    m_FramebufferHeight = framebufferHeight;

    const float width  = static_cast<float>(framebufferWidth);
    const float height = static_cast<float>(framebufferHeight);

    // Uniform scale: the design is never stretched, and the whole 1920x1080
    // safe area always fits.
    m_Scale = std::min(width / kDesignWidth, height / kDesignHeight);

    const float scaledWidth  = kDesignWidth * m_Scale;
    const float scaledHeight = kDesignHeight * m_Scale;

    m_Offset = {(width - scaledWidth) * 0.5f, (height - scaledHeight) * 0.5f};

    // The framebuffer maps onto a canvas region that is centred on the design
    // area and grows past it wherever the window has spare room.
    const float left   = -m_Offset.x / m_Scale;
    const float top    = -m_Offset.y / m_Scale;
    const float right  = left + width / m_Scale;
    const float bottom = top + height / m_Scale;

    m_VisibleCanvas = {left, top, right, bottom};
}

glm::vec2 UIScale::ToCanvas(float screenX, float screenY) const
{
    if (m_Scale <= 0.0f)
        return {screenX, screenY};

    return {(screenX - m_Offset.x) / m_Scale,
            (screenY - m_Offset.y) / m_Scale};
}

glm::vec2 UIScale::ToScreen(float canvasX, float canvasY) const
{
    return {canvasX * m_Scale + m_Offset.x,
            canvasY * m_Scale + m_Offset.y};
}
