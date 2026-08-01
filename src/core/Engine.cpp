#include "Engine.h"

#include "Game.h"

#include "Logger.h"
#include "Timer.h"
#include "Window.h"
#include "../graphics/Renderer.h"
#include "AssetManager.h"
#include "../graphics/Font.h"
#include "../graphics/SpriteBatch.h"
#include "ServiceLocator.h"
#include "networking/NetworkManager.h"

#include <filesystem>

// UI Includes
#include "ui/UIManager.h"
#include "ui/UIPanel.h"

// Screen Includes
#include "screens/SplashScreen.h"
#include "screens/ScreenFactory.h"
#include "core/AuthService.h"
#include "core/WorldManager.h"

// Component and System includes for ECS setup
#include "ecs/TransformComponent.h"
#include "ecs/SpriteComponent.h"
#include "ecs/VelocityComponent.h"
#include "ecs/InputComponent.h"
#include "ecs/NetworkComponent.h"
#include "ecs/PlayerComponent.h"
#include "ecs/MovementSystem.h"
#include "ecs/RenderSystem.h"
#include "ecs/InputSystem.h"
#include "ecs/NetworkSyncSystem.h"
#include "ecs/SystemManager.h"
#include "ecs/PlayerSystem.h"
#include "ecs/Camera2DSystem.h"
#include "ecs/Camera2DComponent.h"

Engine::Engine() = default;

Engine::~Engine()
{
    // RAII safety net for early exits.
    Shutdown();
}

// Screen management
void Engine::SetCurrentScreen(std::unique_ptr<Screen> screen)
{
    // Exit current screen if exists
    if (m_CurrentScreen)
    {
        m_CurrentScreen->OnExit();
    }

    // Set new screen
    m_CurrentScreen = std::move(screen);

    // Enter new screen
    if (m_CurrentScreen)
    {
        m_CurrentScreen->OnEnter();
    }

    // Clear any pending screen and reset transition state (immediate change)
    m_PendingScreen.reset();
    m_TransitionState = TransitionState::None;
    m_TransitionTimer = 0.0f;
}

bool Engine::Initialize(Window* window)
{
    if (m_State != EngineState::Uninitialized)
    {
        Logger::Warning("Engine: Initialize called twice.");
        return m_State == EngineState::Initialized;
    }

    if (!window)
    {
        Logger::Error("Engine: window is null.");
        return false;
    }

    m_Window = window;

    Timer::Initialize();

    // Initialize the renderer after the OpenGL context is current.
    if (!Renderer::Initialize())
    {
        Logger::Error("Engine: failed to initialize renderer.");
        return false;
    }

    // Register the engine itself for systems that need to reach back into it.
    ServiceLocator::Provide(std::shared_ptr<Engine>(this, [](Engine*) {}));

    // Create and register the asset manager.
    m_AssetManager = std::make_shared<AssetManager>();
    ServiceLocator::Provide(m_AssetManager);

    // Create and register UI rendering services.
    m_SpriteBatch = std::make_shared<SpriteBatch>();
    ServiceLocator::Provide(m_SpriteBatch);

    m_Font = std::make_shared<Font>();
    const std::filesystem::path fontCandidates[] =
    {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/calibri.ttf"
    };

    for (const auto& fontPath : fontCandidates)
    {
        if (std::filesystem::exists(fontPath) && m_Font->Load(fontPath.string(), 24))
        {
            break;
        }
    }

    if (!m_Font->IsLoaded())
    {
        Logger::Warning("Engine: no system font could be loaded; text rendering will be disabled.");
    }

    ServiceLocator::Provide(m_Font);

    // Initialize ECS Managers
    m_pEntityManager = std::make_shared<StrixVerse::ECS::EntityManager>();
    m_pComponentManager = std::make_shared<StrixVerse::ECS::ComponentManager>(m_pEntityManager->MAX_ENTITIES);
    m_pSystemManager = std::make_shared<StrixVerse::ECS::SystemManager>(m_pEntityManager.get(), m_pComponentManager.get());

    // Wire up the ECS notification chain
    m_pEntityManager->SetComponentManager(m_pComponentManager.get());
    m_pEntityManager->SetSystemManager(m_pSystemManager.get());
    m_pComponentManager->SetSystemManager(m_pSystemManager.get());

    // Register ECS managers with service locator
    ServiceLocator::Provide(m_pEntityManager);
    ServiceLocator::Provide(m_pComponentManager);
    ServiceLocator::Provide(m_pSystemManager);

    // Register essential components
    m_pComponentManager->registerComponent<StrixVerse::ECS::Transform>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::SpriteComponent>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::VelocityComponent>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::InputComponent>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::NetworkComponent>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::PlayerComponent>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::Camera2DComponent>();

    // Create and register essential systems
    auto movementSystem = m_pSystemManager->createSystem<StrixVerse::ECS::MovementSystem>();
    movementSystem->setSignature<StrixVerse::ECS::Transform, StrixVerse::ECS::VelocityComponent>();
    m_pSystemManager->addSystem(movementSystem);

    auto renderSystem = m_pSystemManager->createSystem<StrixVerse::ECS::RenderSystem>();
    renderSystem->setSignature<StrixVerse::ECS::Transform, StrixVerse::ECS::SpriteComponent>();
    m_pSystemManager->addSystem(renderSystem);

    auto cameraSystem = m_pSystemManager->createSystem<StrixVerse::ECS::Camera2DSystem>();
    cameraSystem->setSignature<StrixVerse::ECS::Camera2DComponent, StrixVerse::ECS::Transform>();
    m_pSystemManager->addSystem(cameraSystem);

    // Initialize UI Manager
    m_UIManager = std::make_shared<UIManager>();
    ServiceLocator::Provide(m_UIManager);

    // Create fade overlay for transitions
    m_FadeOverlay = std::make_shared<UIPanel>();
    int width, height;
    m_Window->GetSize(width, height);
    m_FadeOverlay->setSize(static_cast<float>(width), static_cast<float>(height));
    m_FadeOverlay->setPosition(0.0f, 0.0f);
    m_FadeOverlay->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f}); // start fully transparent
    m_UIManager->addElement(m_FadeOverlay);

    // Initialize AuthService
    m_AuthService = std::make_unique<AuthService>();

    // Initialize WorldManager
    m_WorldManager = std::make_unique<WorldManager>();

    // Initialize network manager
    if (!m_NetworkManager.initialize())
    {
        Logger::Error("Engine: failed to initialize network manager.");
        return false;
    }

    // Initialize screen system - start with splash screen
    SetCurrentScreen(std::make_unique<SplashScreen>(this));

    m_State = EngineState::Initialized;

    Logger::Info("Engine initialized.");

    return true;
}

void Engine::Run()
{
    if (m_State != EngineState::Initialized)
    {
        Logger::Error("Engine: Cannot run engine - not initialized or already running/shutdown.");
        return;
    }

    m_State = EngineState::Running;
    Logger::Info("Engine: Starting main loop");

    // Main game loop
    while (m_State == EngineState::Running)
    {
        // Calculate delta time
        float deltaTime = Timer::GetDeltaTime();
        float fixedDeltaTime = Timer::GetFixedTimestep();

        // Process events (input, window events, etc.)
        ProcessEvents();

        // Update game logic (fixed timestep for physics, etc.)
        Update();

        // Render the frame
        Render();

        // Update timer for next frame
        Timer::Update();
    }

    Logger::Info("Engine: Main loop ended");
}

void Engine::ProcessEvents()
{
    if (!m_Window)
        return;

    // Process window events (keyboard, mouse, window close, etc.)
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        // Handle window events
        if (event.type == SDL_EVENT_QUIT)
        {
            Stop();
            return;
        }

        // Forward event to UI manager
        if (m_UIManager)
        {
            switch (event.type)
            {
                case SDL_EVENT_MOUSE_MOTION:
                    m_UIManager->handleMouseMove(event.motion.x, event.motion.y);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    m_UIManager->handleMouseDown(event.button.x, event.button.y);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    m_UIManager->handleMouseUp(event.button.x, event.button.y);
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key < 256) // Regular character key
                        m_UIManager->handleKeyPressed(static_cast<char>(event.key.key));
                    else // Special key (arrow keys, etc.)
                        m_UIManager->handleSpecialKeyPressed(event.key.key);
                    break;
                default:
                    break;
            }
        }

        // Forward event to current screen
        if (m_CurrentScreen)
        {
            m_CurrentScreen->HandleInput();
        }
    }
}

void Engine::Update()
{
    if (m_State != EngineState::Running)
        return;

    // Get fixed time step for consistent physics/gameplay updates
    float fixedDeltaTime = Timer::GetFixedTimestep();

    // Update ECS systems
    if (m_pSystemManager)
    {
        m_pSystemManager->update(fixedDeltaTime);
    }

    // Update current screen
    if (m_CurrentScreen)
    {
        m_CurrentScreen->Update(fixedDeltaTime);
    }

    // Handle screen transitions
    if (m_PendingScreen)
    {
        // Handle transition based on current state
        switch (m_TransitionState)
        {
            case TransitionState::None:
                // Start fading out
                m_TransitionState = TransitionState::FadingOut;
                m_TransitionTimer = 0.0f;
                m_FadeOverlay->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});
                break;

            case TransitionState::FadingOut:
                m_TransitionTimer += fixedDeltaTime;
                if (m_TransitionTimer >= m_TransitionDuration)
                {
                    // Finish fade out, switch screens
                    m_TransitionTimer = m_TransitionDuration;

                    // Hide current screen
                    if (m_CurrentScreen)
                    {
                        m_CurrentScreen->OnExit();
                    }
                    m_CurrentScreen.reset();

                    // Show new screen
                    m_CurrentScreen = std::move(m_PendingScreen);
                    if (m_CurrentScreen)
                    {
                        m_CurrentScreen->OnEnter();
                    }

                    // Start fading in
                    m_TransitionState = TransitionState::FadingIn;
                    m_TransitionTimer = 0.0f;
                }
                else
                {
                    // Continue fading out
                    float alpha = m_TransitionTimer / m_TransitionDuration;
                    m_FadeOverlay->setBackgroundColor({0.0f, 0.0f, 0.0f, alpha});
                }
                break;

            case TransitionState::FadingIn:
                m_TransitionTimer += fixedDeltaTime;
                if (m_TransitionTimer >= m_TransitionDuration)
                {
                    // Finish fade in
                    m_TransitionTimer = m_TransitionDuration;
                    m_TransitionState = TransitionState::None;
                    m_PendingScreen.reset();
                }
                else
                {
                    // Continue fading in
                    float alpha = 1.0f - (m_TransitionTimer / m_TransitionDuration);
                    m_FadeOverlay->setBackgroundColor({0.0f, 0.0f, 0.0f, alpha});
                }
                break;
        }
    }

    // Update fade overlay (for transitions)
    if (m_FadeOverlay)
    {
        // Fade overlay is updated implicitly through the transition logic above
    }

    // Update HUDs or other UI elements that need per-frame updates
    // (This would typically be handled by individual screens or systems)
}

void Engine::Render()
{
    if (m_State != EngineState::Running)
        return;

    // Clear the screen
    Renderer::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f}); // Dark gray background
    Renderer::BeginFrame();

    // Render ECS systems (sprites, etc.)
    if (m_pSystemManager)
    {
        m_pSystemManager->render();
    }

    // Render current screen
    if (m_CurrentScreen)
    {
        m_CurrentScreen->Render();
    }

    // Render UI (including fade overlay)
    if (m_UIManager)
    {
        m_UIManager->render();
    }

    // Present the frame
    Renderer::EndFrame();
}

void Engine::Shutdown()
{
    if (m_State == EngineState::Shutdown)
        return;

    Logger::Info("Engine: Shutting down");

    // Stop the engine if it's running
    if (m_State == EngineState::Running)
    {
        Stop();
    }

    // Clean up current screen
    if (m_CurrentScreen)
    {
        m_CurrentScreen->OnExit();
        m_CurrentScreen.reset();
    }

    // Clean up pending screen
    m_PendingScreen.reset();

    // Clean up UI manager (this will destroy UI elements and their textures)
    m_UIManager.reset();

    // Clean up ECS managers
    m_pSystemManager.reset();
    m_pComponentManager.reset();
    m_pEntityManager.reset();

    // Clean up asset manager (this will destroy textures, shaders, etc.)
    m_AssetManager.reset();

    // Clean up services (AuthService, WorldManager) - note: these are unique_ptrs
    m_WorldManager.reset();
    m_AuthService.reset();

    // Shutdown renderer (this will destroy the OpenGL context)
    Renderer::Shutdown();

    // Shutdown timer
    // Timer::Shutdown(); // Timer doesn't have a shutdown method

    m_State = EngineState::Shutdown;
    Logger::Info("Engine: Shutdown complete");
}

void Engine::Stop()
{
    if (m_State != EngineState::Running)
        return;

    Logger::Info("Engine: Stopping");
    m_State = EngineState::Stopped;
}

Engine::EngineState Engine::GetState() const
{
    return m_State;
}

bool Engine::IsRunning() const
{
    return m_State == EngineState::Running;
}