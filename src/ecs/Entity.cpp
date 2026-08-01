#include "Entity.h"

namespace StrixVerse
{
    namespace ECS
    {
        Entity::Entity(uint32_t index, uint32_t generation)
            : m_ID{ index, generation }
        {
            id = index;
        }

        bool Entity::isValid() const
        {
            // An entity is valid if its index is not the max value (which we use for null)
            // and the generation matches what the EntityManager has on record.
            // However, the EntityManager is the authority on validity.
            // For now, we'll just check that the index is not null.
            // The EntityManager will set the generation and index, and we trust that.
            // A more robust system would have the EntityManager validate, but we keep it simple.
            return m_ID.index != std::numeric_limits<uint32_t>::max();
        }

        uint32_t Entity::getIndex() const
        {
            return m_ID.index;
        }

        uint32_t Entity::getGeneration() const
        {
            return m_ID.generation;
        }

        EntityID Entity::getID() const
        {
            return m_ID;
        }

        bool Entity::operator==(const Entity& other) const
        {
            return m_ID == other.m_ID;
        }

        bool Entity::operator!=(const Entity& other) const
        {
            return !(*this == other);
        }
    }
}