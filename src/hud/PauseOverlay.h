#pragma once

#include <functional>
#include <memory>

class Engine;
class UIManager;
class UIPanel;
class UIButton;

// -----------------------------------------------------------------------------
// PauseOverlay
//
// The Escape menu: a dim over the live world with a centred PAUSED box and
// three buttons - resume, settings, leave.
//
// An overlay rather than a screen change on purpose. A screen change runs
// OnExit, which tears down the world, the HUD and every player entity and
// rebuilds them all on the way back; pausing leaves everything exactly where
// it is behind the dim.
//
// Follows the WorldManagerPanel lifecycle exactly - built once against the
// UIManager, shown and hidden by visibility, re-inserted on open so it draws
// above anything built after it. What the buttons do is not decided here:
// GameScreen sets the three callbacks, because navigation belongs to the
// screen that owns the session.
// -----------------------------------------------------------------------------
class PauseOverlay
{
public:
    PauseOverlay(Engine* engine, UIManager* uiManager);
    ~PauseOverlay();

    PauseOverlay(const PauseOverlay&) = delete;
    PauseOverlay& operator=(const PauseOverlay&) = delete;

    // Built once and toggled by visibility; rebuilding per open would discard
    // focus and every live callback for no benefit.
    void Build();

    // Re-assert this overlay's place at the top of the draw order.
    //
    // UIManager renders in insertion order, so "on top" means "inserted last"
    // - and anything inserted while an overlay is up therefore outranks it.
    // Notifications, chat bubbles and floating name tags are all created at
    // runtime and were drawing over the pause dim and through open panels.
    // Open() already re-inserts for exactly this reason; this is that same
    // move, available to a caller that has just added something.
    void RaiseToFront();

    void Open();
    void Close();
    bool IsOpen() const { return open_; }

    // Set by GameScreen. RESUME closes the overlay (GameScreen also unpauses
    // movement there); SETTINGS changes to the settings screen; EXIT WORLD
    // sends WorldLeave and returns to world selection.
    std::function<void()> onResume;
    std::function<void()> onSettings;
    std::function<void()> onExitWorld;

private:
    void BuildFrame();

    Engine*    engine_    = nullptr;
    UIManager* uiManager_ = nullptr;

    bool open_ = false;

    // The full-screen dim is the root so it swallows every click; the box is
    // a child of it and follows wherever the dim goes.
    std::shared_ptr<UIPanel> root_;
    std::shared_ptr<UIPanel> box_;
};
