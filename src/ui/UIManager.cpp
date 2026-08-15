#include "UIManager.h"

#include "../graphics/UIRenderer.h"

#include <algorithm>

UIManager::UIManager() = default;

UIManager::~UIManager() = default;

void UIManager::addElement(std::shared_ptr<UIElement> element)
{
    if (!element)
        return;

    elements_.push_back(std::move(element));
}

void UIManager::removeElement(const std::shared_ptr<UIElement>& element)
{
    if (!element)
        return;

    const auto it = std::find(elements_.begin(), elements_.end(), element);
    if (it == elements_.end())
        return;

    elements_.erase(it);

    // Interaction state must never outlive the element it points at. The
    // removed element may be an ancestor of the hovered/pressed/focused one,
    // so check the whole chain.
    auto isRemovedOrDescendant = [&element](const std::shared_ptr<UIElement>& candidate)
    {
        for (const UIElement* node = candidate.get(); node != nullptr; node = node->getParent())
        {
            if (node == element.get())
                return true;
        }
        return false;
    };

    if (hoveredElement_ && isRemovedOrDescendant(hoveredElement_))
        hoveredElement_.reset();

    if (pressedElement_ && isRemovedOrDescendant(pressedElement_))
        pressedElement_.reset();

    if (focusedElement_ && isRemovedOrDescendant(focusedElement_))
    {
        focusedElement_->onFocusLost();
        focusedElement_.reset();
    }
}

void UIManager::bringToFront(const std::shared_ptr<UIElement>& element)
{
    const auto it = std::find(elements_.begin(), elements_.end(), element);
    if (it != elements_.end())
    {
        auto moved = *it;
        elements_.erase(it);
        elements_.push_back(std::move(moved));
    }
}

void UIManager::sendToBack(const std::shared_ptr<UIElement>& element)
{
    const auto it = std::find(elements_.begin(), elements_.end(), element);
    if (it != elements_.end())
    {
        auto moved = *it;
        elements_.erase(it);
        elements_.insert(elements_.begin(), std::move(moved));
    }
}

void UIManager::clearAllElements()
{
    if (focusedElement_)
        focusedElement_->onFocusLost();

    elements_.clear();
    hoveredElement_.reset();
    pressedElement_.reset();
    focusedElement_.reset();
}

std::shared_ptr<UIElement> UIManager::getElementAt(float x, float y)
{
    // Front to back: the last element added is drawn on top.
    for (auto it = elements_.rbegin(); it != elements_.rend(); ++it)
    {
        if (!*it || !(*it)->isVisible())
            continue;

        if (auto hit = (*it)->hitTest(x, y))
            return hit;
    }

    return nullptr;
}

void UIManager::updateHoverState(float x, float y)
{
    auto element = getElementAt(x, y);

    if (element == hoveredElement_)
        return;

    if (hoveredElement_)
        hoveredElement_->onMouseLeave();

    hoveredElement_ = std::move(element);

    if (hoveredElement_)
        hoveredElement_->onMouseEnter();
}

void UIManager::handleMouseMove(float x, float y)
{
    lastMouseX_ = x;
    lastMouseY_ = y;

    updateHoverState(x, y);

    // A held button keeps receiving movement so it can track drag-out.
    if (pressedElement_)
        pressedElement_->onMouseMove(x, y);
    else if (hoveredElement_)
        hoveredElement_->onMouseMove(x, y);
}

void UIManager::handleMouseDown(float x, float y)
{
    lastMouseX_ = x;
    lastMouseY_ = y;

    updateHoverState(x, y);

    auto element = getElementAt(x, y);

    pressedElement_ = element;

    if (element)
        element->onMouseDown(x, y);

    // Clicking a focusable control focuses it; clicking anywhere else - a
    // panel, the background - drops focus, which is what lets a text box
    // release the caret.
    if (element && element->isFocusable())
        setFocusedElement(element);
    else
        clearFocus();
}

void UIManager::handleMouseUp(float x, float y)
{
    lastMouseX_ = x;
    lastMouseY_ = y;

    auto released = getElementAt(x, y);

    if (pressedElement_)
    {
        pressedElement_->onMouseUp(x, y);

        // A click is a press and a release over the same element.
        if (released == pressedElement_ && pressedElement_->isEnabled())
            pressedElement_->onClick();
    }

    pressedElement_.reset();

    updateHoverState(x, y);
}

void UIManager::handleScroll(float x, float y, float delta)
{
    // Walk up from the element under the cursor until something consumes it,
    // so a row inside a scroll panel still scrolls the panel.
    auto element = getElementAt(x, y);

    for (UIElement* node = element.get(); node != nullptr; node = node->getParent())
    {
        node->onScroll(delta);
    }
}

void UIManager::handleTextInput(const std::string& utf8)
{
    if (focusedElement_)
        focusedElement_->onTextInput(utf8);
}

void UIManager::handleKeyDown(int key, bool ctrl, bool shift)
{
    // Tab moves focus regardless of what holds it.
    if (key == UIKey::Tab)
    {
        focusNext(shift);
        return;
    }

    // Escape leaves the focused field. The screen's own Escape handling only
    // runs once nothing has focus, so the first press abandons what is being
    // typed and the second backs out of the screen.
    if (key == UIKey::Escape && focusedElement_)
    {
        clearFocus();
        return;
    }

    if (focusedElement_)
        focusedElement_->onKeyDown(key, ctrl, shift);
}

void UIManager::update(float deltaTime)
{
    // Iterate over a copy: a callback fired during update may add or remove
    // elements (for example a screen swapping its content).
    const auto snapshot = elements_;

    for (const auto& element : snapshot)
    {
        if (element && element->isVisible())
            element->update(deltaTime);
    }
}

void UIManager::render(UIRenderer& renderer)
{
    for (const auto& element : elements_)
    {
        if (element && element->isVisible())
            element->render(renderer, 1.0f);
    }
}

void UIManager::setFocusedElement(const std::shared_ptr<UIElement>& element)
{
    if (focusedElement_ == element)
        return;

    if (focusedElement_)
        focusedElement_->onFocusLost();

    focusedElement_ = element;

    if (focusedElement_)
        focusedElement_->onFocusGained();
}

void UIManager::clearFocus()
{
    setFocusedElement(nullptr);
}

void UIManager::focusNext(bool backwards)
{
    std::vector<std::shared_ptr<UIElement>> focusable;

    for (const auto& element : elements_)
    {
        if (element)
            element->collectFocusable(focusable);
    }

    if (focusable.empty())
        return;

    size_t next = backwards ? focusable.size() - 1 : 0;

    if (focusedElement_)
    {
        const auto it = std::find(focusable.begin(), focusable.end(), focusedElement_);
        if (it != focusable.end())
        {
            const size_t current = static_cast<size_t>(std::distance(focusable.begin(), it));
            next = backwards
                       ? (current + focusable.size() - 1) % focusable.size()
                       : (current + 1) % focusable.size();
        }
    }

    setFocusedElement(focusable[next]);
}

bool UIManager::isTextInputActive() const
{
    return focusedElement_ != nullptr && focusedElement_->isFocusable();
}
