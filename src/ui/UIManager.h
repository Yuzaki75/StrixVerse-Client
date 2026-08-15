#pragma once

#include <memory>
#include <string>
#include <vector>

#include "UIElement.h"

class UIRenderer;

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

    // Drawn back to front; the last element is on top.
    std::vector<std::shared_ptr<UIElement>> elements_;

    std::shared_ptr<UIElement> hoveredElement_;
    std::shared_ptr<UIElement> pressedElement_;
    std::shared_ptr<UIElement> focusedElement_;

    float lastMouseX_ = 0.0f;
    float lastMouseY_ = 0.0f;
};
