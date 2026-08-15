#pragma once

#include <cstdint>

#include "../graphics/Color.h"

// -----------------------------------------------------------------------------
// UITheme
//
// The StrixVerse "Crystal Technology + Fantasy" design tokens, transcribed from
// the Figma style guide (src/styles/theme.css and the GLOBAL_CSS block of
// StrixVerse UI Design/src/app/App.tsx).
//
// Screens must take every colour, radius and type size from here rather than
// hardcoding literals, so the design stays adjustable from one place.
//
// All sizes are in virtual-canvas pixels on the 1920x1080 design canvas. The
// style guide's screen mock-ups are 460px-tall previews of that canvas, so
// their values are scaled by kDesignScale (1080 / 460). Splash is specified
// directly at 1920x1080 in Figma and needs no conversion.
// -----------------------------------------------------------------------------
namespace UITheme
{
    // Converts a 0xRRGGBB literal into the engine's float colour.
    constexpr Color Hex(uint32_t rgb, float alpha = 1.0f)
    {
        return Color(static_cast<float>((rgb >> 16) & 0xFFu) / 255.0f,
                     static_cast<float>((rgb >> 8) & 0xFFu) / 255.0f,
                     static_cast<float>(rgb & 0xFFu) / 255.0f,
                     alpha);
    }

    // Returns the same colour at a different alpha.
    constexpr Color WithAlpha(const Color& color, float alpha)
    {
        return Color(color.r, color.g, color.b, alpha);
    }

    // Scale from the style guide's 818x460 preview canvas to 1920x1080.
    inline constexpr float kDesignScale = 1080.0f / 460.0f;

    // Converts a style-guide pixel value to canvas pixels.
    constexpr float Scaled(float previewPixels)
    {
        return previewPixels * kDesignScale;
    }

    // --- Palette ---------------------------------------------------------
    inline constexpr Color Background = Hex(0x1E2230);   // Game / page background
    inline constexpr Color Panel      = Hex(0x2C3145);   // Windows and panels
    inline constexpr Color Primary    = Hex(0x4F8CFF);   // Primary actions
    inline constexpr Color Secondary  = Hex(0x6C5CE7);   // Secondary / skills
    inline constexpr Color Accent     = Hex(0x4DE1FF);   // Crystal glow
    inline constexpr Color Border     = Hex(0x6A7FB5);   // UI borders
    inline constexpr Color Success    = Hex(0x4CD964);   // Health / OK
    inline constexpr Color Warning    = Hex(0xFFD54A);   // Quests / warnings
    inline constexpr Color Danger     = Hex(0xFF5A5A);   // Errors / damage
    inline constexpr Color Gold       = Hex(0xFFD700);   // Coins / legendary
    inline constexpr Color Text       = Hex(0xFFFFFF);   // Primary text
    inline constexpr Color Subtext    = Hex(0xC7D0E0);   // Labels / captions
    inline constexpr Color Muted      = Hex(0x6A7FB5);   // De-emphasised text
    inline constexpr Color Epic       = Hex(0x9B59B6);   // Epic rarity

    // --- Surfaces --------------------------------------------------------
    inline constexpr Color InputBackground  = Hex(0x141826, 0.85f);
    inline constexpr Color InputBorder      = Hex(0x6A7FB5, 0.40f);
    inline constexpr Color PanelBorder      = Hex(0x6A7FB5, 0.45f);
    inline constexpr Color SubtleBorder     = Hex(0x6A7FB5, 0.20f);
    inline constexpr Color DividerBorder    = Hex(0x6A7FB5, 0.25f);
    inline constexpr Color RowBackground    = Hex(0x0E121E, 0.50f);
    inline constexpr Color ScreenBackground = Hex(0x0E1424);

    // --- Button gradients (.sv-btn and variants) --------------------------
    inline constexpr Color ButtonTop        = Hex(0x3A4D7A);
    inline constexpr Color ButtonBottom     = Hex(0x2D3A5E);
    inline constexpr Color ButtonHoverTop   = Hex(0x4A5E8E);
    inline constexpr Color ButtonHoverBot   = Hex(0x3C4B70);
    inline constexpr Color ButtonBorder     = Hex(0x4F8CFF, 0.60f);

    inline constexpr Color PurpleTop        = Hex(0x3D2F70);
    inline constexpr Color PurpleBottom     = Hex(0x2D2258);
    inline constexpr Color PurpleBorder     = Hex(0x6C5CE7, 0.65f);

    inline constexpr Color SuccessTop       = Hex(0x1E5C30);
    inline constexpr Color SuccessBottom    = Hex(0x164424);
    inline constexpr Color SuccessBorder    = Hex(0x4CD964, 0.65f);

    inline constexpr Color DangerTop        = Hex(0x6E2020);
    inline constexpr Color DangerBottom     = Hex(0x4E1616);
    inline constexpr Color DangerBorder     = Hex(0xFF5A5A, 0.65f);

    inline constexpr Color DisabledFill     = Hex(0x252A3E);
    inline constexpr Color DisabledBorder   = Hex(0x6A7FB5, 0.20f);

    // --- Radii (canvas pixels) -------------------------------------------
    inline constexpr float RadiusPanel  = Scaled(10.0f);
    inline constexpr float RadiusButton = Scaled(6.0f);
    inline constexpr float RadiusInput  = Scaled(6.0f);
    inline constexpr float RadiusChip   = Scaled(3.0f);
    inline constexpr float RadiusBar    = Scaled(4.0f);
    inline constexpr float RadiusCard   = Scaled(9.0f);

    inline constexpr float BorderThin   = Scaled(1.0f);
    inline constexpr float BorderThick  = Scaled(2.0f);

    // --- Type scale (canvas pixels) --------------------------------------
    // Press Start 2P - display, headings, button and tab labels.
    namespace Display
    {
        inline constexpr unsigned int Splash   = 103;   // Figma splash title, exact
        inline constexpr unsigned int Hero     = 61;    // Login brand lockup
        inline constexpr unsigned int Brand    = 56;    // Register brand lockup
        inline constexpr unsigned int Title    = 38;    // Continue "welcome" name
        inline constexpr unsigned int Heading  = 31;    // Screen headings
        inline constexpr unsigned int Subhead  = 28;    // Card titles
        inline constexpr unsigned int Section  = 26;    // Panel titles
        inline constexpr unsigned int Tagline  = 23;    // Splash tagline, exact
        inline constexpr unsigned int Button   = 21;    // Default button label
        inline constexpr unsigned int Label    = 19;    // Field labels
        inline constexpr unsigned int Small    = 16;    // Small labels
        inline constexpr unsigned int Tiny     = 14;    // Eyebrow labels
        inline constexpr unsigned int Micro    = 12;    // Tag chips
    }

    // VT323 - body copy, descriptions, captions.
    namespace Body
    {
        inline constexpr unsigned int Splash  = 52;   // "PRESS ANY KEY", exact
        inline constexpr unsigned int Welcome = 33;   // "WELCOME TO", exact
        inline constexpr unsigned int Large   = 47;
        inline constexpr unsigned int Medium  = 42;
        inline constexpr unsigned int Input   = 38;   // ".sv-input" field text
        inline constexpr unsigned int Regular = 35;
        inline constexpr unsigned int Small   = 33;
        inline constexpr unsigned int Caption = 31;
        inline constexpr unsigned int Tiny    = 28;
    }

    // Share Tech Mono - numbers, coordinates, versions, ping.
    namespace Data
    {
        inline constexpr unsigned int Large   = 26;
        inline constexpr unsigned int Regular = 23;
        inline constexpr unsigned int Small   = 21;   // Splash footer, exact
    }

    // --- Letter spacing (canvas pixels) ----------------------------------
    namespace Tracking
    {
        inline constexpr float SplashTitle   = 6.18f;    // Figma, exact
        inline constexpr float SplashTagline = 3.68f;    // Figma, exact
        inline constexpr float SplashWelcome = 9.90f;    // Figma, exact
        inline constexpr float SplashPrompt  = 8.32f;    // Figma, exact
        inline constexpr float SplashFooter  = 3.36f;    // Figma, exact
        inline constexpr float Wide          = Scaled(0.16f * 8.0f);
        inline constexpr float Normal        = 0.0f;
    }

    // --- Font files ------------------------------------------------------
    inline constexpr const char* FontDisplayPath = "assets/fonts/PressStart2P-Regular.ttf";
    inline constexpr const char* FontBodyPath    = "assets/fonts/VT323-Regular.ttf";
    inline constexpr const char* FontDataPath    = "assets/fonts/ShareTechMono-Regular.ttf";
}
