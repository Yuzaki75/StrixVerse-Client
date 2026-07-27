#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <stdexcept>
#include <bitset>
#include "Entity.h"
#include "Component.h"
#include "System.h" // For MAX_COMPONENTS

namespace StrixVerse
{
    namespace ECS
    {
        // Forward declaration.
        class EntityManager;

        // Interface for a component array.
        class IComponentArray
        {
        public:
            virtual ~IComponentArray() = default;
            virtual void entityDestroyed(Entity entity) = 0;
            // We'll add methods to get the component and check if an entity has it.
            // These will be implemented in the derived template class.
        };

        template<typename T>
        class ComponentArray : public IComponentArray
        {
        public:
            explicit ComponentArray(uint32_t maxEntities)
                : m_ComponentArray(maxEntities), m_EntityMask(maxEntities, false)
            {
                // We assume T is default constructible for the sake of simplicity.
                // If not, we would need to use a different approach (like optional or pointer).
                // For now, we'll proceed with this assumption.
            }

            void insertEntity(Entity entity, T component)
            {
                uint32_t idx = entity.getID().index;
                if (idx >= m_ComponentArray.size())
                {
                    throw std::out_of_range("Entity index out of range.");
                }
                m_ComponentArray[idx] = component;
                m_EntityMask[idx] = true;
            }

            void removeEntity(Entity entity)
            {
                uint32_t idx = entity.getID().index;
                if (idx >= m_EntityMask.size())
                {
                    return;
                }
                m_EntityMask[idx] = false;
            }

            T& getComponent(Entity entity)
            {
                uint32_t idx = entity.getID().index;
                if (idx >= m_ComponentArray.size() || !m_EntityMask[idx])
                {
                    throw std::runtime_error("Component not found for entity.");
                }
                return m_ComponentArray[idx];
            }

            bool hasComponent(Entity entity) const
            {
                uint32_t idx = entity.getID().index;
                if (idx >= m_EntityMask.size())
                {
                    return false;
                }
                return m_EntityMask[idx];
            }

            void entityDestroyed(Entity entity) override
            {
                removeEntity(entity);
            }

        private:
            std::vector<T> m_ComponentArray;
            std::vector<bool> m_EntityMask;
        };

        class ComponentManager
        {
        public:
            explicit ComponentManager(uint32_t maxEntities)
                : m_MaxEntities(maxEntities)
            {
                // Initialize the component signature for each entity.
                m_EntityComponentSignature.resize(maxEntities);
                // Reserve space for component types (we don't know how many, but we can reserve a reasonable amount).
                m_ComponentArrays.reserve(128);
            }

            ~ComponentManager() = default;
            ComponentManager(const ComponentManager&) = delete;
            ComponentManager& operator=(const ComponentManager&) = delete;
            ComponentManager(ComponentManager&&) = delete;
            ComponentManager& operator=(ComponentManager&&) = delete;

            // Register a component type. Must be called before using the component type.
            template<typename T>
            void registerComponent()
            {
                const uint32_t componentID = ComponentType::Get<T>();
                if (componentID >= MAX_COMPONENTS)
                {
                    throw std::runtime_error("Exceeded maximum number of component types.");
                }
                // Ensure our vector is big enough.
                if (componentID >= m_ComponentArrays.size())
                {
                    m_ComponentArrays.resize(componentID + 1, nullptr);
                }
                m_ComponentArrays[componentID] = std::make_unique<ComponentArray<T>>(m_MaxEntities);
            }

            // Add a component to an entity.
            template<typename T>
            void addComponent(Entity entity, T component)
            {
                getComponentArray<T>()->insertEntity(entity, component);
                // Update the entity's component signature.
                uint32_t componentID = ComponentType::Get<T>();
                if (entity.getID().index < m_EntityComponentSignature.size())
                {
                    m_EntityComponentSignature[entity.getID().index].set(componentID);
                }
            }

            // Remove a component from an entity.
            template<typename T>
            void removeComponent(Entity entity)
            {
                getComponentArray<T>()->removeEntity(entity);
                // Update the entity's component signature.
                uint32_t componentID = ComponentType::Get<T>();
                if (entity.getID().index < m_EntityComponentSignature.size())
                {
                    m_EntityComponentSignature[entity.getID().index].reset(componentID);
                }
            }

            // Get a component from an entity.
            template<typename T>
            T& getComponent(Entity entity)
            {
                return getComponentArray<T>()->getComponent(entity);
            }

            // Check if an entity has a component.
            template<typename T>
            bool hasComponent(Entity entity) const
            {
                return getComponentArray<T>()->hasComponent(entity);
            }

            // Notify that an entity has been destroyed (to clean up its components).
            void entityDestroyed(Entity entity)
            {
                // Notify each component array that the entity was destroyed.
                for (auto& componentArray : m_ComponentArrays)
                {
                    if (componentArray)
                    {
                        componentArray->entityDestroyed(entity);
                    }
                }
                // Clear the entity's component signature.
                if (entity.getID().index < m_EntityComponentSignature.size())
                {
                    m_EntityComponentSignature[entity.getID().index].reset();
                }
            }

            // Get the component signature for an entity (which components it has).
            const std::bitset<MAX_COMPONENTS>& getEntitySignature(Entity entity) const
            {
                if (entity.getID().index >= m_EntityComponentSignature.size())
                {
                    static std::bitset<MAX_COMPONENTS> empty;
                    return empty;
                }
                return m_EntityComponentSignature[entity.getID().index];
            }

        private:
            // Helper to get the component array for a given type.
            template<typename T>
            ComponentArray<T>* getComponentArray()
            {
                uint32_t componentID = ComponentType::Get<T>();
                if (componentID >= m_ComponentArrays.size() || !m_ComponentArrays[componentID])
                {
                    throw std::runtime_error("Component not registered.");
                }
                return static_cast<ComponentArray<T>*>(m_ComponentArrays[componentID].get());
            }

            uint32_t m_MaxEntities;
            std::vector<std::unique_ptr<IComponentArray>> m_ComponentArrays;
            // For each entity, store a bitset of which components it has.
            std::vector<std::bitset<MAX_COMPONENTS>> m_EntityComponentSignature;
        };
    }
}