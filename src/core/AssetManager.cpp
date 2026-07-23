#include "AssetManager.h"

#include "Logger.h"
#include "../graphics/Texture.h"
#include "../graphics/Shader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <format>
#include <fstream>
#include <sstream>
#include <iostream>

AssetManager::AssetManager()
{
    // Flip stb_image's loading to have the origin at the top-left (OpenGL convention).
    stbi_set_flip_vertically_on_load(true);
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

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    const char* gShaderCode = geometryCode.c_str();

    // Create Shader object.
    auto shader = std::make_shared<Shader>();
    if (!shader->Load(vertexPath, fragmentPath, geometryPath))
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
            if (it->second.texture.unique())
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
            if (it->second.shader.unique())
            {
                it = m_Shaders.erase(it);
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
}

std::string AssetManager::ShaderKey(const std::string& vertexPath,
                                    const std::string& fragmentPath,
                                    const std::string& geometryPath)
{
    return vertexPath + "|" + fragmentPath + "|" + (geometryPath.empty() ? "" : geometryPath);
}