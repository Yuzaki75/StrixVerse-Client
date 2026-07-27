#pragma once

#include <cstdint>
#include <limits>

namespace StrixVerse
{
    namespace ECS
    {
        // Entity ID consists of an index and a generation (version) to handle recycling.
        struct EntityID
        {
            uint32_t index = 0;
            uint32_t generation = 0;

            bool operator==(const EntityID& other) const
            {
                return index == other.index && generation == other.generation;
            }

            bool operator!=(const EntityID& other) const
            {
                return !(*this == other);
            }
        };

        // Entity class represents a lightweight handle to an entity.
        class Entity
        {
        public:
            Entity() = default;
            Entity(uint32_t index, uint32_t generation);

            // Check if the entity is valid (alive and not recycled).
            bool isValid() const;

            // Get the index and generation.
            uint32_t getIndex() const;
            uint32_t getGeneration() const;

            // Get the raw ID.
            EntityID getID() const;

            // Equality operators.
            bool operator==(const Entity& other) const;
            bool operator!=(const Entity& other) const;

        private:
            EntityID m_ID{};
        };

        // Represents an invalid/null entity.
        inline const Entity NULL_ENTITY{ (std::numeric_limits<uint32_t>::max)(), (std::numeric_limits<uint32_t>::max)() };
    }
}