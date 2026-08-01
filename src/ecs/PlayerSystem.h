#pragma once

#include "System.h"

namespace StrixVerse
{
    namespace ECS
    {
        class PlayerSystem : public System
        {
        public:
            void init(EntityManager* entityManager, ComponentManager* componentManager) override;
            void update(const std::vector<Entity>& entities, float deltaTime) override;

        private:
            float m_MoveSpeed = 100.0f; // pixels per second
        };
    }
}