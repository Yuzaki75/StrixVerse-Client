#include "RenderSystem.h"
#include "ComponentManager.h"
#include "EntityManager.h"
#include "Core/ServiceLocator.h"
#include "Core/AssetManager.h"
#include "Graphics/Texture.h"
#include <algorithm>
#include <vector>

namespace StrixVerse
{
    namespace ECS
    {
        void RenderSystem::init(EntityManager* entityManager, ComponentManager* componentManager)
        {
            System::init(entityManager, componentManager);
            m_SpriteBatch.InitRenderData();
        }

        void RenderSystem::render(const std::vector<Entity>& entities)
        {
            // Collect all entities that have both Transform and Sprite components.
            struct SpriteEntity
            {
                Entity entity;
                SpriteComponent sprite;
                Transform transform;
            };
            std::vector<SpriteEntity> sprites;
            sprites.reserve(entities.size());

            for (Entity entity : entities)
            {
                if (m_pComponentManager->hasComponent<Transform>(entity) &&
                    m_pComponentManager->hasComponent<SpriteComponent>(entity))
                {
                    SpriteComponent sprite = m_pComponentManager->getComponent<SpriteComponent>(entity);
                    Transform transform = m_pComponentManager->getComponent<Transform>(entity);
                    sprites.push_back({ entity, sprite, transform });
                }
            }

            // Sort by layer (ascending), then by texture ID (to batch by texture).
            std::sort(sprites.begin(), sprites.end(), [](const SpriteEntity& a, const SpriteEntity& b)
            {
                if (a.sprite.layer != b.sprite.layer)
                    return a.sprite.layer < b.sprite.layer;
                return a.sprite.textureID < b.sprite.textureID;
            });

            // Begin the sprite batch.
            m_SpriteBatch.Begin();

            unsigned int currentTexture = 0;
            for (const auto& se : sprites)
            {
                // If texture changes, end the current batch and start a new one.
                if (se.sprite.textureID != currentTexture)
                {
                    m_SpriteBatch.End();
                    m_SpriteBatch.Begin();
                    currentTexture = se.sprite.textureID;
                }

                // Get the texture from the AssetManager.
                auto assetManager = ServiceLocator::Get<AssetManager>();
                if (!assetManager)
                {
                    // If we don't have an asset manager, we cannot draw.
                    continue;
                }

                Texture* texture = assetManager->GetTexture(se.sprite.textureID);
                if (!texture)
                {
                    // Texture not found, skip.
                    continue;
                }

                // Get the texture dimensions.
                float width = static_cast<float>(texture->GetWidth());
                float height = static_cast<float>(texture->GetHeight());

                // Draw the sprite.
                // Note: We are not applying rotation or scale for now.
                m_SpriteBatch.Draw(*texture,
                                   transform.position.x,
                                   transform.position.y,
                                   width,
                                   height,
                                   sprite.r, sprite.g, sprite.b, sprite.a);
            }

            m_SpriteBatch.End();
        }
    }
}