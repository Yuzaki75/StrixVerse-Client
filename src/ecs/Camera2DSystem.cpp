#include "Camera2DSystem.h"
#include "EntityManager.h"
#include "ComponentManager.h"
#include "Camera2DComponent.h"
#include "TransformComponent.h"
#include "Core/ServiceLocator.h"
#include "Core/Engine.h"
#include "Graphics/Camera2D.h"
#include "core/Logger.h"
#include "glm/gtc/matrix_transform.hpp"

#include <cmath>
#include <string>

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

        void Camera2DSystem::SetWorldBounds(float widthInPixels, float heightInPixels)
        {
            m_WorldWidth  = widthInPixels  > 0.0f ? widthInPixels  : 0.0f;
            m_WorldHeight = heightInPixels > 0.0f ? heightInPixels : 0.0f;

            // Logged because "the camera is clamped" is otherwise only visible
            // by walking to an edge, and terrain can stop a player long before
            // one is reachable. This line proves the bounds arrived.
            if (HasWorldBounds())
            {
                LOG_INFO("Camera2DSystem: world bounds set to " +
                         std::to_string(static_cast<int>(m_WorldWidth)) + "x" +
                         std::to_string(static_cast<int>(m_WorldHeight)) + " px");
            }
            else
            {
                LOG_INFO("Camera2DSystem: world bounds cleared; the camera is free");
            }
        }

        void Camera2DSystem::SelfTest()
        {
            // The clamp is pure arithmetic over three numbers, so it can be
            // checked outright rather than inferred from a screenshot. Modelled
            // on Totp::SelfTest: run once at startup, report per case, and say
            // plainly whether it passed.
            //
            // A 3584x2048 world seen through a 1280x720 window: the centre may
            // travel x 640..2944 and y 360..1688.
            struct Case
            {
                const char* what;
                float desired;
                float visible;
                float world;
                float expected;
            };

            constexpr Case cases[] = {
                {"interior is untouched",        1472.0f, 1280.0f, 3584.0f, 1472.0f},
                {"left edge pins to half-view",   100.0f, 1280.0f, 3584.0f,  640.0f},
                {"right edge pins to half-view", 3500.0f, 1280.0f, 3584.0f, 2944.0f},
                {"top edge pins to half-view",     32.0f,  720.0f, 2048.0f,  360.0f},
                {"bottom edge pins to half-view",2040.0f,  720.0f, 2048.0f, 1688.0f},
                {"exactly on the limit holds",    640.0f, 1280.0f, 3584.0f,  640.0f},
                {"world narrower than view centres", 10.0f, 1280.0f, 800.0f,  400.0f},
            };

            Camera2DSystem probe;
            int failures = 0;

            for (const Case& c : cases)
            {
                const float got = probe.ClampAxis(c.desired, c.visible, c.world);

                if (std::fabs(got - c.expected) > 0.01f)
                {
                    ++failures;
                    LOG_ERROR(std::string("Camera2DSystem self-test FAILED: ") + c.what +
                              " - wanted " + std::to_string(c.expected) +
                              ", got " + std::to_string(got));
                }
            }

            if (failures == 0)
            {
                LOG_INFO("Camera2DSystem: self-test passed (7 clamp cases)");
            }
        }

        float Camera2DSystem::ClampAxis(float desired, float visible, float worldSize) const
        {
            // A view wider than the world has no legal edge to sit against, so
            // the world is centred and the letterboxing falls evenly on both
            // sides rather than all on one.
            if (visible >= worldSize)
                return worldSize * 0.5f;

            const float half = visible * 0.5f;

            if (desired < half)
                return half;
            if (desired > worldSize - half)
                return worldSize - half;

            return desired;
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

                // Where the camera would like to be, before the world gets a
                // say. Both branches produce a desired centre rather than
                // writing the camera directly, so the clamp below is applied
                // once and cannot be bypassed by whichever branch was taken.
                glm::vec2 desired{transform->position.x, transform->position.y};

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

                        desired = {targetTransform->position.x + cameraComp->offsetX,
                                   targetTransform->position.y + cameraComp->offsetY};
                    }
                }

                // Zoom first: how much world the view covers depends on it, so
                // clamping against a stale zoom would let the edge creep in for
                // one frame after any zoom change.
                camera.SetZoom(cameraComp->zoom);
                camera.SetRotation(cameraComp->rotation);

                if (HasWorldBounds())
                {
                    // The viewport is in window pixels and the camera maps one
                    // world unit to one of them at zoom 1, so the world extent
                    // on screen is the viewport divided by the zoom.
                    const glm::vec2 viewport = camera.GetViewport();
                    const float zoom = cameraComp->zoom > 0.0f ? cameraComp->zoom : 1.0f;

                    const float visibleWidth  = viewport.x / zoom;
                    const float visibleHeight = viewport.y / zoom;

                    desired.x = ClampAxis(desired.x, visibleWidth,  m_WorldWidth);
                    desired.y = ClampAxis(desired.y, visibleHeight, m_WorldHeight);
                }

                camera.SetPosition(desired);
            }
        }
    }
}