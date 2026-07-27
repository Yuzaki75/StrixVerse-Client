#pragma once

#include <vector>
#include <bitset>
#include <cstdint>
#include <type_traits>
#include <algorithm>
#include <stdexcept>

#include "Entity.h"
#include "Component.h"

namespace StrixVerse
{
    namespace ECS
    {
        // Maximum number of component types we support (for bitset).
        static const uint32_t MAX_COMPONENTS = 256;

        // Forward declaration.
        class EntityManager;
        class ComponentManager;

        // Base class for all systems.
        // Systems define which components they are interested in and contain the logic to process entities.
        class System
        {
        public:
            virtual ~System() = default;

            // Delete copy and move to prevent slicing.
            System(const System&) = delete;
            System& operator=(const System&) = delete;
            System(System&&) = delete;
            System& operator=(System&&) = delete;

            // Initialize the system (called once when added to the manager).
            virtual void init(EntityManager* entityManager, ComponentManager* componentManager) {
                (void)entityManager;
                (void)componentManager;
            }

            // Update the system (called every frame).
            // entities: list of entities that match the system's signature.
            // dt is the delta time in seconds.
            virtual void update(const std::vector<Entity>& entities, float dt) {
                (void)entities;
                (void)dt;
            }

            // Render the system (called after update, if applicable).
            // entities: list of entities that match the system's signature.
            virtual void render(const std::vector<Entity>& entities) {
                (void)entities;
            }

            // Set the signature (component types) that this system is interested in.
            // Usage: setSignature<TransformComponent, SpriteComponent>();
            template<typename... T>
            void setSignature()
            {
                static_assert(sizeof...(T) > 0, "Signature must have at least one component.");
                m_Signature.reset();
                (m_Signature.set(ComponentType::Get<T>()), ...);
            }

            // Get the signature.
            const std::bitset<MAX_COMPONENTS>& getSignature() const { return m_Signature; }

        protected:
            System() = default;

            // Pointers to the managers (set by the SystemManager during initialization).
            EntityManager* m_pEntityManager = nullptr;
            ComponentManager* m_pComponentManager = nullptr;

        private:
            // The signature of components this system cares about.
            std::bitset<MAX_COMPONENTS> m_Signature;
        };
    }
}