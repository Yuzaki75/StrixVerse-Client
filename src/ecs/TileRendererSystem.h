#pragma once

#include "System.h"
#include "../core/world/Tile.h"
#include "../core/world/World.h"

#include <memory>
#include <cstdint>
#include <unordered_map>
#include <vector>

class Texture;

namespace StrixVerse
{
    namespace ECS
    {
        // -----------------------------------------------------------------
        // TileRendererSystem
        //
        // Draws the world's tiles straight into the SpriteBatch.
        //
        // Tiles are world data, not entities. An entity per tile is not an
        // option: a 4x4-chunk world is 64 x 64 x 4 = 16,384 tiles against
        // EntityManager's 10,000-entity ceiling, and none of them need
        // per-entity behaviour. Only the tiles inside the camera's view are
        // submitted, so a large world costs nothing until it is on screen.
        //
        // Register this before RenderSystem: both flush the same SpriteBatch,
        // so registration order is what puts tiles underneath the entities
        // standing on them.
        // -----------------------------------------------------------------
        class TileRendererSystem : public System
        {
        public:
            void init(EntityManager* entityManager, ComponentManager* componentManager) override;
            void render(const std::vector<Entity>& entities) override;

            // The world to draw. Null means there is nothing to draw, which is
            // the state until a generator or the server fills one in.
            void SetWorld(StrixVerse::World::World* world) { m_World = world; }
            const StrixVerse::World::World* GetWorld() const { return m_World; }

            // Size of one tile on screen, in world units (pixels at zoom 1).
            void SetTileSize(float pixels);
            float GetTileSize() const { return m_TileSize; }

        private:
            std::shared_ptr<Texture> CreateTileTexture(StrixVerse::World::Tile::Type type) const;
            Texture* GetTileTexture(StrixVerse::World::Tile::Type type);

            // The sprite for a server tile id, or null if that id has no art.
            // Loaded once on first use and cached, including the misses, so a
            // missing file costs one failed load rather than one per frame.
            Texture* GetTileTextureForId(std::uint8_t serverId);

            StrixVerse::World::World* m_World = nullptr;

            // One solid-colour texture per tile type, owned for the lifetime of
            // the system so the batch can reference them freely.
            std::unordered_map<StrixVerse::World::Tile::Type,
                               std::shared_ptr<Texture>> m_TileTextures;

            // Keyed on the server's tile id. A null entry is a remembered miss.
            std::unordered_map<std::uint8_t, std::shared_ptr<Texture>> m_TileSprites;

            float m_TileSize = 32.0f;
        };
    }
}
