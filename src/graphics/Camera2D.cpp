#include "Camera2D.h"

#include <glm/gtc/matrix_transform.hpp>

Camera2D::Camera2D()
{
    // Default viewport will be set via SetViewport or via constructor with size.
}

Camera2D::Camera2D(float width, float height)
{
    SetViewport(width, height);
}

void Camera2D::SetViewport(float width, float height)
{
    m_Viewport.x = width;
    m_Viewport.y = height;
}

glm::vec2 Camera2D::GetViewport() const
{
    return m_Viewport;
}

void Camera2D::SetPosition(const glm::vec2& position)
{
    m_Position = position;
}

glm::vec2 Camera2D::GetPosition() const
{
    return m_Position;
}

void Camera2D::SetZoom(float zoom)
{
    // Prevent zoom from being too small or zero to avoid division by zero.
    if (zoom < 0.001f)
        m_Zoom = 0.001f;
    else
        m_Zoom = zoom;
}

float Camera2D::GetZoom() const
{
    return m_Zoom;
}

void Camera2D::SetRotation(float rotation)
{
    m_Rotation = rotation;
}

float Camera2D::GetRotation() const
{
    return m_Rotation;
}

glm::mat4 Camera2D::GetViewMatrix() const
{
    // The camera position is the point that lands in the middle of the viewport:
    //
    //     screen = (world - position) * zoom, rotated, then offset to the centre
    //
    // Composition runs right to left, so the world is moved relative to the
    // camera first and only then scaled and centred. The previous version
    // translated before scaling, which zoomed about the world origin rather
    // than about the camera and put the camera's target in the top-left corner
    // instead of the middle - unusable for a follow camera.
    glm::mat4 view = glm::mat4(1.0f);

    view = glm::translate(view, glm::vec3(m_Viewport * 0.5f, 0.0f));
    view = glm::rotate(view, -m_Rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    view = glm::scale(view, glm::vec3(m_Zoom, m_Zoom, 1.0f));
    view = glm::translate(view, glm::vec3(-m_Position, 0.0f));

    return view;
}

glm::mat4 Camera2D::GetProjectionMatrix() const
{
    // We want an orthographic projection that matches the viewport with origin at top-left and y increasing downward.
    // left = 0, right = width, bottom = height, top = 0 (note: bottom > top)
    return glm::ortho(0.0f, m_Viewport.x, m_Viewport.y, 0.0f, -1.0f, 1.0f);
}

glm::mat4 Camera2D::GetViewProjectionMatrix() const
{
    return GetProjectionMatrix() * GetViewMatrix();
}

glm::vec2 Camera2D::ScreenToWorld(const glm::vec2& screenCoords) const
{
    // Convert screen coordinates (in pixels, with origin at top-left) to world coordinates.
    // Step 1: Convert screen coordinates to NDC space ([-1,1] x [-1,1]) using the inverse of the viewport transformation.
    // But we can do it by using the inverse of the view-projection matrix.
    glm::vec4 clipSpace = glm::vec4(screenCoords, 0.0f, 1.0f); // z=0, w=1 for 2D point.
    // Actually, we need to convert from window coordinates to clip space.
    // Window coordinates: x in [0, width], y in [0, height] (with y down).
    // We first convert to NDC:
    //   ndc.x = (2.0 * screenCoords.x) / width - 1.0;
    //   ndc.y = 1.0 - (2.0 * screenCoords.y) / height; // because y is down in window coordinates but up in NDC.
    // However, note our projection matrix already flips y? Let's derive properly.

    // Instead, we can use the inverse of the view-projection matrix to transform from clip space to world space.
    // But we need to get the clip space coordinates from screen coordinates.
    // The viewport transformation from NDC to window coordinates is:
    //   x_win = (x_ndc + 1) * width / 2 + x_offset
    //   y_win = (y_ndc + 1) * height / 2 + y_offset
    // Assuming the viewport is at (0,0) and width, height as set.
    // We can invert this to get NDC from window coordinates:
    //   x_ndc = (2.0 * (x_win - x_offset)) / width - 1.0
    //   y_ndc = (2.0 * (y_win - y_offset)) / height - 1.0
    // But note: our y_win increases downward, while Ndc y increases upward. So we need to flip y.
    // Actually, the standard viewport transformation assumes y_win increases upward. Since we have y_win increasing downward,
    // we need to adjust.

    // Given the complexity, and since we have a simple projection matrix, we can do:
    //   worldPos = inverse(projection * view) * clipPos
    // where clipPos is obtained by converting screenCoords to NDC assuming the viewport transformation matches our projection.

    // Let's compute the NDC coordinates assuming the viewport transformation is:
    //   x_ndc = (2.0 * x_win) / width - 1.0
    //   y_ndc = 1.0 - (2.0 * y_win) / height   // flip because our y_win is down and Ndc y is up.
    // This matches if our projection matrix is set up with y increasing upward? Actually, we set our projection to have y increasing downward (bottom=height, top=0).
    // In that case, the Ndc y coordinate will increase downward as well? Let's check:
    //   For a point at y_win = 0 (top), we want y_ndc = 1? or -1?
    //   With our projection: top=0, bottom=height. The Ndc y is mapped such that:
    //      y_ndc = (2 * y_win - (top+bottom)) / (bottom - top)
    //   If we set top=0, bottom=height, then:
    //      y_ndc = (2 * y_win - height) / (-height) = (height - 2 * y_win) / height = 1 - (2 * y_win)/height
    //   So at y_win=0: y_ndc = 1
    //   At y_win=height: y_ndc = -1
    //   So indeed, y_ndc decreases as y_win increases, which matches the standard Ndc (y up) if we consider that y_win increasing downward.
    //   So we can use the standard formula without flipping if we set the projection as we did (top=0, bottom=height).
    //   Therefore:
    //      x_ndc = (2.0 * x_win) / width - 1.0
    //      y_ndc = (2.0 * y_win) / height - 1.0   // but wait, this gives y_ndc = -1 at y_win=0 and 1 at y_win=height? Let's recalc:
    //   Actually, the formula for glViewport is:
    //      x_ndc = (2 * (x_win - viewX)) / width - 1
    //      y_ndc = (2 * (y_win - viewY)) / height - 1
    //   where (viewX, viewY) is the lower left corner of the viewport.
    //   We set the viewport to (0,0) with width and height.
    //   So:
    //      x_ndc = (2 * x_win) / width - 1
    //      y_ndc = (2 * y_win) / height - 1
    //   This yields y_ndc = -1 at y_win=0 and y_ndc = 1 at y_win=height.
    //   So y_ndc increases as y_win increases, meaning y_ndc points downward.
    //   But in NDC, y_ndc increasing upward is the convention. So we have a mismatch.
    //   To fix, we can either flip the y in the projection matrix (which we did by setting bottom=height, top=0) or we can flip the y in the Ndc calculation.
    //   We chose to set the projection matrix with bottom=height, top=0, which should map window y to Ndc y such that increasing window y gives decreasing Ndc y (if we use the standard viewport equations). Let's verify:
    //   The projection matrix we set transforms eye coordinates to clip coordinates. Then the viewport transform maps clip coordinates to window coordinates.
    //   We want: when we send a vertex with y_eye = 0, it should appear at the top of the window (y_win=0) if we want top-left origin.
    //   Let's not get lost. Instead, we'll use the following approach: we'll compute the world position by unprojecting using the viewport and matrices we have.
    //   We'll use glm::unProject which takes window coordinates, model, projection, and viewport.
    //   We have the view matrix and projection matrix. We'll set the viewport to (0,0,width,height).
    //   We'll pass the window coordinates as (x, y, 0.0f) where y is the window y coordinate (with y=0 at top).
    //   But glm::unProject expects window coordinates with y=0 at the bottom of the window. So we need to flip y.
    //   Therefore, we will do:
    //      float yInverted = m_Viewport.y - screenCoords.y;
    //      glm::vec3 world = glm::unProject(glm::vec3(screenCoords.x, yInverted, 0.0f), GetViewMatrix(), GetProjectionMatrix(), glm::vec4(0, 0, m_Viewport.x, m_Viewport.y));
    //   This will give us the world position at the near plane (z=0).
    //   We'll return the x and y components.

    float yInverted = m_Viewport.y - screenCoords.y;
    glm::vec3 worldPos = glm::unProject(
        glm::vec3(screenCoords.x, yInverted, 0.0f),
        GetViewMatrix(),
        GetProjectionMatrix(),
        glm::vec4(0, 0, m_Viewport.x, m_Viewport.y));
    return glm::vec2(worldPos.x, worldPos.y);
}

glm::vec2 Camera2D::WorldToScreen(const glm::vec2& worldCoords) const
{
    // Convert world coordinates to screen coordinates (pixels, origin at top-left).
    // We'll use glm::project.
    glm::vec3 winPos = glm::project(
        glm::vec3(worldCoords.x, worldCoords.y, 0.0f),
        GetViewMatrix(),
        GetProjectionMatrix(),
        glm::vec4(0, 0, m_Viewport.x, m_Viewport.y));
    // glm::project returns y with y=0 at bottom of window. We need to flip to top-left.
    float yInverted = m_Viewport.y - winPos.y;
    return glm::vec2(winPos.x, yInverted);
}