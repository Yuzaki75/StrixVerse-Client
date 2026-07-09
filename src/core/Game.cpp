#include "Game.h"

#include "Logger.h"

namespace
{
    constexpr const char* kStartupScene = "MainMenu";
}

Game::~Game()
{
    // RAII safety net for early exits.
    Shutdown();
}

bool Game::Initialize()
{
    if (m_Initialized)
        return true;

    m_Initialized = true;

    if (!LoadScene(kStartupScene))
    {
        Logger::Error("Game: failed to load startup scene.");
        m_Initialized = false;
        return false;
    }

    Logger::Info("Game initialized.");

    return true;
}

bool Game::LoadScene(const std::string& sceneName)
{
    if (!m_Initialized)
    {
        Logger::Error("Game: LoadScene called before Initialize.");
        return false;
    }

    if (sceneName.empty())
    {
        Logger::Error("Game: scene name is empty.");
        return false;
    }

    if (!m_ActiveScene.empty())
        UnloadScene();

    // Future World subsystem:
    //
    // if (!SceneManager::Load(sceneName))
    //     return false;

    m_ActiveScene = sceneName;

    Logger::Info("Game: loaded scene '" + m_ActiveScene + "'.");

    return true;
}

void Game::UnloadScene()
{
    if (m_ActiveScene.empty())
        return;

    // Future World subsystem:
    //
    // SceneManager::Unload(m_ActiveScene);

    Logger::Info("Game: unloaded scene '" + m_ActiveScene + "'.");

    m_ActiveScene.clear();
}

void Game::Update(float deltaTime)
{
    if (!m_Initialized || m_ActiveScene.empty())
        return;

    (void)deltaTime;

    // Future gameplay systems:
    //
    // SceneManager::Update(deltaTime);
    // CameraController::Update(deltaTime);
}

void Game::FixedUpdate(float fixedDeltaTime)
{
    if (!m_Initialized || m_ActiveScene.empty())
        return;

    (void)fixedDeltaTime;

    // Future deterministic systems:
    //
    // Physics::Step(fixedDeltaTime);
    // Netcode::Tick(fixedDeltaTime);
}

void Game::Render()
{
    if (!m_Initialized || m_ActiveScene.empty())
        return;

    // Future rendering:
    //
    // SceneManager::Render();
}

void Game::Shutdown()
{
    if (!m_Initialized)
        return;

    UnloadScene();

    m_Initialized = false;

    Logger::Info("Game shutdown.");
}

bool Game::IsInitialized() const
{
    return m_Initialized;
}

bool Game::HasActiveScene() const
{
    return !m_ActiveScene.empty();
}

const std::string& Game::GetActiveSceneName() const
{
    return m_ActiveScene;
}
