#pragma once

#include "System.h"

namespace StrixVerse
{
    namespace ECS
    {
        class Camera2DSystem : public System
        {
        public:
            void init(EntityManager *entityManager, ComponentManager *componentManager) override;
            void update(const std::vector<Entity> &entities, float deltaTime) override;

        private:
            // Reference to the main camera (we'll get this from the Engine or Game)
            // For now, we'll store a reference to the camera that we'll update
            // In a more complete implementation, this might come from a CameraManager
            // or be passed in during initialization
        };
    }
}