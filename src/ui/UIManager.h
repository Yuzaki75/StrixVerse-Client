#pragma once

#include <vector>
#include <memory>
#include <functional>
#include "UIElement.h"

/**
 * Manages UI elements, input handling, and rendering
 */
class UIManager {
public:
    UIManager();
    ~UIManager();

    // UI element management
    void addElement(std::shared_ptr<UIElement> element);
    void removeElement(std::shared_ptr<UIElement> element);
    void bringToFront(std::shared_ptr<UIElement> element);
    void sendToBack(std::shared_ptr<UIElement> element);
    void clearAllElements();

    // Input handling (to be called from Engine)
    void handleMouseMove(float x, float y);
    void handleMouseDown(float x, float y);
    void handleMouseUp(float x, float y);
    void handleKeyPressed(char key);
    void handleSpecialKeyPressed(int key); // For arrow keys, etc.

    // Update and render
    void update(float deltaTime);
    void render();

    // Focus management
    void setFocusedElement(std::shared_ptr<UIElement> element);
    std::shared_ptr<UIElement> getFocusedElement() const { return focusedElement_; }

    // Get element at position (for hit testing)
    std::shared_ptr<UIElement> getElementAt(float x, float y) const;

private:
    std::vector<std::shared_ptr<UIElement>> elements_; // Ordered by draw order (back to front)
    std::shared_ptr<UIElement> hoveredElement_;
    std::shared_ptr<UIElement> pressedElement_;
    std::shared_ptr<UIElement> focusedElement_;

    // Helper methods
    void updateHoverState(float x, float y);
    void updatePressState(float x, float y);
    void clearHoverState();
    void clearPressState();
};