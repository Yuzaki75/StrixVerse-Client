#include "CollisionSystem.h"

#include "ColliderComponent.h"
#include "ComponentManager.h"
#include "EntityManager.h"
#include "TransformComponent.h"
#include "VelocityComponent.h"
#include "../core/Logger.h"

#include <algorithm>
#include <cmath>

namespace StrixVerse
{
    namespace ECS
    {
        namespace
        {
            // Keeps a box that ends exactly on a tile boundary from counting as
            // being inside the next tile along.
            constexpr float kEdgeEpsilon = 0.001f;

            int TileIndex(float worldCoordinate, float tileSize)
            {
                return static_cast<int>(std::floor(worldCoordinate / tileSize));
            }
        }

        void CollisionSystem::init(EntityManager *entityManager, ComponentManager *componentManager)
        {
            System::init(entityManager, componentManager);

            setSignature<Transform, VelocityComponent, ColliderComponent>();

            LOG_INFO("CollisionSystem: Initialized");
        }

        void CollisionSystem::SetTileSize(float pixels)
        {
            if (pixels > 0.0f)
                m_TileSize = pixels;
        }

        bool CollisionSystem::Blocks(int tileX, int tileY)
        {
            if (!m_World)
                return false;

            const int width  = m_World->GetWidthInTiles();
            const int height = m_World->GetHeightInTiles();

            // Outside the world blocks; that is what keeps an entity inside it.
            if (tileX < 0 || tileY < 0 || tileX >= width || tileY >= height)
                return true;

            // A column blocks if any of its depth layers does.
            const int depth = m_World->GetDepthInTiles();

            for (int z = 0; z < depth; ++z)
            {
                const auto tile = m_World->GetTileAt(tileX, tileY, z);
                if (tile && !tile->IsWalkable())
                    return true;
            }

            return false;
        }

        bool CollisionSystem::IsAreaBlocked(float left, float top, float width, float height)
        {
            if (!m_World || m_World->GetWidthInTiles() <= 0)
                return false;

            const int x0 = TileIndex(left, m_TileSize);
            const int x1 = TileIndex(left + width - kEdgeEpsilon, m_TileSize);
            const int y0 = TileIndex(top, m_TileSize);
            const int y1 = TileIndex(top + height - kEdgeEpsilon, m_TileSize);

            for (int y = y0; y <= y1; ++y)
                for (int x = x0; x <= x1; ++x)
                    if (Blocks(x, y))
                        return true;

            return false;
        }

        float CollisionSystem::ClampX(float left, float right, float top, float bottom, float delta)
        {
            if (delta == 0.0f)
                return 0.0f;

            const int y0 = TileIndex(top, m_TileSize);
            const int y1 = TileIndex(bottom - kEdgeEpsilon, m_TileSize);

            if (delta > 0.0f)
            {
                const int from = TileIndex(right - kEdgeEpsilon, m_TileSize);
                const int to   = TileIndex(right + delta - kEdgeEpsilon, m_TileSize);

                for (int x = from; x <= to; ++x)
                    for (int y = y0; y <= y1; ++y)
                        if (Blocks(x, y))
                            return std::max(0.0f, static_cast<float>(x) * m_TileSize - right);

                return delta;
            }

            const int from = TileIndex(left, m_TileSize);
            const int to   = TileIndex(left + delta, m_TileSize);

            for (int x = from; x >= to; --x)
                for (int y = y0; y <= y1; ++y)
                    if (Blocks(x, y))
                        return std::min(0.0f, static_cast<float>(x + 1) * m_TileSize - left);

            return delta;
        }

        float CollisionSystem::ClampY(float left, float right, float top, float bottom, float delta)
        {
            if (delta == 0.0f)
                return 0.0f;

            const int x0 = TileIndex(left, m_TileSize);
            const int x1 = TileIndex(right - kEdgeEpsilon, m_TileSize);

            if (delta > 0.0f)
            {
                const int from = TileIndex(bottom - kEdgeEpsilon, m_TileSize);
                const int to   = TileIndex(bottom + delta - kEdgeEpsilon, m_TileSize);

                for (int y = from; y <= to; ++y)
                    for (int x = x0; x <= x1; ++x)
                        if (Blocks(x, y))
                            return std::max(0.0f, static_cast<float>(y) * m_TileSize - bottom);

                return delta;
            }

            const int from = TileIndex(top, m_TileSize);
            const int to   = TileIndex(top + delta, m_TileSize);

            for (int y = from; y >= to; --y)
                for (int x = x0; x <= x1; ++x)
                    if (Blocks(x, y))
                        return std::min(0.0f, static_cast<float>(y + 1) * m_TileSize - top);

            return delta;
        }

        void CollisionSystem::update(const std::vector<Entity> &entities, float dt)
        {
            // With no world there is nothing to collide with, and clamping to a
            // zero-sized world would freeze every entity at the origin.
            if (!m_World || m_World->GetWidthInTiles() <= 0 || dt <= 0.0f)
                return;

            for (Entity entity : entities)
            {
                auto *transform = m_pComponentManager->getComponent<Transform>(entity);
                auto *velocity  = m_pComponentManager->getComponent<VelocityComponent>(entity);
                auto *collider  = m_pComponentManager->getComponent<ColliderComponent>(entity);

                if (!transform || !velocity || !collider || !collider->enabled)
                    continue;

                if (velocity->vx == 0.0f && velocity->vy == 0.0f)
                    continue;

                const float left   = transform->position.x;
                const float top    = transform->position.y;
                const float right  = left + collider->width;
                const float bottom = top + collider->height;

                // Horizontal first, then vertical from the corrected position,
                // so a diagonal run into a wall slides along it.
                const float allowedX = ClampX(left, right, top, bottom, velocity->vx * dt);
                velocity->vx = allowedX / dt;

                const float allowedY = ClampY(left + allowedX, right + allowedX,
                                              top, bottom, velocity->vy * dt);
                velocity->vy = allowedY / dt;
            }
        }
    }
}
