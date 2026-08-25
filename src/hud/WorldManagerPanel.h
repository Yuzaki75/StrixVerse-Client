#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../networking/NetworkManager.h"

class Engine;
class UIManager;
class UIPanel;
class UILabel;
class UIButton;
class UICheckBox;
class UITextBox;
class UIScrollPanel;

// The wrench panel: what a Strix Core opens onto.
//
// Two tabs over one frame. World Manager is identity and people - who owns this
// world, what the Core is, who belongs to it and as what. Lock Manager is the
// Core's protection: the toggles that decide what someone who is *not* on that
// list may do, and the ban list for the people who may do nothing at all.
//
// The panel decides nothing. Every control it shows, and whether it shows it at
// all, comes from the permission flags the server put in WorldInfo; every
// button sends a request that the server re-checks on arrival. Hiding a control
// is a courtesy to the player, not a security boundary - a client that hides a
// button has not been prevented from sending the packet.
class WorldManagerPanel
{
public:
    WorldManagerPanel(Engine* engine, UIManager* uiManager);
    ~WorldManagerPanel();

    WorldManagerPanel(const WorldManagerPanel&) = delete;
    WorldManagerPanel& operator=(const WorldManagerPanel&) = delete;

    // Built once and shown, never rebuilt per open. Creating it per open is the
    // mistake the pause overlay was rewritten to avoid: it threw away focus,
    // scroll position and every live callback on each toggle.
    void Build();

    // See PauseOverlay::RaiseToFront.
    void RaiseToFront();

    void Show();
    void Hide();
    bool IsOpen() const { return open_; }

    // Repopulates from NetworkManager. Called when the world-info or roster
    // revision changes, matching how the HUD refreshes from stats and
    // inventory rather than rebuilding every frame.
    void Refresh();

private:
    enum class Tab { World, Lock };

    void BuildFrame();
    void BuildWorldTab();
    void BuildLockTab();

    void SelectTab(Tab tab);
    void RefreshWorldTab();
    void RefreshLockTab();

    // One member row: name, role, and the buttons that act on it. Rebuilt on
    // refresh because the list itself is what changed.
    void AddMemberRow(const NetworkManager::WorldRosterEntry& entry, bool canManage);
    void AddBanRow(const NetworkManager::WorldRosterEntry& entry, bool canManage);

    // Reads every checkbox and sends them together. The server writes the four
    // settings in one statement, so sending them one at a time would let a
    // half-applied state exist between packets.
    void SendSettings();

    Engine*    engine_    = nullptr;
    UIManager* uiManager_ = nullptr;

    bool open_ = false;
    Tab  tab_  = Tab::World;

    // Revisions last drawn, so Refresh is a no-op until something changes.
    uint32_t infoRevision_    = 0;
    uint32_t membersRevision_ = 0;

    std::shared_ptr<UIPanel>  root_;
    std::shared_ptr<UILabel>  title_;
    std::shared_ptr<UIButton> worldTabButton_;
    std::shared_ptr<UIButton> lockTabButton_;
    std::shared_ptr<UIButton> closeButton_;

    // --- World tab ---------------------------------------------------------
    std::shared_ptr<UIPanel>      worldTab_;
    std::shared_ptr<UILabel>      ownerLabel_;
    std::shared_ptr<UILabel>      levelLabel_;
    std::shared_ptr<UILabel>      memberCountLabel_;
    std::shared_ptr<UIScrollPanel> memberList_;
    std::shared_ptr<UITextBox>    inviteField_;
    std::shared_ptr<UIButton>     inviteButton_;
    std::shared_ptr<UILabel>      inviteHint_;

    // --- Lock tab ----------------------------------------------------------
    std::shared_ptr<UIPanel>      lockTab_;
    std::shared_ptr<UICheckBox>   protectionBox_;
    std::shared_ptr<UICheckBox>   buildingBox_;
    std::shared_ptr<UICheckBox>   breakingBox_;
    std::shared_ptr<UICheckBox>   visitorsBox_;
    std::shared_ptr<UIScrollPanel> banList_;
    std::shared_ptr<UITextBox>    banField_;
    std::shared_ptr<UIButton>     banDurationButton_;
    std::shared_ptr<UIButton>     banButton_;
    int                           banDurationIndex_ = 0;

    // Set while Refresh is writing the checkboxes, so setChecked does not fire
    // setOnChanged and bounce a settings packet straight back at the server for
    // a change the server just told us about.
    bool applyingSettings_ = false;
};
