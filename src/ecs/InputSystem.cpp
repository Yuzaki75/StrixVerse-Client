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

            // Pause joins the same condition rather than being applied after
            // the fact. Clearing the component from GameScreen::Update did not
            // work: this system repopulates the bitset from the live keyboard
            // in the same frame, so the reset was simply overwritten and the
            // player walked around behind the pause overlay.
            const bool blocked = typing || m_GameplayPaused;

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
                    if (blocked)
                    {
                        // Report every key as released, so anything already
                        // held stops the moment the field takes focus or the
                        // game pauses.
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