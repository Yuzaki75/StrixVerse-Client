#pragma once

#include <cstdint>

// -----------------------------------------------------------------------------
// CharacterPalette
// -----------------------------------------------------------------------------
// The colours an appearance index selects.
//
// The server owns this table. It sends *indices*, never colours -- six bytes,
// one per zone -- which is what makes validating a customisation an exact
// bounds check instead of a hopeful inspection of arbitrary RGB. The cost of
// that design is this file: the two tables must agree, entry for entry, or a
// player's look means one thing on the server and another on screen.
//
// Mirrors Server/src/entity/Appearance.h exactly. If a colour is added there,
// add it here in the same position; appending is safe, reordering is not.
//
// An out-of-range index is clamped rather than rejected. The server already
// refuses invalid indices on the way in, so anything arriving here that is out
// of range means the two tables have drifted -- and drawing the wrong colour is
// a better failure than not drawing the player at all.
// -----------------------------------------------------------------------------
namespace StrixVerse
{
    namespace CharacterPalette
    {
        inline constexpr uint32_t Hair[] = {
            0x3B2716, 0xB4462A, 0x4A3524, 0xD8D2C4, 0x1C1917, 0xC9A227,
        };
        inline constexpr uint32_t Skin[] = {
            0xE8B88C, 0xF2C9A0, 0xC68A5E, 0x8A6242, 0x5E4030,
        };
        inline constexpr uint32_t Eyes[] = {
            0x2B2B33, 0x3A6FB0, 0x4E8A46, 0x6B4A2B,
        };
        inline constexpr uint32_t Shirt[] = {
            0x3A6FB0, 0xB4462A, 0x4E8A46, 0x6E6A78, 0xD8B24A, 0x7A4A8A,
        };
        inline constexpr uint32_t Trousers[] = {
            0x3B4252, 0x2E2A28, 0x5A4632, 0x3B3A44,
        };
        inline constexpr uint32_t Boots[] = {
            0x4A3524, 0x3B2716, 0x2E2A28,
        };

        // The six zones, in the order they must be drawn: skin first so hair
        // and eyes sit on top of the face, clothing between. The layer PNGs are
        // disjoint masks of one silhouette, so this ordering only matters where
        // zones meet -- but where they meet, it matters.
        enum class Zone
        {
            Skin = 0,
            Trousers,
            Boots,
            Shirt,
            Hair,
            Eyes,
            Count
        };

        // Filename stem of each zone's layer, under assets/character/.
        inline const char* LayerName(Zone zone)
        {
            switch (zone)
            {
                case Zone::Skin:     return "skin";
                case Zone::Trousers: return "trousers";
                case Zone::Boots:    return "boots";
                case Zone::Shirt:    return "shirt";
                case Zone::Hair:     return "hair";
                case Zone::Eyes:     return "eyes";
                default:             return "skin";
            }
        }

        // The packed 0xRRGGBB for one zone and index, clamped into range.
        inline uint32_t Colour(Zone zone, uint8_t index)
        {
            const auto pick = [](const uint32_t* table, std::size_t count, uint8_t i) {
                return table[i < count ? i : 0];
            };

            switch (zone)
            {
                case Zone::Skin:     return pick(Skin,     std::size(Skin),     index);
                case Zone::Trousers: return pick(Trousers, std::size(Trousers), index);
                case Zone::Boots:    return pick(Boots,    std::size(Boots),    index);
                case Zone::Shirt:    return pick(Shirt,    std::size(Shirt),    index);
                case Zone::Hair:     return pick(Hair,     std::size(Hair),     index);
                case Zone::Eyes:     return pick(Eyes,     std::size(Eyes),     index);
                default:             return 0xFFFFFF;
            }
        }

        inline void ColourFloats(Zone zone, uint8_t index, float& r, float& g, float& b)
        {
            const uint32_t packed = Colour(zone, index);
            r = static_cast<float>((packed >> 16) & 0xFF) / 255.0f;
            g = static_cast<float>((packed >> 8) & 0xFF) / 255.0f;
            b = static_cast<float>(packed & 0xFF) / 255.0f;
        }
    }
}
