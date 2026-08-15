#include "InputSystem.h"
#include "ComponentManager.h"
#include "InputComponent.h"
#include "../core/ServiceLocator.h"
#include "../core/Window.h"
#include "../ui/UIManager.h"

namespace StrixVerse
{
    namespace ECS
    {
        void InputSystem::init(EntityManager *entityManager, ComponentManager *componentManager)
        {
            System::init(entityManager, componentManager);

            setSignature<InputComponent>();
        }

        void InputSystem::update(const std::vector<Entity> &entities, float dt)
        {
            // Input is sampled as level state, so the timestep is unused here.
            (void)dt;

            // While a text field holds focus the keyboard belongs to it. This
            // system reads the hardware state directly and would otherwise walk
            // the player around as the message is typed.
            auto uiManager = ServiceLocator::Get<UIManager>();
            const bool typing = uiManager && uiManager->getFocusedElement() != nullptr;

            // Get the current keyboard state from SDL.
            const bool *state = SDL_GetKeyboardState(nullptr);

            for (Entity entity : entities)
            {
                if (m_pComponentManager->hasComponent<InputComponent>(entity))
                {
                    auto *input = m_pComponentManager->getComponent<InputComponent>(entity);
                    if (!input)
                    {
                        continue;
                    }
                    if (typing)
                    {
                        // Report every key as released, so anything already
                        // held stops the moment the field takes focus.
                        input->keys.reset();
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