#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// -----------------------------------------------------------------------------
// Camera2D
//
// Purpose:
//   Represents a 2D camera for the game world. Handles position, zoom, and
//   rotation, and provides view and projection matrices for rendering.
//
// Responsibilities:
//   - Store camera position (in world coordinates).
//   - Store zoom level (1.0 is normal).
//   - Store rotation (in radians, optional for 2D).
//   - Store viewport size (width, height) for projection matrix.
//   - Provide view matrix (inverse of camera transformation).
//   - Provide projection matrix (orthographic based on viewport).
//   - Provide combined view-projection matrix.
//   - Convert between screen and world coordinates.
//
// Dependencies: glm (OpenGL Mathematics).
// -----------------------------------------------------------------------------
class Camera2D
{
public:
    Camera2D();
    explicit Camera2D(float width, float height);
    ~Camera2D() = default;

    Camera2D(const Camera2D&) = delete;
    Camera2D& operator=(const Camera2D&) = delete;

    // Set the viewport size (called when window resizes).
    void SetViewport(float width, float height);

    // Get the viewport size.
    glm::vec2 GetViewport() const;

    // Set the camera position in world space.
    void SetPosition(const glm::vec2& position);
    glm::vec2 GetPosition() const;

    // Set the zoom level (1.0 is normal, >1 zooms in, <1 zooms out).
    void SetZoom(float zoom);
    float GetZoom() const;

    // Set the rotation in radians (optional for 2D).
    void SetRotation(float rotation);
    float GetRotation() const;

    // Get the view matrix (transforms world to camera space).
    glm::mat4 GetViewMatrix() const;

    // Get the projection matrix (orthographic projection).
    glm::mat4 GetProjectionMatrix() const;

    // Get the combined view-projection matrix.
    glm::mat4 GetViewProjectionMatrix() const;

    // Convert screen coordinates (in pixels, with origin at top-left) to world coordinates.
    glm::vec2 ScreenToWorld(const glm::vec2& screenCoords) const;

    // Convert world coordinates to screen coordinates (in pixels, origin at top-left).
    glm::vec2 WorldToScreen(const glm::vec2& worldCoords) const;

private:
    glm::vec2 m_Position{0.0f, 0.0f};
    float m_Zoom = 1.0f;
    float m_Rotation = 0.0f; // in radians
    glm::vec2 m_Viewport{0.0f, 0.0f}; // width, height
};