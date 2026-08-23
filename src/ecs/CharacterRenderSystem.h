#pragma once

#include "System.h"

#include "CharacterComponent.h"
#include "TransformComponent.h"
#include "graphics/Animation.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

// Texture and SpriteBatch live at global scope, like Camera2D -- declaring
// them inside StrixVerse would silently create second, unrelated types.
class SpriteBatch;
class Texture;

namespace StrixVerse
{
    namespace ECS
    {
        // -------------------------------------------------------------------
        // CharacterRenderSystem
        // -------------------------------------------------------------------
        // Draws players as an animated procedural placeholder figure -- a small
        // robot/golem built from tinted rects through the white-texture
        // SpriteBatch path -- until real spritesheet art arrives.
        //
        // The figure is coloured from the same palette indices every player
        // already carries: shirt tints the body, skin the head, eyes the
        // visor, hair a cap strip, trousers the legs. Identity survives the
        // placeholder; only the silhouette is provisional.
        //
        // Motion comes from each entity's velocity: legs scissor on a phase
        // advanced by speed, the body bobs while walking, legs tuck while
        // airborne, and the head's visor flips to the last direction of
        // travel. Remote players need nothing special -- NetworkSyncSystem has
        // already smoothed their Transform by the time this reads it.
        //
        // Submits straight to the SpriteBatch rather than creating entities per
        // rect, which is the same choice TileRendererSystem makes and for the
        // same reason: the entity ceiling is a budget for things that need
        // identity, and a leg does not.
        // -------------------------------------------------------------------
        class CharacterRenderSystem : public System
        {
        public:
            void init(EntityManager* entityManager, ComponentManager* componentManager) override;
            void update(const std::vector<Entity>& entities, float dt) override;
            void render(const std::vector<Entity>& entities) override;

        private:
            // Per-entity pose state, keyed by entity id like the interpolation
            // buffers in NetworkSyncSystem. Kept here rather than in a
            // component because nothing but drawing consumes it.
            struct AnimState
            {
                Graphics::Animator animator;
                float walkPhase = 0.0f;  // radians; advanced by speed * dt
                float facing    = 1.0f;  // -1 left, +1 right; last direction of travel
                bool  walking   = false;
                bool  airborne  = false;
            };

            // One flat white texture shared by every rect of every character;
            // all colour arrives as the batch's per-draw tint.
            void EnsureWhiteTexture();

            void DrawCharacterFigure(SpriteBatch& spriteBatch,
                                     const AnimState& state,
                                     const Transform& transform,
                                     const CharacterComponent& look);

            std::unordered_map<uint32_t, AnimState> m_AnimStates;
            std::shared_ptr<Texture> m_WhiteTexture;
        };
    }
}
