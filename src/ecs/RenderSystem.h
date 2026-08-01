#pragma once

#include "System.h"

namespace StrixVerse
{
    namespace ECS
    {
        class RenderSystem : public System
        {
        public:
            void render(const std::vector<Entity>& entities) override;
        };
    }
}