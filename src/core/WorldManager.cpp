#include "WorldManager.h"

#include "../core/Logger.h"

#include <algorithm>
#include <chrono>
#include <format>
#include <fstream>

namespace
{
    // Reads "key=value" from one save-file line. Returns false for anything
    // that is not in that shape.
    bool ParseKeyValue(const std::string& line, std::string& outKey, std::string& outValue)
    {
        const size_t separator = line.find('=');
        if (separator == std::string::npos)
            return false;

        const auto trim = [](std::string text)
        {
            const size_t first = text.find_first_not_of(" \t\r\n");
            if (first == std::string::npos)
                return std::string();

            const size_t last = text.find_last_not_of(" \t\r\n");
            return text.substr(first, last - first + 1);
        };

        outKey   = trim(line.substr(0, separator));
        outValue = trim(line.substr(separator + 1));

        return !outKey.empty();
    }

    std::int64_t UnixNow()
    {
        return std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }
}

std::string FormatRelativeTime(std::int64_t unixSeconds)
{
    if (unixSeconds <= 0)
        return "Unknown";

    const std::int64_t elapsed = UnixNow() - unixSeconds;

    // A timestamp from the future means a clock change, not a real duration.
    if (elapsed < 0)
        return "Unknown";

    if (elapsed < 60)
        return "just now";

    const auto plural = [](std::int64_t count, const char* unit)
    {
        return std::format("{} {}{} ago", count, unit, count == 1 ? "" : "s");
    };

    if (elapsed < 60 * 60)
        return plural(elapsed / 60, "minute");

    if (elapsed < 24 * 60 * 60)
        return plural(elapsed / (60 * 60), "hour");

    return plural(elapsed / (24 * 60 * 60), "day");
}

WorldManager::WorldManager()
{
    // Save alongside the executable so a debug and a release run do not fight
    // over the same file.
    m_SaveFilePath = std::filesystem::current_path() / "saves" / "world_save.txt";

    EnsureSaveDirectoryExists();

    // The catalogue starts empty on purpose. The client shows the worlds a
    // server tells it about and nothing else.
}

void WorldManager::SetAvailableWorlds(std::vector<WorldInfo> worlds)
{
    m_Worlds = std::move(worlds);

    LOG_INFO(std::format("WorldManager: world list updated ({} worlds)", m_Worlds.size()));
}

void WorldManager::ClearAvailableWorlds()
{
    m_Worlds.clear();
}

const WorldInfo* WorldManager::FindWorld(const std::string& name) const
{
    const auto it = std::find_if(m_Worlds.begin(), m_Worlds.end(),
                                 [&name](const WorldInfo& world) { return world.name == name; });

    return it != m_Worlds.end() ? &(*it) : nullptr;
}

bool WorldManager::GetLastWorld(const std::string& username, LastWorldSession& outSession) const
{
    if (username.empty() || !std::filesystem::exists(m_SaveFilePath))
        return false;

    std::ifstream inFile(m_SaveFilePath);
    if (!inFile.is_open())
        return false;

    std::string  worldName;
    std::string  savedUser;
    std::int64_t lastPlayed = 0;

    std::string line;
    while (std::getline(inFile, line))
    {
        std::string key;
        std::string value;

        if (!ParseKeyValue(line, key, value))
            continue;

        if (key == "worldName")
        {
            worldName = value;
        }
        else if (key == "username")
        {
            savedUser = value;
        }
        else if (key == "lastPlayed")
        {
            // A save written before timestamps existed simply has no such line,
            // and a malformed one is treated the same way: unknown.
            try
            {
                lastPlayed = std::stoll(value);
            }
            catch (const std::exception&)
            {
                lastPlayed = 0;
            }
        }
    }

    if (worldName.empty())
        return false;

    // A save with no username predates per-account sessions, and one written
    // by another account is not ours to resume. Either way this account has no
    // world to continue into, so it starts at World Selection.
    if (savedUser != username)
    {
        LOG_INFO(std::format("WorldManager: saved session belongs to '{}', not '{}'; ignoring it",
                             savedUser.empty() ? std::string("(unknown)") : savedUser, username));
        return false;
    }

    outSession = LastWorldSession{};
    outSession.lastPlayedUnix = lastPlayed;

    // The saved name is all that is known for certain. If a server has since
    // described this world, take its details; otherwise the remaining fields
    // stay empty and the Continue screen shows them as unknown.
    if (const WorldInfo* world = FindWorld(worldName))
        outSession.world = *world;
    else
        outSession.world.name = worldName;

    return true;
}

void WorldManager::SetLastWorld(const std::string& worldName, const std::string& username)
{
    SaveWorld(worldName, username);
}

void WorldManager::ClearLastWorld()
{
    DeleteSavedWorld();
}

bool WorldManager::EnsureSaveDirectoryExists() const
{
    const std::filesystem::path directory = m_SaveFilePath.parent_path();

    if (std::filesystem::exists(directory))
        return true;

    std::error_code error;
    std::filesystem::create_directories(directory, error);

    if (error)
    {
        LOG_ERROR(std::format("WorldManager: failed to create save directory '{}': {}",
                              directory.string(), error.message()));
        return false;
    }

    LOG_INFO(std::format("WorldManager: created save directory at {}", directory.string()));
    return true;
}

bool WorldManager::SaveWorld(const std::string& worldName, const std::string& username)
{
    if (!EnsureSaveDirectoryExists())
        return false;

    std::ofstream outFile(m_SaveFilePath, std::ios::trunc);
    if (!outFile.is_open())
    {
        LOG_ERROR(std::format("WorldManager: failed to open save file for writing: {}",
                              m_SaveFilePath.string()));
        return false;
    }

    // Only the session identity is stored. A world's contents belong to the
    // server, so there is nothing else here to serialise; the timestamp is what
    // lets "Last Played" report a real elapsed time, and the username is what
    // keeps one account's session from being offered to another.
    outFile << "worldName=" << worldName << "\n";
    outFile << "username=" << username << "\n";
    outFile << "lastPlayed=" << UnixNow() << "\n";

    if (!outFile)
    {
        LOG_ERROR(std::format("WorldManager: failed while writing save file: {}",
                              m_SaveFilePath.string()));
        return false;
    }

    LOG_INFO(std::format("WorldManager: saved world '{}' to {}",
                         worldName, m_SaveFilePath.string()));
    return true;
}

bool WorldManager::LoadWorld(std::string& outWorldName, StrixVerse::World::World& world)
{
    (void)world;   // World deserialisation is not implemented yet.

    if (!std::filesystem::exists(m_SaveFilePath))
    {
        LOG_INFO(std::format("WorldManager: no save file found at {}", m_SaveFilePath.string()));
        return false;
    }

    std::ifstream inFile(m_SaveFilePath);
    if (!inFile.is_open())
    {
        LOG_ERROR(std::format("WorldManager: failed to open save file for reading: {}",
                              m_SaveFilePath.string()));
        return false;
    }

    std::string line;
    if (!std::getline(inFile, line))
    {
        LOG_ERROR(std::format("WorldManager: save file is empty: {}", m_SaveFilePath.string()));
        return false;
    }

    const size_t separator = line.find('=');
    if (separator == std::string::npos || separator + 1 >= line.size())
    {
        LOG_ERROR(std::format("WorldManager: invalid save file format at {}",
                              m_SaveFilePath.string()));
        return false;
    }

    std::string key   = line.substr(0, separator);
    std::string value = line.substr(separator + 1);

    const auto trim = [](std::string& text)
    {
        const size_t first = text.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            text.clear();
            return;
        }
        const size_t last = text.find_last_not_of(" \t\r\n");
        text = text.substr(first, last - first + 1);
    };

    trim(key);
    trim(value);

    if (key != "worldName" || value.empty())
    {
        LOG_ERROR(std::format("WorldManager: invalid save file format at {}",
                              m_SaveFilePath.string()));
        return false;
    }

    outWorldName = value;

    LOG_INFO(std::format("WorldManager: loaded world '{}' from {}",
                         outWorldName, m_SaveFilePath.string()));
    return true;
}

bool WorldManager::HasSavedWorldFor(const std::string& username) const
{
    LastWorldSession session;
    return GetLastWorld(username, session);
}

bool WorldManager::DeleteSavedWorld()
{
    std::error_code error;

    if (!std::filesystem::exists(m_SaveFilePath))
    {
        LOG_INFO(std::format("WorldManager: no save file to delete at {}",
                             m_SaveFilePath.string()));
        return true;
    }

    std::filesystem::remove(m_SaveFilePath, error);

    if (error)
    {
        LOG_ERROR(std::format("WorldManager: failed to delete save file: {}", error.message()));
        return false;
    }

    LOG_INFO(std::format("WorldManager: deleted save file at {}", m_SaveFilePath.string()));
    return true;
}
