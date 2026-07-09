#pragma once

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

class Shader;

struct Character
{
    unsigned int TextureID;

    glm::ivec2 Size;
    glm::ivec2 Bearing;

    unsigned int Advance;
};

class Font
{
public:
    Font();
    ~Font();

    bool Load(
        const std::string& path,
        unsigned int size);

    void Destroy();

    void DrawText(
        Shader& shader,
        const std::string& text,
        float x,
        float y,
        float scale);

    glm::vec2 MeasureText(
        const std::string& text,
        float scale) const;

    bool IsLoaded() const;

private:
    std::unordered_map<char, Character> m_Characters;

    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;

    unsigned int m_Size = 0;

    bool m_Loaded = false;
};