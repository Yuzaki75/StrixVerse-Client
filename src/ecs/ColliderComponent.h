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

            // Written by CollisionSystem every frame: true when solid ground
            // sits directly beneath this collider. Read by PlayerSystem, which
            // runs one system earlier, so it is the previous frame's answer -
            // a frame of latency on a jump, which is not perceptible and is
            // cheaper than a second collision pass.
            bool grounded = false;
        };
    }
}