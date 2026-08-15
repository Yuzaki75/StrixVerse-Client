#include "UIElement.h"

#include "../graphics/UIRenderer.h"

#include <algorithm>

UIElement::UIElement() = default;

UIElement::~UIElement() = default;

void UIElement::setPosition(float x, float y)
{
    x_ = x;
    y_ = y;
}

void UIElement::setSize(float width, float height)
{
    width_  = width;
    height_ = height;
}

void UIElement::setBounds(float x, float y, float width, float height)
{
    setPosition(x, y);
    setSize(width, height);
}

void UIElement::setAnchor(AnchorPoint anchor)
{
    anchor_ = anchor;
}

void UIElement::setOpacity(float opacity)
{
    opacity_ = std::clamp(opacity, 0.0f, 1.0f);
}

void UIElement::calculateAnchorOffset(float& offsetX, float& offsetY) const
{
    offsetX = 0.0f;
    offsetY = 0.0f;

    switch (anchor_)
    {
    case AnchorPoint::TopLeft:
        break;
    case AnchorPoint::TopCenter:
        offsetX = -width_ * 0.5f;
        break;
    case AnchorPoint::TopRight:
        offsetX = -width_;
        break;
    case AnchorPoint::MiddleLeft:
        offsetY = -height_ * 0.5f;
        break;
    case AnchorPoint::Center:
        offsetX = -width_ * 0.5f;
        offsetY = -height_ * 0.5f;
        break;
    case AnchorPoint::MiddleRight:
        offsetX = -width_;
        offsetY = -height_ * 0.5f;
        break;
    case AnchorPoint::BottomLeft:
        offsetY = -height_;
        break;
    case AnchorPoint::BottomCenter:
        offsetX = -width_ * 0.5f;
        offsetY = -height_;
        break;
    case AnchorPoint::BottomRight:
        offsetX = -width_;
        offsetY = -height_;
        break;
    }
}

float UIElement::getAbsoluteX() const
{
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    calculateAnchorOffset(offsetX, offsetY);

    float absolute = x_ + offsetX;

    // Walk up the tree; each ancestor contributes its own anchored origin.
    for (const UIElement* ancestor = parent_; ancestor != nullptr; ancestor = ancestor->parent_)
    {
        float ancestorOffsetX = 0.0f;
        float ancestorOffsetY = 0.0f;
        ancestor->calculateAnchorOffset(ancestorOffsetX, ancestorOffsetY);
        absolute += ancestor->x_ + ancestorOffsetX;
    }

    return absolute;
}

float UIElement::getAbsoluteY() const
{
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    calculateAnchorOffset(offsetX, offsetY);

    float absolute = y_ + offsetY;

    for (const UIElement* ancestor = parent_; ancestor != nullptr; ancestor = ancestor->parent_)
    {
        float ancestorOffsetX = 0.0f;
        float ancestorOffsetY = 0.0f;
        ancestor->calculateAnchorOffset(ancestorOffsetX, ancestorOffsetY);
        absolute += ancestor->y_ + ancestorOffsetY;
    }

    return absolute;
}

void UIElement::addChild(std::shared_ptr<UIElement> child)
{
    if (!child || child.get() == this)
        return;

    // Detach from any previous parent so a child is never in two trees.
    if (child->parent_ && child->parent_ != this)
        child->parent_->removeChild(child);

    child->parent_ = this;
    children_.push_back(std::move(child));
}

void UIElement::removeChild(const std::shared_ptr<UIElement>& child)
{
    if (!child)
        return;

    const auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end())
    {
        (*it)->parent_ = nullptr;
        children_.erase(it);
    }
}

void UIElement::clearChildren()
{
    for (auto& child : children_)
    {
        if (child)
            child->parent_ = nullptr;
    }
    children_.clear();
}

bool UIElement::containsPoint(float x, float y) const
{
    const float left = getAbsoluteX();
    const float top  = getAbsoluteY();

    return x >= left && x <= left + width_ &&
           y >= top  && y <= top + height_;
}

std::shared_ptr<UIElement> UIElement::hitTest(float x, float y)
{
    if (!visible_)
        return nullptr;

    // Children are drawn in order, so the last one is on top and must be
    // tested first.
    for (auto it = children_.rbegin(); it != children_.rend(); ++it)
    {
        if (!*it)
            continue;

        if (auto hit = (*it)->hitTest(x, y))
            return hit;
    }

    if (wantsInput() && enabled_ && containsPoint(x, y))
        return shared_from_this();

    return nullptr;
}

void UIElement::collectFocusable(std::vector<std::shared_ptr<UIElement>>& out)
{
    if (!visible_)
        return;

    if (isFocusable() && enabled_)
        out.push_back(shared_from_this());

    for (auto& child : children_)
    {
        if (child)
            child->collectFocusable(out);
    }
}

void UIElement::onMouseDown(float, float) {}
void UIElement::onMouseUp(float, float) {}
void UIElement::onMouseMove(float, float) {}
void UIElement::onMouseEnter() {}
void UIElement::onMouseLeave() {}
void UIElement::onClick() {}
void UIElement::onScroll(float) {}
void UIElement::onFocusGained() {}
void UIElement::onFocusLost() {}
void UIElement::onTextInput(const std::string&) {}
void UIElement::onKeyDown(int, bool, bool) {}

void UIElement::update(float deltaTime)
{
    for (auto& child : children_)
    {
        if (child && child->isVisible())
            child->update(deltaTime);
    }
}

void UIElement::render(UIRenderer& renderer, float inheritedOpacity) const
{
    if (!visible_)
        return;

    const float effective = inheritedOpacity * opacity_;
    if (effective <= 0.0f)
        return;

    const float previous = renderer.GetGlobalOpacity();

    renderer.SetGlobalOpacity(effective);
    renderSelf(renderer);

    beginChildren(renderer);
    renderChildren(renderer, effective);
    endChildren(renderer);

    renderer.SetGlobalOpacity(previous);
}

void UIElement::beginChildren(UIRenderer&) const {}

void UIElement::endChildren(UIRenderer&) const {}

void UIElement::renderChildren(UIRenderer& renderer, float opacity) const
{
    for (const auto& child : children_)
    {
        if (child && child->isVisible())
            child->render(renderer, opacity);
    }
}
