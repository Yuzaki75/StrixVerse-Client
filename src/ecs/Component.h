#pragma once

#include <cstdint>
#include <typeindex>
#include <unordered_map>

namespace StrixVerse
{
    namespace ECS
    {
        // Maximum number of component types we support (for bitset).
        static constexpr size_t MAX_COMPONENTS = 64;
        // Base class for all components.
        // Components are data-only structs that inherit from this class.
        // Each component type gets a unique ID via the ComponentType system.
        class Component
        {
        public:
            virtual ~Component() = default;

            Component(const Component &) = default;
            Component &operator=(const Component &) = default;
            Component(Component &&) = default;
            Component &operator=(Component &&) = default;

        protected:
            Component() = default;
        };

        // Type ID system for components.
        // Uses a simple counter to assign a unique ID to each component type.
        // Component IDs are stored as uint8_t with a maximum of 64 types (for 64-bit bitmask).
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
                return counter++;
            }
        };
    }
}