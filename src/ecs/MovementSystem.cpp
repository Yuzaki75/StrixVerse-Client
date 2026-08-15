#include "MovementSystem.h"
#include "ComponentManager.h"

namespace StrixVerse
{
    namespace ECS
    {
        void MovementSystem::init(EntityManager *entityManager, ComponentManager *componentManager)
        {
            System::init(entityManager, componentManager);

            setSignature<Transform, VelocityComponent>();
        }

        void MovementSystem::update(const std::vector<Entity> &entities, float dt)
        {
            for (Entity entity : entities)
            {
                // Get the transform and velocity components.
                auto *transform = m_pComponentManager->getComponent<Transform>(entity);
                auto *velocity = m_pComponentManager->getComponent<VelocityComponent>(entity);

                if (!transform || !velocity)
                {
                    continue;
                }

                // Update position based on velocity and delta time.
                transform->position.x += velocity->vx * dt;
                transform->position.y += velocity->vy * dt;
            }
        }
    }
}