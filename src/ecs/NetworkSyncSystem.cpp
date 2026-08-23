#include "NetworkSyncSystem.h"
#include "ComponentManager.h"
#include "TransformComponent.h"
#include "NetworkComponent.h"

#include <algorithm>
#include <chrono>
#include <unordered_set>

namespace StrixVerse
{
    namespace ECS
    {
        namespace
        {
            // Must match the tile size used to convert server tiles to local
            // pixels in GameScreen.
            constexpr float kTileSize = 32.0f;

            // Distance (in pixels) between the rendered position and the
            // newest sample beyond which we teleport instead of gliding.
            // Covers respawns and packet-loss bursts. 8 tiles.
            constexpr float kSnapDistance = 8.0f * kTileSize;

            // Render this far behind the newest arrival, so a late packet
            // still has something to interpolate towards.
            constexpr std::chrono::milliseconds kInterpolationDelay{100};

            // A newest sample older than this means the stream has stalled;
            // start extrapolating from the last known velocity.
            constexpr std::chrono::milliseconds kMaxSampleAge{250};

            // Never extrapolate further than this past the newest sample,
            // so a dead connection coasts briefly and then holds still
            // rather than flying off the map.
            constexpr std::chrono::milliseconds kExtrapolationWindow{100};

            using FloatSeconds = std::chrono::duration<float>;
        }

        void NetworkSyncSystem::init(EntityManager *entityManager, ComponentManager *componentManager)
        {
            System::init(entityManager, componentManager);

            setSignature<NetworkComponent, Transform>();
        }

        void NetworkSyncSystem::update(const std::vector<Entity> &entities, float dt)
        {
            // Timing is driven by packet-arrival timestamps, not the frame delta.
            (void)dt;

            if (!m_pComponentManager)
            {
                return;
            }

            const auto now = std::chrono::steady_clock::now();

            // Lazy pruning: entities that no longer match our signature have
            // been destroyed or stripped of components, so drop their state.
            std::unordered_set<uint32_t> alive;
            alive.reserve(entities.size());
            for (Entity entity : entities)
            {
                alive.insert(entity.id);
            }
            for (auto it = m_entityStates.begin(); it != m_entityStates.end();)
            {
                if (alive.find(it->first) == alive.end())
                {
                    it = m_entityStates.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            for (Entity entity : entities)
            {
                // Check if the entity has both NetworkComponent and TransformComponent.
                if (!m_pComponentManager->hasComponent<NetworkComponent>(entity) ||
                    !m_pComponentManager->hasComponent<Transform>(entity))
                {
                    continue;
                }

                auto *net = m_pComponentManager->getComponent<NetworkComponent>(entity);
                auto *transform = m_pComponentManager->getComponent<Transform>(entity);

                if (!net || !transform)
                {
                    continue;
                }

                if (net->isLocalPlayer)
                {
                    // For the local player, we update the network component with the current transform.
                    // This data will be sent to the server (by some other system).
                    net->x = transform->position.x;
                    net->y = transform->position.y;
                    net->rotation = transform->rotation;
                    continue;
                }

                EntityState &state = m_entityStates[entity.id];

                // The network component is written directly on packet arrival
                // (GameScreen's PlayerMove handler), so any change there is a
                // new server sample. Shift the buffer: previous latest becomes
                // the interpolation source, the new value becomes the target.
                if (!state.hasTarget ||
                    state.lastNetX != net->x ||
                    state.lastNetY != net->y)
                {
                    state.prev = state.latest;
                    state.hasPrev = state.hasTarget;
                    state.latest.position.x = net->x;
                    state.latest.position.y = net->y;
                    state.latest.arrival = now;
                    state.lastNetX = net->x;
                    state.lastNetY = net->y;
                    state.hasTarget = true;
                }

                Transform::Vector2 target = state.latest.position;

                if (state.hasPrev)
                {
                    const auto age = now - state.latest.arrival;

                    if (age > kMaxSampleAge)
                    {
                        // No fresh samples: coast along the last known velocity,
                        // but only for a short window before holding position.
                        const float span = FloatSeconds(state.latest.arrival - state.prev.arrival).count();
                        if (span > 0.0f)
                        {
                            const auto extrapolation =
                                std::min(std::chrono::duration_cast<std::chrono::milliseconds>(age - kMaxSampleAge),
                                         kExtrapolationWindow);
                            const float t = FloatSeconds(extrapolation).count() / span;
                            target.x += (state.latest.position.x - state.prev.position.x) * t;
                            target.y += (state.latest.position.y - state.prev.position.y) * t;
                        }
                    }
                    else
                    {
                        // Interpolate between the two most recent samples at a
                        // point ~100ms behind the newest arrival. If packets
                        // arrive faster than that, rendering a full delay back
                        // would fall off the start of our two-sample buffer,
                        // so shorten the delay to the observed gap.
                        const auto gap = state.latest.arrival - state.prev.arrival;
                        const float span = FloatSeconds(gap).count();
                        if (span > 0.0f)
                        {
                            const auto delay =
                                std::min(kInterpolationDelay,
                                         std::chrono::duration_cast<std::chrono::milliseconds>(gap));
                            const auto renderTime = state.latest.arrival - delay;
                            const float alpha = FloatSeconds(renderTime - state.prev.arrival).count() / span;
                            const float clamped = std::clamp(alpha, 0.0f, 1.0f);
                            target.x = state.prev.position.x +
                                       (state.latest.position.x - state.prev.position.x) * clamped;
                            target.y = state.prev.position.y +
                                       (state.latest.position.y - state.prev.position.y) * clamped;
                        }
                    }
                }

                // A respawn or a burst of lost packets shows up as the remote
                // player being impossibly far from where we render them. Glide
                // would look worse than a teleport, so snap straight to the
                // newest sample and drop the stale half of the buffer.
                const float dx = transform->position.x - state.latest.position.x;
                const float dy = transform->position.y - state.latest.position.y;
                if (dx * dx + dy * dy > kSnapDistance * kSnapDistance)
                {
                    target = state.latest.position;
                    state.prev = state.latest;
                }

                transform->position.x = target.x;
                transform->position.y = target.y;
                transform->rotation = net->rotation;
            }
        }
    }
}
