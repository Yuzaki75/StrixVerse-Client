#include "PlayerSystem.h"

#include <algorithm>

#include "../core/Logger.h"
#include "../core/Window.h"
#include "ColliderComponent.h"
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

            // Require PlayerComponent, InputComponent, and VelocityComponent.
            // ColliderComponent is deliberately not in the signature: it is
            // read when present, for the grounded flag, but a player without
            // one should still be able to walk rather than silently drop out
            // of this system entirely.
            setSignature<PlayerComponent, InputComponent, VelocityComponent>();
            LOG_INFO("PlayerSystem: Initialized");
        }

        void PlayerSystem::update(const std::vector<Entity> &entities, float deltaTime)
        {
            // Horizontal is an intent in pixels per second and MovementSystem
            // owns the integration, so deltaTime must not be applied to it -
            // doing so scaled the movement by dt twice and left the player
            // effectively motionless. Gravity is different: it is an
            // acceleration, so it is integrated into the velocity here and
            // MovementSystem integrates that velocity into a position.
            for (Entity entity : entities)
            {
                auto *input = m_pComponentManager->getComponent<InputComponent>(entity);
                auto *velocity = m_pComponentManager->getComponent<VelocityComponent>(entity);

                if (!input || !velocity)
                    continue;

                auto *collider = m_pComponentManager->getComponent<ColliderComponent>(entity);
                const bool grounded = collider && collider->grounded;

                // --- Horizontal ------------------------------------------------
                float x = 0.0f;

                if (input->keys.test(SDL_SCANCODE_A) || input->keys.test(SDL_SCANCODE_LEFT))
                    x -= 1.0f;
                if (input->keys.test(SDL_SCANCODE_D) || input->keys.test(SDL_SCANCODE_RIGHT))
                    x += 1.0f;

                velocity->vx = x * m_MoveSpeed * m_SpeedMultiplier;

                if (!m_GravityEnabled)
                {
                    // No world to fall onto. Hold still vertically rather than
                    // accelerating into nothing.
                    velocity->vy = 0.0f;
                    continue;
                }

                // --- Jump ------------------------------------------------------
                // Space is the jump key; W and Up are accepted too, because
                // they were the "move up" keys before gravity existed and the
                // muscle memory is worth keeping. InputSystem already reports
                // every key as released while a text field has focus or the
                // game is paused, so there is nothing to gate here.
                const bool jumpDown = input->keys.test(SDL_SCANCODE_SPACE) ||
                                      input->keys.test(SDL_SCANCODE_W) ||
                                      input->keys.test(SDL_SCANCODE_UP);

                bool &wasHeld = m_JumpHeld[entity.getIndex()];

                if (jumpDown && !wasHeld && grounded)
                {
                    // Screen Y runs downward, so up is negative. This is an
                    // assignment rather than an addition: adding to whatever
                    // gravity had already accumulated would make a jump taken
                    // on the first frame of a fall weaker than one taken while
                    // standing still.
                    velocity->vy = -m_JumpSpeed;

                    // The impulse landed: this is the jump event.
                    ++m_JumpsPending;
                }

                wasHeld = jumpDown;

                // --- Gravity ---------------------------------------------------
                velocity->vy += m_Gravity * deltaTime;
                velocity->vy = std::min(velocity->vy, m_MaxFallSpeed);
            }
        }
    }
}
