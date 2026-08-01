#pragma once

#include <vector>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <memory>
#include "Entity.h"
#include "Component.h"
#include "SystemManager.h"

namespace StrixVerse
{
    namespace ECS
    {
        struct ComponentTypeInfo
        {
            std::size_t size = 0;
            std::size_t alignment = 0;
            std::uint32_t id = 0; // Will be set to ComponentType::template Get<T>()
        };

        class EntityManager;

        class ComponentManager
        {
        public:
            ComponentManager();
            explicit ComponentManager(size_t maxEntities);
            ~ComponentManager() = default;

            // Initialize the component manager with the maximum number of entities.
            void Initialize(size_t maxEntities);

            // Set the system manager for notification chain.
            void SetSystemManager(SystemManager *sm) { m_pSystemManager = sm; }

            // Register a component type (called automatically when first used).
            template <typename T>
            void registerComponent()
            {
                const size_t compID = ComponentType::template Get<T>();
                assert(compID < MAX_COMPONENTS && "Component type ID out of range.");

                // Store type info
                m_ComponentTypes[compID] = {sizeof(T), alignof(T), static_cast<std::uint32_t>(compID)};

                // Ensure storage exists for this component type
                if (!m_ComponentStorage[compID])
                {
                    m_ComponentStorage[compID] = std::make_unique<std::vector<std::byte>>();
                    // Reserve space for all entities
                    m_ComponentStorage[compID]->resize(m_MaxEntities * sizeof(T));
                }

                // Ensure entity tracking exists
                if (m_ComponentEntities.size() <= compID)
                {
                    m_ComponentEntities.resize(MAX_COMPONENTS);
                    m_ComponentCounts.resize(MAX_COMPONENTS, 0);
                }
                if (m_ComponentEntities[compID].empty())
                {
                    // One bit per entity, rounded up to 64-bit words
                    size_t numWords = (m_MaxEntities + 63) / 64;
                    m_ComponentEntities[compID].assign(numWords, 0);
                }
            }

            // Add a component of type T to the given entity.
            // Returns a reference to the added component.
            template <typename T>
            T &addComponent(Entity entity, T component)
            {
                const size_t compID = ComponentType::template Get<T>();
                assert(compID < MAX_COMPONENTS && "Component type ID out of range.");
                assert(m_ComponentTypes[compID].size == sizeof(T) && "Component type mismatch.");

                // Ensure storage exists
                if (!m_ComponentStorage[compID])
                {
                    m_ComponentStorage[compID] = std::make_unique<std::vector<std::byte>>();
                    m_ComponentStorage[compID]->resize(m_MaxEntities * sizeof(T));
                }

                // Ensure entity tracking exists
                if (m_ComponentEntities.size() <= compID)
                {
                    m_ComponentEntities.resize(MAX_COMPONENTS);
                    m_ComponentCounts.resize(MAX_COMPONENTS, 0);
                }
                if (m_ComponentEntities[compID].empty())
                {
                    size_t numWords = (m_MaxEntities + 63) / 64;
                    m_ComponentEntities[compID].assign(numWords, 0);
                }

                // Placement new to construct the object
                std::size_t offset = entity.id * sizeof(T);
                void *ptr = &(*m_ComponentStorage[compID])[offset];
                T *constructed = new (ptr) T(std::move(component));

                // Mark that this entity has this component
                size_t wordIndex = entity.id / 64;
                if (wordIndex < m_ComponentEntities[compID].size())
                {
                    m_ComponentEntities[compID][wordIndex] |= (1ULL << (entity.id % 64));
                }

                // Update count (approximate - we're not tracking removals perfectly here)
                if (m_ComponentCounts[compID] <= entity.id)
                {
                    m_ComponentCounts[compID] = entity.id + 1;
                }

                // Notify system manager about signature change
                if (m_pSystemManager)
                {
                    m_pSystemManager->onEntitySignatureChanged(entity);
                }

                return *constructed;
            }

            // Remove a component of type T from the given entity.
            template <typename T>
            void removeComponent(Entity entity)
            {
                const size_t compID = ComponentType::template Get<T>();
                assert(compID < MAX_COMPONENTS && "Component type ID out of range.");

                if (m_ComponentTypes[compID].size > 0 &&
                    m_ComponentStorage[compID] &&
                    entity.id < m_MaxEntities)
                {
                    // Call destructor
                    std::size_t offset = entity.id * sizeof(T);
                    if (offset + sizeof(T) <= m_ComponentStorage[compID]->size())
                    {
                        void *ptr = &(*m_ComponentStorage[compID])[offset];
                        std::destroy_at(static_cast<T *>(ptr));
                    }

                    // Mark that this entity no longer has this component
                    if (m_ComponentEntities.size() > compID && !m_ComponentEntities[compID].empty())
                    {
                        size_t wordIndex = entity.id / 64;
                        if (wordIndex < m_ComponentEntities[compID].size())
                        {
                            m_ComponentEntities[compID][wordIndex] &= ~(1ULL << (entity.id % 64));
                        }
                    }

                    // Notify system manager about signature change
                    if (m_pSystemManager)
                    {
                        m_pSystemManager->onEntitySignatureChanged(entity);
                    }
                }
            }

            // Get a reference to the component of type T for the given entity.
            // Returns nullptr if the entity does not have the component.
            template <typename T>
            T *getComponent(Entity entity)
            {
                const size_t compID = ComponentType::template Get<T>();
                assert(compID < MAX_COMPONENTS && "Component type ID out of range.");

                if (m_ComponentTypes[compID].size == 0 ||
                    !m_ComponentStorage[compID] ||
                    entity.id >= m_MaxEntities)
                {
                    return nullptr;
                }

                // Check if entity has this component
                if (m_ComponentEntities.size() > compID && !m_ComponentEntities[compID].empty())
                {
                    size_t wordIndex = entity.id / 64;
                    if (wordIndex < m_ComponentEntities[compID].size() &&
                        (m_ComponentEntities[compID][wordIndex] & (1ULL << (entity.id % 64))))
                    {
                        // Return pointer to the constructed object
                        std::size_t offset = entity.id * sizeof(T);
                        if (offset + sizeof(T) <= m_ComponentStorage[compID]->size())
                        {
                            return reinterpret_cast<T *>(&(*m_ComponentStorage[compID])[offset]);
                        }
                    }
                }

                return nullptr;
            }

            // Get a const reference to the component of type T for the given entity.
            // Returns nullptr if the entity does not have the component.
            template <typename T>
            const T *getComponent(Entity entity) const
            {
                const size_t compID = ComponentType::template Get<T>();
                assert(compID < MAX_COMPONENTS && "Component type ID out of range.");

                if (m_ComponentTypes[compID].size == 0 ||
                    !m_ComponentStorage[compID] ||
                    entity.id >= m_MaxEntities)
                {
                    return nullptr;
                }

                // Check if entity has this component
                if (m_ComponentEntities.size() > compID && !m_ComponentEntities[compID].empty())
                {
                    size_t wordIndex = entity.id / 64;
                    if (wordIndex < m_ComponentEntities[compID].size() &&
                        (m_ComponentEntities[compID][wordIndex] & (1ULL << (entity.id % 64))))
                    {
                        // Return pointer to the constructed object
                        std::size_t offset = entity.id * sizeof(T);
                        if (offset + sizeof(T) <= m_ComponentStorage[compID]->size())
                        {
                            return reinterpret_cast<const T *>(&(*m_ComponentStorage[compID])[offset]);
                        }
                    }
                }

                return nullptr;
            }

            // Check if the entity has a component of type T.
            template <typename T>
            bool hasComponent(Entity entity) const
            {
                const size_t compID = ComponentType::template Get<T>();
                assert(compID < MAX_COMPONENTS && "Component type ID out of range.");

                if (!m_ComponentTypes[compID].size ||
                    !m_ComponentStorage[compID] ||
                    entity.id >= m_MaxEntities ||
                    m_ComponentEntities.size() <= compID ||
                    m_ComponentEntities[compID].empty())
                {
                    return false;
                }

                // Check if this specific entity has this component
                if (entity.id < m_ComponentEntities[compID].size() * 64)
                {
                    size_t wordIndex = entity.id / 64;
                    if (wordIndex < m_ComponentEntities[compID].size())
                    {
                        return (m_ComponentEntities[compID][wordIndex] & (1ULL << (entity.id % 64))) != 0;
                    }
                }

                return false;
            }

            // Get the signature (bitset) of the entity, indicating which components it has.
            std::bitset<MAX_COMPONENTS> getEntitySignature(Entity entity) const;

            // Remove all components from the given entity (called when entity is destroyed).
            void entityDestroyed(Entity entity);

        private:
            // For each component type, we store the type information
            std::vector<ComponentTypeInfo> m_ComponentTypes;

            // Storage for component data: one vector per component type
            // Each vector holds raw bytes for all entities (indexed by entity ID)
            std::vector<std::unique_ptr<std::vector<std::byte>>> m_ComponentStorage;

            // For each component type, we store a bit array indicating which entities have that component
            // Each inner vector contains 64-bit words, where each bit represents an entity
            std::vector<std::vector<uint64_t>> m_ComponentEntities;

            // Track the number of components of each type (approximate)
            std::vector<size_t> m_ComponentCounts;

            // Track the maximum number of entities we can handle
            size_t m_MaxEntities = 0;

            // Non-owning pointer to the system manager for notification chain
            SystemManager *m_pSystemManager = nullptr;
        };
    }
}