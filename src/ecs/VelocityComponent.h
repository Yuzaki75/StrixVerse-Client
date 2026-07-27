#pragma once

#include "Component.h"

namespace StrixVerse
{
    namespace ECS
    {
        struct VelocityComponent : public Component
        {
            float vx = 0.0f;
            float vy = 0.0f;
        };
    }
}