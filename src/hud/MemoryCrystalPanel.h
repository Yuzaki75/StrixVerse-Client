#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

class Engine;
class UIManager;
class UIPanel;
class UILabel;

// -----------------------------------------------------------------------------
// MemoryCrystalPanel
//
// Lost Technology interface shell for the Memory Crystal - a recovered device
// whose stored log lines scroll past as readable history. Cyan-teal accented
// card: up to a fixed number of the most recent entries are shown (no
// scrollbar yet), with EXTRACT and CLOSE along the bottom.
//
// Follows the WorldManagerPanel lifecycle exactly - built once against the
// UIManager, shown and hidden by visibility, re-inserted on open so it draws
// above anything built after it. What EXTRACT does is not decided here:
// GameScreen sets the callback, because crystal protocol belongs to the screen
// that owns the session.
// -----------------------------------------------------------------------------
class MemoryCrystalPanel
{
public:
    MemoryCrystalPanel(Engine* engine, UIManager* uiManager);
    ~MemoryCrystalPanel();

    MemoryCrystalPanel(const MemoryCrystalPanel&) = delete;
    MemoryCrystalPanel& operator=(const MemoryCrystalPanel&) = delete;

    // Built once and toggled by visibility; rebuilding per open would discard
    // focus and every live callback for no benefit.
    void Build();

    void Open();
    void Close();
    void Toggle();
    bool IsOpen() const { return open_; }

    // Stash-and-refresh setter, same idiom as PlayerListPanel: applied
    // immediately while open, lazily on the next Open otherwise.
    void SetEntries(const std::vector<std::string>& lines);

    // Set by GameScreen. The panel only raises the intent; CLOSE is handled
    // internally by Close().
    std::function<void()> onExtract;

private:
    void BuildFrame();
    void RebuildRows();

    Engine*    engine_    = nullptr;
    UIManager* uiManager_ = nullptr;

    bool open_ = false;

    std::vector<std::string> entries_;

    std::shared_ptr<UIPanel> root_;
    std::shared_ptr<UIPanel> list_;
};
