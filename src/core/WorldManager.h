#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "world/World.h"

class WorldManager
{
public:
    WorldManager();
    ~WorldManager() = default;

    // Save the current world data
    bool SaveWorld(const std::string& worldName);

    // Load the last saved world
    bool LoadWorld(std::string& outWorldName, StrixVerse::World::World& world);

    // Check if a saved world exists
    bool HasSavedWorld() const;

    // Delete the saved world
    bool DeleteSavedWorld();

private:
    // World instance that holds the actual world data
    StrixVerse::World::World m_World;

    // Path to the save file
    std::filesystem::path m_SaveFilePath;

    // Create the save directory if it doesn't exist
    bool EnsureSaveDirectoryExists() const;
};