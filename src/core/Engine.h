#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>

#include "../graphics/Camera2D.h"
#include "../graphics/Font.h"
#include "../graphics/SpriteBatch.h"

class Window;
class Game;
class AssetManager;
class ServiceLocator;
#include "networking/NetworkManager.h"

// ECS Includes
#include "ecs/EntityManager.h"
#include "ecs/ComponentManager.h"
#include "ecs/SystemManager.h"

// UI Includes
#include "ui/UIManager.h"
#include "ui/UIPanel.h"

// Screen Includes
#include "screens/Screen.h"
#include "core/AuthService.h"

// World System
#include "core/WorldManager.h"

#include "screens/ScreenIDs.h"
#include "screens/ScreenFactory.h"

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

    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    bool Initialize(Window *window);

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
    NetworkManager &getNetworkManager() { return m_NetworkManager; }
    const NetworkManager &getNetworkManager() const { return m_NetworkManager; }

    // ECS Manager accessors
    StrixVerse::ECS::EntityManager &GetEntityManager() { return *m_pEntityManager; }
    const StrixVerse::ECS::EntityManager &GetEntityManager() const { return *m_pEntityManager; }
    StrixVerse::ECS::ComponentManager &GetComponentManager() { return *m_pComponentManager; }
    const StrixVerse::ECS::ComponentManager &GetComponentManager() const { return *m_pComponentManager; }
    StrixVerse::ECS::SystemManager &GetSystemManager() { return *m_pSystemManager; }
    const StrixVerse::ECS::SystemManager &GetSystemManager() const { return *m_pSystemManager; }

    // AuthService access
    AuthService *GetAuthService() { return m_AuthService.get(); }
    const AuthService *GetAuthService() const { return m_AuthService.get(); }

    // WorldManager access
    WorldManager *GetWorldManager() { return m_WorldManager.get(); }
    const WorldManager *GetWorldManager() const { return m_WorldManager.get(); }

    // Camera access
    Camera2D &GetCamera() { return m_Camera; }
    const Camera2D &GetCamera() const { return m_Camera; }

    // Window access
    Window *GetWindow() { return m_Window; }
    const Window *GetWindow() const { return m_Window; }

    // UIManager access
    UIManager *GetUIManager() { return m_UIManager.get(); }
    const UIManager *GetUIManager() const { return m_UIManager.get(); }

    // Selected world management (for world selection flow)
    void SetSelectedWorldName(const std::string &name) { m_SelectedWorldName = name; }
    const std::string &GetSelectedWorldName() const { return m_SelectedWorldName; }

private:
    void ProcessEvents();
    void Update();
    void Render();

private:
    Window *m_Window = nullptr;
    std::unique_ptr<Game> m_Game;
    std::shared_ptr<AssetManager> m_AssetManager;
    std::shared_ptr<StrixVerse::ECS::EntityManager> m_pEntityManager;
    std::shared_ptr<StrixVerse::ECS::ComponentManager> m_pComponentManager;
    std::shared_ptr<StrixVerse::ECS::SystemManager> m_pSystemManager;
    std::shared_ptr<SpriteBatch> m_SpriteBatch;
    std::shared_ptr<Font> m_Font;
    EngineState m_State = EngineState::Uninitialized;
    NetworkManager m_NetworkManager;

    // UI Manager
    std::shared_ptr<UIManager> m_UIManager;
    // Fade overlay for transitions
    std::shared_ptr<UIPanel> m_FadeOverlay;

    // Screen management
    std::unique_ptr<Screen> m_CurrentScreen;
    std::unique_ptr<Screen> m_PendingScreen; // screen to transition to
    void SetCurrentScreen(std::unique_ptr<Screen> screen);

    // Transition state
    enum class TransitionState
    {
        None,
        FadingOut,
        FadingIn
    };
    TransitionState m_TransitionState = TransitionState::None;
    float m_TransitionTimer = 0.0f;
    float m_TransitionDuration = 0.5f; // seconds

    // AuthService for handling authentication
    std::unique_ptr<AuthService> m_AuthService;

    // WorldManager for handling world saves/loads
    std::unique_ptr<WorldManager> m_WorldManager;

    // Selected world name (set by WorldBrowserScreen, used by LoadingScreen and GameScreen)
    std::string m_SelectedWorldName;

    // Main camera
    Camera2D m_Camera;
};