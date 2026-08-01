#include "InputSystem.h"
#include "ComponentManager.h"
#include "InputComponent.h"
#include "../core/Window.h"

namespace StrixVerse
{
    namespace ECS
    {
        void InputSystem::update(const std::vector<Entity>& entities, float dt)
        {
            // Get the current keyboard state from SDL.
            const bool* state = SDL_GetKeyboardState(nullptr);

            for (Entity entity : entities)
            {
                if (m_pComponentManager->hasComponent<InputComponent>(entity))
                {
                    auto* input = m_pComponentManager->getComponent<InputComponent>(entity);
                    if (!input)
                    {
                        continue;
                    }
                    // Copy the keyboard state into our bitset.
                    // We'll copy up to MAX_KEYS.
                    for (int i = 0; i < InputComponent::MAX_KEYS; ++i)
                    {
                        input->keys[i] = state[i];
                    }
                }
            }
        }
    }
}