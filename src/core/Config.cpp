#include "Config.h"

#include <filesystem>
#include <fstream>

bool Config::Load()
{
    std::filesystem::create_directories(
        "configs");

    if (!std::filesystem::exists(
        "configs/client.json"))
    {
        Save();
        return true;
    }

    return true;
}

bool Config::Save()
{
    std::ofstream file(
        "configs/client.json");

    if (!file.is_open())
        return false;

    file <<
R"({
    "window":
    {
        "width": 1280,
        "height": 720
    }
})";

    return true;
}

int Config::GetWidth() const
{
    return m_Width;
}

int Config::GetHeight() const
{
    return m_Height;
}