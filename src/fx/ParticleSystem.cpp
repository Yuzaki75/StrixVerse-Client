#include "ParticleSystem.h"

#include "../graphics/SpriteBatch.h"
#include "../graphics/Texture.h"

#include <algorithm>
#include <cmath>

namespace
{
    // Matches GameScreen's tile size: block-break coordinates arrive as tile
    // coordinates and are converted to world pixels against this.
    constexpr float kTileSize = 32.0f;

    // Debris tuning for EmitBlockBreak. Roughly one tile's worth of chips that
    // pop upward then fall, biased down because broken material falls.
    constexpr int   kBlockBreakCount   = 12;
    constexpr float kDebrisSpeedMin    = 30.0f;
    constexpr float kDebrisSpeedMax    = 110.0f;
    constexpr float kDebrisGravity     = 340.0f;
    constexpr float kDebrisLife        = 0.65f;
    constexpr float kDebrisSizeMin     = 3.0f;
    constexpr float kDebrisSizeMax     = 6.0f;

    // Aether sparkle tuning: slow rise, short life, small and bright.
    constexpr float kAetherRiseMin  = 8.0f;
    constexpr float kAetherRiseMax  = 22.0f;
    constexpr float kAetherDrift    = 10.0f;
    constexpr float kAetherLifeMin  = 0.5f;
    constexpr float kAetherLifeMax  = 0.9f;
    constexpr float kAetherSizeMin  = 2.0f;
    constexpr float kAetherSizeMax  = 4.0f;

    // The two ends of the Aether palette. Bursts alternate between them so a
    // cluster reads violet-blue rather than either alone.
    const Color kAetherViolet{0x6C / 255.0f, 0x5C / 255.0f, 0xE7 / 255.0f, 1.0f};
    const Color kAetherBlue  {0x4D / 255.0f, 0xE1 / 255.0f, 0xFF / 255.0f, 1.0f};

    float RandomInRange(std::mt19937& rng, float lo, float hi)
    {
        return std::uniform_real_distribution<float>(lo, hi)(rng);
    }
}

namespace StrixVerse
{
    namespace FX
    {
        ParticleSystem::ParticleSystem()
            : m_Rng(0xC0FFEEu)
        {
        }

        void ParticleSystem::Emit(const Particle& p)
        {
            if (m_Count >= kMaxParticles)
                return;

            m_Pool[m_Count] = p;
            ++m_Count;
        }

        void ParticleSystem::EmitBurst(float x, float y, int count,
                                       float speedMin, float speedMax,
                                       const Color& color, float gravity,
                                       float lifetime)
        {
            if (count <= 0 || speedMax < speedMin)
                return;

            for (int i = 0; i < count; ++i)
            {
                Particle p;

                p.x = x;
                p.y = y;

                // Uniform direction, random magnitude: a round burst rather
                // than a square one.
                const float angle  = RandomInRange(m_Rng, 0.0f, 6.2831853f);
                const float speed  = RandomInRange(m_Rng, speedMin, speedMax);

                p.vx = std::cos(angle) * speed;
                p.vy = std::sin(angle) * speed;

                // A little per-particle life jitter keeps the burst from
                // vanishing in one visible step.
                p.maxLife = lifetime * RandomInRange(m_Rng, 0.7f, 1.0f);
                p.life    = p.maxLife;

                p.size    = RandomInRange(m_Rng, kDebrisSizeMin, kDebrisSizeMax);
                p.color   = color;
                p.gravity = gravity;

                Emit(p);

                if (IsFull())
                    return;
            }
        }

        void ParticleSystem::EmitBlockBreak(float tileX, float tileY,
                                            const Color& blockColor)
        {
            // Centre of the named tile, in world pixels.
            const float cx = tileX * kTileSize + kTileSize * 0.5f;
            const float cy = tileY * kTileSize + kTileSize * 0.5f;

            for (int i = 0; i < kBlockBreakCount; ++i)
            {
                Particle p;

                p.x = cx + RandomInRange(m_Rng, -kTileSize * 0.35f, kTileSize * 0.35f);
                p.y = cy + RandomInRange(m_Rng, -kTileSize * 0.35f, kTileSize * 0.35f);

                // Any sideways scatter at all, but always upward to start with:
                // the gravity term does the falling, so the burst reads as
                // debris rather than an explosion ring.
                p.vx = RandomInRange(m_Rng, -kDebrisSpeedMax, kDebrisSpeedMax);
                p.vy = -RandomInRange(m_Rng, kDebrisSpeedMin, kDebrisSpeedMax);

                p.maxLife = kDebrisLife * RandomInRange(m_Rng, 0.7f, 1.15f);
                p.life    = p.maxLife;

                p.size    = RandomInRange(m_Rng, kDebrisSizeMin, kDebrisSizeMax);

                // The chip colour is the block's own colour darkened slightly
                // and varied per particle, the same way real debris is never
                // one flat shade.
                const float shade = RandomInRange(m_Rng, 0.75f, 1.05f);
                p.color = Color(std::clamp(blockColor.r * shade, 0.0f, 1.0f),
                                std::clamp(blockColor.g * shade, 0.0f, 1.0f),
                                std::clamp(blockColor.b * shade, 0.0f, 1.0f),
                                blockColor.a);

                p.gravity = kDebrisGravity;

                Emit(p);

                if (IsFull())
                    return;
            }
        }

        void ParticleSystem::EmitAether(float x, float y)
        {
            // One sparkle per call. Callers that want a plume emit on a timer
            // or in a loop; the effect itself is a single short-lived mote.
            Particle p;

            p.x = x + RandomInRange(m_Rng, -3.0f, 3.0f);
            p.y = y + RandomInRange(m_Rng, -3.0f, 3.0f);

            // Slow rise with a gentle sideways drift.
            p.vx = RandomInRange(m_Rng, -kAetherDrift, kAetherDrift);
            p.vy = -RandomInRange(m_Rng, kAetherRiseMin, kAetherRiseMax);

            p.maxLife = RandomInRange(m_Rng, kAetherLifeMin, kAetherLifeMax);
            p.life    = p.maxLife;

            p.size    = RandomInRange(m_Rng, kAetherSizeMin, kAetherSizeMax);

            // Negative gravity would keep lifting it after death; zero means
            // the initial rise carries it the whole way.
            p.gravity = 0.0f;

            // Alternate the two ends of the Aether palette so a cluster of
            // motes reads violet-blue rather than either alone.
            static bool violetFirst = true;
            p.color = violetFirst ? kAetherViolet : kAetherBlue;
            violetFirst = !violetFirst;

            Emit(p);
        }

        void ParticleSystem::Update(float dt)
        {
            if (dt <= 0.0f)
                return;

            for (std::size_t i = 0; i < m_Count; )
            {
                Particle& p = m_Pool[i];

                p.life -= dt;

                // Swap-with-last compaction: the pool stays dense and no slot
                // is ever revisited after this line moves i forward.
                if (p.life <= 0.0f)
                {
                    m_Pool[i] = m_Pool[m_Count - 1];
                    --m_Count;
                    continue;
                }

                p.vy += p.gravity * dt;
                p.x  += p.vx * dt;
                p.y  += p.vy * dt;

                ++i;
            }
        }

        void ParticleSystem::Render(SpriteBatch& batch,
                                    const Texture& whiteTexture) const
        {
            if (m_Count == 0)
                return;

            for (std::size_t i = 0; i < m_Count; ++i)
            {
                const Particle& p = m_Pool[i];

                // Fade by remaining life, matching how the HUD fades things out
                // without ever popping.
                const float alpha = std::clamp(p.life / p.maxLife, 0.0f, 1.0f) *
                                    p.color.a;

                const float half = p.size * 0.5f;

                batch.Draw(whiteTexture,
                           p.x - half, p.y - half,
                           p.size, p.size,
                           p.color.r, p.color.g, p.color.b, alpha);
            }
        }
    }
}
