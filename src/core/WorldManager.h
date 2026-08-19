#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "world/World.h"

/**
 * Description of a joinable world, as shown by the World Selection screen.
 *
 * Every field beyond the name is optional: the client only knows a world's
 * type, owner or population once a server tells it. Empty strings and a
 * maxPlayers of zero mean "not known", and the screens render those as a dash
 * rather than inventing a value.
 */
struct WorldInfo
{
    std::string name;
    std::string owner;
    std::string type;          // Survival, Adventure, Creative, Trading, Event
    std::string description;

    int  players    = 0;
    int  maxPlayers = 0;
    bool favourite  = false;
    bool recent     = false;

    bool HasPopulation() const { return maxPlayers > 0; }
    bool IsFull() const { return maxPlayers > 0 && players >= maxPlayers; }
};

/**
 * The player's most recent session, as shown by the Continue screen.
 *
 * Only the world name and the timestamp are ever recovered from disk. The
 * remaining fields stay unset until the server describes the world, so nothing
 * on the Continue card is fabricated.
 */
struct LastWorldSession
{
    WorldInfo world;

    // Seconds since the Unix epoch. Zero when the save file carried no
    // timestamp, which is what makes "Last Played" read as unknown.
    std::int64_t lastPlayedUnix = 0;

    // The player's position in the world is only known once the world reports
    // it, which needs the chunk/player packets the client does not send yet.
    bool hasPosition = false;
    int  positionX   = 0;
    int  positionY   = 0;
};

/**
 * Renders a Unix timestamp as "just now", "12 minutes ago", "2 hours ago" or
 * "3 days ago". Returns "Unknown" for a zero or future timestamp.
 */
std::string FormatRelativeTime(std::int64_t unixSeconds);

/**
 * WorldManager
 *
 * Owns world persistence and the list of worlds the client can join.
 *
 * The list starts empty and stays empty until something fills it. There is no
 * built-in catalogue: the client never claims a world exists that it has not
 * been told about. SetAvailableWorlds is the seam the server's world-list
 * packet plugs into.
 */
class WorldManager
{
public:
    WorldManager();
    ~WorldManager() = default;

    // --- Persistence ------------------------------------------------------
    // The saved session belongs to one account. The username is stored with it
    // and checked on read, so a new or different account never inherits
    // somebody else's last world - it goes to World Selection instead.
    // Records which world this account was last in, and when.
    //
    // Named for what it stores. It used to be SaveWorld/LoadWorld against a
    // file called world_save.txt, with LoadWorld taking a World& it discarded
    // with a (void) cast - so the API read as a half-finished world serialiser
    // when it is a complete and correct session record. A world's contents
    // belong to the server, so there is deliberately nothing else to write.
    //
    // The file keeps its old name so an existing "last world" is not orphaned.
    bool SaveLastSession(const std::string& worldName, const std::string& username);
    bool LoadLastSession(std::string& outWorldName);

    // True only when a session is saved for this specific account.
    bool HasSavedWorldFor(const std::string& username) const;

    bool DeleteSavedWorld();

    // --- World catalogue --------------------------------------------------
    // Worlds available to join. Empty until a server supplies a list.
    const std::vector<WorldInfo>& GetAvailableWorlds() const { return m_Worlds; }

    // Bumped whenever the catalogue changes. The world browser polls this so
    // the list appears when the server's reply lands, rather than only when
    // the screen is opened or Refresh is pressed.
    uint32_t GetAvailableWorldsRevision() const { return m_WorldsRevision; }

    // Replaces the catalogue wholesale. Called when a world list arrives.
    void SetAvailableWorlds(std::vector<WorldInfo> worlds);

    // Drops the catalogue, e.g. on disconnect, so a stale list is never shown
    // as if it were current.
    void ClearAvailableWorlds();

    // Looks a world up by name; returns nullptr when it is not in the list.
    const WorldInfo* FindWorld(const std::string& name) const;

    // Details of the last world this account was in. Returns false when there
    // is no saved session for `username`, which is what sends the player to
    // World Selection.
    bool GetLastWorld(const std::string& username, LastWorldSession& outSession) const;

    // Records the world the player has just joined, so Continue can offer it
    // next time.
    void SetLastWorld(const std::string& worldName, const std::string& username);

    // Forgets the saved session. Called when the player deliberately leaves a
    // world, so the next login starts at World Selection rather than offering
    // to continue into a world they chose to leave.
    void ClearLastWorld();

private:
    bool EnsureSaveDirectoryExists() const;

    StrixVerse::World::World m_World;
    std::filesystem::path    m_SaveFilePath;

    std::vector<WorldInfo> m_Worlds;
    uint32_t               m_WorldsRevision = 0;
};
