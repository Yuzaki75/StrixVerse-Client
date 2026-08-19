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

            // Sprites are keyed on the server's tile id from here on. The
            // vertical slice that loaded dirt alone proved what carried risk --
            // that AssetManager loads and caches the file, that the blend state
            // suits finished art, and that the UVs are the right way up -- so
            // the rest was mechanical. Loading is lazy rather than up front:
            // a world uses a handful of the eighteen, and there is no reason to
            // read files for ores nobody has dug to.
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

        Texture* TileRendererSystem::GetTileTextureForId(std::uint8_t serverId)
        {
            // Remembered, including misses: a null entry means "looked, found
            // nothing", so a tile with no art costs one failed load and not one
            // per frame.
            if (const auto cached = m_TileSprites.find(serverId); cached != m_TileSprites.end())
            {
                return cached->second.get();
            }

            // The art ships named by the id it belongs to, so this table is the
            // filenames rather than a second opinion about what a tile is.
            // 14 is absent from the set; anything not listed falls back to the
            // flat colour for its Type.
            static const std::unordered_map<std::uint8_t, const char*> kSpriteForId = {
                {1,  "001_dirt"},        {2,  "002_stone"},      {3,  "003_grass"},
                {4,  "004_wood"},        {5,  "005_leaves"},     {6,  "006_bedrock"},
                {7,  "007_water"},       {8,  "008_torch"},      {9,  "009_chest"},
                {10, "010_door"},        {11, "011_copper_ore"}, {12, "012_lantern"},
                {13, "013_lava"},        {15, "015_coal_ore"},   {16, "016_iron_ore"},
                {17, "017_gold_ore"},    {18, "018_diamond_ore"},{19, "019_sapling"},
            };

            const auto named = kSpriteForId.find(serverId);
            if (named == kSpriteForId.end())
            {
                m_TileSprites[serverId] = nullptr;
                return nullptr;
            }

            std::shared_ptr<Texture> sprite;

            if (auto assets = ServiceLocator::Get<AssetManager>())
            {
                // LoadTexture, not GetTexture: the latter only consults the
                // cache. Mipmaps off, because these are 32x32 pixel art drawn
                // at 1:1 and minification filtering turns crisp pixels to mush.
                sprite = assets->LoadTexture(std::string("assets/tiles/") + named->second + ".png",
                                             false, false);
            }

            if (!sprite)
            {
                LOG_WARN(std::string("TileRendererSystem: assets/tiles/") + named->second +
                         ".png did not load; that tile keeps its flat colour");
            }

            Texture* raw = sprite.get();
            m_TileSprites[serverId] = std::move(sprite);
            return raw;
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

                        // The id decides the sprite; the Type is only the
                        // fallback colour for an id with no art.
                        Texture *texture = GetTileTextureForId(tile->GetServerId());
                        if (!texture)
                            texture = GetTileTexture(tile->GetType());
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
