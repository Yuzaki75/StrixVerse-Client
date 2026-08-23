#pragma once

#include <functional>
#include <memory>
#include <string>

class Engine;
class UIManager;
class UIPanel;
class UILabel;

// -----------------------------------------------------------------------------
// GatePanel
//
// Lost Technology interface shell for the Aether Gate - a paired portal whose
// status and destination are reported here and whose ACTIVATE control lives
// here. Violet-accented card: dormant gates read as a muted line of text,
// active ones light up green.
//
// Follows the WorldManagerPanel lifecycle exactly - built once against the
// UIManager, shown and hidden by visibility, re-inserted on open so it draws
// above anything built after it. What ACTIVATE does is not decided here:
// GameScreen sets the callback, because gate protocol belongs to the screen
// that owns the session.
// -----------------------------------------------------------------------------
class GatePanel
{
public:
    GatePanel(Engine* engine, UIManager* uiManager);
    ~GatePanel();

    GatePanel(const GatePanel&) = delete;
    GatePanel& operator=(const GatePanel&) = delete;

    // Built once and toggled by visibility; rebuilding per open would discard
    // focus and every live callback for no benefit.
    void Build();

    void Open();
    void Close();
    void Toggle();
    bool IsOpen() const { return open_; }

    // Stash-and-refresh setters, same idiom as PlayerListPanel: applied
    // immediately while open, lazily on the next Open otherwise.
    void SetStatus(bool active);
    void SetDestination(const std::string& destination);

    // Set by GameScreen. The panel only raises the intent.
    std::function<void()> onActivate;

private:
    void BuildFrame();
    void RefreshStatus();

    Engine*    engine_    = nullptr;
    UIManager* uiManager_ = nullptr;

    bool open_ = false;

    bool        active_      = false;
    std::string destination_;

    std::shared_ptr<UIPanel> root_;
    std::shared_ptr<UILabel> statusLabel_;
    std::shared_ptr<UILabel> destinationLabel_;
};
