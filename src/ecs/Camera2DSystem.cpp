#include "Camera2DSystem.h"
#include "EntityManager.h"
#include "ComponentManager.h"
#include "Camera2DComponent.h"
#include "TransformComponent.h"
#include "Core/ServiceLocator.h"
#include "Core/Engine.h"
#include "Graphics/Camera2D.h"
#include "glm/gtc/matrix_transform.hpp"

namespace StrixVerse
{
    namespace ECS
    {
        void Camera2DSystem::init(EntityManager *entityManager, ComponentManager *componentManager)
        {
            System::init(entityManager, componentManager);

            // Set the signature - we're interested in entities with both Camera2DComponent and Transform
            setSignature<Camera2DComponent, Transform>();
        }

        void Camera2DSystem::update(const std::vector<Entity> &entities, float deltaTime)
        {
            // Camera follow is snapped rather than smoothed, so the timestep is
            // unused for now.
            (void)deltaTime;

            // Get the engine to access the main camera
            auto engine = ServiceLocator::Get<Engine>();
            if (!engine)
            {
                return;
            }

            // Get the main camera from the engine
            auto &camera = engine->GetCamera(); // Assuming Engine has a GetCamera() method

            // Process all entities that have both Camera2DComponent and Transform
            auto entityManager = m_pEntityManager;
            auto componentManager = m_pComponentManager;

            for (Entity entity : entities)
            {
                // Skip if entity doesn't have both components (shouldn't happen with proper signature, but safe)
                if (!componentManager->hasComponent<Camera2DComponent>(entity) ||
                    !componentManager->hasComponent<Transform>(entity))
                {
                    continue;
                }

                // Get the camera component and transform
                auto *cameraComp = componentManager->getComponent<Camera2DComponent>(entity);
                auto *transform = componentManager->getComponent<Transform>(entity);
                if (!cameraComp || !transform)
                {
                    continue;
                }

                // If this camera should follow a target
                if (cameraComp->followTarget && cameraComp->targetEntity != NULL_ENTITY)
                {
                    // Check if the target entity exists and has a Transform component
                    if (entityManager->isValid(cameraComp->targetEntity) &&
                        componentManager->hasComponent<Transform>(cameraComp->targetEntity))
                    {
                        auto *targetTransform = componentManager->getComponent<Transform>(cameraComp->targetEntity);
                        if (!targetTransform)
                        {
                            continue;
                        }

                        // Set camera position to target position with offset
                        camera.SetPosition({targetTransform->position.x + cameraComp->offsetX,
                                            targetTransform->position.y + cameraComp->offsetY});
                    }
                }
                else
                {
                    // Camera follows its own transform
                    camera.SetPosition({transform->position.x, transform->position.y});
                }

                // Apply zoom and rotation
                camera.SetZoom(cameraComp->zoom);
                camera.SetRotation(cameraComp->rotation);
            }
        }
    }
}