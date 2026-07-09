#include "Engine.h"

#include "Logger.h"
#include "Time.h"
#include "Window.h"

Engine::Engine()
    : m_Window(nullptr),
      m_Running(false)
{
}

Engine::~Engine()
{
}

bool Engine::Initialize(Window* window)
{
    m_Window = window;
    m_Running = true;

    Time::Initialize();

    Logger::Info("Engine initialized.");

    return true;
}

void Engine::Run()
{
    Logger::Info("Entering main loop...");

    while (m_Running)
    {
        Time::Update();

        ProcessEvents();

        Update();

        Render();
    }
}

void Engine::Shutdown()
{
    Logger::Info("Engine shutdown.");
}

void Engine::Stop()
{
    m_Running = false;
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
    // Future systems
    //
    // InputManager::Update();
    // NetworkManager::Update();
    // AudioManager::Update();
    // SceneManager::Update();
    // UIManager::Update();
}

void Engine::Render()
{
    if (!m_Window)
        return;

    m_Window->BeginFrame();

    // Future rendering
    //
    // Renderer::Begin();
    // SceneManager::Render();
    // UIManager::Render();
    // Renderer::End();

    m_Window->EndFrame();
}