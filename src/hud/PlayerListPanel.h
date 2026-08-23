#pragma once

#include <memory>
#include <string>
#include <vector>

class Engine;
class UIManager;
class UIPanel;
class UILabel;

// -----------------------------------------------------------------------------
// PlayerListPanel
//
// The scoreboard overlay: everyone currently connected, one row per player with
// their name coloured by role.
//
// Follows the WorldManagerPanel lifecycle exactly - built once against the
// UIManager, shown and hidden by visibility, repopulated from data handed in
// rather than rebuilt per frame. The panel decides nothing about who is
// connected; SetPlayers is the whole truth it displays.
// -----------------------------------------------------------------------------
class PlayerListPanel
{
public:
    struct Entry
    {
        std::string name;
        std::string role;   // "Developer", "Moderator" or "Player".
    };

    PlayerListPanel(Engine* engine, UIManager* uiManager);
    ~PlayerListPanel();

    PlayerListPanel(const PlayerListPanel&) = delete;
    PlayerListPanel& operator=(const PlayerListPanel&) = delete;

    // Built once and toggled by visibility, matching WorldManagerPanel: a
    // rebuild per open would throw away layout state for no benefit.
    void Build();

    // See PauseOverlay::RaiseToFront.
    void RaiseToFront();

    void Open();
    void Close();
    void Toggle();
    bool IsOpen() const { return open_; }

    // Replaces the roster. Rows are rebuilt immediately while open; changes
    // made while closed are applied on the next Open.
    void SetPlayers(const std::vector<Entry>& players);

private:
    void BuildFrame();
    void RebuildRows();

    // Resolves a role string to its display colour. Unknown roles read as
    // plain players rather than as an error.
    static const class Color& RoleColor(const std::string& role);

    Engine*    engine_    = nullptr;
    UIManager* uiManager_ = nullptr;

    bool open_ = false;

    std::vector<Entry> players_;

    std::shared_ptr<UIPanel> root_;
    std::shared_ptr<UIPanel> list_;
};
