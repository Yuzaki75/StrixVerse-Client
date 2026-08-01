#include "SystemManager.h"
#include "EntityManager.h"
#include "ComponentManager.h"

namespace StrixVerse
{
    namespace ECS
    {
        SystemManager::SystemManager(EntityManager *entityManager, ComponentManager *componentManager)
            : m_pEntityManager(entityManager), m_pComponentManager(componentManager)
        {
        }

        void SystemManager::addSystem(std::shared_ptr<System> system)
        {
            m_Systems.push_back(system);
            m_SystemEntities.resize(m_Systems.size());

            auto entities = m_pEntityManager->getLivingEntities();
            for (const auto &entity : entities)
            {
                if (matchesSignature(entity, system->getSignature()))
                {
                    m_SystemEntities.back().push_back(entity);
                }
            }
        }

        void SystemManager::update(float dt)
        {
            for (size_t i = 0; i < m_Systems.size(); ++i)
            {
                m_Systems[i]->update(m_SystemEntities[i], dt);
            }
        }

        void SystemManager::render()
        {
            for (size_t i = 0; i < m_Systems.size(); ++i)
            {
                m_Systems[i]->render(m_SystemEntities[i]);
            }
        }

        void SystemManager::onEntityDestroyed(Entity entity)
        {
            for (auto &entityList : m_SystemEntities)
            {
                auto it = std::remove_if(entityList.begin(), entityList.end(),
                                         [&](const Entity &e)
                                         {
                                             return e == entity;
                                         });
                entityList.erase(it, entityList.end());
            }
        }

        void SystemManager::onEntitySignatureChanged(Entity entity)
        {
            for (size_t i = 0; i < m_Systems.size(); ++i)
            {
                bool matches = matchesSignature(entity, m_Systems[i]->getSignature());
                auto &entityList = m_SystemEntities[i];

                auto it = std::find(entityList.begin(), entityList.end(), entity);
                bool inList = (it != entityList.end());

                if (matches && !inList)
                {
                    entityList.push_back(entity);
                }
                else if (!matches && inList)
                {
                    entityList.erase(it);
                }
            }
        }

        bool SystemManager::matchesSignature(Entity entity, const std::bitset<MAX_COMPONENTS> &signature)
        {
            auto entitySignature = m_pComponentManager->getEntitySignature(entity);
            return (entitySignature & signature) == signature;
        }
    }
}