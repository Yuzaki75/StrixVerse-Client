#pragma once

#include <memory>
#include <optional>
#include "../ui/UIElement.h"
#include "ScreenIDs.h"

// Forward declarations to break circular dependency
class Engine;
class UIManager;

/**
 * Base class for all game screens (menus, gameplay screens, etc.).
 */
class Screen {
public:
    explicit Screen(Engine* engine);
    virtual ~Screen() = default;

    // Called when the screen becomes active
    virtual void OnEnter() {}

    // Called when the screen is about to be deactivated
    virtual void OnExit() {}

    // Update the screen logic
    virtual void Update(float deltaTime);

    // Render the screen
    virtual void Render() const;

    // Handle input events (if needed)
    virtual void HandleInput() {}

    // Request to switch to another screen by ID
    void RequestScreenChange(ScreenID nextScreen);

    // Check if there is a pending screen change
    bool HasPendingChange() const { return pendingChange_.has_value(); }
    ScreenID GetPendingChange() const { return *pendingChange_; }
    void ClearPendingChange() { pendingChange_.reset(); }

protected:
    // Helper methods to get ECS managers
    Engine* getEngine() const { return engine_; }
    UIManager* getUIManager() const { return uiManager_; }

    // Virtual methods for game logic and rendering (to be overridden by derived classes)
    virtual void UpdateGameLogic() {}
    virtual void RenderGame() const {}

    // Non-owning pointer to the engine
    Engine* engine_ = nullptr;
    // Non-owning pointer to the UIManager (from the engine)
    UIManager* uiManager_ = nullptr;

private:
    std::optional<ScreenID> pendingChange_;
};