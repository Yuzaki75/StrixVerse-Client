#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../graphics/Color.h"

class UIRenderer;

// Keyboard keys the UI reacts to. The Engine translates SDL keycodes into
// these so nothing under ui/ has to include SDL.
namespace UIKey
{
    enum Key : int
    {
        None = 0,
        Backspace,
        Delete,
        Left,
        Right,
        Up,
        Down,
        Home,
        End,
        Tab,
        Enter,
        Escape,
        Digit0,
        Digit1,
        Digit2,
        Digit3,
        Digit4,
        Digit5,
        Digit6,
        Digit7,
        Digit8,
        Digit9
    };
}

// -----------------------------------------------------------------------------
// UIElement
//
// Base class for everything in the UI tree.
//
// Coordinates are in the 1920x1080 design canvas and are relative to the parent
// element, so a panel can be moved and its whole contents follow. Absolute
// positions are resolved on demand via GetAbsoluteX()/GetAbsoluteY(); hit
// testing and rendering both use those, which is what makes children of a
// panel clickable.
//
// Ownership is shared_ptr downwards (a parent owns its children) and a raw
// non-owning pointer upwards.
// -----------------------------------------------------------------------------
class UIElement : public std::enable_shared_from_this<UIElement>
{
public:
    enum class AnchorPoint
    {
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

    UIElement(const UIElement&) = delete;
    UIElement& operator=(const UIElement&) = delete;

    // --- Geometry --------------------------------------------------------
    void setPosition(float x, float y);
    void setSize(float width, float height);
    void setBounds(float x, float y, float width, float height);
    void setAnchor(AnchorPoint anchor);

    float getX() const { return x_; }
    float getY() const { return y_; }
    float getWidth() const { return width_; }
    float getHeight() const { return height_; }

    struct Position
    {
        float x;
        float y;
    };
    Position getPosition() const { return {x_, y_}; }

    // Position of this element's top-left corner in canvas space, with the
    // anchor and every ancestor's offset applied.
    float getAbsoluteX() const;
    float getAbsoluteY() const;

    // --- State -----------------------------------------------------------
    void setVisible(bool visible) { visible_ = visible; }

    // Virtual so widgets can drop transient state (hover, press) when they are
    // disabled out from under the mouse.
    virtual void setEnabled(bool enabled) { enabled_ = enabled; }
    void setOpacity(float opacity);

    bool isVisible() const { return visible_; }
    bool isEnabled() const { return enabled_; }
    float getOpacity() const { return opacity_; }

    // Whether this element should receive mouse events. Containers stay
    // transparent to input unless they opt in.
    virtual bool wantsInput() const { return blocksInput_; }
    void setBlocksInput(bool blocks) { blocksInput_ = blocks; }

    // Whether this element takes keyboard focus (and joins the Tab order).
    virtual bool isFocusable() const { return false; }

    // Whether this element consumes typing while it holds focus.
    //
    // Distinct from isFocusable, and the distinction matters: a button is
    // focusable so it can be reached by keyboard, but focusing one must not
    // stop the player walking or building. Gameplay asks this question, not
    // "is anything focused" - which was true the instant a hotbar slot was
    // clicked, and froze movement and world edits until the player happened to
    // click somewhere blank.
    virtual bool consumesTextInput() const { return false; }

    // --- Hierarchy -------------------------------------------------------
    void addChild(std::shared_ptr<UIElement> child);
    void removeChild(const std::shared_ptr<UIElement>& child);
    void clearChildren();

    const std::vector<std::shared_ptr<UIElement>>& getChildren() const { return children_; }
    UIElement* getParent() const { return parent_; }

    // Returns the topmost descendant (or this) that wants input at the given
    // canvas position, or nullptr.
    virtual std::shared_ptr<UIElement> hitTest(float x, float y);

    // Appends this subtree's focusable elements in visual order.
    void collectFocusable(std::vector<std::shared_ptr<UIElement>>& out);

    virtual bool containsPoint(float x, float y) const;

    // --- Input events ----------------------------------------------------
    virtual void onMouseDown(float x, float y);
    virtual void onMouseUp(float x, float y);
    virtual void onMouseMove(float x, float y);
    virtual void onMouseEnter();
    virtual void onMouseLeave();
    virtual void onClick();
    virtual void onScroll(float delta);

    // Optional scroll callback so a panel can react without a subclass.
    void setOnScroll(std::function<void(float)> callback) { onScroll_ = std::move(callback); }
    virtual void onFocusGained();
    virtual void onFocusLost();
    virtual void onTextInput(const std::string& utf8);
    virtual void onKeyDown(int key, bool ctrl, bool shift);

    // --- Frame -----------------------------------------------------------
    virtual void update(float deltaTime);

    // Renders this element and its children. inheritedOpacity multiplies down
    // the tree so a whole panel can fade as one.
    void render(UIRenderer& renderer, float inheritedOpacity = 1.0f) const;

protected:
    // Draws only this element. Children are handled by render().
    virtual void renderSelf(UIRenderer& renderer) const = 0;

    // Bracket the children pass. Containers that clip or offset their content
    // (UIPanel, UIScrollPanel) push state in the first and undo it in the
    // second, so the state is always balanced even for empty containers.
    virtual void beginChildren(UIRenderer& renderer) const;
    virtual void endChildren(UIRenderer& renderer) const;

    void renderChildren(UIRenderer& renderer, float opacity) const;

    // Offset applied by the anchor, in the parent's coordinate space.
    void calculateAnchorOffset(float& offsetX, float& offsetY) const;

    float x_ = 0.0f;
    float y_ = 0.0f;
    float width_ = 0.0f;
    float height_ = 0.0f;

    AnchorPoint anchor_ = AnchorPoint::TopLeft;

    bool  visible_ = true;
    bool  enabled_ = true;
    bool  blocksInput_ = false;
    float opacity_ = 1.0f;

    UIElement* parent_ = nullptr;

    std::vector<std::shared_ptr<UIElement>> children_;

    std::function<void(float)> onScroll_;
};
