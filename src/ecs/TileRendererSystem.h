#pragma once

#include "System.h"
#include "../core/world/Tile.h"
#include "../core/world/Chunk.h"
#include "../core/world/World.h"
#include "../graphics/Texture.h"

#include <unordered_map>
#include <vector>
#include <memory>

namespace StrixVerse
{
    namespace ECS
    {
        class TileRendererSystem : public System
        {
        public:
            void init(EntityManager* entityManager, ComponentManager* componentManager) override;
            void update(const std::vector<Entity>& entities, float dt) override;
            void render(const std::vector<Entity>& entities) override;

            // Set the world reference
            void SetWorld(StrixVerse::World::World* world) { m_World = world; }

        private:
            // Reference to the world (we'll get this from the GameScreen or Engine)
            StrixVerse::World::World* m_World = nullptr;

            // Map of tile types to texture IDs
            std::unordered_map<StrixVerse::World::Tile::Type, uint32_t> m_TileTextures;

            // Entities we've created for tiles (so we can update/remove them)
            std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, Entity>>> m_TileEntities;

            // Texture size in world units
            float m_TileSize = 1.0f;

            // Owned textures (kept alive for the lifetime of this system)
            std::vector<std::shared_ptr<class Texture>> m_OwnedTextures;

            // Create a solid color texture for a tile type
            uint32_t CreateTileTexture(StrixVerse::World::Tile::Type type);

            // Get or create a texture for a tile type
            uint32_t GetTileTexture(StrixVerse::World::Tile::Type type);

            // Create/update an entity for a tile at the given world position
            void EnsureTileEntity(int worldX, int worldY, int worldZ);

            // Remove an entity for a tile at the given world position
            void RemoveTileEntity(int worldX, int worldY, int worldZ);
        };
    }
}