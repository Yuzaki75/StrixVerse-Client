#include "PlayerSystem.h"

#include <cmath>

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
        void PlayerSystem::init(EntityManager *entityManager, ComponentManager *componentManager)
        {
            System::init(entityManager, componentManager);

            // Require PlayerComponent, InputComponent, and VelocityComponent
            setSignature<PlayerComponent, InputComponent, VelocityComponent>();
            LOG_INFO("PlayerSystem: Initialized");
        }

        void PlayerSystem::update(const std::vector<Entity> &entities, float deltaTime)
        {
            // This system produces an intent - a velocity in pixels per second.
            // MovementSystem owns the integration, so deltaTime is deliberately
            // not applied here: doing so scaled the movement by dt twice and
            // left the player effectively motionless.
            (void)deltaTime;

            for (Entity entity : entities)
            {
                auto *input = m_pComponentManager->getComponent<InputComponent>(entity);
                auto *velocity = m_pComponentManager->getComponent<VelocityComponent>(entity);

                if (!input || !velocity)
                    continue;

                float x = 0.0f;
                float y = 0.0f;

                if (input->keys.test(SDL_SCANCODE_W) || input->keys.test(SDL_SCANCODE_UP))
                    y -= 1.0f;
                if (input->keys.test(SDL_SCANCODE_S) || input->keys.test(SDL_SCANCODE_DOWN))
                    y += 1.0f;
                if (input->keys.test(SDL_SCANCODE_A) || input->keys.test(SDL_SCANCODE_LEFT))
                    x -= 1.0f;
                if (input->keys.test(SDL_SCANCODE_D) || input->keys.test(SDL_SCANCODE_RIGHT))
                    x += 1.0f;

                // Normalise so diagonals are not faster than the cardinals.
                const float length = std::sqrt(x * x + y * y);
                if (length > 0.0f)
                {
                    x /= length;
                    y /= length;
                }

                velocity->vx = x * m_MoveSpeed;
                velocity->vy = y * m_MoveSpeed;
            }
        }
    }
}