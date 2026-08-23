#pragma once

#include <functional>
#include <memory>

class Engine;
class UIManager;
class UIPanel;
class UILabel;

// -----------------------------------------------------------------------------
// StabilizerPanel
//
// Lost Technology interface shell for the Aether Stabilizer - the device that
// holds a region's aetheric field steady. Blue-accented card showing the
// current stability as a crystal-gradient bar, how many stabiliser upgrades
// are installed, and the STABILIZE control.
//
// Follows the WorldManagerPanel lifecycle exactly - built once against the
// UIManager, shown and hidden by visibility, re-inserted on open so it draws
// above anything built after it. What STABILIZE does is not decided here:
// GameScreen sets the callback, because stabiliser protocol belongs to the
// screen that owns the session.
// -----------------------------------------------------------------------------
class StabilizerPanel
{
public:
    StabilizerPanel(Engine* engine, UIManager* uiManager);
    ~StabilizerPanel();

    StabilizerPanel(const StabilizerPanel&) = delete;
    StabilizerPanel& operator=(const StabilizerPanel&) = delete;

    // Built once and toggled by visibility; rebuilding per open would discard
    // focus and every live callback for no benefit.
    void Build();

    void Open();
    void Close();
    void Toggle();
    bool IsOpen() const { return open_; }

    // Stash-and-refresh setters, same idiom as PlayerListPanel: applied
    // immediately while open, lazily on the next Open otherwise.
    void SetStability(float v01);
    void SetUpgrades(int current, int max);

    // Set by GameScreen. The panel only raises the intent.
    std::function<void()> onStabilize;

private:
    void BuildFrame();
    void ApplyBar();
    void RefreshLabels();

    Engine*    engine_    = nullptr;
    UIManager* uiManager_ = nullptr;

    bool open_ = false;

    float stability01_ = 0.0f;
    int   upgradesCurrent_ = 0;
    int   upgradesMax_     = 0;

    std::shared_ptr<UIPanel> root_;
    std::shared_ptr<UIPanel> barFill_;
    std::shared_ptr<UILabel> upgradesLabel_;
};
