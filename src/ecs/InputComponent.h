#pragma once

#include <bitset>
#include "Component.h"

namespace StrixVerse
{
    namespace ECS
    {
        // We'll use a bitset for keyboard state. We'll assume we only need to track a limited number of keys for now.
        // In a real game, we might want to map SDL scancodes to game actions.
        struct InputComponent : public Component
        {
            // We'll use a bitset for the current state of each key (pressed or not).
            // We'll size it to SDL_SCANCODE_COUNT, but we don't want to include SDL.h here.
            // We'll define a maximum number of keys we care about, say 512.
            static const int MAX_KEYS = 512;
            std::bitset<MAX_KEYS> keys;
        };
    }
}