#pragma once

#include <string>
#include <vector>

namespace StrixVerse {
    namespace World {

        /**
         * Tile represents a single tile in the game world.
         * Contains properties like terrain type, whether it's walkable, etc.
         */
        class Tile {
        public:
            enum class Type {
                Grass,
                Dirt,
                Stone,
                Water,
                Sand,
                Count
            };

            Tile();
            Tile(Type type);
            ~Tile();

            // Getters and Setters
            Type GetType() const { return m_Type; }
            void SetType(Type type) { m_Type = type; }

            bool IsWalkable() const { return m_Walkable; }
            void SetWalkable(bool walkable) { m_Walkable = walkable; }

            const std::string& GetTextureName() const { return m_TextureName; }
            void SetTextureName(const std::string& textureName) { m_TextureName = textureName; }

        private:
            Type m_Type;
            bool m_Walkable;
            std::string m_TextureName;
        };

    } // namespace World
} // namespace StrixVerse