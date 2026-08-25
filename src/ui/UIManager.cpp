#include "UIManager.h"

#include "../core/Engine.h"
#include "../core/ServiceLocator.h"
#include "../graphics/Font.h"
#include "../graphics/UIRenderer.h"
#include "UIFonts.h"
#include "UILabel.h"
#include "UIPanel.h"
#include "UIScale.h"
#include "UITheme.h"

#include <algorithm>

namespace
{
    // Cursor dwell before the shared tooltip appears.
    constexpr float kTooltipDwellSeconds = 0.4f;

    // Tooltip offset from the cursor, in canvas pixels.
    constexpr float kTooltipOffsetX = UITheme::Scaled(14.0f);
    constexpr float kTooltipOffsetY = UITheme::Scaled(18.0f);

    // Padding around the tooltip label, in canvas pixels.
    constexpr float kTooltipPadX = UITheme::Scaled(8.0f);
    constexpr float kTooltipPadY = UITheme::Scaled(5.0f);
}

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

    // The tooltip is a UIManager-owned element; if someone removed it (or an
    // ancestor of it) the cached pointers must not survive.
    if (tooltipPanel_ && isRemovedOrDescendant(tooltipPanel_))
    {
        tooltipPanel_.reset();
        tooltipLabel_.reset();
        tooltipVisible_ = false;
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

    // The tooltip was in the tree and went with it.
    tooltipPanel_.reset();
    tooltipLabel_.reset();
    tooltipShownText_.clear();
    tooltipVisible_ = false;
    tooltipDwell_   = 0.0f;
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

    // The dwell timer restarts whenever the cursor's target changes; a
    // tooltip shown for the previous target must not survive onto this one.
    hideTooltip();
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

bool UIManager::isPointOverElement(float x, float y)
{
    return getElementAt(x, y) != nullptr;
}

void UIManager::handleMouseDown(float x, float y)
{
    lastMouseX_ = x;
    lastMouseY_ = y;

    updateHoverState(x, y);

    // A click anywhere dismisses the tooltip; it re-arms after the dwell.
    hideTooltip();

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
    // Walk up from the element under the cursor and stop at the first one
    // that consumes the scroll. Before the bool return this broadcast to the
    // entire ancestor chain, so an inner list and the page behind it both
    // moved on one wheel notch.
    auto element = getElementAt(x, y);

    for (UIElement* node = element.get(); node != nullptr; node = node->getParent())
    {
        if (node->onScroll(delta))
            break;
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

    updateTooltip(deltaTime);
}

void UIManager::render(UIRenderer& renderer)
{
    for (const auto& element : elements_)
    {
        if (element && element->isVisible())
            element->render(renderer, 1.0f);
    }
}

bool UIManager::isTextInputFocused() const
{
    return focusedElement_ && focusedElement_->consumesTextInput();
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

void UIManager::ensureTooltipBuilt()
{
    if (tooltipPanel_)
        return;

    Font* caption = nullptr;
    if (auto fonts = ServiceLocator::Get<UIFonts>())
        caption = fonts->Get(UIFonts::Typeface::Body, UITheme::Body::Caption);

    // One shared tooltip for every element, styled per the design: dark
    // surface, thin accent border, chip corners.
    tooltipPanel_ = std::make_shared<UIPanel>();
    tooltipPanel_->setBackgroundColor(UITheme::WithAlpha(UITheme::Panel, 0.92f));
    tooltipPanel_->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.70f), UITheme::BorderThin);
    tooltipPanel_->setBorderRadius(UITheme::RadiusChip);

    tooltipLabel_ = std::make_shared<UILabel>();
    tooltipLabel_->setFont(caption);
    tooltipLabel_->setTextColor(UITheme::Subtext);
    tooltipPanel_->addChild(tooltipLabel_);

    tooltipPanel_->setVisible(false);

    // Added last so it renders above everything already in the tree; it does
    // not want input, so it can never steal a hover or click itself.
    addElement(tooltipPanel_);
}

void UIManager::updateTooltip(float deltaTime)
{
    const bool hasText = hoveredElement_ && !hoveredElement_->getTooltipText().empty();

    if (!hasText)
    {
        hideTooltip();
        return;
    }

    if (tooltipVisible_)
    {
        refreshTooltipPosition();
    }
    else
    {
        tooltipDwell_ += deltaTime;
        if (tooltipDwell_ >= kTooltipDwellSeconds)
            showTooltip();
    }
}

void UIManager::showTooltip()
{
    ensureTooltipBuilt();

    const std::string& text = hoveredElement_->getTooltipText();

    // Rebuild the label only when the text actually changed, so a static
    // tooltip costs no allocations per frame.
    if (text != tooltipShownText_)
    {
        tooltipShownText_ = text;
        tooltipLabel_->setText(text);

        const float textWidth = tooltipLabel_->measureTextWidth();
        const float lineHeight =
            tooltipLabel_->getFont()
                ? tooltipLabel_->getFont()->GetLineHeight()
                : static_cast<float>(UITheme::Body::Caption) * 1.2f;

        tooltipLabel_->setPosition(kTooltipPadX, kTooltipPadY);
        tooltipLabel_->setSize(textWidth, lineHeight);
        tooltipPanel_->setSize(textWidth + kTooltipPadX * 2.0f,
                               lineHeight + kTooltipPadY * 2.0f);
    }

    refreshTooltipPosition();
    bringToFront(tooltipPanel_);
    tooltipPanel_->setVisible(true);
    tooltipVisible_ = true;
}

void UIManager::refreshTooltipPosition()
{
    if (!tooltipPanel_)
        return;

    float x = lastMouseX_ + kTooltipOffsetX;
    float y = lastMouseY_ + kTooltipOffsetY;

    // Clamp inside the visible canvas so an edge tooltip stays readable.
    float left   = 0.0f;
    float top    = 0.0f;
    float right  = UIScale::kDesignWidth;
    float bottom = UIScale::kDesignHeight;

    if (auto engine = ServiceLocator::Get<Engine>())
    {
        const UIScale& scale = engine->GetUIScale();

        left   = scale.GetVisibleLeft();
        top    = scale.GetVisibleTop();
        right  = scale.GetVisibleCanvas().z;
        bottom = scale.GetVisibleCanvas().w;
    }

    x = std::clamp(x, left, std::max(left, right - tooltipPanel_->getWidth()));
    y = std::clamp(y, top, std::max(top, bottom - tooltipPanel_->getHeight()));

    tooltipPanel_->setPosition(x, y);
}

void UIManager::hideTooltip()
{
    tooltipDwell_   = 0.0f;
    tooltipVisible_ = false;

    if (tooltipPanel_)
        tooltipPanel_->setVisible(false);
}
