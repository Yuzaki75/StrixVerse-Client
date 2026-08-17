#pragma once

#include "System.h"

#include <memory>

// Texture lives at global scope, like SpriteBatch and Camera2D -- declaring it
// inside StrixVerse would silently create a second, unrelated type.
class Texture;

namespace StrixVerse
{
    namespace ECS
    {
        // -------------------------------------------------------------------
        // CharacterRenderSystem
        // -------------------------------------------------------------------
        // Draws players from their appearance indices.
        //
        // A character is one silhouette masked into six greyscale layers -- skin,
        // trousers, boots, shirt, hair, eyes -- each tinted at draw time with
        // the palette colour that player's index selects. Six draws per
        // character, from six textures shared by everyone on screen.
        //
        // Greyscale masks rather than flat white so shading survives: the batch
        // multiplies, so 255 comes out as the palette colour exactly and the
        // darker pixels come out as shaded versions of it. That is also how the
        // mouth exists without a seventh palette zone.
        //
        // Submits straight to the SpriteBatch rather than creating six entities
        // per player, which is the same choice TileRendererSystem makes and for
        // the same reason: the entity ceiling is a budget for things that need
        // identity, and a shirt does not.
        // -------------------------------------------------------------------
        class CharacterRenderSystem : public System
        {
        public:
            void init(EntityManager* entityManager, ComponentManager* componentManager) override;
            void render(const std::vector<Entity>& entities) override;

        private:
            // Loads the six layers on first use. Returns false if any is
            // missing, which disables the system rather than drawing a
            // partial character.
            bool EnsureLayers();

            std::shared_ptr<Texture> m_Layers[6];
            bool m_Loaded = false;
            bool m_LoadFailed = false;
        };
    }
}
