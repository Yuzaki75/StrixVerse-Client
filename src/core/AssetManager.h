#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations.
class Texture;
class Shader;
class Font;

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

    // Registers a texture built in memory (procedural noise, generated
    // gradients) under a cache key, so callers can look it up like a file and
    // never rebuild it per frame. Returns the cached copy if the key exists.
    std::shared_ptr<Texture> CreateTexture(const std::string& key,
                                           unsigned int width,
                                           unsigned int height,
                                           const unsigned char* pixels,
                                           int channels);

    // Get a texture by its file path (without loading). Returns nullptr if not loaded.
    std::shared_ptr<Texture> GetTexture(const std::string& filePath) const;

    // Get a texture by its OpenGL renderer ID. Returns nullptr if not found.
    Texture* GetTextureByRendererID(unsigned int rendererID) const;

    // Get a shader by its vertex and fragment shader paths (without loading).
    std::shared_ptr<Shader> GetShader(const std::string& vertexPath,
                                      const std::string& fragmentPath,
                                      const std::string& geometryPath = "") const;

    // Load a font face rasterised at a specific pixel size. Faces are cached
    // per (path, pixelSize) because each size needs its own glyph atlas, and
    // rebuilding one mid-frame would be ruinous.
    std::shared_ptr<Font> LoadFont(const std::string& filePath, unsigned int pixelSize);

    // Get an already-loaded font. Returns nullptr if that size is not cached.
    std::shared_ptr<Font> GetFont(const std::string& filePath, unsigned int pixelSize) const;

    // Unload a specific font size.
    void UnloadFont(const std::string& filePath, unsigned int pixelSize);

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

    struct FontInfo
    {
        std::shared_ptr<Font> font;
    };

    using TextureCache = std::unordered_map<std::string, TextureInfo>;
    using ShaderCache  = std::unordered_map<std::string, ShaderInfo>;
    using FontCache    = std::unordered_map<std::string, FontInfo>;

    TextureCache m_Textures;
    ShaderCache  m_Shaders;
    FontCache    m_Fonts;

    // Helper to generate a key for shaders based on the three paths.
    static std::string ShaderKey(const std::string& vertexPath,
                                 const std::string& fragmentPath,
                                 const std::string& geometryPath);

    // Helper to generate a key for a font face at a specific pixel size.
    static std::string FontKey(const std::string& filePath, unsigned int pixelSize);
};