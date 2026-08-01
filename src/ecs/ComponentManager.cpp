#include "ComponentManager.h"
#include <algorithm>
#include <memory>
#include <vector>
#include <cstring>
#include <type_traits>
#include <utility>
#include <cstdint>
#include <bitset>

namespace StrixVerse
{
    namespace ECS
    {
        ComponentManager::ComponentManager(size_t maxEntities)
        {
            Initialize(maxEntities);
        }

        ComponentManager::ComponentManager()
        {
            // Default constructor - Initialize must be called separately
        }

        void ComponentManager::Initialize(size_t maxEntities)
        {
            m_MaxEntities = maxEntities;
            m_ComponentTypes.clear();
            m_ComponentTypes.resize(MAX_COMPONENTS, ComponentTypeInfo{});

            // Initialize storage for each component type
            m_ComponentStorage.clear();
            m_ComponentStorage.resize(MAX_COMPONENTS);

            // Initialize entity tracking for each component type (bit arrays)
            m_ComponentEntities.clear();
            m_ComponentEntities.resize(MAX_COMPONENTS);

            // Initialize component counts
            m_ComponentCounts.clear();
            m_ComponentCounts.resize(MAX_COMPONENTS, 0);
        }

        std::bitset<MAX_COMPONENTS> ComponentManager::getEntitySignature(Entity entity) const
        {
            std::bitset<MAX_COMPONENTS> signature;

            // Check each component type to see if the entity has it
            for (size_t compID = 0; compID < MAX_COMPONENTS; ++compID)
            {
                if (m_ComponentTypes[compID].size > 0 && // This component type is registered
                    m_ComponentStorage[compID] &&        // Storage exists
                    entity.id < m_MaxEntities)           // Entity ID is valid
                {
                    // Check if this specific entity has this component
                    if (entity.id < m_ComponentEntities[compID].size() * 64) // Check bounds
                    {
                        size_t wordIndex = entity.id / 64;
                        if (wordIndex < m_ComponentEntities[compID].size())
                        {
                            if (m_ComponentEntities[compID][wordIndex] & (1ULL << (entity.id % 64)))
                            {
                                signature.set(compID);
                            }
                        }
                    }
                }
            }

            return signature;
        }

        void ComponentManager::entityDestroyed(Entity entity)
        {
            // Remove all components for this entity
            for (size_t compID = 0; compID < MAX_COMPONENTS; ++compID)
            {
                if (m_ComponentTypes[compID].size > 0 && // This component type is registered
                    m_ComponentStorage[compID] &&        // Storage exists
                    entity.id < m_MaxEntities)           // Entity ID is valid
                {
                    std::size_t offset = entity.id * m_ComponentTypes[compID].size;
                    if (offset + m_ComponentTypes[compID].size <= m_ComponentStorage[compID]->size())
                    {
                        std::fill_n(&(*m_ComponentStorage[compID])[offset], m_ComponentTypes[compID].size, std::byte{0});
                    }

                    // Mark that this entity no longer has this component
                    if (entity.id < m_ComponentEntities[compID].size() * 64) // Check bounds
                    {
                        size_t wordIndex = entity.id / 64;
                        if (wordIndex < m_ComponentEntities[compID].size())
                        {
                            m_ComponentEntities[compID][wordIndex] &= ~(1ULL << (entity.id % 64));
                        }
                    }
                }
            }
        }
    }
}