#pragma once

#include <memory>
#include <string>

#include "../audio/AudioManager.h"
#include "../graphics/Camera2D.h"
#include "../graphics/SpriteBatch.h"
#include "../graphics/UIRenderer.h"

#include "networking/NetworkManager.h"

// ECS Includes
#include "ecs/EntityManager.h"
#include "ecs/ComponentManager.h"
#include "ecs/SystemManager.h"

// UI Includes
#include "ui/UIManager.h"
#include "ui/UIFonts.h"
#include "ui/UIScale.h"

// Screen Includes
#include "screens/Screen.h"
#include "screens/ScreenIDs.h"
#include "screens/ScreenFactory.h"

#include "core/AuthService.h"
#include "core/WorldManager.h"

class Window;
class AssetManager;
class Config;

// -----------------------------------------------------------------------------
// Engine
//
// Owns the main loop and the frame structure (events -> update -> render) and
// drives the Screen layer. It borrows the Window (non-owning; the Application
// owns it) and owns everything else it creates.
//
// Screen changes are requested by the active screen and performed here: the
// Engine fades out, tears the old screen down, builds the next one through
// ScreenFactory, and fades back in.
// -----------------------------------------------------------------------------
class Engine
{
public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool Initialize(Window* window, Config* config);

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

    // Opens the server session using the configured host and port. Safe to
    // call repeatedly; returns true when a session is already established.
    bool ConnectToServer();

    // Non-blocking form. Screens should prefer these: ConnectToServer() stalls
    // the whole window for the connect timeout, which is invisible on
    // localhost and very visible when the host is a friend's machine that is
    // switched off.
    using ConnectProgress = NetworkManager::ConnectProgress;

    bool            BeginConnectToServer();
    ConnectProgress PollConnectToServer();

    // True when the client is configured to run without a server.
    bool IsOfflineMode() const;

    NetworkManager& getNetworkManager() { return m_NetworkManager; }
    const NetworkManager& getNetworkManager() const { return m_NetworkManager; }

    // ECS Manager accessors
    StrixVerse::ECS::EntityManager& GetEntityManager() { return *m_pEntityManager; }
    const StrixVerse::ECS::EntityManager& GetEntityManager() const { return *m_pEntityManager; }
    StrixVerse::ECS::ComponentManager& GetComponentManager() { return *m_pComponentManager; }
    const StrixVerse::ECS::ComponentManager& GetComponentManager() const { return *m_pComponentManager; }
    StrixVerse::ECS::SystemManager& GetSystemManager() { return *m_pSystemManager; }
    const StrixVerse::ECS::SystemManager& GetSystemManager() const { return *m_pSystemManager; }

    AuthService* GetAuthService() { return m_AuthService.get(); }
    const AuthService* GetAuthService() const { return m_AuthService.get(); }

    WorldManager* GetWorldManager() { return m_WorldManager.get(); }
    const WorldManager* GetWorldManager() const { return m_WorldManager.get(); }

    Camera2D& GetCamera() { return m_Camera; }
    const Camera2D& GetCamera() const { return m_Camera; }

    Window* GetWindow() { return m_Window; }
    const Window* GetWindow() const { return m_Window; }

    UIManager* GetUIManager() { return m_UIManager.get(); }
    const UIManager* GetUIManager() const { return m_UIManager.get(); }

    AssetManager* GetAssetManager() { return m_AssetManager.get(); }
    const AssetManager* GetAssetManager() const { return m_AssetManager.get(); }

    UIFonts* GetUIFonts() { return m_UIFonts.get(); }
    const UIFonts* GetUIFonts() const { return m_UIFonts.get(); }

    AudioManager& GetAudio() { return m_Audio; }
    const AudioManager& GetAudio() const { return m_Audio; }

    // The screen shown before the current one. Settings uses it to know where
    // Back should return to, since it is reachable from both the main menu and
    // from gameplay.
    ScreenID GetPreviousScreenId() const { return m_PreviousScreenId; }

    UIRenderer* GetUIRenderer() { return m_UIRenderer.get(); }

    // Mapping from the 1920x1080 design canvas to the current window.
    const UIScale& GetUIScale() const { return m_UIScale; }

    // --- Session state shared between screens -----------------------------
    void SetSelectedWorldName(const std::string& name) { m_SelectedWorldName = name; }
    const std::string& GetSelectedWorldName() const { return m_SelectedWorldName; }

    void SetSignedInUser(const std::string& username) { m_SignedInUser = username; }
    const std::string& GetSignedInUser() const { return m_SignedInUser; }

    // Requests a screen change from outside the active screen (used by the
    // Engine itself and by any system that needs to force a state).
    void RequestScreenChange(ScreenID id);

private:
    void ProcessEvents();
    void Update(float deltaTime);
    void Render();

    void UpdateTransition(float deltaTime);
    void SwitchScreen(ScreenID id);
    void HandleResize();

private:
    Window* m_Window = nullptr;
    Config* m_Config = nullptr;

    std::shared_ptr<AssetManager> m_AssetManager;

    std::shared_ptr<StrixVerse::ECS::EntityManager>    m_pEntityManager;
    std::shared_ptr<StrixVerse::ECS::ComponentManager> m_pComponentManager;
    std::shared_ptr<StrixVerse::ECS::SystemManager>    m_pSystemManager;

    std::shared_ptr<SpriteBatch> m_SpriteBatch;
    std::shared_ptr<UIRenderer>  m_UIRenderer;
    std::shared_ptr<UIFonts>     m_UIFonts;

    AudioManager m_Audio;
    std::shared_ptr<UIManager>   m_UIManager;

    UIScale m_UIScale;

    EngineState    m_State = EngineState::Uninitialized;
    NetworkManager m_NetworkManager;

    // --- Screen management -------------------------------------------------
    std::unique_ptr<Screen> m_CurrentScreen;

    enum class TransitionState
    {
        None,
        FadingOut,
        FadingIn
    };

    TransitionState m_TransitionState  = TransitionState::None;
    float           m_TransitionTimer  = 0.0f;
    float           m_TransitionLength = 0.28f;   // Seconds per half.
    float           m_FadeAlpha        = 0.0f;
    ScreenID        m_NextScreen       = ScreenID::Splash;
    ScreenID        m_CurrentScreenId  = ScreenID::Splash;
    ScreenID        m_PreviousScreenId = ScreenID::MainMenu;
    bool            m_HasNextScreen    = false;

    std::unique_ptr<AuthService>  m_AuthService;
    std::unique_ptr<WorldManager> m_WorldManager;

    std::string m_SelectedWorldName;
    std::string m_SignedInUser;

    Camera2D m_Camera;
};
