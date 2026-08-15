#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

// -----------------------------------------------------------------------------
// Font
//
// A single font face rasterised at one pixel size into one packed alpha atlas.
// Rasterising per size (rather than scaling a single face at draw time) is what
// keeps the pixel typefaces used by the design - Press Start 2P, VT323,
// Share Tech Mono - crisp instead of blurred.
//
// The atlas texture and per-glyph metrics are exposed so UIRenderer can emit
// glyph quads into the same batch as the rest of the UI, preserving draw order.
// Instances are cached by (path, pixelSize) in AssetManager; nothing should
// construct a Font per frame.
// -----------------------------------------------------------------------------

struct Glyph
{
    glm::vec2  uvMin{0.0f, 0.0f};   // Atlas coordinates, normalised.
    glm::vec2  uvMax{0.0f, 0.0f};
    glm::ivec2 size{0, 0};          // Bitmap size in pixels.
    glm::ivec2 bearing{0, 0};       // Left / top bearing in pixels.
    float      advance = 0.0f;      // Pen advance in pixels.
};

class Font
{
public:
    Font();
    ~Font();

    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    // Rasterises the face at the given pixel size. Any previously loaded
    // atlas is released first, so reloading is safe.
    bool Load(const std::string& path, unsigned int pixelSize);

    void Destroy();

    bool IsLoaded() const { return m_Loaded; }

    // Returns nullptr when the face has no glyph for this code point.
    const Glyph* GetGlyph(char32_t codePoint) const;

    unsigned int GetAtlasTexture() const { return m_AtlasTexture; }
    unsigned int GetPixelSize() const { return m_PixelSize; }

    // Vertical metrics in pixels. Ascent is positive above the baseline.
    float GetAscent() const { return m_Ascent; }
    float GetDescent() const { return m_Descent; }
    float GetLineHeight() const { return m_LineHeight; }

    // Advance width of a UTF-8 run including the design's letter spacing
    // (tracking), which is applied between glyphs but not after the last one.
    float MeasureWidth(const std::string& utf8, float letterSpacing = 0.0f) const;

    // Width in x, line height in y. Use the line height - not the tallest
    // glyph - so that vertical centring does not shift with the text content.
    glm::vec2 MeasureText(const std::string& utf8, float letterSpacing = 0.0f) const;

    // Decodes UTF-8, substituting U+FFFD for malformed sequences.
    static std::u32string DecodeUtf8(const std::string& utf8);

private:
    // Code points rasterised into the atlas: ASCII, Latin-1 Supplement and the
    // handful of symbols the design uses (bullet, check, arrows, stars...).
    static std::vector<char32_t> BuildCharacterSet();

    std::unordered_map<char32_t, Glyph> m_Glyphs;

    unsigned int m_AtlasTexture = 0;
    unsigned int m_AtlasWidth   = 0;
    unsigned int m_AtlasHeight  = 0;
    unsigned int m_PixelSize    = 0;

    float m_Ascent     = 0.0f;
    float m_Descent    = 0.0f;
    float m_LineHeight = 0.0f;

    bool m_Loaded = false;
};
