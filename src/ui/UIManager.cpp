#include "UIManager.h"
#include "../graphics/SpriteBatch.h"
#include "../graphics/Font.h"
#include "../core/ServiceLocator.h"
#include "../core/Logger.h"

UIManager::UIManager()
    : hoveredElement_(nullptr)
    , pressedElement_(nullptr)
    , focusedElement_(nullptr) {}

UIManager::~UIManager() = default;

void UIManager::addElement(std::shared_ptr<UIElement> element) {
    elements_.push_back(element);
}

void UIManager::removeElement(std::shared_ptr<UIElement> element) {
    auto it = std::find(elements_.begin(), elements_.end(), element);
    if (it != elements_.end()) {
        elements_.erase(it);

        // Clear references if they point to the removed element
        if (hoveredElement_ == element) hoveredElement_ = nullptr;
        if (pressedElement_ == element) pressedElement_ = nullptr;
        if (focusedElement_ == element) focusedElement_ = nullptr;
    }
}

void UIManager::bringToFront(std::shared_ptr<UIElement> element) {
    auto it = std::find(elements_.begin(), elements_.end(), element);
    if (it != elements_.end()) {
        elements_.erase(it);
        elements_.push_back(element); // Move to end (drawn last = on top)
    }
}

void UIManager::sendToBack(std::shared_ptr<UIElement> element) {
    auto it = std::find(elements_.begin(), elements_.end(), element);
    if (it != elements_.end()) {
        elements_.erase(it);
        elements_.insert(elements_.begin(), element); // Move to beginning (drawn first = in back)
    }
}

void UIManager::clearAllElements() {
    elements_.clear();
    hoveredElement_ = nullptr;
    pressedElement_ = nullptr;
    focusedElement_ = nullptr;
}

std::shared_ptr<UIElement> UIManager::getElementAt(float x, float y) const {
    // Check from front to back (last to first) to find topmost element
    for (auto it = elements_.rbegin(); it != elements_.rend(); ++it) {
        if ((*it)->isVisible() && (*it)->containsPoint(x, y)) {
            return *it;
        }
    }
    return nullptr;
}

void UIManager::updateHoverState(float x, float y) {
    auto element = getElementAt(x, y);
    if (element != hoveredElement_) {
        // Mouse left the previously hovered element
        if (hoveredElement_) {
            hoveredElement_->onMouseLeave();
        }

        // Mouse entered the new element
        if (element) {
            element->onMouseEnter();
        }

        hoveredElement_ = element;
    }
}

void UIManager::updatePressState(float x, float y) {
    auto element = getElementAt(x, y);
    if (element != pressedElement_) {
        // Mouse button released on different element or released entirely
        if (pressedElement_) {
            pressedElement_->onMouseUp(x, y);
            // Check if it's still the same element for click
            if (element == pressedElement_) {
                // This was a click
                element->onClick();
            }
        }

        // Mouse button pressed on new element
        if (element) {
            element->onMouseDown(x, y);
        }

        pressedElement_ = element;
    }
}

void UIManager::clearHoverState() {
    if (hoveredElement_) {
        hoveredElement_->onMouseLeave();
        hoveredElement_ = nullptr;
    }
}

void UIManager::clearPressState() {
    if (pressedElement_) {
        pressedElement_->onMouseUp(0, 0); // Coordinates don't matter for release
        pressedElement_ = nullptr;
    }
}

void UIManager::handleMouseMove(float x, float y) {
    updateHoverState(x, y);
    // Update press state if mouse button is held down
    // This would need to track mouse button state - simplified here
}

void UIManager::handleMouseDown(float x, float y) {
    updatePressState(x, y);
    // Set focus to clicked element
    auto element = getElementAt(x, y);
    if (element) {
        setFocusedElement(element);
    }
}

void UIManager::handleMouseUp(float x, float y) {
    updatePressState(x, y);
    clearPressState();
}

void UIManager::handleKeyPressed(char key) {
    if (focusedElement_) {
        // For simplicity, we're passing all key presses to the focused element
        // In a real implementation, we'd distinguish between printable chars and special keys
        focusedElement_->onKeyPressed(key);
    }
}

void UIManager::handleSpecialKeyPressed(int key) {
    if (focusedElement_) {
        focusedElement_->onSpecialKeyPressed(key);
    }
}

void UIManager::update(float deltaTime) {
    // Update all elements
    for (auto& element : elements_) {
        if (element->isVisible()) {
            element->update(deltaTime);
        }
    }
}

void UIManager::render() {
    // Get services
    auto spriteBatch = ServiceLocator::Get<SpriteBatch>();
    auto font = ServiceLocator::Get<Font>();

    if (!spriteBatch || !font) {
        return;
    }

    // Render all elements in order (back to front)
    for (auto& element : elements_) {
        if (element->isVisible()) {
            element->render(*spriteBatch, *font);
        }
    }
}

void UIManager::setFocusedElement(std::shared_ptr<UIElement> element) {
    if (focusedElement_ != element) {
        // Remove focus from previous element
        if (focusedElement_) {
            focusedElement_->onFocusLost();
        }

        // Set focus to new element
        focusedElement_ = element;

        // Give focus to new element
        if (focusedElement_) {
            focusedElement_->onFocusGained();
        }
    }
}