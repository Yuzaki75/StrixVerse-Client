#pragma once

#include <memory>
#include <optional>
#include <string>

#include "ScreenIDs.h"
#include "../graphics/Color.h"

class AssetManager;
class Engine;
class Font;
class Texture;
class UIFonts;
class UIManager;
class UIPanel;
class UIScale;

// -----------------------------------------------------------------------------
// Screen
//
// Base class for every full-screen state (splash, login, gameplay...).
//
// A screen builds its UI in OnEnter() under a single root panel that spans the
// visible canvas, and the base class tears that root down in OnExit(), which is
// what stops elements - and focus - leaking between screens.
//
// Navigation is a request: RequestScreenChange() records the target and the
// Engine performs the fade and the swap through ScreenFactory.
// -----------------------------------------------------------------------------
class Screen
{
public:
    explicit Screen(Engine* engine);
    virtual ~Screen();

    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;

    // Called when the screen becomes active.
    virtual void OnEnter() {}

    // Called before the screen is destroyed. Overrides must call the base so
    // the root panel is removed.
    virtual void OnExit();

    virtual void Update(float deltaTime);

    // Extra drawing beneath the UI (the world, for gameplay screens).
    virtual void RenderGame() const {}

    // --- Input -----------------------------------------------------------
    // Delivered after the UI has had its chance, so a screen-level "press any
    // key" never steals a keystroke from a focused text box.
    virtual void OnKeyDown(int key, bool ctrl, bool shift);
    virtual void OnMouseDown(float x, float y);

    // True while the screen wants raw key/mouse notifications even when a UI
    // element is focused. Only the splash screen needs this.
    virtual bool WantsRawInput() const { return false; }

    // --- Navigation -------------------------------------------------------
    void RequestScreenChange(ScreenID nextScreen);

    bool HasPendingChange() const { return pendingChange_.has_value(); }
    ScreenID GetPendingChange() const { return *pendingChange_; }
    void ClearPendingChange() { pendingChange_.reset(); }

protected:
    Engine* getEngine() const { return engine_; }
    UIManager* getUIManager() const { return uiManager_; }

    // --- Helpers for building screens -------------------------------------
    // Creates a transparent root panel covering the whole visible canvas and
    // registers it with the UIManager. Screens add their content to it.
    std::shared_ptr<UIPanel> CreateRoot();
    void DestroyRoot();

    const std::shared_ptr<UIPanel>& Root() const { return root_; }

    // Adds the full-bleed gradient background, optionally overlaid with the
    // design's ".sv-pixel-grid" dot lattice. Returns the background panel so
    // callers can size content against it.
    std::shared_ptr<UIPanel> AddBackdrop(const Color& top,
                                         const Color& bottom,
                                         bool pixelGrid);

    // Top-left of the 1920x1080 design area inside the root panel. On a window
    // that is not 16:9 the root extends past the design area, so screen content
    // is positioned relative to this origin.
    float DesignOriginX() const;
    float DesignOriginY() const;

    UIFonts* Fonts() const;
    AssetManager* Assets() const;
    const UIScale* Scale() const;

    // Convenience wrappers around UIFonts, returning nullptr when a face is
    // unavailable (callers pass the result straight to setFont()).
    Font* DisplayFont(unsigned int pixelSize) const;
    Font* BodyFont(unsigned int pixelSize) const;
    Font* DataFont(unsigned int pixelSize) const;

    std::shared_ptr<Texture> LoadTexture(const std::string& path) const;

    Engine*    engine_    = nullptr;
    UIManager* uiManager_ = nullptr;

    std::shared_ptr<UIPanel> root_;

private:
    std::optional<ScreenID> pendingChange_;
};
