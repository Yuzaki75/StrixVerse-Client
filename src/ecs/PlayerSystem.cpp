#include "PlayerSystem.h"
#include "../core/Logger.h"
#include "../core/Window.h"
#include "ComponentManager.h"
#include "InputComponent.h"
#include "PlayerComponent.h"
#include "VelocityComponent.h"
#include "TransformComponent.h"

namespace StrixVerse
{
    namespace ECS
    {
        void PlayerSystem::init(EntityManager* entityManager, ComponentManager* componentManager)
        {
            // Require PlayerComponent, InputComponent, and VelocityComponent
            setSignature<PlayerComponent, InputComponent, VelocityComponent>();
            LOG_INFO("PlayerSystem: Initialized");
        }

        void PlayerSystem::update(const std::vector<Entity>& entities, float deltaTime)
        {
            for (Entity entity : entities)
            {
                auto* input = m_pComponentManager->getComponent<InputComponent>(entity);
                auto* velocity = m_pComponentManager->getComponent<VelocityComponent>(entity);

                if (!input || !velocity)
                    continue;

                // Reset velocity
                velocity->vx = 0.0f;
                velocity->vy = 0.0f;

                // Check for movement keys (WASD)
                bool moving = false;

                if (input->keys.test(SDL_SCANCODE_W))
                {
                    velocity->vy -= m_MoveSpeed * deltaTime;
                    moving = true;
                }
                if (input->keys.test(SDL_SCANCODE_S))
                {
                    velocity->vy += m_MoveSpeed * deltaTime;
                    moving = true;
                }
                if (input->keys.test(SDL_SCANCODE_A))
                {
                    velocity->vx -= m_MoveSpeed * deltaTime;
                    moving = true;
                }
                if (input->keys.test(SDL_SCANCODE_D))
                {
                    velocity->vx += m_MoveSpeed * deltaTime;
                    moving = true;
                }

                if (m_pComponentManager->hasComponent<Transform>(entity))
                {
                    // The MovementSystem will handle the position update based on velocity.
                }

                // Optional: log when moving
                if (moving)
                {
                    // LOG_DEBUG("Player moving: vx={}, vy={}", velocity->vx, velocity->vy);
                }
            }
        }
    }
}