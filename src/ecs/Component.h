#pragma once

#include <cstdint>
#include <typeindex>
#include <unordered_map>

namespace StrixVerse
{
    namespace ECS
    {
        // Base class for all components.
        // Components are data-only structs that inherit from this class.
        // Each component type gets a unique ID via the ComponentType system.
        class Component
        {
        public:
            virtual ~Component() = default;

            // Disable copy and move to prevent slicing and unintended behavior.
            Component(const Component&) = delete;
            Component& operator=(const Component&) = delete;
            Component(Component&&) = delete;
            Component& operator=(Component&&) = delete;

        protected:
            Component() = default;
        };

        // Type ID system for components.
        // We'll use a simple counter to assign a unique ID to each component type.
        // Note: We limit the number of component types to 64 so we can use a bitmask.
        class ComponentType
        {
        public:
            template <typename T>
            static uint8_t Get()
            {
                static uint8_t typeID = GetNextID();
                return typeID;
            }

        private:
            static uint8_t GetNextID()
            {
                static uint8_t counter = 0;
                // Ensure we don't exceed 64 (for bitmask).
                static_assert(sizeof(counter) * 8 >= 6, " We are using uint8_t, which can hold up to 255, but we want to limit to 64 for bitmask.");
                return counter++;
            }
        };
    }
}