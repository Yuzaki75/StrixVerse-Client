#include "CharacterRenderSystem.h"

#include "CharacterComponent.h"
#include "ComponentManager.h"
#include "ColliderComponent.h"
#include "EntityManager.h"
#include "TransformComponent.h"
#include "VelocityComponent.h"

#include "core/AssetManager.h"
#include "core/ServiceLocator.h"
#include "graphics/CharacterPalette.h"
#include "graphics/SpriteBatch.h"
#include "graphics/Texture.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace StrixVerse
{
    namespace ECS
    {
        namespace
        {
            // Below this |vx| the player counts as standing still. Move speed
            // is 100 px/s, so anything an eighth of that is intent, not drift.
            constexpr float kWalkSpeedThreshold = 12.0f;

            // Without a collider there is no grounded flag, so airborne is
            // inferred from vy: rising past this is a jump, falling past it a
            // drop. Small values are gravity noise while standing and read as
            // grounded on purpose.
            constexpr float kAirborneVyThreshold = 60.0f;

            // One full leg cycle covers this many pixels of travel, which at
            // the 100 px/s move speed is roughly two strides a second.
            constexpr float kPhasePixelsPerCycle = 48.0f;

            constexpr float kPi = 3.14159265358979f;

            // The sheet layout, which the generator in
            // scratchpad/make_character.py writes and this reads. One strip of
            // 24x48 frames per zone: Idle, then Walk, then Jump.
            constexpr int kFrameW = 24;
            constexpr int kFrameH = 48;

            constexpr int kIdleFirst  = 0;
            constexpr int kIdleCount  = 4;
            constexpr int kWalkFirst  = 4;
            constexpr int kWalkCount  = 6;
            constexpr int kJumpFirst  = 10;
            constexpr int kJumpCount  = 2;

            // Drawn back to front. Trousers and boots first so the shirt hem
            // sits over them, then skin, then hair, then eyes on top.
            constexpr CharacterPalette::Zone kZoneOrder[] = {
                CharacterPalette::Zone::Trousers,
                CharacterPalette::Zone::Boots,
                CharacterPalette::Zone::Shirt,
                CharacterPalette::Zone::Skin,
                CharacterPalette::Zone::Hair,
                CharacterPalette::Zone::Eyes,
            };

            const char* SheetPathFor(CharacterPalette::Zone zone)
            {
                switch (zone)
                {
                case CharacterPalette::Zone::Skin:     return "assets/character/skin.png";
                case CharacterPalette::Zone::Trousers: return "assets/character/trousers.png";
                case CharacterPalette::Zone::Boots:    return "assets/character/boots.png";
                case CharacterPalette::Zone::Shirt:    return "assets/character/shirt.png";
                case CharacterPalette::Zone::Hair:     return "assets/character/hair.png";
                case CharacterPalette::Zone::Eyes:     return "assets/character/eyes.png";
                default:                               return nullptr;
                }
            }

            // The palette index a character carries for one zone.
            uint8_t LookIndex(const CharacterComponent& look, CharacterPalette::Zone zone)
            {
                switch (zone)
                {
                case CharacterPalette::Zone::Skin:     return look.skin;
                case CharacterPalette::Zone::Trousers: return look.trousers;
                case CharacterPalette::Zone::Boots:    return look.boots;
                case CharacterPalette::Zone::Shirt:    return look.shirt;
                case CharacterPalette::Zone::Hair:     return look.hair;
                case CharacterPalette::Zone::Eyes:     return look.eyes;
                default:                               return 0;
                }
            }

            float Clamp01(float v)
            {
                return std::clamp(v, 0.0f, 1.0f);
            }

            // Multiplies a palette colour, clamped, for the darker stacked
            // body segment and other shading.
            void Shade(float f, float& r, float& g, float& b)
            {
                r = Clamp01(r * f);
                g = Clamp01(g * f);
                b = Clamp01(b * f);
            }

            // Mixes a palette colour toward white for the lighter head.
            void Lighten(float f, float& r, float& g, float& b)
            {
                r = Clamp01(r + (1.0f - r) * f);
                g = Clamp01(g + (1.0f - g) * f);
                b = Clamp01(b + (1.0f - b) * f);
            }
        }

        void CharacterRenderSystem::init(EntityManager* entityManager,
                                         ComponentManager* componentManager)
        {
            System::init(entityManager, componentManager);

            setSignature<Transform, CharacterComponent>();

            EnsureWhiteTexture();
            EnsureSheets();
        }

        void CharacterRenderSystem::EnsureSheets()
        {
            auto assets = ServiceLocator::Get<AssetManager>();
            if (!assets)
            {
                return;
            }

            bool all = true;

            for (CharacterPalette::Zone zone : kZoneOrder)
            {
                const auto slot = static_cast<std::size_t>(zone);
                if (slot >= std::size(m_ZoneSheets))
                {
                    all = false;
                    continue;
                }

                if (!m_ZoneSheets[slot])
                {
                    // No mipmaps and no sRGB: this is pixel art sampled at or
                    // near 1:1, and either would blur it.
                    m_ZoneSheets[slot] = assets->LoadTexture(SheetPathFor(zone), false, false);
                }

                if (!m_ZoneSheets[slot])
                {
                    all = false;
                }
            }

            m_SheetsReady = all;
        }

        void CharacterRenderSystem::EnsureWhiteTexture()
        {
            if (m_WhiteTexture)
            {
                return;
            }

            // Flat colour, big enough to sample cleanly; like the tile
            // textures TileRendererSystem builds, sized by the draw call.
            constexpr unsigned int kSize = 4;
            std::vector<unsigned char> pixels(kSize * kSize * 4, 255);

            auto texture = std::make_shared<Texture>();
            texture->Create(kSize, kSize, pixels.data(), 4, false, false);

            m_WhiteTexture = std::move(texture);
        }

        void CharacterRenderSystem::update(const std::vector<Entity>& entities, float dt)
        {
            if (!m_pComponentManager)
            {
                return;
            }

            // Retried until it succeeds rather than attempted once in init:
            // init can run before there is a GL context to upload into, and a
            // single failed attempt would leave the placeholder figure in
            // charge for the rest of the session.
            if (!m_SheetsReady)
            {
                EnsureSheets();
            }

            for (Entity entity : entities)
            {
                auto* transform = m_pComponentManager->getComponent<Transform>(entity);
                if (!transform)
                {
                    continue;
                }

                AnimState& state = m_AnimStates[entity.id];

                // Registered only once the sheets exist, so a clip can never be
                // created pointing at a sheet that had not loaded yet - AddClip
                // runs once per entity and would keep that null forever.
                if (m_SheetsReady && state.animator.CurrentClipName().empty())
                {
                    // First sighting: register the placeholder clips. Sheets
                    // are filled in when real art lands; nothing else changes.
                    // All three clips share one sheet per zone and differ
                    // only in where they start, which is what firstFrame is
                    // for. The sheet pointer is the skin one purely so the
                    // frame maths has a size to work from - every zone sheet
                    // has identical dimensions, and the draw picks the right
                    // texture per layer.
                    Graphics::AnimationClip clip;
                    clip.sheet       = m_ZoneSheets[static_cast<std::size_t>(
                                           CharacterPalette::Zone::Skin)].get();
                    clip.frameWidth  = kFrameW;
                    clip.frameHeight = kFrameH;

                    clip.firstFrame = kIdleFirst;
                    clip.frameCount = kIdleCount; clip.loop = true;  clip.fps = 6.0f;
                    state.animator.AddClip("Idle", clip);

                    clip.firstFrame = kWalkFirst;
                    clip.frameCount = kWalkCount; clip.loop = true;  clip.fps = 10.0f;
                    state.animator.AddClip("Walk", clip);

                    clip.firstFrame = kJumpFirst;
                    clip.frameCount = kJumpCount; clip.loop = false; clip.fps = 8.0f;
                    state.animator.AddClip("Jump", clip);
                }

                // --- Pose inputs --------------------------------------------
                const VelocityComponent* velocity =
                    m_pComponentManager->hasComponent<VelocityComponent>(entity)
                        ? m_pComponentManager->getComponent<VelocityComponent>(entity)
                        : nullptr;

                const ColliderComponent* collider =
                    m_pComponentManager->hasComponent<ColliderComponent>(entity)
                        ? m_pComponentManager->getComponent<ColliderComponent>(entity)
                        : nullptr;

                const float vx = velocity ? velocity->vx : 0.0f;
                const float vy = velocity ? velocity->vy : 0.0f;

                // Facing follows the last direction of travel, not the current
                // frame's: releasing A mid-stride keeps the player facing left.
                if (vx > kWalkSpeedThreshold)
                {
                    state.facing = 1.0f;
                }
                else if (vx < -kWalkSpeedThreshold)
                {
                    state.facing = -1.0f;
                }

                // Grounded comes from CollisionSystem when a collider exists;
                // otherwise the vy fallback above decides.
                state.airborne = collider
                    ? !collider->grounded && std::abs(vy) > kAirborneVyThreshold
                    : std::abs(vy) > kAirborneVyThreshold;

                state.walking = !state.airborne && std::abs(vx) > kWalkSpeedThreshold;

                // --- State machine: Jump / Walk / Idle ----------------------
                // TODO(art): these three Play() names are the contract the real
                // spritesheets must provide. Nothing else about driving them
                // should need to change.
                if (state.airborne)
                {
                    state.animator.Play("Jump");
                }
                else if (state.walking)
                {
                    state.animator.Play("Walk");
                    state.walkPhase += (std::abs(vx) * dt / kPhasePixelsPerCycle) * 2.0f * kPi;
                }
                else
                {
                    state.animator.Play("Idle");
                    // Phase decays toward a whole stride so the legs settle
                    // together instead of freezing mid-step.
                    state.walkPhase = std::nearbyint(state.walkPhase / kPi) * kPi;
                }

                state.animator.Update(dt);
            }
        }

        void CharacterRenderSystem::render(const std::vector<Entity>& entities)
        {
            auto spriteBatch = ServiceLocator::Get<SpriteBatch>();
            if (!spriteBatch || !m_WhiteTexture || !m_pComponentManager)
            {
                return;
            }

            struct Drawable
            {
                uint32_t entityId = 0;
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
                    drawables.push_back({entity.id, transform, look});
                }
            }

            if (drawables.empty())
            {
                return;
            }

            // Same ordering the layered renderer used: draw order decides who
            // overlaps whom, and the layer index is what the server means by it.
            std::sort(drawables.begin(), drawables.end(),
                      [](const Drawable& a, const Drawable& b) {
                          return a.look->layer < b.look->layer;
                      });

            spriteBatch->Begin();

            for (const Drawable& d : drawables)
            {
                if (m_SheetsReady)
                {
                    DrawCharacterSprite(*spriteBatch,
                                        m_AnimStates[d.entityId],
                                        *d.transform,
                                        *d.look);
                }
                else
                {
                    DrawCharacterFigure(*spriteBatch,
                                        m_AnimStates[d.entityId],
                                        *d.transform,
                                        *d.look);
                }
            }

            spriteBatch->End();
        }

        void CharacterRenderSystem::DrawCharacterSprite(SpriteBatch& spriteBatch,
                                                        const AnimState& state,
                                                        const Transform& transform,
                                                        const CharacterComponent& look)
        {
            const float x = transform.position.x;
            const float y = transform.position.y;
            const float w = transform.scale.x;
            const float h = transform.scale.y;

            float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
            if (!state.animator.GetFrameUV(state.animator.CurrentFrame(), u0, v0, u1, v1))
            {
                // No playable clip yet - one frame of nothing beats one frame
                // of the whole sheet squashed into the character's box.
                return;
            }

            // Facing by swapping the U pair. The art holds one direction and
            // this mirrors it, so there is no second set of frames to keep in
            // step with the first.
            if (state.facing < 0.0f)
            {
                std::swap(u0, u1);
            }

            for (CharacterPalette::Zone zone : kZoneOrder)
            {
                const auto slot = static_cast<std::size_t>(zone);
                if (slot >= std::size(m_ZoneSheets) || !m_ZoneSheets[slot])
                {
                    continue;
                }

                float r = 1.0f, g = 1.0f, b = 1.0f;
                CharacterPalette::ColourFloats(zone, LookIndex(look, zone), r, g, b);

                // The sheet's grey levels multiply the tint, so shading comes
                // out of the art and the palette stays the palette.
                spriteBatch.DrawUV(*m_ZoneSheets[slot], x, y, w, h,
                                   u0, v0, u1, v1, r, g, b, 1.0f);
            }
        }

        void CharacterRenderSystem::DrawCharacterFigure(SpriteBatch& spriteBatch,
                                                        const AnimState& state,
                                                        const Transform& transform,
                                                        const CharacterComponent& look)
        {
            const Texture& white = *m_WhiteTexture;

            const float x = transform.position.x;
            const float y = transform.position.y;
            const float w = transform.scale.x;   // scale IS the on-screen size,
            const float h = transform.scale.y;   // same convention as ever

            // Palette colours, resolved once. The tint logic is exactly what
            // the layered renderer used -- indices in, floats out.
            float bodyR = 1.0f, bodyG = 1.0f, bodyB = 1.0f;
            CharacterPalette::ColourFloats(CharacterPalette::Zone::Shirt,
                                           look.shirt, bodyR, bodyG, bodyB);

            float headR = bodyR, headG = bodyG, headB = bodyB;
            CharacterPalette::ColourFloats(CharacterPalette::Zone::Skin,
                                           look.skin, headR, headG, headB);
            Lighten(0.25f, headR, headG, headB);

            float capR = bodyR, capG = bodyG, capB = bodyB;
            CharacterPalette::ColourFloats(CharacterPalette::Zone::Hair,
                                           look.hair, capR, capG, capB);

            float visorR = 0.15f, visorG = 0.15f, visorB = 0.18f;
            CharacterPalette::ColourFloats(CharacterPalette::Zone::Eyes,
                                           look.eyes, visorR, visorG, visorB);

            float legR = bodyR, legG = bodyG, legB = bodyB;
            CharacterPalette::ColourFloats(CharacterPalette::Zone::Trousers,
                                           look.trousers, legR, legG, legB);

            float lowerR = bodyR, lowerG = bodyG, lowerB = bodyB;
            Shade(0.75f, lowerR, lowerG, lowerB);

            // Vertical budget, fractions of the character box, drawn bottom-up
            // so the feet stay planted whatever the pose does above them.
            const float headH   = h * 0.30f;
            const float bodyUH  = h * 0.28f;
            const float bodyLH  = h * 0.16f;
            const float legH    = h * 0.26f;

            const float footY = y + h;

            // Body bob while walking: a small lift on everything above the
            // hips, twice per stride.
            const float bob = state.walking
                ? -std::abs(std::sin(state.walkPhase)) * h * 0.04f
                : 0.0f;

            // TODO(art): replace procedural figure with animator frame draws.
            // The state machine in update() already plays Idle/Walk/Jump, so
            // the whole swap is the rect block below becoming:
            //
            //     float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
            //     if (state.animator.GetFrameUV(state.animator.CurrentFrame(),
            //                                   u0, v0, u1, v1))
            //     {
            //         spriteBatch.Draw(*sheet, x, y + bob, w, h, 1, 1, 1, 1);
            //     }
            //
            // where 'sheet' is the current clip's AnimationClip::sheet, filled
            // in beside the AddClip calls in update(). Everything above this
            // comment -- pose inputs, facing, phase, bob -- stays as-is.

            // --- BEGIN PROCEDURAL FIGURE --------------------------------

            // Head: slightly lighter shade, with a cap strip on top and the
            // visor strip on the leading edge of travel.
            const float headW = w * 0.72f;
            const float headX = x + (w - headW) * 0.5f;
            const float headY = y + bob;

            spriteBatch.Draw(white, headX, headY, headW, headH,
                             headR, headG, headB, 1.0f);

            const float capH = headH * 0.30f;
            spriteBatch.Draw(white, headX, headY, headW, capH,
                             capR, capG, capB, 1.0f);

            // Visor: pinned to whichever side the player last moved toward.
            const float visorH  = std::max(headH * 0.22f, 2.0f);
            const float visorW  = headW * 0.55f;
            const float visorPad = headW * 0.10f;
            const float visorX = state.facing > 0.0f
                ? headX + headW - visorW - visorPad
                : headX + visorPad;
            const float visorY = headY + capH + headH * 0.18f;

            spriteBatch.Draw(white, visorX, visorY, visorW, visorH,
                             visorR, visorG, visorB, 1.0f);

            // Body: two stacked rects for a rounded feel -- wide shoulders over
            // a narrower waist, both tinted with the player's shirt colour.
            const float bodyUX = x + (w - w * 0.84f) * 0.5f;
            const float bodyUY = headY + headH;
            spriteBatch.Draw(white, bodyUX, bodyUY, w * 0.84f, bodyUH,
                             bodyR, bodyG, bodyB, 1.0f);

            const float bodyLX = x + (w - w * 0.66f) * 0.5f;
            spriteBatch.Draw(white, bodyLX, bodyUY + bodyUH, w * 0.66f, bodyLH,
                             lowerR, lowerG, lowerB, 1.0f);

            // Legs: two small rects under the hips. Walking scissors them
            // against each other on the phase; being airborne tucks them up
            // and shortens them; standing plants them side by side.
            const float legW = w * 0.20f;
            const float gap  = w * 0.06f;

            float legLength = legH;
            float legLift   = 0.0f;
            float swingA = 0.0f, swingB = 0.0f;

            if (state.airborne)
            {
                legLength = legH * 0.55f;
                legLift   = legH * 0.35f;
            }
            else if (state.walking)
            {
                const float swing = h * 0.06f;
                swingA = std::sin(state.walkPhase) * swing;
                swingB = -swingA;
            }

            const float centreX = x + w * 0.5f;
            const float legAX = centreX - legW - gap * 0.5f;
            const float legBX = centreX + gap * 0.5f;

            // Lift applies to both legs (tuck); swing raises one while the
            // other reaches down, so neither leaves the ground plane empty.
            spriteBatch.Draw(white, legAX, footY - legLength + legLift - std::max(0.0f, swingA),
                             legW, legLength - legLift + std::max(0.0f, swingA),
                             legR, legG, legB, 1.0f);

            spriteBatch.Draw(white, legBX, footY - legLength + legLift - std::max(0.0f, swingB),
                             legW, legLength - legLift + std::max(0.0f, swingB),
                             legR, legG, legB, 1.0f);

            // --- END PROCEDURAL FIGURE ----------------------------------
        }
    }
}
