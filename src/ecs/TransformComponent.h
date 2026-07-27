#pragma once

#include "Component.h"

namespace StrixVerse
{
    namespace ECS
    {
        struct Transform : public Component
        {
            struct Vector2
            {
                float x = 0.0f;
                float y = 0.0f;
            };

            Vector2 position;
            float rotation = 0.0f; // in radians
            Vector2 scale{1.0f, 1.0f};
        };
    }
}