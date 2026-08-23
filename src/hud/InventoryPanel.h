#pragma once

#include <memory>
#include <string>
#include <vector>

class Engine;
class UIManager;
class UIPanel;

// -----------------------------------------------------------------------------
// InventoryPanel
//
// The full-inventory overlay: a grid of slots with quantities and a selected
// highlight, mirroring the HUD hotbar's look at a larger size.
//
// Follows the WorldManagerPanel lifecycle exactly - built once against the
// UIManager, shown and hidden by visibility, repopulated from data handed in.
// The panel decides nothing about what the player carries; SetSlots is the
// whole truth it displays, and the server remains authoritative.
// -----------------------------------------------------------------------------
class InventoryPanel
{
public:
    struct Slot
    {
        std::string itemId;
        std::string name;
        int         quantity = 0;
        std::string iconPath;
        bool        selected = false;
    };

    InventoryPanel(Engine* engine, UIManager* uiManager);
    ~InventoryPanel();

    InventoryPanel(const InventoryPanel&) = delete;
    InventoryPanel& operator=(const InventoryPanel&) = delete;

    // Built once and toggled by visibility; rebuilding per open would discard
    // layout state for no benefit.
    void Build();

    // See PauseOverlay::RaiseToFront.
    void RaiseToFront();

    void Open();
    void Close();
    void Toggle();
    bool IsOpen() const { return open_; }

    // Replaces the slot list. Rows are rebuilt only while open; changes made
    // while closed are applied on the next Open.
    void SetSlots(const std::vector<Slot>& slots);

    // Moves the selection highlight without rebuilding every row's content.
    // -1 clears it.
    void SetSelectedIndex(int index);

private:
    void BuildFrame();
    void RebuildGrid();

    Engine*    engine_    = nullptr;
    UIManager* uiManager_ = nullptr;

    bool open_ = false;

    std::vector<Slot> slots_;
    int               selectedIndex_ = -1;

    std::shared_ptr<UIPanel> root_;
    std::shared_ptr<UIPanel> grid_;
};
