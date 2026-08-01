#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <stdexcept>
#include "Entity.h"
#include "Component.h"
#include "System.h"

namespace StrixVerse
{
    namespace ECS
    {
        class EntityManager;
        class ComponentManager;

        class SystemManager
        {
        public:
            explicit SystemManager(EntityManager *entityManager, ComponentManager *componentManager);
            ~SystemManager() = default;

            SystemManager(const SystemManager &) = delete;
            SystemManager &operator=(const SystemManager &) = delete;
            SystemManager(SystemManager &&) = delete;
            SystemManager &operator=(SystemManager &&) = delete;

            // Create a system (but do not add it to the manager).
            template <typename T, typename... TArgs>
            std::shared_ptr<T> createSystem(TArgs &&...args)
            {
                static_assert(std::is_base_of<System, T>::value, "Template argument must inherit from System.");

                auto system = std::make_shared<T>(std::forward<TArgs>(args)...);
                // Initialize the system with the managers.
                system->init(m_pEntityManager, m_pComponentManager);
                return system;
            }

            // Add a system to the manager.
            void addSystem(std::shared_ptr<System> system);

            // Remove a system (by type). Returns true if found and removed.
            template <typename T>
            bool removeSystem();

            // Get a system (by type).
            template <typename T>
            std::shared_ptr<T> getSystem();

            // Update all systems.
            void update(float dt);

            // Render all systems (if they have a render method).
            void render();

            // Called when an entity is destroyed.
            void onEntityDestroyed(Entity entity);

            // Called when an entity's component has been added or removed.
            // We need to check if the entity still matches each system's signature.
            void onEntitySignatureChanged(Entity entity);

        private:
            // Helper to check if an entity matches a signature.
            bool matchesSignature(Entity entity, const std::bitset<MAX_COMPONENTS> &signature);

            EntityManager *m_pEntityManager = nullptr;
            ComponentManager *m_pComponentManager = nullptr;
            std::vector<std::shared_ptr<System>> m_Systems;
            std::vector<std::vector<Entity>> m_SystemEntities;
        };

        template <typename T>
        bool SystemManager::removeSystem()
        {
            auto it = std::find_if(m_Systems.begin(), m_Systems.end(),
                                   [](const std::shared_ptr<System> &ptr)
                                   {
                                       return dynamic_cast<T *>(ptr.get()) != nullptr;
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

        template <typename T>
        std::shared_ptr<T> SystemManager::getSystem()
        {
            auto it = std::find_if(m_Systems.begin(), m_Systems.end(),
                                   [](const std::shared_ptr<System> &ptr)
                                   {
                                       return dynamic_cast<T *>(ptr.get()) != nullptr;
                                   });

            if (it != m_Systems.end())
            {
                return std::static_pointer_cast<T>(*it);
            }
            return nullptr;
        }
    }
}