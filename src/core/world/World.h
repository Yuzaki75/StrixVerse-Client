#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include "Chunk.h"

namespace StrixVerse {
    namespace World {

        /**
         * World represents the entire game world.
         * Manages chunks and provides methods for world operations.
         */
        class World {
        public:
            World();
            ~World();

            // Get chunk at world coordinates (converts to chunk coordinates)
            std::shared_ptr<Chunk> GetChunkAt(int worldX, int worldY, int worldZ);
            std::shared_ptr<Chunk> GetChunkAtChunkCoords(int chunkX, int chunkY, int chunkZ);

            // Get tile at world coordinates
            std::shared_ptr<Tile> GetTileAt(int worldX, int worldY, int worldZ);
            void SetTileAt(int worldX, int worldY, int worldZ, std::shared_ptr<Tile> tile);

            // World generation
            void GenerateNewWorld(int widthInChunks, int heightInChunks, int depthInChunks);
            void GenerateFlatWorld(int widthInChunks, int heightInChunks, int depthInChunks, Tile::Type surfaceType);

            // World persistence (to be implemented with WorldManager)
            bool SaveWorld(const std::string& worldName);
            bool LoadWorld(const std::string& worldName);

            // Get world dimensions in chunks
            int GetWidthInChunks() const { return m_WidthInChunks; }
            int GetHeightInChunks() const { return m_HeightInChunks; }
            int GetDepthInChunks() const { return m_DepthInChunks; }

            // Get world dimensions in tiles
            int GetWidthInTiles() const { return m_WidthInChunks * Chunk::GetWidth(); }
            int GetHeightInTiles() const { return m_HeightInChunks * Chunk::GetHeight(); }
            int GetDepthInTiles() const { return m_DepthInChunks * Chunk::GetDepth(); }

        private:
            // Convert world coordinates to chunk coordinates and local coordinates
            void WorldToChunkCoords(int worldX, int worldY, int worldZ,
                                  int& chunkX, int& chunkY, int& chunkZ,
                                  int& localX, int& localY, int& localZ) const;

            // 3D array of chunks: [chunkX][chunkY][chunkZ]
            std::vector<std::vector<std::vector<std::shared_ptr<Chunk>>>> m_Chunks;

            int m_WidthInChunks = 0;
            int m_HeightInChunks = 0;
            int m_DepthInChunks = 0;
        };

    } // namespace World
} // namespace StrixVerse