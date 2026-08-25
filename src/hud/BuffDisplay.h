#pragma once

#include <memory>
#include <string>
#include <vector>

class Engine;
class UIManager;
class UIPanel;
class UILabel;

// -----------------------------------------------------------------------------
// BuffDisplay
//
// A standing HUD element, not a toggle screen: one small row per active buff,
// stacked in a column under the HUD's stat panels.
//
// The server is authoritative about what is active and when it expires. This
// element only animates between packets - Update counts each bar down from
// what the last packet said, so a bar moves smoothly instead of stepping once
// per refresh - and SetBuffs is always the correction that wins.
// -----------------------------------------------------------------------------
class BuffDisplay
{
public:
    struct BuffEntry
    {
        std::string id;
        std::string name;

        float remainingSeconds = 0.0f;
        float totalSeconds     = 0.0f;
    };

    BuffDisplay(Engine* engine, UIManager* uiManager);
    ~BuffDisplay();

    BuffDisplay(const BuffDisplay&) = delete;
    BuffDisplay& operator=(const BuffDisplay&) = delete;

    // Built once against the UIManager, like every other HUD-adjacent overlay;
    // hidden until the first non-empty buff list arrives.
    void Build();

    // Replaces the visible buff set. Rows are rebuilt because the count itself
    // changed; Update never rebuilds, it only resizes bars.
    void SetBuffs(const std::vector<BuffEntry>& buffs);

    void Clear();

    // Client-side countdown display only. dt in seconds.
    void Update(float dt);

private:
    struct Row
    {
        std::shared_ptr<UIPanel> backing;
        std::shared_ptr<UILabel> name;
        std::shared_ptr<UILabel> seconds;
        std::shared_ptr<UIPanel> barBackground;
        std::shared_ptr<UIPanel> barFill;

        // The buff this row was built for. Expiry ghosting matches on it.
        std::string id;

        // The whole-second figure currently shown; the string is only
        // rebuilt when this changes, never per frame.
        int shownSecond = -1;
    };

    // A row whose buff vanished between packets. It keeps drawing while its
    // opacity runs down to nothing, then is removed from the root. Capped so
    // a mass expiry cannot stack an unbounded number of fading panels.
    struct Ghost
    {
        std::shared_ptr<UIPanel> backing;
        float remaining = 0.0f;
    };

    void BuildFrame();

    // Pins the column to the window's top-left, the same edge the health and
    // level panels above it are pinned to. Called on build and whenever the
    // framebuffer changes shape.
    void LayoutForCanvas();

    int laidOutWidth_  = 0;
    int laidOutHeight_ = 0;

    void RebuildRows(bool fadeExpiredRows);
    void ApplyBars() const;
    void UpdateVisibility();
    void FadeGhosts(float dt);

    Engine*    engine_    = nullptr;
    UIManager* uiManager_ = nullptr;

    bool built_ = false;

    std::vector<BuffEntry> buffs_;
    std::vector<Row>       rows_;
    std::vector<Ghost>     ghosts_;

    std::shared_ptr<UIPanel> root_;
};
