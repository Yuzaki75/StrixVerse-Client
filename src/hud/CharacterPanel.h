#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

class Engine;
class UIManager;
class UIPanel;
class UILabel;

// -----------------------------------------------------------------------------
// CharacterPanel
//
// The inspect overlay: who the player is, and the stats the server says they
// have. Nothing here is invented client-side; an absent stat simply has no row.
//
// Follows the WorldManagerPanel lifecycle exactly - built once against the
// UIManager, shown and hidden by visibility, repopulated from data handed in.
// -----------------------------------------------------------------------------
class CharacterPanel
{
public:
    struct CharacterInfo
    {
        std::string name;
        std::string role;

        int level = 1;

        // Rendered as label rows in map order (Health, Defense, Speed, ...).
        // std::map keeps a stable ordering between packets, so rows do not
        // shuffle when only one value changed.
        std::map<std::string, int> stats;
    };

    CharacterPanel(Engine* engine, UIManager* uiManager);
    ~CharacterPanel();

    CharacterPanel(const CharacterPanel&) = delete;
    CharacterPanel& operator=(const CharacterPanel&) = delete;

    // Built once and toggled by visibility; rebuilding per open would discard
    // layout state for no benefit.
    void Build();

    // See PauseOverlay::RaiseToFront.
    void RaiseToFront();

    void Open();
    void Close();
    void Toggle();
    bool IsOpen() const { return open_; }

    // Replaces the displayed character. Rows are rebuilt only while open;
    // changes made while closed are applied on the next Open.
    void SetCharacter(const CharacterInfo& info);

private:
    void BuildFrame();
    void Refresh();

    Engine*    engine_    = nullptr;
    UIManager* uiManager_ = nullptr;

    bool open_ = false;

    CharacterInfo info_;

    std::shared_ptr<UIPanel> root_;

    // Header labels are stable members; the stat rows are rebuilt per refresh
    // because their count follows the server's data.
    std::shared_ptr<UILabel> nameLabel_;
    std::shared_ptr<UILabel> roleLabel_;
    std::shared_ptr<UILabel> levelLabel_;

    std::vector<std::shared_ptr<UILabel>> statRows_;
};
