#pragma once

#include "Component.h"

namespace StrixVerse
{
    namespace ECS
    {
        struct SpriteComponent : public Component
        {
            // Texture ID from the AssetManager.
            uint32_t textureID = 0;

            // Color tint.
            float r = 1.0f;
            float g = 1.0f;
            float b = 1.0f;
            float a = 1.0f;

            // Rendering layer (lower values rendered first).
            int layer = 0;
        };
    }
}