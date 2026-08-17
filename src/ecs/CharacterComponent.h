#pragma once

#include "Component.h"

#include <cstdint>

namespace StrixVerse
{
    namespace ECS
    {
        // What a player looks like: six palette indices, exactly as the server
        // stores and sends them.
        //
        // Indices, not colours. The server owns the palette and validates a
        // customisation as a bounds check; the client resolves an index to a
        // colour through CharacterPalette at draw time. Storing RGB here would
        // put a second, drifting copy of the palette in the component.
        //
        // An entity carrying this is drawn by CharacterRenderSystem and must
        // NOT also carry a SpriteComponent, or it will be drawn twice -- once
        // as a character and once as whatever the sprite says.
        struct CharacterComponent : public Component
        {
            uint8_t hair     = 0;
            uint8_t skin     = 0;
            uint8_t eyes     = 0;
            uint8_t shirt    = 0;
            uint8_t trousers = 0;
            uint8_t boots    = 0;

            // Drawn above the tiles, like the sprite layer it replaces.
            int layer = 10;
        };
    }
}
