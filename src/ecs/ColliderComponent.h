#pragma once

#include "Component.h"

namespace StrixVerse
{
    namespace ECS
    {
        struct ColliderComponent : public Component
        {
            float width = 0.5f;
            float height = 0.5f;
            bool enabled = true;
        };
    }
}