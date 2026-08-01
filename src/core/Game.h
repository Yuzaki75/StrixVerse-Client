#pragma once

#include <string>
#include <memory>
#include "ecs/Entity.h"

// Forward declarations
namespace StrixVerse { namespace ECS { class EntityManager; class ComponentManager; class SystemManager; } }

// UI Forward declaration
class UIManager;
class UIButton;
class UILabel;

// -----------------------------------------------------------------------------
// Game
//
// Purpose:
//   Gameplay layer of the client. The Engine drives it (Clean Architecture:
//   Engine knows Game, Game knows nothing about Engine or Window), and it is
//   the single entry point for scene lifetime and per-frame gameplay logic.
//
// Responsibilities:
//   - Initialize()   : prepare gameplay state, load the startup scene
//   - LoadScene()    : switch the active scene by name
//   - UnloadScene()  : unload the active scene, release gameplay state
//   - Update()       : variable-rate gameplay logic (animation, UI, camera)
//   - FixedUpdate()  : deterministic logic at a fixed rate (physics, netcode)
//   - Render()       : issue gameplay render commands
//   - Shutdown()     : unload the active scene, release gameplay state
//
// Dependencies: Logger (Core). Scene content itself belongs to the future
// World subsystem; this class only owns the lifecycle hooks so the Engine
// loop is already correct when that subsystem lands.
// -----------------------------------------------------------------------------
class Game
{
public:
    Game() = default;
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // Prepares gameplay state and loads the startup scene.
    bool Initialize();

    // Unloads the current scene (if any) and loads the named scene.
    bool LoadScene(const std::string& sceneName);

    // Unloads the active scene without loading a new one.
    void UnloadScene();

    // Variable-timestep update, called once per frame.
    void Update(float deltaTime);

    // Fixed-timestep update, called zero or more times per frame.
    void FixedUpdate(float fixedDeltaTime);

    // Called once per frame between Window::BeginFrame and EndFrame.
    void Render();

    // Releases all gameplay state. Safe to call multiple times.
    void Shutdown();

    bool IsInitialized() const;
    bool HasActiveScene() const;
    const std::string& GetActiveSceneName() const;

private:
    bool m_Initialized = false;
    std::string m_ActiveScene;

    // ECS entities for testing
    StrixVerse::ECS::Entity m_TestEntity;
    StrixVerse::ECS::Entity m_TestEntity2;

    // UI components for testing
    std::unique_ptr<UIManager> m_UIManager;
    std::shared_ptr<UIButton> m_TestButton;
    std::shared_ptr<UILabel> m_TestLabel;
};