#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "../graphics/Color.h"

// Forward declarations
class SpriteBatch;
class Font;

/**
 * Base class for all UI elements
 */
class UIElement {
public:
    enum class AnchorPoint {
        TopLeft,
        TopCenter,
        TopRight,
        MiddleLeft,
        Center,
        MiddleRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    };

    UIElement();
    virtual ~UIElement();

    // Position and size
    void setPosition(float x, float y);
    void setSize(float width, float height);
    void setAnchor(AnchorPoint anchor);

    float getX() const { return x_; }
    float getY() const { return y_; }
    float getWidth() const { return width_; }
    float getHeight() const { return height_; }

    struct Position { float x; float y; };
    Position getPosition() const { return { x_, y_ }; }

    // Visibility and interaction
    void setVisible(bool visible);
    void setEnabled(bool enabled);
    bool isVisible() const { return visible_; }
    bool isEnabled() const { return enabled_; }

    // Input handling
    virtual bool containsPoint(float x, float y) const;
    virtual void onMouseDown(float x, float y);
    virtual void onMouseUp(float x, float y);
    virtual void onMouseMove(float x, float y);
    virtual void onMouseEnter();
    virtual void onMouseLeave();
    virtual void onClick();
    virtual void onFocusGained();
    virtual void onFocusLost();
    virtual void onKeyPressed(char key);
    virtual void onSpecialKeyPressed(int key);

    // Update and render
    virtual void update(float deltaTime);
    virtual void render(SpriteBatch& spriteBatch, Font& font) const;

    // Children management (for containers)
    void addChild(std::shared_ptr<UIElement> child);
    void removeChild(std::shared_ptr<UIElement> child);
    void clearChildren();
    const std::vector<std::shared_ptr<UIElement>>& getChildren() const { return children_; }

protected:
    float x_, y_;           // Position
    float width_, height_;  // Size
    AnchorPoint anchor_;    // Anchor point

    bool visible_;          // Whether the element is visible
    bool enabled_;          // Whether the element is interactive

    std::vector<std::shared_ptr<UIElement>> children_; // Child elements

    // Helper methods for rendering
    virtual void renderSelf(SpriteBatch& spriteBatch, Font& font) const = 0;
    void renderChildren(SpriteBatch& spriteBatch, Font& font) const;

    // Calculate actual position based on anchor
    void calculateRenderPosition(float& renderX, float& renderY) const;
};