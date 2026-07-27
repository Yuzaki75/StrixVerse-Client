#pragma once

#include <memory>
#include <vector>
#include <string>

class Window;
class Game;
class AssetManager;
class ServiceLocator;
#include "networking/NetworkManager.h"

// ECS Includes
#include "ecs/EntityManager.h"
#include "ecs/ComponentManager.h"
#include "ecs/SystemManager.h"

// -----------------------------------------------------------------------------
// Engine
//
// Purpose:
//   Owns the main loop and the frame structure (events -> fixed updates ->
//   update -> render) and drives the Game layer. It borrows the Window
//   (non-owning pointer, the Application owns it) and owns the Game.
// -----------------------------------------------------------------------------
class Engine
{
public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool Initialize(Window* window);

    void Run();

    void Shutdown();

    void Stop();

    enum class EngineState
    {
        Uninitialized,
        Initialized,
        Running,
        Stopped,
        Shutdown
    };

    EngineState GetState() const;
    bool IsRunning() const;

    // Get reference to the network manager (for game to use)
    NetworkManager& getNetworkManager() { return m_NetworkManager; }
    const NetworkManager& getNetworkManager() const { return m_NetworkManager; }

    // ECS Manager accessors
    StrixVerse::ECS::EntityManager& GetEntityManager() { return *m_pEntityManager; }
    const StrixVerse::ECS::EntityManager& GetEntityManager() const { return *m_pEntityManager; }
    StrixVerse::ECS::ComponentManager& GetComponentManager() { return *m_pComponentManager; }
    const StrixVerse::ECS::ComponentManager& GetComponentManager() const { return *m_pComponentManager; }
    StrixVerse::ECS::SystemManager& GetSystemManager() { return *m_pSystemManager; }
    const StrixVerse::ECS::SystemManager& GetSystemManager() const { return *m_pSystemManager; }

private:
    void ProcessEvents();
    void Update();
    void Render();

private:
    Window* m_Window = nullptr;
    std::unique_ptr<Game> m_Game;
    std::shared_ptr<AssetManager> m_AssetManager;
    EngineState m_State = EngineState::Uninitialized;
    NetworkManager m_NetworkManager;

    // ECS Managers
    std::shared_ptr<StrixVerse::ECS::EntityManager> m_pEntityManager;
    std::shared_ptr<StrixVerse::ECS::ComponentManager> m_pComponentManager;
    std::shared_ptr<StrixVerse::ECS::SystemManager> m_pSystemManager;
};