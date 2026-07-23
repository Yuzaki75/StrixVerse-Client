#pragma once

#include <glad/glad.h>

class Texture
{
public:
    Texture();
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    bool Create(unsigned int width, unsigned int height, unsigned char* data,
                int channels, bool generateMipmaps = true, bool srgb = false);
    void Destroy();

    void Bind(unsigned int slot = 0) const;
    void Unbind();

    unsigned int GetWidth() const { return m_Width; }
    unsigned int GetHeight() const { return m_Height; }
    unsigned int GetRendererID() const { return m_ID; }

private:
    unsigned int m_ID;
    unsigned int m_Width;
    unsigned int m_Height;
};