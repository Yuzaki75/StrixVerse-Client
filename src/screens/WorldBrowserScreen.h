#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Screen.h"
#include "../core/WorldManager.h"

class UIButton;
class UILabel;
class UIPanel;
class UIScrollPanel;
class UITextBox;

/**
 * World Selection screen.
 *
 * Implements the style guide's world browser: the header with live search and
 * All / Recent / Favorites filters, the scrolling world list with per-world
 * population and Join action, the empty-search state, and the footer count.
 *
 * The world list comes from WorldManager and is empty until a server supplies
 * one - the client ships with no worlds of its own. While it is empty, the
 * search field doubles as a world-name field: typing a name and pressing ENTER
 * (or CREATE WORLD) asks the server for that world, which is the only way in
 * until a world-list packet exists.
 */
class WorldBrowserScreen : public Screen
{
public:
    explicit WorldBrowserScreen(Engine* engine);
    ~WorldBrowserScreen() override = default;

    void OnEnter() override;
    void OnKeyDown(int key, bool ctrl, bool shift) override;

private:
    enum class Filter
    {
        All,
        Recent,
        Favorites
    };

    void BuildHeader(float x, float y, float width);
    void BuildFooter(float x, float y, float width);

    // Rebuilds the visible rows from the current query and filter.
    void RebuildList();

    std::shared_ptr<UIPanel> BuildWorldRow(const WorldInfo& world, float width, float y);

    void SetFilter(Filter filter);
    void JoinWorld(const WorldInfo& world);

    // Enters the world named in the search field, reusing the listed world's
    // details when the name matches one.
    void EnterTypedWorld();

    std::vector<WorldInfo> worlds_;

    std::shared_ptr<UIScrollPanel> list_;
    std::shared_ptr<UITextBox>     searchBox_;
    std::shared_ptr<UILabel>       countLabel_;
    std::shared_ptr<UILabel>       selectionLabel_;

    std::shared_ptr<UIButton> allButton_;
    std::shared_ptr<UIButton> recentButton_;
    std::shared_ptr<UIButton> favoritesButton_;
    std::shared_ptr<UIButton> createButton_;

    std::string query_;
    Filter      filter_ = Filter::All;

    std::string selectedWorld_;
    bool        joining_ = false;
};
