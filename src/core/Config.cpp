#include "Config.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{
    constexpr int kMinWidth = 320;
    constexpr int kMinHeight = 240;
    constexpr int kMaxWidth = 7680;
    constexpr int kMaxHeight = 4320;

    // Minimal, dependency-free extraction of a value for "key" from a flat
    // JSON document. Sufficient for the client config schema; swap in a full
    // JSON library later without touching any caller of Config.
    bool FindValueToken(
        const std::string& json,
        const std::string& key,
        std::string& outToken)
    {
        const std::string quoted = "\"" + key + "\"";

        std::size_t pos = json.find(quoted);
        if (pos == std::string::npos)
            return false;

        pos = json.find(':', pos + quoted.size());
        if (pos == std::string::npos)
            return false;

        ++pos;
        while (pos < json.size() &&
               std::isspace(static_cast<unsigned char>(json[pos])))
            ++pos;

        std::size_t end = pos;
        while (end < json.size() &&
               json[end] != ',' && json[end] != '}' &&
               json[end] != '\n' && json[end] != '\r')
            ++end;

        outToken = json.substr(pos, end - pos);

        while (!outToken.empty() &&
               std::isspace(static_cast<unsigned char>(outToken.back())))
            outToken.pop_back();

        return !outToken.empty();
    }

    // Reads a quoted string value. The token grabbed by FindValueToken still
    // has its surrounding quotes, so they are stripped here.
    bool ParseString(const std::string& json, const std::string& key, std::string& out)
    {
        std::string token;
        if (!FindValueToken(json, key, token))
            return false;

        if (token.size() >= 2 && token.front() == '"' && token.back() == '"')
            token = token.substr(1, token.size() - 2);

        if (token.empty())
            return false;

        out = token;
        return true;
    }

    bool ParseInt(const std::string& json, const std::string& key, int& out)
    {
        std::string token;
        if (!FindValueToken(json, key, token))
            return false;

        try
        {
            out = std::stoi(token);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool ParseBool(const std::string& json, const std::string& key, bool& out)
    {
        std::string token;
        if (!FindValueToken(json, key, token))
            return false;

        if (token == "true")  { out = true;  return true; }
        if (token == "false") { out = false; return true; }
        return false;
    }
}

bool Config::Load()
{
    std::filesystem::path path(m_FilePath);

    if (path.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec)
            return false;
    }

    if (!std::filesystem::exists(path))
    {
        // First run: generate the default configuration file.
        ResetToDefaults();
        return Save();
    }

    std::ifstream file(path);
    if (!file.is_open())
        return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string json = buffer.str();

    ParseInt(json, "width", m_Width);
    ParseInt(json, "height", m_Height);
    ParseBool(json, "fullscreen", m_Fullscreen);
    ParseBool(json, "vsync", m_VSync);

    ParseInt(json, "port", m_ServerPort);
    ParseString(json, "host", m_ServerHost);
    ParseBool(json, "offline", m_Offline);

    Validate();

    return true;
}

bool Config::Save() const
{
    std::filesystem::path path(m_FilePath);

    if (path.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
        if (ec)
            return false;
    }

    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open())
        return false;

    file << "{\n"
         << "    \"window\":\n"
         << "    {\n"
         << "        \"width\": " << m_Width << ",\n"
         << "        \"height\": " << m_Height << ",\n"
         << "        \"fullscreen\": " << (m_Fullscreen ? "true" : "false") << ",\n"
         << "        \"vsync\": " << (m_VSync ? "true" : "false") << "\n"
         << "    }\n"
         << "}\n";

    return file.good();
}

void Config::ResetToDefaults()
{
    m_Width = 1280;
    m_Height = 720;
    m_Fullscreen = false;
    m_VSync = true;
}

bool Config::Validate()
{
    bool valid = true;

    const int width = std::clamp(m_Width, kMinWidth, kMaxWidth);
    const int height = std::clamp(m_Height, kMinHeight, kMaxHeight);

    if (width != m_Width || height != m_Height)
        valid = false;

    m_Width = width;
    m_Height = height;

    return valid;
}

int Config::GetWidth() const { return m_Width; }
int Config::GetHeight() const { return m_Height; }
bool Config::IsFullscreen() const { return m_Fullscreen; }
bool Config::IsVSyncEnabled() const { return m_VSync; }

void Config::SetWidth(int width) { m_Width = width; Validate(); }
void Config::SetHeight(int height) { m_Height = height; Validate(); }
void Config::SetFullscreen(bool fullscreen) { m_Fullscreen = fullscreen; }
void Config::SetVSync(bool vsync) { m_VSync = vsync; }

const std::string& Config::GetServerHost() const { return m_ServerHost; }
int Config::GetServerPort() const { return m_ServerPort; }

void Config::SetServerHost(const std::string& host)
{
    if (!host.empty())
        m_ServerHost = host;
}

bool Config::IsOfflineMode() const { return m_Offline; }

void Config::SetOfflineMode(bool offline) { m_Offline = offline; }

void Config::SetServerPort(int port)
{
    // Port 0 is "any" and privileged ports are not what a game server uses.
    if (port > 0 && port <= 65535)
        m_ServerPort = port;
}

const std::string& Config::GetFilePath() const { return m_FilePath; }
