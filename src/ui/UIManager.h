#pragma once

#include <memory>
#include <string>
#include <vector>

#include "UIElement.h"

class UIRenderer;
class UILabel;
class UIPanel;

// -----------------------------------------------------------------------------
// UIManager
//
// Owns the root-level UI elements for the active screen and routes input to
// them.
//
// All coordinates arriving here are already in the 1920x1080 design canvas -
// the Engine converts raw window pixels through UIScale first - so hit testing
// always agrees with what was drawn.
//
// Hit testing descends the whole tree, so controls nested inside panels are
// reachable, and a press followed by a release over the same element produces
// exactly one click.
// -----------------------------------------------------------------------------
class UIManager
{
public:
    UIManager();
    ~UIManager();

    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    // --- Element management ----------------------------------------------
    void addElement(std::shared_ptr<UIElement> element);
    void removeElement(const std::shared_ptr<UIElement>& element);
    void bringToFront(const std::shared_ptr<UIElement>& element);
    void sendToBack(const std::shared_ptr<UIElement>& element);

    // Removes every element and drops hover/press/focus with them, so no
    // state leaks across a screen change.
    void clearAllElements();

    // True when a visible UI element sits under this canvas point.
    //
    // Clicks used to reach both the UI and the screen, so selecting a hotbar
    // slot also swung the tool at whatever tile happened to be behind the HUD.
    //
    // "Over an element" means over something that answers wantsInput(), which
    // is buttons and anything given setBlocksInput(true) - not every panel.
    // A bare UIPanel is scenery and deliberately lets the click through, so a
    // decorative frame drawn over the world does not create a dead zone in it.
    bool isPointOverElement(float x, float y);

    // --- Input -----------------------------------------------------------
    void handleMouseMove(float x, float y);
    void handleMouseDown(float x, float y);
    void handleMouseUp(float x, float y);
    void handleScroll(float x, float y, float delta);
    void handleTextInput(const std::string& utf8);
    void handleKeyDown(int key, bool ctrl, bool shift);

    // --- Frame -----------------------------------------------------------
    void update(float deltaTime);
    void render(UIRenderer& renderer);

    // --- Focus -----------------------------------------------------------
    void setFocusedElement(const std::shared_ptr<UIElement>& element);
    std::shared_ptr<UIElement> getFocusedElement() const { return focusedElement_; }

    // True when a focused element is taking the keyboard - a text field with a
    // caret in it. Gameplay gates on this rather than on focus itself, because
    // a focused button still leaves the player free to move and build.
    bool isTextInputFocused() const;
    void clearFocus();

    // Moves focus along the visual order; wraps at either end.
    void focusNext(bool backwards);

    // True when a focusable element currently holds focus, so the Engine knows
    // whether the platform text input service should be running.
    bool isTextInputActive() const;

    // Topmost element wanting input at the given canvas position.
    std::shared_ptr<UIElement> getElementAt(float x, float y);

private:
    void updateHoverState(float x, float y);

    // --- Shared tooltip --------------------------------------------------
    // One tooltip for the whole tree: an element with tooltip text under a
    // dwelling cursor shows it; everything else keeps it hidden. Built once,
    // lazily, and kept at the top of the draw order.
    void ensureTooltipBuilt();
    void updateTooltip(float deltaTime);
    void showTooltip();
    void refreshTooltipPosition();
    void hideTooltip();

    // Drawn back to front; the last element is on top.
    std::vector<std::shared_ptr<UIElement>> elements_;

    std::shared_ptr<UIElement> hoveredElement_;
    std::shared_ptr<UIElement> pressedElement_;
    std::shared_ptr<UIElement> focusedElement_;

    float lastMouseX_ = 0.0f;
    float lastMouseY_ = 0.0f;

    std::shared_ptr<UIPanel> tooltipPanel_;
    std::shared_ptr<UILabel> tooltipLabel_;
    bool        tooltipVisible_   = false;
    float       tooltipDwell_     = 0.0f;
    std::string tooltipShownText_;
};
