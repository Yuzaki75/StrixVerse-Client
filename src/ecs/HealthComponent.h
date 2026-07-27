#pragma once

#include "Component.h"

namespace StrixVerse
{
    namespace ECS
    {
        struct HealthComponent : public Component
        {
            float maxHealth = 100.0f;
            float currentHealth = 100.0f;
            bool isDead = false;
        };
    }
}