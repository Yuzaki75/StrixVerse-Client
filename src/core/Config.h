#pragma once

#include <string>

// -----------------------------------------------------------------------------
// Config
//
// Purpose:
//   Owns the client configuration. Loads settings from a JSON file on disk,
//   writes them back on save, generates a default file when none exists, and
//   validates every value before it is handed to other systems.
//
// Responsibilities:
//   - Load()      : read configs/client.json (creates defaults if missing)
//   - Save()      : serialize current values back to disk
//   - Validate()  : clamp values into safe ranges
//   - Accessors   : typed, validated runtime access for other systems
//
// Dependencies: standard library only. No other Core class depends on how
// the config is stored, which keeps coupling minimal.
// -----------------------------------------------------------------------------
class Config
{
public:
    bool Load();
    bool Save() const;

    // Restores every setting to its built-in default value.
    void ResetToDefaults();

    // Clamps all values into valid ranges. Returns false if any value had
    // to be corrected (the corrected values are kept).
    bool Validate();

    // --- Network --------------------------------------------------------
    // Address of the StrixVerse server. Defaults match Server/config/server.json.
    const std::string& GetServerHost() const;
    int GetServerPort() const;

    void SetServerHost(const std::string& host);
    void SetServerPort(int port);

    // When true the client never contacts the server and authentication is
    // resolved locally. Off by default: an unreachable server is reported as a
    // failure rather than silently faked.
    bool IsOfflineMode() const;
    void SetOfflineMode(bool offline);

    // --- Window ---------------------------------------------------------
    int GetWidth() const;
    int GetHeight() const;
    bool IsFullscreen() const;
    bool IsVSyncEnabled() const;

    void SetWidth(int width);
    void SetHeight(int height);
    void SetFullscreen(bool fullscreen);
    void SetVSync(bool vsync);

    // --- Audio ----------------------------------------------------------
    // Music level, 0 (silent) to 100. Read as "musicVolume" in the config.
    int GetMusicVolume() const;
    void SetMusicVolume(int volume);

    // Sound effect level, 0.0 (silent) to 1.0. Read as "sfxVolume" in the config.
    float GetSfxVolume() const;
    void SetSfxVolume(float volume);

    // Path of the backing file (relative to the working directory).
    const std::string& GetFilePath() const;

private:
    std::string m_FilePath = "configs/client.json";

    std::string m_ServerHost = "127.0.0.1";
    int m_ServerPort = 17091;
    bool m_Offline = false;

    int m_MusicVolume = 70;
    float m_SfxVolume = 1.0f;

    int m_Width = 1280;
    int m_Height = 720;
    bool m_Fullscreen = false;
    bool m_VSync = true;
};
