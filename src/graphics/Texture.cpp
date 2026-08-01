#include "Texture.h"

#include "core/Logger.h"

Texture::Texture()
    : m_ID(0), m_Width(0), m_Height(0)
{
}

Texture::~Texture()
{
    Destroy();
}

bool Texture::Create(unsigned int width, unsigned int height, unsigned char* data,
                     int channels, bool generateMipmaps, bool srgb,
                     GLint wrapS, GLint wrapT, GLint minFilter, GLint magFilter)
{
    // Delete any existing texture
    if (m_ID != 0)
    {
        glDeleteTextures(1, &m_ID);
    }

    m_Width = width;
    m_Height = height;

    glGenTextures(1, &m_ID);
    glBindTexture(GL_TEXTURE_2D, m_ID);

    // Set texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

    // Set texture filtering parameters
    // If minFilter or magFilter is 0, use default based on generateMipmaps
    GLint effectiveMinFilter = minFilter;
    if (effectiveMinFilter == 0)
    {
        effectiveMinFilter = generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
    }
    GLint effectiveMagFilter = magFilter;
    if (effectiveMagFilter == 0)
    {
        effectiveMagFilter = GL_LINEAR;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, effectiveMinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, effectiveMagFilter);

    // Determine internal format and format based on channels and srgb
    GLint internalFormat = 0;
    GLenum format = 0;
    if (channels == 1)
    {
        internalFormat = srgb ? GL_R8 : GL_R8;
        format = GL_RED;
    }
    else if (channels == 2)
    {
        internalFormat = srgb ? GL_RG8 : GL_RG8;
        format = GL_RG;
    }
    else if (channels == 3)
    {
        internalFormat = srgb ? GL_SRGB8 : GL_RGB8;
        format = GL_RGB;
    }
    else if (channels == 4)
    {
        internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        format = GL_RGBA;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    if (generateMipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void Texture::Destroy()
{
    if (m_ID != 0)
    {
        glDeleteTextures(1, &m_ID);
        m_ID = 0;
    }
}

void Texture::Bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_ID);
}

void Texture::Unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}