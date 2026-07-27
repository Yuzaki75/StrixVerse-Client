#pragma once

#include "System.h"
#include "TransformComponent.h"
#include "VelocityComponent.h"

namespace StrixVerse
{
    namespace ECS
    {
        class MovementSystem : public System
        {
        public:
            void update(const std::vector<Entity>& entities, float dt) override;
        };
    }
}