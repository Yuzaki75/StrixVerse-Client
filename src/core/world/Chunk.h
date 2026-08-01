#pragma once

#include <vector>
#include <array>
#include <memory>
#include "Tile.h"

namespace StrixVerse {
    namespace World {

        constexpr int CHUNK_SIZE = 16; // 16x16 tiles per chunk
        constexpr int CHUNK_HEIGHT = 4; // 4 layers high

        /**
         * Chunk represents a section of the world map.
         * Contains a 3D array of tiles (width x height x depth).
         */
        class Chunk {
        public:
            Chunk();
            ~Chunk();

            // Get/set tile at position (x, y, z)
            // Returns nullptr if coordinates are out of bounds
            std::shared_ptr<Tile> GetTile(int x, int y, int z);
            void SetTile(int x, int y, int z, std::shared_ptr<Tile> tile);

            // Get chunk dimensions
            static int GetWidth() { return CHUNK_SIZE; }
            static int GetHeight() { return CHUNK_SIZE; }
            static int GetDepth() { return CHUNK_HEIGHT; }

            // Check if coordinates are within chunk bounds
            static bool IsValidPosition(int x, int y, int z);

            // Generate a random chunk (for testing/procedural generation)
            void GenerateRandom();

        private:
            // 3D array: [x][y][z] - width x height x depth
            std::array<std::array<std::array<std::shared_ptr<Tile>, CHUNK_HEIGHT>, CHUNK_SIZE>, CHUNK_SIZE> m_Tiles;
        };

    } // namespace World
} // namespace StrixVerse