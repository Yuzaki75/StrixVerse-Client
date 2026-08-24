#pragma once

#include <memory>
#include <cstdint>
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
    void OnResize() override;
    void OnKeyDown(int key, bool ctrl, bool shift) override;
    void Update(float deltaTime) override;

private:
    enum class Filter
    {
        All,
        Recent,
        Favorites
    };

    // Everything between CreateRoot and the first RebuildList. Split out of
    // OnEnter so a resize can run it again without repeating the audio, the
    // network request and the state reset that OnEnter also does.
    void BuildUI();

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

    // Last catalogue revision drawn, so the list is rebuilt when the server's
    // reply lands rather than every frame.
    uint32_t worldsRevision_ = 0;

    std::shared_ptr<UIScrollPanel> list_;
    std::shared_ptr<UITextBox>     searchBox_;
    std::shared_ptr<UILabel>       countLabel_;
    std::shared_ptr<UILabel>       selectionLabel_;

    std::shared_ptr<UIButton> allButton_;
    std::shared_ptr<UIButton> recentButton_;
    std::shared_ptr<UIButton> favoritesButton_;
    std::shared_ptr<UIButton> createButton_;
    std::shared_ptr<UIButton> clearSearchButton_;

    std::string query_;
    Filter      filter_ = Filter::All;

    std::string selectedWorld_;
    bool        joining_ = false;
};
