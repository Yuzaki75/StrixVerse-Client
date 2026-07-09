#include "Font.h"

#include "Shader.h"

#include <glad/glad.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>

Font::Font()
{
}

Font::~Font()
{
    Destroy();
}

bool Font::Load(
    const std::string& path,
    unsigned int size)
{
    FT_Library library;

    if (FT_Init_FreeType(&library))
    {
        std::cerr << "Failed to initialize FreeType." << std::endl;
        return false;
    }

    FT_Face face;

    if (FT_New_Face(
            library,
            path.c_str(),
            0,
            &face))
    {
        std::cerr << "Failed to load font: "
                  << path
                  << std::endl;

        FT_Done_FreeType(library);

        return false;
    }

    FT_Set_Pixel_Sizes(
        face,
        0,
        size);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; c++)
    {
        if (FT_Load_Char(
                face,
                c,
                FT_LOAD_RENDER))
        {
            continue;
        }

        unsigned int texture;

        glGenTextures(
            1,
            &texture);

        glBindTexture(
            GL_TEXTURE_2D,
            texture);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR);

        Character character =
        {
            texture,

            glm::ivec2(
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows),

            glm::ivec2(
                face->glyph->bitmap_left,
                face->glyph->bitmap_top),

            static_cast<unsigned int>(
                face->glyph->advance.x)
        };

        m_Characters.emplace(
            c,
            character);
    }

    FT_Done_Face(face);

    FT_Done_FreeType(library);

    glGenVertexArrays(
        1,
        &m_VAO);

    glGenBuffers(
        1,
        &m_VBO);

    m_Size = size;

    m_Loaded = true;

    return true;
}

void Font::Destroy()
{
    for (auto& glyph : m_Characters)
    {
        glDeleteTextures(
            1,
            &glyph.second.TextureID);
    }

    m_Characters.clear();

    if (m_VAO)
    {
        glDeleteVertexArrays(
            1,
            &m_VAO);

        m_VAO = 0;
    }

    if (m_VBO)
    {
        glDeleteBuffers(
            1,
            &m_VBO);

        m_VBO = 0;
    }

    m_Loaded = false;
}

void Font::DrawText(
    Shader& shader,
    const std::string& text,
    float x,
    float y,
    float scale)
{
    if (!m_Loaded)
        return;

    shader.Bind();

    glBindVertexArray(m_VAO);

    for (char c : text)
    {
        auto it = m_Characters.find(c);

        if (it == m_Characters.end())
            continue;

        Character& ch = it->second;

        float xpos =
            x + ch.Bearing.x * scale;

        float ypos =
            y - (ch.Size.y - ch.Bearing.y) * scale;

        float width =
            ch.Size.x * scale;

        float height =
            ch.Size.y * scale;

        (void)xpos;
        (void)ypos;
        (void)width;
        (void)height;

        x += (ch.Advance >> 6) * scale;
    }

    glBindVertexArray(0);
}

glm::vec2 Font::MeasureText(
    const std::string& text,
    float scale) const
{
    float width = 0.0f;
    float height = 0.0f;

    for (char c : text)
    {
        auto it = m_Characters.find(c);

        if (it == m_Characters.end())
            continue;

        width +=
            (it->second.Advance >> 6) * scale;

        if (it->second.Size.y * scale > height)
        {
            height =
                it->second.Size.y * scale;
        }
    }

    return glm::vec2(
        width,
        height);
}

bool Font::IsLoaded() const
{
    return m_Loaded;
}