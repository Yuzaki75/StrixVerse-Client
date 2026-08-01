#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations.
class Texture;
class Shader;

class AssetManager
{
public:
    AssetManager();
    ~AssetManager();

    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // Load a texture from file. Returns a shared pointer to the texture.
    // If the texture is already loaded, returns the cached copy.
    std::shared_ptr<Texture> LoadTexture(const std::string& filePath,
                                         bool generateMipmaps = true,
                                         bool srgb = false);

    // Load a shader from vertex and fragment shader files.
    // Returns a shared pointer to the shader program.
    std::shared_ptr<Shader> LoadShader(const std::string& vertexPath,
                                       const std::string& fragmentPath,
                                       const std::string& geometryPath = "");

    // Get a texture by its file path (without loading). Returns nullptr if not loaded.
    std::shared_ptr<Texture> GetTexture(const std::string& filePath) const;

    // Get a texture by its OpenGL renderer ID. Returns nullptr if not found.
    Texture* GetTextureByRendererID(unsigned int rendererID) const;

    // Get a shader by its vertex and fragment shader paths (without loading).
    std::shared_ptr<Shader> GetShader(const std::string& vertexPath,
                                      const std::string& fragmentPath,
                                      const std::string& geometryPath = "") const;

    // Unload a specific texture.
    void UnloadTexture(const std::string& filePath);

    // Unload a specific shader.
    void UnloadShader(const std::string& vertexPath,
                      const std::string& fragmentPath,
                      const std::string& geometryPath = "");

    // Unload all assets that are no longer used outside of this manager.
    // Returns the number of assets unloaded.
    size_t UnloadUnused();

    // Unload all assets.
    void UnloadAll();

private:
    mutable std::mutex m_Mutex;

    struct TextureInfo
    {
        std::shared_ptr<Texture> texture;
        // We could add metadata like usage count, but we rely on shared_ptr.
    };

    struct ShaderInfo
    {
        std::shared_ptr<Shader> shader;
    };

    using TextureCache = std::unordered_map<std::string, TextureInfo>;
    using ShaderCache  = std::unordered_map<std::string, ShaderInfo>;

    TextureCache m_Textures;
    ShaderCache  m_Shaders;

    // Helper to generate a key for shaders based on the three paths.
    static std::string ShaderKey(const std::string& vertexPath,
                                 const std::string& fragmentPath,
                                 const std::string& geometryPath);
};