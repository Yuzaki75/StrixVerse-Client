#include "CharacterRenderSystem.h"

#include "CharacterComponent.h"
#include "ComponentManager.h"
#include "EntityManager.h"
#include "TransformComponent.h"

#include "core/AssetManager.h"
#include "core/Logger.h"
#include "core/ServiceLocator.h"
#include "graphics/CharacterPalette.h"
#include "graphics/SpriteBatch.h"
#include "graphics/Texture.h"

#include <algorithm>
#include <string>
#include <vector>

namespace StrixVerse
{
    namespace ECS
    {
        void CharacterRenderSystem::init(EntityManager* entityManager,
                                         ComponentManager* componentManager)
        {
            System::init(entityManager, componentManager);

            setSignature<Transform, CharacterComponent>();
        }

        bool CharacterRenderSystem::EnsureLayers()
        {
            if (m_Loaded)
            {
                return true;
            }
            if (m_LoadFailed)
            {
                return false;
            }

            auto assets = ServiceLocator::Get<AssetManager>();
            if (!assets)
            {
                return false;
            }

            for (int i = 0; i < static_cast<int>(CharacterPalette::Zone::Count); ++i)
            {
                const auto zone = static_cast<CharacterPalette::Zone>(i);
                const std::string path =
                    std::string("assets/character/") + CharacterPalette::LayerName(zone) + ".png";

                m_Layers[i] = assets->LoadTexture(path);
                if (!m_Layers[i])
                {
                    // All or nothing. Drawing four of six layers produces a
                    // player with no head, which is harder to diagnose than a
                    // player who is simply absent with a line in the log.
                    LOG_WARN("CharacterRenderSystem: " + path +
                             " did not load; characters will not be drawn");
                    m_LoadFailed = true;
                    for (auto& layer : m_Layers)
                    {
                        layer.reset();
                    }
                    return false;
                }
            }

            m_Loaded = true;
            LOG_INFO("CharacterRenderSystem: six character layers loaded");
            return true;
        }

        void CharacterRenderSystem::render(const std::vector<Entity>& entities)
        {
            if (!EnsureLayers())
            {
                return;
            }

            auto spriteBatch = ServiceLocator::Get<SpriteBatch>();
            if (!spriteBatch || !m_pComponentManager)
            {
                return;
            }

            struct Drawable
            {
                Transform* transform = nullptr;
                CharacterComponent* look = nullptr;
            };

            std::vector<Drawable> drawables;
            drawables.reserve(entities.size());

            for (Entity entity : entities)
            {
                if (!m_pComponentManager->hasComponent<Transform>(entity) ||
                    !m_pComponentManager->hasComponent<CharacterComponent>(entity))
                {
                    continue;
                }

                auto* transform = m_pComponentManager->getComponent<Transform>(entity);
                auto* look = m_pComponentManager->getComponent<CharacterComponent>(entity);
                if (transform && look)
                {
                    drawables.push_back({transform, look});
                }
            }

            if (drawables.empty())
            {
                return;
            }

            std::sort(drawables.begin(), drawables.end(),
                      [](const Drawable& a, const Drawable& b) {
                          return a.look->layer < b.look->layer;
                      });

            spriteBatch->Begin();

            // Zone-major rather than character-major: every character's skin is
            // drawn, then every character's trousers, and so on. Same number of
            // quads either way, but the batch only binds six textures for the
            // whole frame instead of six per player.
            //
            // Safe because the zones are disjoint masks of one silhouette, so
            // two characters can only overlap where they already overlap as
            // whole sprites -- which the sort above orders.
            for (int i = 0; i < static_cast<int>(CharacterPalette::Zone::Count); ++i)
            {
                const auto zone = static_cast<CharacterPalette::Zone>(i);
                const Texture& texture = *m_Layers[i];

                for (const Drawable& d : drawables)
                {
                    uint8_t index = 0;
                    switch (zone)
                    {
                        case CharacterPalette::Zone::Skin:     index = d.look->skin;     break;
                        case CharacterPalette::Zone::Trousers: index = d.look->trousers; break;
                        case CharacterPalette::Zone::Boots:    index = d.look->boots;    break;
                        case CharacterPalette::Zone::Shirt:    index = d.look->shirt;    break;
                        case CharacterPalette::Zone::Hair:     index = d.look->hair;     break;
                        case CharacterPalette::Zone::Eyes:     index = d.look->eyes;     break;
                        default: break;
                    }

                    float r = 1.0f, g = 1.0f, b = 1.0f;
                    CharacterPalette::ColourFloats(zone, index, r, g, b);

                    // scale IS the on-screen size in pixels, not a multiplier
                    // of the texture. That is the convention player entities
                    // already used when they were a 1x1 white texture stretched
                    // to the player's size, and keeping it means the character
                    // occupies exactly the box the collider does -- multiplying
                    // by the 20x30 layer instead would draw them twenty times
                    // too wide.
                    spriteBatch->Draw(texture,
                                      d.transform->position.x,
                                      d.transform->position.y,
                                      d.transform->scale.x,
                                      d.transform->scale.y,
                                      r, g, b, 1.0f);
                }
            }

            spriteBatch->End();
        }
    }
}
