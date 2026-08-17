#include "TileRendererSystem.h"

#include "ComponentManager.h"
#include "EntityManager.h"
#include "../core/AssetManager.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../core/ServiceLocator.h"
#include "../graphics/Camera2D.h"
#include "../graphics/SpriteBatch.h"
#include "../graphics/Texture.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace StrixVerse
{
    namespace ECS
    {
        namespace
        {
            // Tile art is a flat colour for now; the texture only has to be big
            // enough to sample cleanly, and the draw call sizes it to the tile.
            constexpr int kTileTexturePixels = 16;

            struct TileColor
            {
                unsigned char r, g, b, a;
            };

            TileColor ColorFor(StrixVerse::World::Tile::Type type)
            {
                switch (type)
                {
                case StrixVerse::World::Tile::Type::Grass: return {34, 139, 34, 255};
                case StrixVerse::World::Tile::Type::Dirt:  return {139, 69, 19, 255};
                case StrixVerse::World::Tile::Type::Stone: return {112, 128, 144, 255};
                case StrixVerse::World::Tile::Type::Water: return {30, 110, 190, 255};
                case StrixVerse::World::Tile::Type::Sand:  return {237, 201, 145, 255};
                default:                                   return {255, 0, 255, 255};
                }
            }
        }

        void TileRendererSystem::init(EntityManager *entityManager, ComponentManager *componentManager)
        {
            System::init(entityManager, componentManager);

            // No signature is declared: this system draws world data and ignores
            // the entity list it is handed.

            for (int i = 0; i < static_cast<int>(StrixVerse::World::Tile::Type::Count); ++i)
            {
                const auto type = static_cast<StrixVerse::World::Tile::Type>(i);
                m_TileTextures[type] = CreateTileTexture(type);
            }

            // Vertical slice: one real sprite alongside four generated colours.
            //
            // The art the server ships is 32x32, named by tile id, and matches
            // kTileSize exactly. Before keying the whole renderer on tile id --
            // which means carrying that id through the world build instead of
            // collapsing it onto five types -- this proves the parts that
            // actually carry risk: that AssetManager loads and caches the file,
            // that the blend state suits finished art, and that the UVs are the
            // right way up after the loader stopped flipping images.
            //
            // If dirt draws correctly next to flat-coloured stone, the path is
            // proven and the rest is mechanical. If it does not, this is one
            // line to remove.
            if (auto assets = ServiceLocator::Get<AssetManager>())
            {
                // LoadTexture, not GetTexture: the latter only consults the
                // cache and returns null for anything not already loaded.
                // Mipmaps off: these are 32x32 pixel art drawn at 1:1, and
                // minification filtering is what turns crisp pixels to mush.
                if (auto sprite = assets->LoadTexture("assets/tiles/001_dirt.png", false, false))
                {
                    m_TileTextures[StrixVerse::World::Tile::Type::Dirt] = std::move(sprite);
                    LOG_INFO("TileRendererSystem: dirt is using the server sprite");
                }
                else
                {
                    LOG_WARN("TileRendererSystem: assets/tiles/001_dirt.png did not load; "
                             "keeping the generated colour");
                }
            }

            LOG_INFO("TileRendererSystem: initialised");
        }

        void TileRendererSystem::SetTileSize(float pixels)
        {
            if (pixels > 0.0f)
                m_TileSize = pixels;
        }

        std::shared_ptr<Texture> TileRendererSystem::CreateTileTexture(
            StrixVerse::World::Tile::Type type) const
        {
            const TileColor color = ColorFor(type);

            std::vector<unsigned char> pixels(
                static_cast<size_t>(kTileTexturePixels) * kTileTexturePixels * 4);

            for (size_t i = 0; i < pixels.size(); i += 4)
            {
                pixels[i + 0] = color.r;
                pixels[i + 1] = color.g;
                pixels[i + 2] = color.b;
                pixels[i + 3] = color.a;
            }

            auto texture = std::make_shared<Texture>();
            texture->Create(kTileTexturePixels, kTileTexturePixels, pixels.data(), 4, false, false);

            return texture;
        }

        Texture *TileRendererSystem::GetTileTexture(StrixVerse::World::Tile::Type type)
        {
            const auto it = m_TileTextures.find(type);
            return it != m_TileTextures.end() ? it->second.get() : nullptr;
        }

        void TileRendererSystem::render(const std::vector<Entity> &entities)
        {
            // Tiles are world data; the entity list is not this system's input.
            (void)entities;

            if (!m_World)
                return;

            const int worldWidth  = m_World->GetWidthInTiles();
            const int worldHeight = m_World->GetHeightInTiles();
            const int worldDepth  = m_World->GetDepthInTiles();

            if (worldWidth <= 0 || worldHeight <= 0 || worldDepth <= 0)
                return;

            auto spriteBatch = ServiceLocator::Get<SpriteBatch>();
            auto engine      = ServiceLocator::Get<Engine>();

            if (!spriteBatch || !engine)
                return;

            const Camera2D &camera = engine->GetCamera();

            const glm::vec2 viewport = camera.GetViewport();
            const glm::vec2 centre   = camera.GetPosition();
            const float     zoom     = camera.GetZoom() > 0.0f ? camera.GetZoom() : 1.0f;

            // Half the visible extent in world units, plus a tile of margin so
            // partly visible edge tiles are still drawn.
            const float halfWidth  = (viewport.x * 0.5f) / zoom + m_TileSize;
            const float halfHeight = (viewport.y * 0.5f) / zoom + m_TileSize;

            const int minX = std::max(0,
                static_cast<int>(std::floor((centre.x - halfWidth) / m_TileSize)));
            const int minY = std::max(0,
                static_cast<int>(std::floor((centre.y - halfHeight) / m_TileSize)));
            const int maxX = std::min(worldWidth - 1,
                static_cast<int>(std::ceil((centre.x + halfWidth) / m_TileSize)));
            const int maxY = std::min(worldHeight - 1,
                static_cast<int>(std::ceil((centre.y + halfHeight) / m_TileSize)));

            if (minX > maxX || minY > maxY)
                return;

            spriteBatch->Begin();

            // Back to front, so a foreground layer covers the one behind it.
            for (int z = 0; z < worldDepth; ++z)
            {
                for (int y = minY; y <= maxY; ++y)
                {
                    for (int x = minX; x <= maxX; ++x)
                    {
                        const auto tile = m_World->GetTileAt(x, y, z);
                        if (!tile)
                            continue;

                        Texture *texture = GetTileTexture(tile->GetType());
                        if (!texture)
                            continue;

                        spriteBatch->Draw(*texture,
                                          static_cast<float>(x) * m_TileSize,
                                          static_cast<float>(y) * m_TileSize,
                                          m_TileSize, m_TileSize,
                                          1.0f, 1.0f, 1.0f, 1.0f);
                    }
                }
            }

            spriteBatch->End();
        }
    }
}
