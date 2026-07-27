#include "Engine.h"

#include "Game.h"
#include "Logger.h"
#include "Timer.h"
#include "Window.h"
#include "../graphics/Renderer.h"
#include "AssetManager.h"
#include "ServiceLocator.h"
#include "networking/NetworkManager.h"
#include <glad/glad.h>

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

Engine::Engine() = default;

Engine::~Engine()
{
    // RAII safety net for early exits.
    Shutdown();
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

    // Create and register the asset manager.
    m_AssetManager = std::make_shared<AssetManager>();
    ServiceLocator::Provide(m_AssetManager);

    // Initialize ECS Managers
    m_pEntityManager = std::make_shared<StrixVerse::ECS::EntityManager>();
    m_pComponentManager = std::make_shared<StrixVerse::ECS::ComponentManager>(m_pEntityManager->MAX_ENTITIES);
    m_pSystemManager = std::make_shared<StrixVerse::ECS::SystemManager>(m_pEntityManager.get(), m_pComponentManager.get());

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

    // Create and register essential systems
    auto movementSystem = m_pSystemManager->createSystem<StrixVerse::ECS::MovementSystem>();
    movementSystem->setSignature<StrixVerse::ECS::Transform, StrixVerse::ECS::VelocityComponent>();
    m_pSystemManager->addSystem(movementSystem);

    auto renderSystem = m_pSystemManager->createSystem<StrixVerse::ECS::RenderSystem>();
    renderSystem->setSignature<StrixVerse::ECS::Transform, StrixVerse::ECS::SpriteComponent>();
    m_pSystemManager->addSystem(renderSystem);

    auto inputSystem = m_pSystemManager->createSystem<StrixVerse::ECS::InputSystem>();
    inputSystem->setSignature<StrixVerse::ECS::InputComponent>();
    m_pSystemManager->addSystem(inputSystem);

    auto networkSyncSystem = m_pSystemManager->createSystem<StrixVerse::ECS::NetworkSyncSystem>();
    networkSyncSystem->setSignature<StrixVerse::ECS::Transform, StrixVerse::ECS::NetworkComponent>();
    m_pSystemManager->addSystem(networkSyncSystem);

    m_Game = std::make_unique<Game>();

    if (!m_Game->Initialize())
    {
        Logger::Error("Engine: failed to initialize game layer.");
        m_Game.reset();
        return false;
    }

    // Initialize network manager
    if (!m_NetworkManager.initialize())
    {
        Logger::Error("Engine: failed to initialize network manager.");
        return false;
    }

    m_State = EngineState::Initialized;

    Logger::Info("Engine initialized.");

    return true;
}

void Engine::Run()
{
    if (m_State != EngineState::Initialized)
    {
        Logger::Error("Engine: Run called before successful Initialize.");
        return;
    }

    m_State = EngineState::Running;

    Logger::Info("Entering main loop...");

    while (m_State == EngineState::Running)
    {
        Timer::Update();

        ProcessEvents();

        Update();

        Render();
    }
}

void Engine::Shutdown()
{
    if (m_State == EngineState::Shutdown ||
        m_State == EngineState::Uninitialized)
    {
        return;
    }

    // If we are still running, stop the loop.
    if (m_State == EngineState::Running)
        Stop();

    // Shutdown the game.
    if (m_Game)
    {
        m_Game->Shutdown();
        m_Game.reset();
    }

    // Disconnect the network manager.
    m_NetworkManager.disconnect();

    // Unregister ECS managers from service locator
    ServiceLocator::Remove<StrixVerse::ECS::EntityManager>();
    ServiceLocator::Remove<StrixVerse::ECS::ComponentManager>();
    ServiceLocator::Remove<StrixVerse::ECS::SystemManager>();

    // Shutdown ECS (in reverse order of initialization).
    m_pSystemManager.reset();
    m_pComponentManager.reset();
    m_pEntityManager.reset();

    // Unregister and destroy the asset manager.
    if (m_AssetManager)
    {
        ServiceLocator::Remove<AssetManager>();
        m_AssetManager.reset();
    }

    // Clear the window pointer (the Window object owns the SDL window and context).
    m_Window = nullptr;

    m_State = EngineState::Shutdown;

    Logger::Info("Engine shutdown.");
}

void Engine::Stop()
{
    if (m_State == EngineState::Running)
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

void Engine::ProcessEvents()
{
    if (!m_Window)
        return;

    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            Stop();
        }
        else if (event.type == SDL_EVENT_WINDOW_RESIZED)
        {
            // Update the viewport to match the new window size.
            int width = 0, height = 0;
            SDL_GetWindowSize(m_Window->GetSDLWindow(), &width, &height);
            glViewport(0, 0, width, height);
        }
    }
}

void Engine::Update()
{
    if (!m_Game)
        return;

    // Fixed timestep update (physics, netcode).
    while (Timer::ConsumeFixedStep())
    {
        m_Game->FixedUpdate(Timer::GetFixedTimestep());
        m_NetworkManager.update(Timer::GetFixedTimestep());
    }

    // Variable-rate gameplay logic.
    m_Game->Update(Timer::GetDeltaTime());

    // Update ECS systems.
    // Note: The order of updates is important. We'll update systems in the order they were added.
    // We'll update all systems with the variable time step (dt). Some systems might want to use fixed timestep.
    // For simplicity, we'll pass the variable dt to all systems. Systems that need fixed timestep can use a fixed dt from Timer.
    float dt = Timer::GetDeltaTime();
    m_pSystemManager->update(dt);
}

void Engine::Render()
{
    if (!m_Window)
        return;

    // Clear the screen and begin the frame.
    Renderer::BeginFrame();

    // Render ECS systems (which will draw sprites, etc.)
    m_pSystemManager->render();

    // Present the frame.
    m_Window->EndFrame();
}