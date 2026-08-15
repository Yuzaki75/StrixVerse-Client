#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

class AssetManager;
class Font;

// -----------------------------------------------------------------------------
// UIFonts
//
// The three typefaces the design specifies, resolved to a rasterised Font for a
// given pixel size.
//
// Each size needs its own glyph atlas, so the sizes named in UITheme are
// rasterised up front during initialisation; asking for an unlisted size still
// works but loads on first use. Instances live in AssetManager's cache, so
// nothing here rebuilds an atlas per frame.
//
// UI elements hold the resulting Font* directly rather than resolving a service
// every frame.
// -----------------------------------------------------------------------------
class UIFonts
{
public:
    enum class Typeface
    {
        Display,   // Press Start 2P  - titles, headings, buttons, tab labels
        Body,      // VT323           - body copy, descriptions, captions
        Data       // Share Tech Mono - numbers, coordinates, versions, ping
    };

    UIFonts() = default;

    UIFonts(const UIFonts&) = delete;
    UIFonts& operator=(const UIFonts&) = delete;

    // Rasterises the type scale declared in UITheme. Returns false only when
    // no face could be loaded at all; a single missing size is logged and
    // skipped so the client still starts.
    bool Initialize(AssetManager& assets);

    // Returns the face at that pixel size, loading it if necessary.
    // Returns nullptr when the font file is unavailable.
    Font* Get(Typeface face, unsigned int pixelSize) const;

    // True when at least one face loaded successfully.
    bool IsReady() const { return m_Ready; }

    static const char* GetPath(Typeface face);

private:
    static uint64_t MakeKey(Typeface face, unsigned int pixelSize);

    AssetManager* m_Assets = nullptr;

    // Mutable so Get() can stay const while lazily filling the cache.
    mutable std::unordered_map<uint64_t, std::shared_ptr<Font>> m_Cache;

    bool m_Ready = false;
};
