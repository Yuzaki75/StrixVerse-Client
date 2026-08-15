#include "UIFonts.h"

#include "UITheme.h"
#include "../core/AssetManager.h"
#include "../core/Logger.h"
#include "../graphics/Font.h"

#include <array>
#include <filesystem>
#include <format>

namespace
{
    // Every size the seven designed screens ask for. Rasterising these up front
    // keeps the first frame of a screen free of atlas-building hitches.
    constexpr std::array kDisplaySizes = {
        UITheme::Display::Micro,   UITheme::Display::Tiny,    UITheme::Display::Small,
        UITheme::Display::Label,   UITheme::Display::Button,  UITheme::Display::Tagline,
        UITheme::Display::Section, UITheme::Display::Subhead, UITheme::Display::Heading,
        UITheme::Display::Title,   UITheme::Display::Brand,   UITheme::Display::Hero,
        UITheme::Display::Splash,
    };

    constexpr std::array kBodySizes = {
        UITheme::Body::Tiny,    UITheme::Body::Caption, UITheme::Body::Small,
        UITheme::Body::Regular, UITheme::Body::Input,   UITheme::Body::Medium,
        UITheme::Body::Large,   UITheme::Body::Welcome, UITheme::Body::Splash,
    };

    constexpr std::array kDataSizes = {
        UITheme::Data::Small, UITheme::Data::Regular, UITheme::Data::Large,
    };
}

const char* UIFonts::GetPath(Typeface face)
{
    switch (face)
    {
    case Typeface::Display: return UITheme::FontDisplayPath;
    case Typeface::Body:    return UITheme::FontBodyPath;
    case Typeface::Data:    return UITheme::FontDataPath;
    }
    return UITheme::FontBodyPath;
}

uint64_t UIFonts::MakeKey(Typeface face, unsigned int pixelSize)
{
    return (static_cast<uint64_t>(face) << 32) | pixelSize;
}

bool UIFonts::Initialize(AssetManager& assets)
{
    m_Assets = &assets;
    m_Ready  = false;

    // Report a missing file once per face rather than once per size.
    const std::array<std::pair<Typeface, const char*>, 3> faces = {{
        {Typeface::Display, "Press Start 2P"},
        {Typeface::Body,    "VT323"},
        {Typeface::Data,    "Share Tech Mono"},
    }};

    for (const auto& [face, name] : faces)
    {
        const char* path = GetPath(face);
        if (!std::filesystem::exists(path))
        {
            Logger::Error(std::format(
                "UIFonts: '{}' is missing at '{}'. Text in that role will not render.", name, path));
            continue;
        }

        size_t loaded = 0;

        auto loadAll = [&](const auto& sizes)
        {
            for (unsigned int size : sizes)
            {
                if (Get(face, size) != nullptr)
                    ++loaded;
            }
        };

        switch (face)
        {
        case Typeface::Display: loadAll(kDisplaySizes); break;
        case Typeface::Body:    loadAll(kBodySizes);    break;
        case Typeface::Data:    loadAll(kDataSizes);    break;
        }

        if (loaded > 0)
        {
            m_Ready = true;
            Logger::Info(std::format("UIFonts: '{}' ready ({} sizes).", name, loaded));
        }
    }

    if (!m_Ready)
        Logger::Error("UIFonts: no design font could be loaded; UI text will be blank.");

    return m_Ready;
}

Font* UIFonts::Get(Typeface face, unsigned int pixelSize) const
{
    if (pixelSize == 0)
        return nullptr;

    const uint64_t key = MakeKey(face, pixelSize);

    const auto cached = m_Cache.find(key);
    if (cached != m_Cache.end())
        return cached->second.get();

    if (!m_Assets)
        return nullptr;

    // AssetManager owns the atlas; this cache only avoids the string keying
    // and mutex on the hot path.
    std::shared_ptr<Font> font = m_Assets->LoadFont(GetPath(face), pixelSize);

    // Cache the failure too, so a missing file is not retried every frame.
    m_Cache.emplace(key, font);

    return font.get();
}
