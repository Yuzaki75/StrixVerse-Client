#include "RenderSystem.h"
#include "ComponentManager.h"
#include "EntityManager.h"
#include "core/ServiceLocator.h"
#include "core/AssetManager.h"
#include "../graphics/SpriteBatch.h"
#include "TransformComponent.h"
#include "SpriteComponent.h"
#include "graphics/Texture.h"
#include <algorithm>
#include <vector>

namespace StrixVerse
{
    namespace ECS
    {
        void RenderSystem::render(const std::vector<Entity>& entities)
        {
            auto spriteBatch = ServiceLocator::Get<SpriteBatch>();
            if (!spriteBatch)
            {
                return;
            }

            // Collect all entities that have both Transform and Sprite components.
            struct SpriteEntity
            {
                Entity entity;
                SpriteComponent* sprite = nullptr;
                Transform* transform = nullptr;
            };
            std::vector<SpriteEntity> sprites;
            sprites.reserve(entities.size());

            for (Entity entity : entities)
            {
                if (m_pComponentManager->hasComponent<Transform>(entity) &&
                    m_pComponentManager->hasComponent<SpriteComponent>(entity))
                {
                    auto* sprite = m_pComponentManager->getComponent<SpriteComponent>(entity);
                    auto* transform = m_pComponentManager->getComponent<Transform>(entity);
                    if (!sprite || !transform)
                    {
                        continue;
                    }
                    sprites.push_back({ entity, sprite, transform });
                }
            }

            // Sort by layer (ascending), then by texture ID (to batch by texture).
            std::sort(sprites.begin(), sprites.end(), [](const SpriteEntity& a, const SpriteEntity& b)
            {
                if (a.sprite->layer != b.sprite->layer)
                    return a.sprite->layer < b.sprite->layer;
                return a.sprite->textureID < b.sprite->textureID;
            });

            // Begin the sprite batch.
            spriteBatch->Begin();

            for (const auto& se : sprites)
            {
                // Get the texture from the AssetManager by renderer ID.
                auto assetManager = ServiceLocator::Get<AssetManager>();
                if (!assetManager)
                {
                    continue;
                }

                Texture* texture = assetManager->GetTextureByRendererID(se.sprite->textureID);
                if (!texture)
                {
                    continue;
                }

                // Draw the sprite.
                float width = static_cast<float>(texture->GetWidth());
                float height = static_cast<float>(texture->GetHeight());

                spriteBatch->Draw(*texture,
                                  se.transform->position.x,
                                  se.transform->position.y,
                                  width * se.transform->scale.x,
                                  height * se.transform->scale.y,
                                  se.sprite->r, se.sprite->g, se.sprite->b, se.sprite->a);
            }

            spriteBatch->End();
        }
    }
}