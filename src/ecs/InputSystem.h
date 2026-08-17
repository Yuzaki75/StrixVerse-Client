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

            // Set while the pause overlay is up. Gameplay keys are reported as
            // released, exactly as they are while a text field holds focus, so
            // there is one place that decides whether input reaches the player.
            void SetGameplayPaused(bool paused) { m_GameplayPaused = paused; }
            bool IsGameplayPaused() const { return m_GameplayPaused; }

        private:
            bool m_GameplayPaused = false;
        };
    }
}