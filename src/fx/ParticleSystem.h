#pragma once

#include <array>
#include <cstddef>
#include <random>

#include "../graphics/Color.h"

class SpriteBatch;
class Texture;

namespace StrixVerse
{
    namespace FX
    {
        // Fixed pool size. Emitting into a full pool drops the new particle
        // rather than growing: a burst is cosmetic, and an unbounded pool would
        // turn one bad packet into a heap spike.
        inline constexpr std::size_t kMaxParticles = 2048;

        // -----------------------------------------------------------------------------
        // Particle
        //
        // One gameplay effect particle, in local world pixels (the same space
        // the tile renderer draws in). life counts down from maxLife; alpha is
        // derived at render time as life / maxLife.
        // -----------------------------------------------------------------------------
        // Default burst particle dimensions - the block-debris chip size, which
        // is what EmitBurst produced for every caller before it took a size.
        inline constexpr float kDefaultBurstSizeMin = 3.0f;
        inline constexpr float kDefaultBurstSizeMax = 6.0f;

        struct Particle
        {
            float x = 0.0f;
            float y = 0.0f;
            float vx = 0.0f;
            float vy = 0.0f;

            float life    = 0.0f;
            float maxLife = 0.0f;
            float size    = 4.0f;

            Color color{1.0f, 1.0f, 1.0f, 1.0f};

            float gravity = 0.0f;   // Pixels per second added to vy.
        };

        // -----------------------------------------------------------------------------
        // ParticleSystem
        //
        // Lightweight CPU particle pool for gameplay effects: block-break debris,
        // Aether sparkles and generic bursts.
        //
        // The pool is allocated once and never per frame. Dead particles are
        // compacted with swap-with-last during Update, so iteration stays dense
        // and there is no tombstone pass at render time.
        // -----------------------------------------------------------------------------
        class ParticleSystem
        {
        public:
            ParticleSystem();

            ParticleSystem(const ParticleSystem&) = delete;
            ParticleSystem& operator=(const ParticleSystem&) = delete;

            // Adds one particle verbatim. Silently dropped when the pool is full.
            void Emit(const Particle& p);

            // count particles flung from (x, y) in uniformly random directions,
            // speed between speedMin and speedMax, all sharing colour, gravity
            // and lifetime (with a small per-particle jitter so a burst does not
            // die in one visible step).
            //
            // sizeMin/sizeMax default to the block-debris dimensions, which is
            // what this used to hardcode: every caller of this general-purpose
            // burst got rock-chip particles whatever it was depicting, so the
            // Strix Core puff was made of gravel. They are parameters now, and
            // the defaults keep block debris looking exactly as it did.
            void EmitBurst(float x, float y, int count,
                           float speedMin, float speedMax,
                           const Color& color, float gravity, float lifetime,
                           float sizeMin = kDefaultBurstSizeMin,
                           float sizeMax = kDefaultBurstSizeMax);

            // Block debris: a downward-biased burst of block-coloured chips that
            // fall and settle. tileX/tileY are tile coordinates; they are
            // converted to world pixels against the 32px tile size here so
            // callers never keep two units straight.
            void EmitBlockBreak(float tileX, float tileY, const Color& blockColor);

            // A short-lived violet/blue sparkle rising slowly off the world.
            void EmitAether(float x, float y);

            // Integrates velocity and gravity, decrements life, compacts the
            // pool. dt in seconds.
            void Update(float dt);

            // Draws every live particle through SpriteBatch using the same 1x1
            // white texture GameScreen tints its player sprite with, scaled to
            // each particle's size and tinted per particle.
            void Render(SpriteBatch& batch, const Texture& whiteTexture) const;

            std::size_t GetActiveCount() const { return m_Count; }
            bool IsFull() const { return m_Count >= kMaxParticles; }

            void Clear() { m_Count = 0; }

        private:
            // Which end of the Aether palette the next sparkle takes. See
            // EmitAether.
            bool m_AetherViolet = true;

            std::array<Particle, kMaxParticles> m_Pool{};
            std::size_t m_Count = 0;

            // Seeded once in the constructor. Re-seeding per emission would make
            // bursts deterministic relative to their spawn time, which reads as
            // mechanical on screen.
            std::mt19937 m_Rng;
        };
    }
}
