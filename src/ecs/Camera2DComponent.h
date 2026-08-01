#pragma once

#include "Component.h"
#include "Entity.h"

namespace StrixVerse
{
    namespace ECS
    {
        // Component to hold camera data for entities
        struct Camera2DComponent : public Component
        {
            // Camera properties
            float zoom = 1.0f;     // Zoom level (1.0 = normal)
            float rotation = 0.0f; // Rotation in radians

            // Target to follow (optional)
            bool followTarget = false;
            Entity targetEntity = NULL_ENTITY; // Entity ID to follow

            // Offset from target when following
            float offsetX = 0.0f;
            float offsetY = 0.0f;
        };
    }
}