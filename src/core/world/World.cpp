#include "World.h"
#include "../Logger.h"
#include <algorithm>
#include <fstream>
#include <cstdint>
#include <filesystem>

namespace StrixVerse
{
    namespace World
    {

        World::World()
        {
            LOG_INFO("World: Created new world instance");
        }

        World::~World()
        {
            LOG_INFO("World: Destroyed world instance");
        }

        std::shared_ptr<Chunk> World::GetChunkAt(int worldX, int worldY, int worldZ)
        {
            int chunkX, chunkY, chunkZ;
            int localX, localY, localZ;

            WorldToChunkCoords(worldX, worldY, worldZ, chunkX, chunkY, chunkZ, localX, localY, localZ);
            return GetChunkAtChunkCoords(chunkX, chunkY, chunkZ);
        }

        std::shared_ptr<Chunk> World::GetChunkAtChunkCoords(int chunkX, int chunkY, int chunkZ)
        {
            // Check if coordinates are within bounds
            if (chunkX < 0 || chunkX >= m_WidthInChunks ||
                chunkY < 0 || chunkY >= m_HeightInChunks ||
                chunkZ < 0 || chunkZ >= m_DepthInChunks)
            {
                return nullptr;
            }

            return m_Chunks[chunkX][chunkY][chunkZ];
        }

        std::shared_ptr<Tile> World::GetTileAt(int worldX, int worldY, int worldZ)
        {
            int chunkX, chunkY, chunkZ;
            int localX, localY, localZ;

            WorldToChunkCoords(worldX, worldY, worldZ, chunkX, chunkY, chunkZ, localX, localY, localZ);

            auto chunk = GetChunkAtChunkCoords(chunkX, chunkY, chunkZ);
            if (!chunk)
            {
                return nullptr;
            }

            return chunk->GetTile(localX, localY, localZ);
        }

        void World::SetTileAt(int worldX, int worldY, int worldZ, std::shared_ptr<Tile> tile)
        {
            int chunkX, chunkY, chunkZ;
            int localX, localY, localZ;

            WorldToChunkCoords(worldX, worldY, worldZ, chunkX, chunkY, chunkZ, localX, localY, localZ);

            auto chunk = GetChunkAtChunkCoords(chunkX, chunkY, chunkZ);
            if (chunk)
            {
                chunk->SetTile(localX, localY, localZ, tile);
            }
        }

        void World::GenerateNewWorld(int widthInChunks, int heightInChunks, int depthInChunks)
        {
            // Clean up any existing chunks
            m_Chunks.clear();

            m_WidthInChunks = widthInChunks;
            m_HeightInChunks = heightInChunks;
            m_DepthInChunks = depthInChunks;

            // Initialize the 3D chunk array
            m_Chunks.resize(widthInChunks);
            for (int x = 0; x < widthInChunks; ++x)
            {
                m_Chunks[x].resize(heightInChunks);
                for (int y = 0; y < heightInChunks; ++y)
                {
                    m_Chunks[x][y].resize(depthInChunks);
                    for (int z = 0; z < depthInChunks; ++z)
                    {
                        m_Chunks[x][y][z] = std::make_shared<Chunk>();
                    }
                }
            }

            // Generate terrain for each chunk
            for (int x = 0; x < widthInChunks; ++x)
            {
                for (int y = 0; y < heightInChunks; ++y)
                {
                    for (int z = 0; z < depthInChunks; ++z)
                    {
                        m_Chunks[x][y][z]->GenerateRandom();
                    }
                }
            }

            LOG_INFO("World: Generated new world " + std::to_string(widthInChunks) + "x" + std::to_string(heightInChunks) + "x" + std::to_string(depthInChunks) + " chunks");
        }

        void World::GenerateFlatWorld(int widthInChunks, int heightInChunks, int depthInChunks, Tile::Type surfaceType)
        {
            // Clean up any existing chunks
            m_Chunks.clear();

            m_WidthInChunks = widthInChunks;
            m_HeightInChunks = heightInChunks;
            m_DepthInChunks = depthInChunks;

            // Initialize the 3D chunk array
            m_Chunks.resize(widthInChunks);
            for (int x = 0; x < widthInChunks; ++x)
            {
                m_Chunks[x].resize(heightInChunks);
                for (int y = 0; y < heightInChunks; ++y)
                {
                    m_Chunks[x][y].resize(depthInChunks);
                    for (int z = 0; z < depthInChunks; ++z)
                    {
                        m_Chunks[x][y][z] = std::make_shared<Chunk>();

                        // Set all tiles in this chunk to the surface type
                        for (int cx = 0; cx < Chunk::GetWidth(); ++cx)
                        {
                            for (int cy = 0; cy < Chunk::GetHeight(); ++cy)
                            {
                                for (int cz = 0; cz < Chunk::GetDepth(); ++cz)
                                {
                                    auto tile = std::make_shared<Tile>(surfaceType);
                                    m_Chunks[x][y][z]->SetTile(cx, cy, cz, tile);
                                }
                            }
                        }
                    }
                }
            }

            LOG_INFO("World: Generated flat world " + std::to_string(widthInChunks) + "x" + std::to_string(heightInChunks) + "x" + std::to_string(depthInChunks) + " chunks with " + std::to_string(static_cast<int>(surfaceType)) + " surface type");
        }

        bool World::SaveWorld(const std::string &worldName)
        {
            // Create saves directory if it doesn't exist
            std::filesystem::path saveDir = std::filesystem::current_path() / "saves";
            if (!std::filesystem::exists(saveDir))
            {
                if (!std::filesystem::create_directories(saveDir))
                {
                    LOG_ERROR("World: Failed to create saves directory");
                    return false;
                }
            }

            std::filesystem::path filePath = saveDir / (worldName + ".dat");
            std::ofstream outFile(filePath, std::ios::binary);
            if (!outFile.is_open())
            {
                LOG_ERROR("World: Failed to open file for writing: " + filePath.string());
                return false;
            }

            // Write header: version and dimensions
            uint32_t magic = 0x574F524C; // "WORL" in ASCII
            uint32_t version = 1;
            uint32_t width = m_WidthInChunks;
            uint32_t height = m_HeightInChunks;
            uint32_t depth = m_DepthInChunks;

            outFile.write(reinterpret_cast<const char *>(&magic), sizeof(magic));
            outFile.write(reinterpret_cast<const char *>(&version), sizeof(version));
            outFile.write(reinterpret_cast<const char *>(&width), sizeof(width));
            outFile.write(reinterpret_cast<const char *>(&height), sizeof(height));
            outFile.write(reinterpret_cast<const char *>(&depth), sizeof(depth));

            if (!outFile.good())
            {
                LOG_ERROR("World: Failed to write header to file: " + filePath.string());
                outFile.close();
                return false;
            }

            // Write chunk data
            for (uint32_t x = 0; x < width; ++x)
            {
                for (uint32_t y = 0; y < height; ++y)
                {
                    for (uint32_t z = 0; z < depth; ++z)
                    {
                        auto chunk = m_Chunks[x][y][z];
                        if (!chunk)
                        {
                            LOG_ERROR("World: Null chunk at (" + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z) + ")");
                            outFile.close();
                            return false;
                        }

                        // Write each tile's type in the chunk
                        for (uint32_t cx = 0; cx < Chunk::GetWidth(); ++cx)
                        {
                            for (uint32_t cy = 0; cy < Chunk::GetHeight(); ++cy)
                            {
                                for (uint32_t cz = 0; cz < Chunk::GetDepth(); ++cz)
                                {
                                    auto tile = chunk->GetTile(cx, cy, cz);
                                    if (!tile)
                                    {
                                        LOG_ERROR("World: Null tile in chunk at (" + std::to_string(cx) + "," + std::to_string(cy) + "," + std::to_string(cz) + ")");
                                        outFile.close();
                                        return false;
                                    }

                                    uint8_t type = static_cast<uint8_t>(tile->GetType());
                                    outFile.write(reinterpret_cast<const char *>(&type), sizeof(type));

                                    if (!outFile.good())
                                    {
                                        LOG_ERROR("World: Failed to write tile data to file: {}", filePath.string());
                                        outFile.close();
                                        return false;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            outFile.close();
            if (!outFile.good())
            {
                LOG_ERROR("World: Failed to flush file data: {}", filePath.string());
                return false;
            }

            LOG_INFO("World: Saved world '" + worldName + "' to " + filePath.string() + " (" + std::to_string(width) + "x" + std::to_string(height) + "x" + std::to_string(depth) + " chunks)");
            return true;
        }

        bool World::LoadWorld(const std::string &worldName)
        {
            std::filesystem::path filePath = std::filesystem::current_path() / "saves" / (worldName + ".dat");
            if (!std::filesystem::exists(filePath))
            {
                LOG_INFO("World: Save file not found: " + filePath.string());
                return false;
            }

            std::ifstream inFile(filePath, std::ios::binary);
            if (!inFile.is_open())
            {
                LOG_ERROR("World: Failed to open file for reading: {}", filePath.string());
                return false;
            }

            // Read header
            uint32_t magic, version, width, height, depth;
            inFile.read(reinterpret_cast<char *>(&magic), sizeof(magic));
            inFile.read(reinterpret_cast<char *>(&version), sizeof(version));
            inFile.read(reinterpret_cast<char *>(&width), sizeof(width));
            inFile.read(reinterpret_cast<char *>(&height), sizeof(height));
            inFile.read(reinterpret_cast<char *>(&depth), sizeof(depth));

            if (!inFile.good())
            {
                LOG_ERROR("World: Failed to read header from file: {}", filePath.string());
                inFile.close();
                return false;
            }

            // Validate magic and version
            if (magic != 0x574F524C)
            { // "WORL"
                LOG_ERROR("World: Invalid magic number in file: {}", filePath.string());
                inFile.close();
                return false;
            }

            if (version != 1)
            {
                LOG_ERROR("World: Unsupported version {} in file: {}", version, filePath.string());
                inFile.close();
                return false;
            }

            // Clear existing world data
            m_Chunks.clear();
            m_WidthInChunks = width;
            m_HeightInChunks = height;
            m_DepthInChunks = depth;

            // Resize the chunk array
            m_Chunks.resize(width);
            for (uint32_t x = 0; x < width; ++x)
            {
                m_Chunks[x].resize(height);
                for (uint32_t y = 0; y < height; ++y)
                {
                    m_Chunks[x][y].resize(depth);
                }
            }

            // Read chunk data
            for (uint32_t x = 0; x < width; ++x)
            {
                for (uint32_t y = 0; y < height; ++y)
                {
                    for (uint32_t z = 0; z < depth; ++z)
                    {
                        // Create chunk if it doesn't exist
                        m_Chunks[x][y][z] = std::make_shared<Chunk>();
                        auto chunk = m_Chunks[x][y][z];

                        // Read each tile's type in the chunk
                        for (uint32_t cx = 0; cx < Chunk::GetWidth(); ++cx)
                        {
                            for (uint32_t cy = 0; cy < Chunk::GetHeight(); ++cy)
                            {
                                for (uint32_t cz = 0; cz < Chunk::GetDepth(); ++cz)
                                {
                                    uint8_t type;
                                    inFile.read(reinterpret_cast<char *>(&type), sizeof(type));

                                    if (!inFile.good())
                                    {
                                        LOG_ERROR("World: Failed to read tile data from file: " + filePath.string());
                                        inFile.close();
                                        return false;
                                    }

                                    auto tile = std::make_shared<Tile>(static_cast<Tile::Type>(type));
                                    chunk->SetTile(cx, cy, cz, tile);
                                }
                            }
                        }
                    }
                }
            }

            inFile.close();
            if (!inFile.fail() && !inFile.eof())
            {
                // Check if there's extra data (should be exactly at EOF)
                char extra;
                if (inFile.get(extra))
                {
                    LOG_WARN("World: Extra data found at end of file: " + filePath.string());
                }
            }

            LOG_INFO("World: Loaded world '" + worldName + "' from " + filePath.string() + " (" + std::to_string(width) + "x" + std::to_string(height) + "x" + std::to_string(depth) + " chunks)");
            return true;
        }

        void World::WorldToChunkCoords(int worldX, int worldY, int worldZ,
                                       int &chunkX, int &chunkY, int &chunkZ,
                                       int &localX, int &localY, int &localZ) const
        {
            int chunkWidth = Chunk::GetWidth();
            int chunkHeight = Chunk::GetHeight();
            int chunkDepth = Chunk::GetDepth();

            // Handle negative coordinates correctly
            chunkX = (worldX >= 0) ? (worldX / chunkWidth) : ((worldX + 1) / chunkWidth - 1);
            chunkY = (worldY >= 0) ? (worldY / chunkHeight) : ((worldY + 1) / chunkHeight - 1);
            chunkZ = (worldZ >= 0) ? (worldZ / chunkDepth) : ((worldZ + 1) / chunkDepth - 1);

            // Convert to positive remainder
            localX = (worldX % chunkWidth + chunkWidth) % chunkWidth;
            localY = (worldY % chunkHeight + chunkHeight) % chunkHeight;
            localZ = (worldZ % chunkDepth + chunkDepth) % chunkDepth;
        }

    } // namespace World
} // namespace StrixVerse