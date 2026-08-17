#pragma once

#include <string>
#include <vector>

namespace StrixVerse {
    namespace World {

        /**
         * Tile represents a single tile in the game world.
         *
         * Solidity is the property the collision system reads, and it mirrors
         * the server's `World::Tile::IsSolid()` so the two agree about where a
         * player can be. The rule is deliberately the simple one: a tile that
         * exists is solid, and air is the *absence* of a tile - a null entry in
         * the chunk, which the renderer skips and the collision system walks
         * straight through.
         *
         * This replaces an earlier `IsWalkable()` that came from a top-down
         * design, where it meant "you may walk over this" and so reported true
         * for dirt, stone, grass and sand. Read as a collision answer that says
         * the whole world is passable, which is why the player fell through the
         * ground the moment gravity was applied.
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

            bool IsSolid() const { return m_Solid; }
            void SetSolid(bool solid) { m_Solid = solid; }

            const std::string& GetTextureName() const { return m_TextureName; }
            void SetTextureName(const std::string& textureName) { m_TextureName = textureName; }

        private:
            Type m_Type;
            bool m_Solid;
            std::string m_TextureName;
        };

    } // namespace World
} // namespace StrixVerse