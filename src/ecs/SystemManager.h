#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include "Entity.h"
#include "Component.h"
#include "System.h"
#include "EntityManager.h"
#include "ComponentManager.h"

namespace StrixVerse
{
    namespace ECS
    {
        class SystemManager
        {
        public:
            explicit SystemManager(EntityManager* entityManager, ComponentManager* componentManager);
            ~SystemManager() = default;

            SystemManager(const SystemManager&) = delete;
            SystemManager& operator=(const SystemManager&) = delete;
            SystemManager(SystemManager&&) = delete;
            SystemManager& operator=(SystemManager&&) = delete;

            // Create a system (but do not add it to the manager).
            template<typename T, typename... TArgs>
            std::shared_ptr<T> createSystem(TArgs&&... args)
            {
                static_assert(std::is_base_of<System, T>::value, "Template argument must inherit from System.");

                auto system = std::make_shared<T>(std::forward<TArgs>(args)...);
                // Initialize the system with the managers.
                system->init(m_pEntityManager, m_pComponentManager);
                return system;
            }

            // Add a system to the manager.
            void addSystem(std::shared_ptr<System> system)
            {
                m_Systems.push_back(system);
                m_SystemEntities.resize(m_Systems.size());

                // Now, we need to assign entities to this system based on its signature.
                // We'll iterate over all living entities and see if they match the system's signature.
                auto entities = m_pEntityManager->getLivingEntities();
                for (const auto& entity : entities)
                {
                    if (matchesSignature(entity, system->getSignature()))
                    {
                        m_SystemEntities.back().push_back(entity);
                    }
                }
            }

            // Remove a system (by type). Returns true if found and removed.
            template<typename T>
            bool removeSystem()
            {
                auto it = std::find_if(m_Systems.begin(), m_Systems.end(),
                    [](const std::shared_ptr<System>& ptr)
                    {
                        return dynamic_cast<T*>(ptr.get()) != nullptr;
                    });

                if (it != m_Systems.end())
                {
                    size_t index = std::distance(m_Systems.begin(), it);
                    m_Systems.erase(it);
                    m_SystemEntities.erase(m_SystemEntities.begin() + index);
                    return true;
                }
                return false;
            }

            // Get a system (by type).
            template<typename T>
            std::shared_ptr<T> getSystem()
            {
                auto it = std::find_if(m_Systems.begin(), m_Systems.end(),
                    [](const std::shared_ptr<System>& ptr)
                    {
                        return dynamic_cast<T*>(ptr.get()) != nullptr;
                    });

                if (it != m_Systems.end())
                {
                    return std::static_pointer_cast<T>(*it);
                }
                return nullptr;
            }

            // Update all systems.
            void update(float dt)
            {
                // Update each system with its list of entities.
                for (size_t i = 0; i < m_Systems.size(); ++i)
                {
                    m_Systems[i]->update(m_SystemEntities[i], dt);
                }
            }

            // Render all systems (if they have a render method).
            void render()
            {
                for (size_t i = 0; i < m_Systems.size(); ++i)
                {
                    m_Systems[i]->render(m_SystemEntities[i]);
                }
            }

            // Called when an entity is destroyed.
            void onEntityDestroyed(Entity entity)
            {
                // Remove the entity from all system entity lists.
                for (auto& entityList : m_SystemEntities)
                {
                    auto it = std::remove_if(entityList.begin(), entityList.end(),
                        [&](const Entity& e)
                        {
                            return e == entity;
                        });
                    entityList.erase(it, entityList.end());
                }
            }

            // Called when an entity's component has been added or removed.
            // We need to check if the entity still matches each system's signature.
            void onEntitySignatureChanged(Entity entity)
            {
                // For each system, check if the entity matches the signature.
                // If it does and it's not in the list, add it.
                // If it doesn't match and it is in the list, remove it.
                for (size_t i = 0; i < m_Systems.size(); ++i)
                {
                    bool matches = matchesSignature(entity, m_Systems[i]->getSignature());
                    auto& entityList = m_SystemEntities[i];

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

        private:
            // Helper to check if an entity matches a signature.
            bool matchesSignature(Entity entity, const std::bitset<MAX_COMPONENTS>& signature)
            {
                auto entitySignature = m_pComponentManager->getEntitySignature(entity);
                return (entitySignature & signature) == signature;
            }

            EntityManager* m_pEntityManager = nullptr;
            ComponentManager* m_pComponentManager = nullptr;
            std::vector<std::shared_ptr<System>> m_Systems;
            std::vector<std::vector<Entity>> m_SystemEntities;
        };
    }
}