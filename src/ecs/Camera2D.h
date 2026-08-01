#pragma once

#include <glm/glm.hpp>

namespace StrixVerse
{
    // Simple camera class for 2D games.
    // Stores position, zoom, rotation, and viewport size.
    class Camera2D
    {
    public:
        Camera2D()
            : position(0.0f, 0.0f)
            , zoom(1.0f)
            , rotation(0.0f)
            , viewportSize(800.0f, 600.0f) // default size
        {}

        // Position of the camera in world space.
        glm::vec2 position;
        // Zoom level (1.0 is normal, >1 zooms in, <1 zooms out).
        float zoom;
        // Rotation in radians.
        float rotation;
        // Size of the viewport (in pixels).
        glm::vec2 viewportSize;
    };
}