#include "EntityManager.h"
#include <stdexcept>

namespace StrixVerse
{
    namespace ECS
    {
        EntityManager::EntityManager()
        {
            // Initialize the entity vector with max entities, each with generation 0.
            m_Entities.resize(MAX_ENTITIES, EntityID{0});
            // Initially, all entities are not alive.
            m_EntityAlive.resize(MAX_ENTITIES, false);
            // Initially, all entities are free (we can use them).
            for (uint32_t i = 0; i < MAX_ENTITIES; ++i)
            {
                m_FreeIndices.push_back(i);
            }
        }

        Entity EntityManager::createEntity()
        {
            if (m_LivingEntityCount >= MAX_ENTITIES)
            {
                throw std::runtime_error("Too many entities. Maximum reached.");
            }

            // Get an index from the free list.
            uint32_t id = m_FreeIndices.back();
            m_FreeIndices.pop_back();
            ++m_LivingEntityCount;

            // Mark the entity as alive.
            m_EntityAlive[id] = true;
            // Increment the generation for this id.
            m_Entities[id].generation++;

            // Return the entity with the index and current generation.
            return Entity{id, m_Entities[id].generation};
        }

        void EntityManager::destroyEntity(Entity entity)
        {
            if (!isValid(entity))
            {
                return; // Already destroyed or invalid.
            }

            // Mark the entity as not alive.
            m_EntityAlive[entity.id] = false;
            // Add its index to the free list.
            m_FreeIndices.push_back(entity.id);
            --m_LivingEntityCount;
        }

        bool EntityManager::isValid(Entity entity) const
        {
            // Check if the entity index is within bounds and the entity is alive.
            if (entity.id >= m_Entities.size())
            {
                return false;
            }
            return m_EntityAlive[entity.id] && (m_Entities[entity.id].generation == entity.generation);
        }

        uint32_t EntityManager::getLivingEntityCount() const
        {
            return m_LivingEntityCount;
        }

        bool EntityManager::isEntityFull() const
        {
            return (m_LivingEntityCount >= MAX_ENTITIES);
        }

        std::vector<Entity> EntityManager::getLivingEntities() const
        {
            std::vector<Entity> entities;
            entities.reserve(m_LivingEntityCount);
            for (uint32_t i = 0; i < m_Entities.size(); ++i)
            {
                if (m_EntityAlive[i])
                {
                    entities.emplace_back(i, m_Entities[i].generation);
                }
            }
            return entities;
        }
    }
}