#include "WorldManager.h"
#include "../core/Logger.h"
#include "../core/ServiceLocator.h"

#include <fstream>
#include <sstream>

WorldManager::WorldManager()
{
    // Set the save file path to a "saves" directory in the working directory
    m_SaveFilePath = std::filesystem::current_path() / "saves" / "world_save.txt";

    // Ensure the saves directory exists
    EnsureSaveDirectoryExists();
}

bool WorldManager::EnsureSaveDirectoryExists() const
{
    std::filesystem::path dirPath = m_SaveFilePath.parent_path();
    if (!std::filesystem::exists(dirPath))
    {
        try
        {
            std::filesystem::create_directories(dirPath);
            LOG_INFO("WorldManager: Created save directory at {}", dirPath.string());
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            LOG_ERROR("WorldManager: Failed to create save directory: {}", e.what());
            return false;
        }
    }
    return true;
}

bool WorldManager::SaveWorld(const std::string &worldName)
{
    try
    {
        // Ensure save directory exists
        if (!EnsureSaveDirectoryExists())
        {
            return false;
        }

        // Save the world name to file (in a real implementation, we'd serialize the entire world)
        std::ofstream outFile(m_SaveFilePath);
        if (!outFile.is_open())
        {
            LOG_ERROR("WorldManager: Failed to open save file for writing: {}", m_SaveFilePath.string());
            return false;
        }

        outFile << "worldName=" << worldName << std::endl;
        outFile.close();

        LOG_INFO("WorldManager: Saved world '{}' to {}", worldName, m_SaveFilePath.string());

        // In a full implementation, we would serialize the entire world data here
        // For now, we're just saving the world name as a placeholder
        return true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("WorldManager: Exception while saving world: {}", e.what());
        return false;
    }
}

bool WorldManager::LoadWorld(std::string &outWorldName, StrixVerse::World::World &world)
{
    try
    {
        // Check if save file exists
        if (!std::filesystem::exists(m_SaveFilePath))
        {
            LOG_INFO("WorldManager: No save file found at {}", m_SaveFilePath.string());
            return false;
        }

        // Read from file
        std::ifstream inFile(m_SaveFilePath);
        if (!inFile.is_open())
        {
            LOG_ERROR("WorldManager: Failed to open save file for reading: {}", m_SaveFilePath.string());
            return false;
        }

        // Read line and parse
        std::string line;
        if (std::getline(inFile, line))
        {
            inFile.close();

            // Parse format: worldName=value
            std::size_t pos = line.find('=');
            if (pos != std::string::npos && pos < line.length() - 1)
            {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t\n\r"));
                key.erase(key.find_last_not_of(" \t\n\r") + 1);
                value.erase(0, value.find_first_not_of(" \t\n\r"));
                value.erase(value.find_last_not_of(" \t\n\r") + 1);

                if (key == "worldName")
                {
                    outWorldName = value;
                    LOG_INFO("WorldManager: Loaded world '{}' from {}", outWorldName, m_SaveFilePath.string());

                    // In a full implementation, we would deserialize the world data here
                    // For now, we just return the world name
                    return true;
                }
            }
        }

        inFile.close();
        LOG_ERROR("WorldManager: Invalid save file format at {}", m_SaveFilePath.string());
        return false;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("WorldManager: Exception while loading world: {}", e.what());
        return false;
    }
}

bool WorldManager::HasSavedWorld() const
{
    return std::filesystem::exists(m_SaveFilePath);
}

bool WorldManager::DeleteSavedWorld()
{
    try
    {
        if (std::filesystem::exists(m_SaveFilePath))
        {
            std::filesystem::remove(m_SaveFilePath);
            LOG_INFO("WorldManager: Deleted save file at {}", m_SaveFilePath.string());
            return true;
        }
        else
        {
            LOG_INFO("WorldManager: No save file to delete at {}", m_SaveFilePath.string());
            return true; // Nothing to delete is still a success
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("WorldManager: Exception while deleting save file: {}", e.what());
        return false;
    }
}