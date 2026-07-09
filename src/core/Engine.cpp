#include "Engine.h"

#include "Game.h"
#include "Logger.h"
#include "Time.h"
#include "Window.h"

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

    Time::Initialize();

    m_Game = std::make_unique<Game>();

    if (!m_Game->Initialize())
    {
        Logger::Error("Engine: failed to initialize game layer.");
        m_Game.reset();
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
        Time::Update();

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
        m_State = EngineState::Shutdown;
        return;
    }

    if (m_Game)
    {
        m_Game->Shutdown();
        m_Game.reset();
    }

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
    while (Time::ConsumeFixedStep())
    {
        m_Game->FixedUpdate(Time::GetFixedTimestep());
    }

    // Variable-rate gameplay logic.
    m_Game->Update(Time::GetDeltaTime());

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

    m_Window->BeginFrame();

    if (m_Game)
        m_Game->Render();

    // Future rendering:
    //
    // Renderer::Begin();
    // UIManager::Render();
    // Renderer::End();

    m_Window->EndFrame();
}
