#include "Font.h"

#include "../core/Logger.h"

#include <glad/glad.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <format>

namespace
{
    // FreeType is expensive to initialise and is needed once per font/size
    // combination, so the library handle is shared for the process lifetime.
    // It owns no GL resources, so tearing it down at static destruction is safe.
    class FreeTypeLibrary
    {
    public:
        static FT_Library Get()
        {
            static FreeTypeLibrary instance;
            return instance.m_Library;
        }

        FreeTypeLibrary(const FreeTypeLibrary&) = delete;
        FreeTypeLibrary& operator=(const FreeTypeLibrary&) = delete;

    private:
        FreeTypeLibrary()
        {
            if (FT_Init_FreeType(&m_Library) != 0)
            {
                Logger::Error("Font: failed to initialise FreeType.");
                m_Library = nullptr;
            }
        }

        ~FreeTypeLibrary()
        {
            if (m_Library)
                FT_Done_FreeType(m_Library);
        }

        FT_Library m_Library = nullptr;
    };

    constexpr unsigned int kAtlasPadding = 1;   // Guards against filter bleed.
    constexpr unsigned int kMinAtlasSize = 256;
    constexpr unsigned int kMaxAtlasSize = 4096;
}

Font::Font() = default;

Font::~Font()
{
    Destroy();
}

std::vector<char32_t> Font::BuildCharacterSet()
{
    std::vector<char32_t> set;
    set.reserve(320);

    // Printable ASCII.
    for (char32_t cp = 0x20; cp <= 0x7E; ++cp)
        set.push_back(cp);

    // Latin-1 Supplement: covers the copyright sign and middle dot the design
    // uses, plus accented characters for player names.
    for (char32_t cp = 0xA0; cp <= 0xFF; ++cp)
        set.push_back(cp);

    // Symbols referenced by the Figma screens. Faces that lack any of these
    // simply skip them; callers fall back to drawn shapes.
    const char32_t symbols[] = {
        0x2022,  // bullet, splash tagline separator
        0x2013, 0x2014,          // en/em dash
        0x2018, 0x2019, 0x201C, 0x201D,
        0x2190, 0x2192,          // arrows
        0x21BA,                  // refresh
        0x25B6, 0x25C0, 0x25B8,  // play / back / list marker
        0x25CB, 0x25CF,          // pending / done bullets
        0x2605, 0x2606,          // filled / hollow star
        0x2713, 0x2714,          // check
        0x2715, 0x2717, 0x00D7,  // close
    };
    for (char32_t cp : symbols)
        set.push_back(cp);

    return set;
}

std::u32string Font::DecodeUtf8(const std::string& utf8)
{
    std::u32string out;
    out.reserve(utf8.size());

    const auto* bytes = reinterpret_cast<const unsigned char*>(utf8.data());
    const size_t length = utf8.size();

    for (size_t i = 0; i < length;)
    {
        const unsigned char lead = bytes[i];

        unsigned int extra = 0;
        char32_t     cp    = 0;

        if (lead < 0x80)             { cp = lead;         extra = 0; }
        else if ((lead & 0xE0) == 0xC0) { cp = lead & 0x1Fu; extra = 1; }
        else if ((lead & 0xF0) == 0xE0) { cp = lead & 0x0Fu; extra = 2; }
        else if ((lead & 0xF8) == 0xF0) { cp = lead & 0x07u; extra = 3; }
        else
        {
            out.push_back(0xFFFD);
            ++i;
            continue;
        }

        // The continuation bytes must all be present.
        if (i + extra >= length)
        {
            out.push_back(0xFFFD);
            break;
        }

        bool valid = true;
        for (unsigned int k = 1; k <= extra; ++k)
        {
            const unsigned char continuation = bytes[i + k];
            if ((continuation & 0xC0) != 0x80)
            {
                valid = false;
                break;
            }
            cp = (cp << 6) | (continuation & 0x3Fu);
        }

        if (!valid)
        {
            out.push_back(0xFFFD);
            ++i;
            continue;
        }

        out.push_back(cp);
        i += extra + 1;
    }

    return out;
}

bool Font::Load(const std::string& path, unsigned int pixelSize)
{
    Destroy();

    if (pixelSize == 0)
    {
        Logger::Error("Font: pixel size must be greater than zero.");
        return false;
    }

    FT_Library library = FreeTypeLibrary::Get();
    if (!library)
        return false;

    FT_Face face = nullptr;
    if (FT_New_Face(library, path.c_str(), 0, &face) != 0)
    {
        Logger::Error(std::format("Font: failed to load face '{}'.", path));
        return false;
    }

    if (FT_Set_Pixel_Sizes(face, 0, pixelSize) != 0)
    {
        Logger::Error(std::format("Font: failed to set pixel size {} on '{}'.", pixelSize, path));
        FT_Done_Face(face);
        return false;
    }

    // ---------------------------------------------------------------------
    // Pass 1: rasterise every requested glyph and keep the bitmaps around so
    // the atlas can be sized to fit rather than guessed at.
    // ---------------------------------------------------------------------
    struct PendingGlyph
    {
        char32_t                   codePoint;
        unsigned int               width;
        unsigned int               height;
        int                        bearingX;
        int                        bearingY;
        float                      advance;
        std::vector<unsigned char> pixels;
    };

    std::vector<PendingGlyph> pending;
    const std::vector<char32_t> charset = BuildCharacterSet();
    pending.reserve(charset.size());

    for (char32_t codePoint : charset)
    {
        if (FT_Get_Char_Index(face, static_cast<FT_ULong>(codePoint)) == 0)
            continue;   // Face has no glyph for this code point.

        if (FT_Load_Char(face, static_cast<FT_ULong>(codePoint), FT_LOAD_RENDER) != 0)
            continue;

        const FT_GlyphSlot slot = face->glyph;

        PendingGlyph glyph{};
        glyph.codePoint = codePoint;
        glyph.width     = slot->bitmap.width;
        glyph.height    = slot->bitmap.rows;
        glyph.bearingX  = slot->bitmap_left;
        glyph.bearingY  = slot->bitmap_top;
        glyph.advance   = static_cast<float>(slot->advance.x) / 64.0f;

        const size_t pixelCount = static_cast<size_t>(glyph.width) * glyph.height;
        glyph.pixels.resize(pixelCount);
        if (pixelCount > 0 && slot->bitmap.buffer)
        {
            // FreeType rows may be padded; copy row by row using the pitch.
            for (unsigned int row = 0; row < glyph.height; ++row)
            {
                const unsigned char* src = slot->bitmap.buffer + static_cast<ptrdiff_t>(row) * slot->bitmap.pitch;
                std::copy(src, src + glyph.width, glyph.pixels.begin() + static_cast<ptrdiff_t>(row) * glyph.width);
            }
        }

        pending.push_back(std::move(glyph));
    }

    if (pending.empty())
    {
        Logger::Error(std::format("Font: '{}' produced no glyphs.", path));
        FT_Done_Face(face);
        return false;
    }

    m_Ascent     = static_cast<float>(face->size->metrics.ascender) / 64.0f;
    m_Descent    = -static_cast<float>(face->size->metrics.descender) / 64.0f;
    m_LineHeight = static_cast<float>(face->size->metrics.height) / 64.0f;

    FT_Done_Face(face);

    // Tallest glyph first keeps the shelves tight.
    std::vector<const PendingGlyph*> ordered;
    ordered.reserve(pending.size());
    for (const auto& glyph : pending)
        ordered.push_back(&glyph);

    std::sort(ordered.begin(), ordered.end(),
              [](const PendingGlyph* a, const PendingGlyph* b) { return a->height > b->height; });

    // ---------------------------------------------------------------------
    // Pass 2: shelf-pack into the smallest power-of-two atlas that fits.
    // ---------------------------------------------------------------------
    struct Placement
    {
        const PendingGlyph* glyph;
        unsigned int        x;
        unsigned int        y;
    };

    std::vector<Placement> placements;
    unsigned int atlasSize = kMinAtlasSize;
    bool         packed    = false;

    while (!packed && atlasSize <= kMaxAtlasSize)
    {
        placements.clear();
        placements.reserve(ordered.size());

        unsigned int penX      = kAtlasPadding;
        unsigned int penY      = kAtlasPadding;
        unsigned int shelfHigh = 0;
        packed                 = true;

        for (const PendingGlyph* glyph : ordered)
        {
            const unsigned int w = glyph->width;
            const unsigned int h = glyph->height;

            if (penX + w + kAtlasPadding > atlasSize)
            {
                penX = kAtlasPadding;
                penY += shelfHigh + kAtlasPadding;
                shelfHigh = 0;
            }

            if (penY + h + kAtlasPadding > atlasSize)
            {
                packed = false;
                break;
            }

            placements.push_back({glyph, penX, penY});

            penX += w + kAtlasPadding;
            shelfHigh = std::max(shelfHigh, h);
        }

        if (!packed)
            atlasSize *= 2;
    }

    if (!packed)
    {
        Logger::Error(std::format("Font: '{}' at {}px does not fit a {}px atlas.",
                                  path, pixelSize, kMaxAtlasSize));
        return false;
    }

    // ---------------------------------------------------------------------
    // Pass 3: blit into a single-channel atlas and upload once.
    // ---------------------------------------------------------------------
    std::vector<unsigned char> atlas(static_cast<size_t>(atlasSize) * atlasSize, 0);

    for (const Placement& placement : placements)
    {
        const PendingGlyph& glyph = *placement.glyph;

        for (unsigned int row = 0; row < glyph.height; ++row)
        {
            const size_t dstOffset = static_cast<size_t>(placement.y + row) * atlasSize + placement.x;
            const size_t srcOffset = static_cast<size_t>(row) * glyph.width;
            std::copy(glyph.pixels.begin() + static_cast<ptrdiff_t>(srcOffset),
                      glyph.pixels.begin() + static_cast<ptrdiff_t>(srcOffset + glyph.width),
                      atlas.begin() + static_cast<ptrdiff_t>(dstOffset));
        }

        const float inv = 1.0f / static_cast<float>(atlasSize);

        Glyph entry{};
        entry.uvMin   = {static_cast<float>(placement.x) * inv,
                         static_cast<float>(placement.y) * inv};
        entry.uvMax   = {static_cast<float>(placement.x + glyph.width) * inv,
                         static_cast<float>(placement.y + glyph.height) * inv};
        entry.size    = {static_cast<int>(glyph.width), static_cast<int>(glyph.height)};
        entry.bearing = {glyph.bearingX, glyph.bearingY};
        entry.advance = glyph.advance;

        m_Glyphs.emplace(glyph.codePoint, entry);
    }

    GLint previousUnpackAlignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glGenTextures(1, &m_AtlasTexture);
    glBindTexture(GL_TEXTURE_2D, m_AtlasTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                 static_cast<GLsizei>(atlasSize), static_cast<GLsizei>(atlasSize),
                 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Pixel typefaces are rasterised at their exact display size, so nearest
    // filtering keeps the blocky edges the design depends on.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);

    m_AtlasWidth  = atlasSize;
    m_AtlasHeight = atlasSize;
    m_PixelSize   = pixelSize;
    m_Loaded      = true;

    Logger::Debug(std::format("Font: loaded '{}' at {}px ({} glyphs, {}x{} atlas).",
                              path, pixelSize, m_Glyphs.size(), atlasSize, atlasSize));

    return true;
}

void Font::Destroy()
{
    if (m_AtlasTexture != 0)
    {
        glDeleteTextures(1, &m_AtlasTexture);
        m_AtlasTexture = 0;
    }

    m_Glyphs.clear();

    m_AtlasWidth  = 0;
    m_AtlasHeight = 0;
    m_PixelSize   = 0;
    m_Ascent      = 0.0f;
    m_Descent     = 0.0f;
    m_LineHeight  = 0.0f;
    m_Loaded      = false;
}

const Glyph* Font::GetGlyph(char32_t codePoint) const
{
    const auto it = m_Glyphs.find(codePoint);
    return it != m_Glyphs.end() ? &it->second : nullptr;
}

float Font::MeasureWidth(const std::string& utf8, float letterSpacing) const
{
    if (!m_Loaded || utf8.empty())
        return 0.0f;

    const std::u32string codePoints = DecodeUtf8(utf8);

    float width = 0.0f;
    size_t drawn = 0;

    for (char32_t codePoint : codePoints)
    {
        const Glyph* glyph = GetGlyph(codePoint);
        if (!glyph)
            continue;

        width += glyph->advance;
        ++drawn;
    }

    // Tracking sits between glyphs, never after the last one.
    if (drawn > 1)
        width += letterSpacing * static_cast<float>(drawn - 1);

    return width;
}

glm::vec2 Font::MeasureText(const std::string& utf8, float letterSpacing) const
{
    return {MeasureWidth(utf8, letterSpacing), m_LineHeight};
}
