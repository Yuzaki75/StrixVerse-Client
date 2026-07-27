#pragma once

#include <string>
#include "Component.h"

namespace StrixVerse
{
    namespace ECS
    {
        struct PlayerComponent : public Component
        {
            std::string username;
            int score = 0;
        };
    }
}