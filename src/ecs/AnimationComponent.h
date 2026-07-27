#pragma once

#include "Component.h"

namespace StrixVerse
{
    namespace ECS
    {
        struct AnimationComponent : public Component
        {
            bool isAnimated = false;
            float frameTime = 0.1f; // seconds per frame
            float currentTime = 0.0f;
            int startFrame = 0;
            int endFrame = 0;
            int currentFrame = 0;
            bool loop = true;
        };
    }
}