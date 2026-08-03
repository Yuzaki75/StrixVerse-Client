#include "TileRendererSystem.h"
#include "EntityManager.h"
#include "ComponentManager.h"
#include "../core/Logger.h"
#include "../core/ServiceLocator.h"
#include "../graphics/Renderer.h"
#include "../graphics/Texture.h"
#include "../core/AssetManager.h"
#include "TransformComponent.h"
#include "SpriteComponent.h"

#include <algorithm>

namespace StrixVerse
{
    namespace ECS
    {
        void TileRendererSystem::init(EntityManager *entityManager, ComponentManager *componentManager)
        {
            // Store the entity and component managers
            m_pEntityManager = entityManager;
            m_pComponentManager = componentManager;

            // Require TransformComponent and SpriteComponent (we'll add these to our tile entities)
            setSignature<Transform, SpriteComponent>();

            // Get the asset manager for texture creation
            auto assetManager = ServiceLocator::Get<AssetManager>();
            if (!assetManager)
            {
                LOG_ERROR("TileRendererSystem: AssetManager not available");
                return;
            }

            // Create textures for each tile type
            m_TileTextures[StrixVerse::World::Tile::Type::Grass] = CreateTileTexture(StrixVerse::World::Tile::Type::Grass);
            m_TileTextures[StrixVerse::World::Tile::Type::Dirt] = CreateTileTexture(StrixVerse::World::Tile::Type::Dirt);
            m_TileTextures[StrixVerse::World::Tile::Type::Stone] = CreateTileTexture(StrixVerse::World::Tile::Type::Stone);
            m_TileTextures[StrixVerse::World::Tile::Type::Water] = CreateTileTexture(StrixVerse::World::Tile::Type::Water);
            m_TileTextures[StrixVerse::World::Tile::Type::Sand] = CreateTileTexture(StrixVerse::World::Tile::Type::Sand);

            LOG_INFO("TileRendererSystem: Initialized");
        }

        void TileRendererSystem::update(const std::vector<Entity> &entities, float dt)
        {
            // In a real game, we might want to animate tiles (like water) or handle dynamic changes
            // For now, this is a placeholder
            (void)entities;
            (void)dt;
        }

        void TileRendererSystem::render(const std::vector<Entity> &entities)
        {
            // This system doesn't render directly - it creates entities that are rendered by the RenderSystem
            // The RenderSystem will handle all entities with Transform and Sprite components
        }

        uint32_t TileRendererSystem::CreateTileTexture(StrixVerse::World::Tile::Type type)
        {
            // Create a 16x16 pixel texture with a solid color for each tile type
            const int textureSize = 16;
            std::vector<unsigned char> pixels(textureSize * textureSize * 4); // RGBA

            // Set color based on tile type
            unsigned char r = 0, g = 0, b = 0, a = 255;

            switch (type)
            {
            case StrixVerse::World::Tile::Type::Grass:
                r = 34;
                g = 139;
                b = 34; // Forest green
                break;
            case StrixVerse::World::Tile::Type::Dirt:
                r = 139;
                g = 69;
                b = 19; // Saddle brown
                break;
            case StrixVerse::World::Tile::Type::Stone:
                r = 128;
                g = 128;
                b = 128; // Gray
                break;
            case StrixVerse::World::Tile::Type::Water:
                r = 30;
                g = 144;
                b = 255; // Dodger blue
                break;
            case StrixVerse::World::Tile::Type::Sand:
                r = 238;
                g = 203;
                b = 173; // Peach puff
                break;
            default:
                r = 255;
                g = 0;
                b = 255; // Magenta for error
                a = 255;
                break;
            }

            // Fill the texture with the color
            for (int i = 0; i < textureSize * textureSize; ++i)
            {
                pixels[i * 4 + 0] = r; // R
                pixels[i * 4 + 1] = g; // G
                pixels[i * 4 + 2] = b; // B
                pixels[i * 4 + 3] = a; // A
            }

            // Create the texture using the asset manager
            auto assetManager = ServiceLocator::Get<AssetManager>();
            if (assetManager)
            {
                // Use LoadTexture with data - but this expects file paths.
                // Create a unique name for the procedural texture
                std::string textureName = "tile_" + std::to_string(static_cast<int>(type));
                // Check if already cached
                auto cachedTexture = assetManager->GetTexture(textureName);
                if (cachedTexture)
                {
                    return cachedTexture->GetRendererID();
                }
                // Create texture manually and store in asset manager
                auto texture = std::make_shared<Texture>();
                texture->Create(textureSize, textureSize, pixels.data(), 4, false, false);
                // We need to register this texture - use LoadTexture path as workaround
                // For now, store the renderer ID and return it
                // The AssetManager's public API only accepts file paths,
                // so we manage this texture ourselves.
                m_OwnedTextures.push_back(texture);
                return texture->GetRendererID();
            }
            else
            {
                auto texture = std::make_shared<Texture>();
                texture->Create(textureSize, textureSize, pixels.data(), 4, false, false);
                m_OwnedTextures.push_back(texture);
                return texture->GetRendererID();
            }
        }

        uint32_t TileRendererSystem::GetTileTexture(StrixVerse::World::Tile::Type type)
        {
            auto it = m_TileTextures.find(type);
            if (it != m_TileTextures.end())
            {
                return it->second;
            }

            // Default to magenta if not found (error color)
            LOG_WARN("TileRendererSystem: Texture not found for tile type " + std::to_string(static_cast<int>(type)) + ", using default");
            return CreateTileTexture(StrixVerse::World::Tile::Type::Grass); // Fallback
        }

        void TileRendererSystem::EnsureTileEntity(int worldX, int worldY, int worldZ)
        {
            if (!m_World || !m_pEntityManager || !m_pComponentManager)
            {
                LOG_WARN("TileRendererSystem: World or managers not set");
                return;
            }

            // Check if we already have an entity for this position
            auto itX = m_TileEntities.find(worldX);
            if (itX != m_TileEntities.end())
            {
                auto itY = itX->second.find(worldY);
                if (itY != itX->second.end())
                {
                    auto itZ = itY->second.find(worldZ);
                    if (itZ != itY->second.end())
                    {
                        // Entity already exists, just update it if needed
                        Entity entity = itZ->second;

                        // Get the tile to see if it changed
                        auto tile = m_World->GetTileAt(worldX, worldY, worldZ);
                        if (tile)
                        {
                            auto *sprite = m_pComponentManager->getComponent<SpriteComponent>(entity);
                            if (sprite)
                            {
                                // Update the texture if it changed
                                uint32_t textureID = GetTileTexture(tile->GetType());
                                if (sprite->textureID != textureID)
                                {
                                    sprite->textureID = textureID;
                                }
                            }
                        }
                        return;
                    }
                }
            }

            // No entity exists, create one
            auto tile = m_World->GetTileAt(worldX, worldY, worldZ);
            if (!tile)
            {
                return;
            }

            // Create entity
            Entity entity = m_pEntityManager->createEntity();

            // Add transform component
            Transform transform;
            transform.position.x = (float)worldX * m_TileSize;
            transform.position.y = (float)worldY * m_TileSize;
            transform.rotation = 0.0f;
            transform.scale = {m_TileSize, m_TileSize};
            m_pComponentManager->addComponent<Transform>(entity, transform);

            // Add sprite component
            SpriteComponent sprite;
            sprite.textureID = GetTileTexture(tile->GetType());
            sprite.r = 1.0f;
            sprite.g = 1.0f;
            sprite.b = 1.0f;
            sprite.a = 1.0f;
            sprite.layer = 0; // Background layer
            m_pComponentManager->addComponent<SpriteComponent>(entity, sprite);

            // Store the entity reference
            m_TileEntities[worldX][worldY][worldZ] = entity;
        }

        void TileRendererSystem::RemoveTileEntity(int worldX, int worldY, int worldZ)
        {
            auto itX = m_TileEntities.find(worldX);
            if (itX != m_TileEntities.end())
            {
                auto itY = itX->second.find(worldY);
                if (itY != itX->second.end())
                {
                    auto itZ = itY->second.find(worldZ);
                    if (itZ != itY->second.end())
                    {
                        Entity entity = itZ->second;
                        m_pEntityManager->destroyEntity(entity);
                        itY->second.erase(itZ);

                        // Clean up empty maps
                        if (itY->second.empty())
                        {
                            itX->second.erase(itY);
                        }
                        if (itX->second.empty())
                        {
                            m_TileEntities.erase(itX);
                        }
                    }
                }
            }
        }
    }
}