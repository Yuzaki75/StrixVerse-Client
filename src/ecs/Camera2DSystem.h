#pragma once

#include "System.h"

namespace StrixVerse
{
    namespace ECS
    {
        // -----------------------------------------------------------------
        // Camera2DSystem
        //
        // Drives the Engine's single Camera2D from whichever entity carries a
        // Camera2DComponent, following its target if it has one.
        //
        // It also keeps the view inside the world. Without that the camera
        // follows the player straight past the edge of the map and half the
        // screen fills with empty background - which cost nothing while
        // movement was level and bounded, and became obvious the moment a fall
        // could carry the player down a shaft near an edge.
        // -----------------------------------------------------------------
        class Camera2DSystem : public System
        {
        public:
            void init(EntityManager *entityManager, ComponentManager *componentManager) override;
            void update(const std::vector<Entity> &entities, float deltaTime) override;

            // The rectangle the view may not leave, in world pixels, with the
            // origin at the top-left. Passing a non-positive size disables
            // clamping, which is the state before a world exists - the same
            // convention CollisionSystem uses for a null world.
            void SetWorldBounds(float widthInPixels, float heightInPixels);

            // Checks the clamp against known inputs and logs the result. Run
            // once at startup: the alternative is walking to a world edge, and
            // terrain can wedge a player long before one is reachable.
            static void SelfTest();
            void ClearWorldBounds() { m_WorldWidth = 0.0f; m_WorldHeight = 0.0f; }
            bool HasWorldBounds() const { return m_WorldWidth > 0.0f && m_WorldHeight > 0.0f; }

        private:
            // Pulls a desired camera centre back inside the world. `visible` is
            // the full width or height of the view in world units; when the
            // world is smaller than that, the axis is centred instead, because
            // there is no position that fills the view and jamming it against
            // one edge just picks a side arbitrarily.
            float ClampAxis(float desired, float visible, float worldSize) const;

            float m_WorldWidth  = 0.0f;
            float m_WorldHeight = 0.0f;
        };
    }
}