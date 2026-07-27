#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include "Entity.h"

namespace StrixVerse
{
    namespace ECS
    {
        class EntityManager
        {
        public:
            static const uint32_t MAX_ENTITIES = 10000;

            EntityManager();
            ~EntityManager() = default;

            EntityManager(const EntityManager&) = delete;
            EntityManager& operator=(const EntityManager&) = delete;
            EntityManager(EntityManager&&) = delete;
            EntityManager& operator=(EntityManager&&) = delete;

            Entity createEntity();
            void destroyEntity(Entity entity);
            bool isValid(Entity entity) const;

            uint32_t getLivingEntityCount() const;
            bool isEntityFull() const;

            // Get a vector of all living entities.
            std::vector<Entity> getLivingEntities() const;

        private:
            // Each index in this vector corresponds to an entity index.
            // The value is the generation for that index.
            std::vector<EntityID> m_Entities{};
            // Tracks whether an entity is currently alive.
            std::vector<char> m_EntityAlive{};
            // Free list of indices that have been destroyed and can be reused.
            std::vector<uint32_t> m_FreeIndices{};
            uint32_t m_LivingEntityCount = 0;
        };
    }
}