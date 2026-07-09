#include "Texture.h"

Texture::Texture()
{
}

Texture::~Texture()
{
    Destroy();
}

bool Texture::Load(const std::string& path)
{
    // TODO:
    // Load image using stb_image
    // Upload texture to OpenGL

    m_Loaded = false;

    return false;
}

void Texture::Destroy()
{
    if (m_ID != 0)
    {
        // glDeleteTextures(1, &m_ID);

        m_ID = 0;
    }

    m_Loaded = false;
}

void Texture::Bind(unsigned int slot) const
{
    // glActiveTexture(GL_TEXTURE0 + slot);
    // glBindTexture(GL_TEXTURE_2D, m_ID);
}

unsigned int Texture::GetWidth() const
{
    return m_Width;
}

unsigned int Texture::GetHeight() const
{
    return m_Height;
}

bool Texture::IsLoaded() const
{
    return m_Loaded;
}