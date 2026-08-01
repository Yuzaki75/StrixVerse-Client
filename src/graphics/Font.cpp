#include "Font.h"

#include "Shader.h"

#include <glad/glad.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <iostream>
#include <vector>

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

    // Configure VAO/VBO for texture quads
    glGenVertexArrays(
        1,
        &m_VAO);
    glGenBuffers(
        1,
        &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // We'll allocate enough space for 6 vertices (2 triangles) per character, but we'll update per character
    // We allocate a buffer that can hold 6 vertices * 4 floats (x, y, u, v) per vertex
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // We'll iterate over each character and draw a quad for it
    float cursorX = x;
    float cursorY = y;

    for (char c : text)
    {
        auto it = m_Characters.find(c);
        if (it == m_Characters.end())
        {
            // Move cursor forward by the advance of a space? Or just skip?
            // We'll skip and not advance the cursor.
            continue;
        }

        Character& ch = it->second;

        // Calculate the vertex positions and texture coordinates for the glyph's quad
        float xpos = cursorX + ch.Bearing.x * scale;
        float ypos = cursorY - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        // Vertices for two triangles (6 vertices) in the order:
        // bottom-left, top-left, top-right, bottom-left, top-right, bottom-right
        float vertices[6][4] = {
            // x,      y,      u,      v
            { xpos,     ypos + h,   0.0f, 0.0f },             // bottom-left
            { xpos,     ypos,       0.0f, 1.0f },             // top-left
            { xpos + w, ypos,       1.0f, 1.0f },             // top-right
            { xpos,     ypos + h,   0.0f, 0.0f },             // bottom-left (again)
            { xpos + w, ypos,       1.0f, 1.0f },             // top-right (again)
            { xpos + w, ypos + h,   1.0f, 0.0f }              // bottom-right
        };

        // Bind the glyph's texture
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update the VBO with the vertex data for this glyph
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        // Draw the 6 vertices (2 triangles)
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance the cursor for the next character
        cursorX += (ch.Advance >> 6) * scale; // Bitshift by 6 to get pixels (since 1/64th of a pixel)
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
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