#pragma once

#include "Component.h"

namespace StrixVerse
{
    namespace ECS
    {
        struct NetworkComponent : public Component
        {
            uint64_t networkID = 0;
            bool isLocalPlayer = false;

            // Synchronized transform (position and rotation).
            float x = 0.0f;
            float y = 0.0f;
            float rotation = 0.0f;
        };
    }
}