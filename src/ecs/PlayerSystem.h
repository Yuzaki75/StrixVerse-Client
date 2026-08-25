#pragma once

#include "System.h"

#include <unordered_map>

namespace StrixVerse
{
    namespace ECS
    {
        // -----------------------------------------------------------------
        // PlayerSystem
        //
        // Turns held keys into a velocity in pixels per second. It does not
        // integrate: MovementSystem is the only thing that moves anything, and
        // CollisionSystem clamps this velocity in between.
        //
        // Horizontal is direct - A/D set it outright, so releasing a key stops
        // the player on the same frame. Vertical is not: gravity accumulates
        // into the existing vertical velocity, and a jump is a single impulse
        // written into it. That is the whole difference between this and the
        // four-directional free flight it replaces, where W and S wrote vy the
        // same way A and D write vx and the player simply hovered wherever
        // they were left.
        // -----------------------------------------------------------------
        class PlayerSystem : public System
        {
        public:
            void init(EntityManager* entityManager, ComponentManager* componentManager) override;
            void update(const std::vector<Entity>& entities, float deltaTime) override;

            // Gravity only applies where there is a world to fall onto.
            // CollisionSystem stops entirely when it holds no world, so leaving
            // gravity on in that state accelerates the player downward with
            // nothing to catch them. GameScreen turns it on with the world and
            // off again when the world goes away.
            void SetGravityEnabled(bool enabled) { m_GravityEnabled = enabled; }
            bool IsGravityEnabled() const { return m_GravityEnabled; }

            // Movement scale from the server's Speed Boost buff. GameScreen
            // reads the buff set the server owns and pushes the strongest
            // multiplier here; 1.0 is the plain walk. The server scales its
            // own anti-speed-hack cap by the same value, so the faster pace
            // never reads as a hack.
            void SetSpeedMultiplier(float multiplier) { m_SpeedMultiplier = multiplier; }
            float GetSpeedMultiplier() const { return m_SpeedMultiplier; }

            // Jumps triggered since the last consume: incremented where the
            // jump impulse is actually written (key edge AND grounded), so a
            // held key or a jump refused mid-air counts zero. Single game
            // thread, so no synchronisation.
            int ConsumeJumpCount()
            {
                const int jumps = m_JumpsPending;
                m_JumpsPending = 0;
                return jumps;
            }

        private:
            float m_MoveSpeed = 100.0f;      // pixels per second, horizontal
            float m_SpeedMultiplier = 1.0f;  // Speed Boost buff, server-owned

            // Downward acceleration, pixels per second squared. 1600 is 50
            // tiles/s^2 at the 32px tile size.
            float m_Gravity = 1600.0f;

            // Upward impulse, pixels per second. The apex is v^2 / 2g = 96px,
            // three tiles, which leaves a tile of headroom under the server's
            // MaxJumpHeight of 4 - the anti-hover check rejects anything that
            // climbs further above the ground it left.
            float m_JumpSpeed = 555.0f;

            // Terminal velocity, pixels per second. At the 0.1s position send
            // interval this is 2.8 tiles per packet, comfortably inside the
            // server's MaxSingleMoveDistance of 16.
            float m_MaxFallSpeed = 900.0f;

            bool m_GravityEnabled = false;

            // Jump is edge-triggered per entity, so holding the key through a
            // landing does not bounce the player straight back up. Keyed by
            // entity index rather than by Entity, which has no std::hash; a
            // recycled index can carry one stale frame, which at worst costs a
            // single edge and is not worth a specialisation.
            std::unordered_map<uint32_t, bool> m_JumpHeld;

            // Jumps taken but not yet consumed by ConsumeJumpCount().
            int m_JumpsPending = 0;
        };
    }
}
