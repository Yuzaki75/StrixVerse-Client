#include "Chunk.h"
#include "../Logger.h"
#include <random>

namespace StrixVerse {
    namespace World {

        Chunk::Chunk() {
            // Initialize all tiles to grass by default
            for (int x = 0; x < CHUNK_SIZE; ++x) {
                for (int y = 0; y < CHUNK_SIZE; ++y) {
                    for (int z = 0; z < CHUNK_HEIGHT; ++z) {
                        m_Tiles[x][y][z] = std::make_shared<Tile>(Tile::Type::Grass);
                    }
                }
            }
            LOG_DEBUG("Chunk: Created chunk with {}x{}x{} tiles", CHUNK_SIZE, CHUNK_SIZE, CHUNK_HEIGHT);
        }

        Chunk::~Chunk() {
            LOG_DEBUG("Chunk: Destroyed chunk");
        }

        std::shared_ptr<Tile> Chunk::GetTile(int x, int y, int z) {
            if (!IsValidPosition(x, y, z)) {
                return nullptr;
            }
            return m_Tiles[x][y][z];
        }

        void Chunk::SetTile(int x, int y, int z, std::shared_ptr<Tile> tile) {
            if (IsValidPosition(x, y, z)) {
                m_Tiles[x][y][z] = tile;
            }
        }

        bool Chunk::IsValidPosition(int x, int y, int z) {
            return x >= 0 && x < CHUNK_SIZE &&
                   y >= 0 && y < CHUNK_SIZE &&
                   z >= 0 && z < CHUNK_HEIGHT;
        }

        void Chunk::GenerateRandom() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, static_cast<int>(Tile::Type::Count) - 1);

            for (int x = 0; x < CHUNK_SIZE; ++x) {
                for (int y = 0; y < CHUNK_SIZE; ++y) {
                    for (int z = 0; z < CHUNK_HEIGHT; ++z) {
                        // Generate different terrain based on height (z-level)
                        Tile::Type type;
                        if (z == 0) {
                            // Bottom layer - mostly stone/dirt
                            std::uniform_int_distribution<> groundType(0, 2); // Grass, Dirt, Stone
                            type = static_cast<Tile::Type>(groundType(gen));
                        } else if (z == 1) {
                            // Middle layer - mix of dirt and stone
                            std::uniform_int_distribution<> midType(1, 2); // Dirt, Stone
                            type = static_cast<Tile::Type>(midType(gen));
                        } else {
                            // Top layers - mostly grass/dirt with some stone
                            std::uniform_int_distribution<> topType(0, 2); // Grass, Dirt, Stone
                            type = static_cast<Tile::Type>(topType(gen));
                        }

                        m_Tiles[x][y][z] = std::make_shared<Tile>(type);
                    }
                }
            }

            LOG_DEBUG("Chunk: Generated random chunk terrain");
        }

    } // namespace World
} // namespace StrixVerse