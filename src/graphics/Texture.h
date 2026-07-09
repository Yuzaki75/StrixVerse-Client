#pragma once

#include <string>

class Texture
{
public:
    Texture();

    ~Texture();

    bool Load(
        const std::string& path);

    void Destroy();

    void Bind(
        unsigned int slot = 0) const;

    unsigned int GetWidth() const;

    unsigned int GetHeight() const;

    bool IsLoaded() const;

private:
    unsigned int m_ID = 0;

    unsigned int m_Width = 0;
    unsigned int m_Height = 0;

    bool m_Loaded = false;
};