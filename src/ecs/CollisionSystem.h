#pragma once

#include "System.h"
#include "../core/world/World.h"

#include <vector>

namespace StrixVerse
{
    namespace ECS
    {
        // -----------------------------------------------------------------
        // CollisionSystem
        //
        // Stops colliders passing through blocking tiles or leaving the world.
        //
        // It runs *before* MovementSystem and clamps each velocity component to
        // the largest step that stays clear, so MovementSystem remains the only
        // thing that integrates position and an entity always lands flush
        // against what stopped it. Resolving X before Y is what lets an entity
        // slide along a wall instead of sticking to it.
        //
        // A tile blocks when it exists and reports IsSolid(). Change Tile's
        // constructor to change what is solid - this system deliberately holds
        // no opinion of its own. A null tile is empty space, so a generator
        // carves rooms by leaving tiles unset.
        //
        // It also reports whether each collider is standing on something, in
        // ColliderComponent::grounded, by probing a short way below the
        // resolved position. PlayerSystem reads that to decide whether a jump
        // is allowed. The probe is explicit rather than inferred from "the Y
        // step was clamped" so that a player resting perfectly still - whose
        // clamped step is zero either way - still reads as grounded.
        // -----------------------------------------------------------------
        class CollisionSystem : public System
        {
        public:
            void init(EntityManager* entityManager, ComponentManager* componentManager) override;
            void update(const std::vector<Entity>& entities, float dt) override;

            // The world to collide against. Null disables collision entirely,
            // which is the state before a generator has filled one in.
            void SetWorld(StrixVerse::World::World* world) { m_World = world; }
            const StrixVerse::World::World* GetWorld() const { return m_World; }

            // Must match the tile size the renderer draws with.
            void SetTileSize(float pixels);
            float GetTileSize() const { return m_TileSize; }

            // True when the given box overlaps a blocking tile or the world
            // edge. Callers use this to pick a spawn that is not inside a wall.
            bool IsAreaBlocked(float left, float top, float width, float height);

        private:
            // True when this tile column stops movement.
            bool Blocks(int tileX, int tileY);

            // Largest horizontal / vertical step from the given box that stays
            // clear, in the same direction and no larger than `delta`.
            float ClampX(float left, float right, float top, float bottom, float delta);
            float ClampY(float left, float right, float top, float bottom, float delta);

            StrixVerse::World::World* m_World = nullptr;

            float m_TileSize = 32.0f;
        };
    }
}
