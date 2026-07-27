#include "MovementSystem.h"
#include "ComponentManager.h"

namespace StrixVerse
{
    namespace ECS
    {
        void MovementSystem::update(const std::vector<Entity>& entities, float dt)
        {
            for (Entity entity : entities)
            {
                // Get the transform and velocity components.
                Transform& transform = m_pComponentManager->getComponent<Transform>(entity);
                Velocity& velocity = m_pComponentManager->getComponent<Velocity>(entity);

                // Update position based on velocity and delta time.
                transform.position.x += velocity.vx * dt;
                transform.position.y += velocity.vy * dt;
            }
        }
    }
}