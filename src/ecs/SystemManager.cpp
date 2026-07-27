#include "SystemManager.h"
#include "EntityManager.h"
#include "ComponentManager.h"

namespace StrixVerse
{
    namespace ECS
    {
        SystemManager::SystemManager(EntityManager* entityManager, ComponentManager* componentManager)
            : m_pEntityManager(entityManager), m_pComponentManager(componentManager)
        {
        }
    }
}