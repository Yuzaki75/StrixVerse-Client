#include "UIElement.h"
#include "../graphics/SpriteBatch.h"
#include "../graphics/Font.h"
#include "../core/Logger.h"

UIElement::UIElement()
    : x_(0), y_(0), width_(0), height_(0),
      anchor_(AnchorPoint::TopLeft),
      visible_(true), enabled_(true) {}

UIElement::~UIElement() = default;

void UIElement::setPosition(float x, float y) {
    x_ = x;
    y_ = y;
}

void UIElement::setSize(float width, float height) {
    width_ = width;
    height_ = height;
}

void UIElement::setAnchor(AnchorPoint anchor) {
    anchor_ = anchor;
}

void UIElement::setVisible(bool visible) {
    visible_ = visible;
}

void UIElement::setEnabled(bool enabled) {
    enabled_ = enabled;
}

bool UIElement::containsPoint(float x, float y) const {
    float renderX, renderY;
    calculateRenderPosition(renderX, renderY);

    return (x >= renderX && x <= renderX + width_ &&
            y >= renderY && y <= renderY + height_);
}

void UIElement::onMouseDown(float x, float y) {
    // Default implementation - can be overridden
}

void UIElement::onMouseUp(float x, float y) {
    // Default implementation - can be overridden
}

void UIElement::onMouseMove(float x, float y) {
    // Default implementation - can be overridden
}

void UIElement::onMouseEnter() {
    // Default implementation - can be overridden
}

void UIElement::onMouseLeave() {
    // Default implementation - can be overridden
}

void UIElement::onClick() {
    // Default implementation - can be overridden
}

void UIElement::onFocusGained() {
    // Default implementation - can be overridden
}

void UIElement::onFocusLost() {
    // Default implementation - can be overridden
}

void UIElement::onKeyPressed(char key) {
    // Default implementation - can be overridden
}

void UIElement::onSpecialKeyPressed(int key) {
    // Default implementation - can be overridden
}

void UIElement::update(float deltaTime) {
    // Update children
    for (auto& child : children_) {
        if (child->isVisible()) {
            child->update(deltaTime);
        }
    }
}

void UIElement::render(SpriteBatch& spriteBatch, Font& font) const {
    if (!visible_) return;

    renderSelf(spriteBatch, font);
    renderChildren(spriteBatch, font);
}

void UIElement::addChild(std::shared_ptr<UIElement> child) {
    children_.push_back(child);
}

void UIElement::removeChild(std::shared_ptr<UIElement> child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        children_.erase(it);
    }
}

void UIElement::clearChildren() {
    children_.clear();
}

void UIElement::renderChildren(SpriteBatch& spriteBatch, Font& font) const {
    for (const auto& child : children_) {
        if (child->isVisible()) {
            child->render(spriteBatch, font);
        }
    }
}

void UIElement::calculateRenderPosition(float& renderX, float& renderY) const {
    renderX = x_;
    renderY = y_;

    switch (anchor_) {
        case AnchorPoint::TopLeft:
            // No adjustment needed
            break;
        case AnchorPoint::TopCenter:
            renderX -= width_ / 2.0f;
            break;
        case AnchorPoint::TopRight:
            renderX -= width_;
            break;
        case AnchorPoint::MiddleLeft:
            renderY -= height_ / 2.0f;
            break;
        case AnchorPoint::Center:
            renderX -= width_ / 2.0f;
            renderY -= height_ / 2.0f;
            break;
        case AnchorPoint::MiddleRight:
            renderX -= width_;
            renderY -= height_ / 2.0f;
            break;
        case AnchorPoint::BottomLeft:
            renderY -= height_;
            break;
        case AnchorPoint::BottomCenter:
            renderX -= width_ / 2.0f;
            renderY -= height_;
            break;
        case AnchorPoint::BottomRight:
            renderX -= width_;
            renderY -= height_;
            break;
    }
}