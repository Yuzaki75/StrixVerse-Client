#pragma once

#include <vector>
#include <algorithm>
#include "Texture.h"

class SpriteBatch
{
public:
    SpriteBatch(int maxSprites = 2048);
    ~SpriteBatch();

    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;

    void Begin();
    void Draw(Texture& texture, float x, float y, float width, float height,
              float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);
    void End();
    void Flush();

private:
    struct Sprite
    {
        Texture* texture;
        float x, y, width, height;
        float r, g, b, a;
    };

    struct Vertex
    {
        float x, y;      // position
        float u, v;      // texCoord
        float r, g, b, a; // color
    };

    void InitRenderData();

    std::vector<Sprite> m_Sprites;
    GLuint m_VAO, m_VBO;
    int m_MaxSprites;
    int m_SpriteCount;
};