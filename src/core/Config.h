#pragma once

class Config
{
public:
    bool Load();
    bool Save();

    int GetWidth() const;
    int GetHeight() const;

private:
    int m_Width = 1280;
    int m_Height = 720;
};