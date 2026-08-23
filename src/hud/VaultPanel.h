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
// VaultPanel
//
// Lost Technology interface shell for the Aether Vault - the settlement's item
// storage. A gold-accented science-fantasy card listing what the vault holds,
// one row per resource with the quantity right-aligned in the mono data face,
// and the three actions the vault supports along the bottom.
//
// Follows the WorldManagerPanel lifecycle exactly - built once against the
// UIManager, shown and hidden by visibility, re-inserted on open so it draws
// above anything built after it. What WITHDRAW, DEPOSIT and MANAGE actually do
// is not decided here: GameScreen sets the callbacks, because vault protocol
// belongs to the screen that owns the session.
// -----------------------------------------------------------------------------
class VaultPanel
{
public:
    struct ResourceEntry
    {
        std::string name;
        int         quantity = 0;
    };

    VaultPanel(Engine* engine, UIManager* uiManager);
    ~VaultPanel();

    VaultPanel(const VaultPanel&) = delete;
    VaultPanel& operator=(const VaultPanel&) = delete;

    // Built once and toggled by visibility; rebuilding per open would discard
    // focus and every live callback for no benefit.
    void Build();

    void Open();
    void Close();
    void Toggle();
    bool IsOpen() const { return open_; }

    // Stashes the contents; rendered immediately while open, lazily on the
    // next Open otherwise, so the panel never shows a stale vault.
    void SetResources(const std::vector<ResourceEntry>& resources);

    // Set by GameScreen. The panel only raises the intent.
    std::function<void()> onWithdraw;
    std::function<void()> onDeposit;
    std::function<void()> onManage;

private:
    void BuildFrame();
    void RebuildRows();

    Engine*    engine_    = nullptr;
    UIManager* uiManager_ = nullptr;

    bool open_ = false;

    std::vector<ResourceEntry> resources_;

    std::shared_ptr<UIPanel> root_;
    std::shared_ptr<UIPanel> list_;
    std::shared_ptr<UILabel> emptyLabel_;
};
