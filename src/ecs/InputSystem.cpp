#include "InputSystem.h"
#include "ComponentManager.h"
#include "InputComponent.h"
#include <SDL.h>

namespace StrixVerse
{
    namespace ECS
    {
        void InputSystem::update(const std::vector<Entity>& entities, float dt)
        {
            // Get the current keyboard state from SDL.
            const Uint8* state = SDL_GetKeyboardState(nullptr);

            for (Entity entity : entities)
            {
                if (m_pComponentManager->hasComponent<InputComponent>(entity))
                {
                    InputComponent& input = m_pComponentManager->getComponent<InputComponent>(entity);
                    // Copy the keyboard state into our bitset.
                    // We'll copy up to MAX_KEYS.
                    for (int i = 0; i < InputComponent::MAX_KEYS; ++i)
                    {
                        input.keys[i] = (state[i] == SDL_PRESSED);
                    }
                }
            }
        }
    }
}