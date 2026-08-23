#pragma once

#include "System.h"
#include "TransformComponent.h"

#include <chrono>
#include <cstdint>
#include <unordered_map>

namespace StrixVerse
{
    namespace ECS
    {
        class NetworkSyncSystem : public System
        {
        public:
            void init(EntityManager* entityManager, ComponentManager* componentManager) override;
            void update(const std::vector<Entity>& entities, float dt) override;

        private:
            // One buffered server position, timestamped when it arrived.
            struct PositionSample
            {
                Transform::Vector2 position{};
                std::chrono::steady_clock::time_point arrival{};
            };

            // Per-entity interpolation state, kept here rather than in the
            // components so the packet writer and the smoother never fight
            // over the same fields: the writer touches NetworkComponent,
            // only this system touches Transform.
            struct EntityState
            {
                // Last values seen in NetworkComponent. A change there is
                // treated as a freshly arrived server sample.
                float lastNetX = 0.0f;
                float lastNetY = 0.0f;
                bool  hasTarget = false;

                // The two most recent buffered samples.
                PositionSample prev;
                PositionSample latest;
                bool           hasPrev = false;
            };

            std::unordered_map<uint32_t, EntityState> m_entityStates;
        };
    }
}
