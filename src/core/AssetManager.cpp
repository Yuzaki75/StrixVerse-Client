#include "AssetManager.h"

#include "Logger.h"
#include "../graphics/Texture.h"
#include "../graphics/Shader.h"
#include "../graphics/Font.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <format>
#include <fstream>
#include <sstream>
#include <iostream>

AssetManager::AssetManager()
{
    // No vertical flip. stb_image already returns rows top-down, which is what
    // every consumer here wants: the UI renderer draws in screen space with y
    // and v both increasing downward, so texture row 0 must be the image's top
    // row. Flipping (the usual choice for bottom-left GL texture space) drew
    // every loaded image upside-down.
    stbi_set_flip_vertically_on_load(false);
}

AssetManager::~AssetManager()
{
    UnloadAll();
}

std::shared_ptr<Texture> AssetManager::LoadTexture(const std::string& filePath,
                                                   bool generateMipmaps,
                                                   bool srgb)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_Textures.find(filePath);
    if (it != m_Textures.end())
    {
        // Return the cached texture.
        return it->second.texture;
    }

    // Load the texture data from file.
    int width, height, channels;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    if (!data)
    {
        std::string errorMsg = std::format("Failed to load texture: {}", filePath);
        Logger::Error(errorMsg);
        return nullptr;
    }

    // Create a Texture object.
    auto texture = std::make_shared<Texture>();
    if (!texture->Create(width, height, data, channels, generateMipmaps, srgb))
    {
        std::string errorMsg = std::format("Failed to create texture from data: {}", filePath);
        Logger::Error(errorMsg);
        stbi_image_free(data);
        return nullptr;
    }

    stbi_image_free(data);

    // Cache the texture.
    m_Textures[filePath] = {texture};
    return texture;
}

std::shared_ptr<Shader> AssetManager::LoadShader(const std::string& vertexPath,
                                                 const std::string& fragmentPath,
                                                 const std::string& geometryPath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    std::string key = ShaderKey(vertexPath, fragmentPath, geometryPath);
    auto it = m_Shaders.find(key);
    if (it != m_Shaders.end())
    {
        return it->second.shader;
    }

    // Read shader source from files.
    std::string vertexCode, fragmentCode, geometryCode;
    std::ifstream vShaderFile, fShaderFile, gShaderFile;
    // Ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // Open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // Read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // Close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // Convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
        // If geometry shader path is present, also load a geometry shader
        if (!geometryPath.empty())
        {
            gShaderFile.open(geometryPath);
            std::stringstream gShaderStream;
            gShaderStream << gShaderFile.rdbuf();
            gShaderFile.close();
            geometryCode = gShaderStream.str();
        }
    }
    catch (std::ifstream::failure& e)
    {
        std::string errorMsg = std::format("Failed to read shader files: {}", e.what());
        Logger::Error(errorMsg);
        return nullptr;
    }

    // Create Shader object using pre-read source code (avoids double file I/O)
    auto shader = std::make_shared<Shader>();
    if (!shader->LoadFromSource(vertexCode, fragmentCode, geometryCode))
    {
        std::string errorMsg = std::format("Failed to compile shader: {}", key);
        Logger::Error(errorMsg);
        return nullptr;
    }

    // Cache the shader.
    m_Shaders[key] = {shader};
    return shader;
}

std::shared_ptr<Texture> AssetManager::GetTexture(const std::string& filePath) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Textures.find(filePath);
    if (it != m_Textures.end())
    {
        return it->second.texture;
    }
    return nullptr;
}

Texture* AssetManager::GetTextureByRendererID(unsigned int rendererID) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& pair : m_Textures)
    {
        if (pair.second.texture && pair.second.texture->GetRendererID() == rendererID)
        {
            return pair.second.texture.get();
        }
    }
    return nullptr;
}

std::shared_ptr<Shader> AssetManager::GetShader(const std::string& vertexPath,
                                                const std::string& fragmentPath,
                                                const std::string& geometryPath) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::string key = ShaderKey(vertexPath, fragmentPath, geometryPath);
    auto it = m_Shaders.find(key);
    if (it != m_Shaders.end())
    {
        return it->second.shader;
    }
    return nullptr;
}

std::shared_ptr<Texture> AssetManager::CreateTexture(const std::string& key,
                                                     unsigned int width,
                                                     unsigned int height,
                                                     const unsigned char* pixels,
                                                     int channels)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_Textures.find(key);
    if (it != m_Textures.end())
    {
        return it->second.texture;
    }

    if (width == 0 || height == 0 || !pixels)
    {
        Logger::Error(std::format("Failed to create procedural texture: {}", key));
        return nullptr;
    }

    auto texture = std::make_shared<Texture>();

    // Texture::Create does not modify the pixel data; the const_cast is only
    // needed because it takes a non-const pointer for the GL call.
    if (!texture->Create(width, height, const_cast<unsigned char*>(pixels), channels,
                         /*generateMipmaps*/ false, /*srgb*/ false))
    {
        Logger::Error(std::format("Failed to upload procedural texture: {}", key));
        return nullptr;
    }

    m_Textures[key] = {texture};
    return texture;
}

std::shared_ptr<Font> AssetManager::LoadFont(const std::string& filePath, unsigned int pixelSize)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    const std::string key = FontKey(filePath, pixelSize);

    auto it = m_Fonts.find(key);
    if (it != m_Fonts.end())
    {
        return it->second.font;
    }

    auto font = std::make_shared<Font>();
    if (!font->Load(filePath, pixelSize))
    {
        // Font::Load already logged the specific failure.
        return nullptr;
    }

    m_Fonts[key] = {font};
    return font;
}

std::shared_ptr<Font> AssetManager::GetFont(const std::string& filePath, unsigned int pixelSize) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_Fonts.find(FontKey(filePath, pixelSize));
    if (it != m_Fonts.end())
    {
        return it->second.font;
    }
    return nullptr;
}

void AssetManager::UnloadFont(const std::string& filePath, unsigned int pixelSize)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_Fonts.find(FontKey(filePath, pixelSize));
    if (it != m_Fonts.end())
    {
        m_Fonts.erase(it);
    }
}

void AssetManager::UnloadTexture(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Textures.find(filePath);
    if (it != m_Textures.end())
    {
        m_Textures.erase(it);
    }
}

void AssetManager::UnloadShader(const std::string& vertexPath,
                                const std::string& fragmentPath,
                                const std::string& geometryPath)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::string key = ShaderKey(vertexPath, fragmentPath, geometryPath);
    auto it = m_Shaders.find(key);
    if (it != m_Shaders.end())
    {
        m_Shaders.erase(it);
    }
}

size_t AssetManager::UnloadUnused()
{
    size_t count = 0;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        // Remove textures that are no longer shared outside.
        for (auto it = m_Textures.begin(); it != m_Textures.end();)
        {
            if (it->second.texture.use_count() == 1)
            {
                it = m_Textures.erase(it);
                ++count;
            }
            else
            {
                ++it;
            }
        }
        // Remove shaders that are no longer shared outside.
        for (auto it = m_Shaders.begin(); it != m_Shaders.end();)
        {
            if (it->second.shader.use_count() == 1)
            {
                it = m_Shaders.erase(it);
                ++count;
            }
            else
            {
                ++it;
            }
        }
        // Remove font atlases that are no longer shared outside.
        for (auto it = m_Fonts.begin(); it != m_Fonts.end();)
        {
            if (it->second.font.use_count() == 1)
            {
                it = m_Fonts.erase(it);
                ++count;
            }
            else
            {
                ++it;
            }
        }
    }
    return count;
}

void AssetManager::UnloadAll()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Textures.clear();
    m_Shaders.clear();
    m_Fonts.clear();
}

std::string AssetManager::ShaderKey(const std::string& vertexPath,
                                    const std::string& fragmentPath,
                                    const std::string& geometryPath)
{
    return vertexPath + "|" + fragmentPath + "|" + (geometryPath.empty() ? "" : geometryPath);
}

std::string AssetManager::FontKey(const std::string& filePath, unsigned int pixelSize)
{
    return std::format("{}#{}", filePath, pixelSize);
}