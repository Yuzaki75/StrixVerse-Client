#pragma once

#include "System.h"

namespace StrixVerse
{
    namespace ECS
    {
        class NetworkSyncSystem : public System
        {
        public:
            void update(const std::vector<Entity>& entities, float dt) override;
        };
    }
}