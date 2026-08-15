#pragma once

#include "System.h"

namespace StrixVerse
{
    namespace ECS
    {
        class InputSystem : public System
        {
        public:
            void init(EntityManager* entityManager, ComponentManager* componentManager) override;
            void update(const std::vector<Entity>& entities, float dt) override;
        };
    }
}