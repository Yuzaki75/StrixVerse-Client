#include "Engine.h"

#include "Game.h"
#include "Logger.h"
#include "Timer.h"
#include "Window.h"
#include "../graphics/Renderer.h"
#include "AssetManager.h"
#include "ServiceLocator.h"

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

    m_Game = std::make_unique<Game>();

    if (!m_Game->Initialize())
    {
        Logger::Error("Engine: failed to initialize game layer.");
        m_Game.reset();
        return false;
    }

    // Create and register the asset manager.
    m_AssetManager = std::make_shared<AssetManager>();
    ServiceLocator::Provide(m_AssetManager);

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

    // Shutdown the renderer.
    Renderer::Shutdown();

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

EngineState Engine::GetState() const
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

    m_Window->PollEvents();

    if (m_Window->ShouldClose())
    {
        Stop();
    }
}

void Engine::Update()
{
    if (!m_Game)
        return;

    // Deterministic simulation at a fixed rate (physics, netcode).
    while (Timer::ConsumeFixedStep())
    {
        m_Game->FixedUpdate(Timer::GetFixedTimestep());
    }

    // Variable-rate gameplay logic.
    m_Game->Update(Timer::GetDeltaTime());

    // Future systems:
    //
    // InputManager::Update();
    // NetworkManager::Update();
    // AudioManager::Update();
    // UIManager::Update();
}

void Engine::Render()
{
    if (!m_Window)
        return;

    // Clear the screen and begin the frame.
    Renderer::BeginFrame();

    // Let the game render its content.
    if (m_Game)
        m_Game->Render();

    // Present the frame.
    m_Window->EndFrame();
}