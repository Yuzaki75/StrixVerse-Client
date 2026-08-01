#include "SpriteBatch.h"

#include <glad/glad.h>
#include <algorithm>
#include <iostream>

SpriteBatch::SpriteBatch(int maxSprites)
    : m_MaxSprites(maxSprites), m_SpriteCount(0)
{
    InitRenderData();
}

SpriteBatch::~SpriteBatch()
{
    if (m_VBO)
        glDeleteBuffers(1, &m_VBO);
    if (m_VAO)
        glDeleteVertexArrays(1, &m_VAO);
}

void SpriteBatch::InitRenderData()
{
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    // We'll allocate buffer for max vertices (maxSprites * 6 vertices)
    // Each vertex: 2 pos + 2 tex + 4 color = 8 floats
    int maxVertices = m_MaxSprites * 6;
    glBufferData(GL_ARRAY_BUFFER, maxVertices * 8 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // TexCoord attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(2 * sizeof(float)));
    // Color attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));

    glBindVertexArray(0);
}

void SpriteBatch::Begin()
{
    m_SpriteCount = 0;
    m_Sprites.clear();
    m_Sprites.reserve(m_MaxSprites);
}

void SpriteBatch::Draw(Texture& texture, float x, float y, float width, float height,
                       float r, float g, float b, float a)
{
    if (m_SpriteCount >= m_MaxSprites)
    {
        std::cerr << "SpriteBatch: Exceeded maximum sprite count (" << m_MaxSprites << ")" << std::endl;
        return;
    }

    Sprite sprite;
    sprite.texture = &texture;
    sprite.x = x;
    sprite.y = y;
    sprite.width = width;
    sprite.height = height;
    sprite.r = r;
    sprite.g = g;
    sprite.b = b;
    sprite.a = a;

    m_Sprites.push_back(sprite);
    m_SpriteCount++;
}

void SpriteBatch::End()
{
    // Nothing to do here - Flush() will handle the actual rendering
}

void SpriteBatch::Flush()
{
    if (m_SpriteCount == 0)
        return;

    std::vector<Vertex> vertices;
    vertices.reserve(m_SpriteCount * 6);

    // Group sprites by texture to minimize texture switches
    std::sort(m_Sprites.begin(), m_Sprites.end(), [](const Sprite& a, const Sprite& b) {
        return a.texture < b.texture;
    });

    Texture* currentTexture = nullptr;
    for (const Sprite& sprite : m_Sprites)
    {
        if (sprite.texture != currentTexture)
        {
            currentTexture = sprite.texture;
            if (currentTexture)
                currentTexture->Bind();
        }

        // Calculate texture coordinates (assuming texture coordinates are 0-1)
        float tx = 0.0f;
        float ty = 0.0f;
        float tw = 1.0f;
        float th = 1.0f;

        // Create 6 vertices for two triangles (a quad)
        // Triangle 1
        vertices.push_back({sprite.x,           sprite.y,           tx, ty, sprite.r, sprite.g, sprite.b, sprite.a}); // Top-left
        vertices.push_back({sprite.x + sprite.width, sprite.y,           tx + tw, ty, sprite.r, sprite.g, sprite.b, sprite.a}); // Top-right
        vertices.push_back({sprite.x,           sprite.y + sprite.height, tx, ty + th, sprite.r, sprite.g, sprite.b, sprite.a}); // Bottom-left

        // Triangle 2
        vertices.push_back({sprite.x + sprite.width, sprite.y,           tx + tw, ty, sprite.r, sprite.g, sprite.b, sprite.a}); // Top-right
        vertices.push_back({sprite.x + sprite.width, sprite.y + sprite.height, tx + tw, ty + th, sprite.r, sprite.g, sprite.b, sprite.a}); // Bottom-right
        vertices.push_back({sprite.x,           sprite.y + sprite.height, tx, ty + th, sprite.r, sprite.g, sprite.b, sprite.a}); // Bottom-left
    }

    // Upload vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    void* ptr = glMapBufferRange(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    if (ptr)
    {
        memcpy(ptr, vertices.data(), vertices.size() * sizeof(Vertex));
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    else
    {
        // Fallback if mapping fails
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex), vertices.data());
    }

    // Draw
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);

    // Unbind VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);
}