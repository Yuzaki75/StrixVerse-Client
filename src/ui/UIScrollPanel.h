#pragma once

#include <memory>

#include "UIPanel.h"

// -----------------------------------------------------------------------------
// UIScrollPanel
//
// A panel whose contents scroll vertically inside a clipped viewport, with the
// thin translucent thumb the style guide uses (".sv-scroll").
//
// Content is added through addContent() and lives on an inner element that is
// offset by the scroll position, so rows keep simple top-down coordinates and
// the existing parent-relative layout does the rest.
//
// Used by World Selection for the world list.
// -----------------------------------------------------------------------------
class UIScrollPanel : public UIPanel
{
public:
    UIScrollPanel();
    ~UIScrollPanel() override = default;

    // Adds a row. Position it relative to the top of the content area.
    void addContent(std::shared_ptr<UIElement> child);
    void clearContent();

    const std::vector<std::shared_ptr<UIElement>>& getContent() const;

    // Recomputes the scrollable height from the current rows. Call after
    // rebuilding the content.
    void refreshContentHeight();

    float getContentHeight() const { return contentHeight_; }
    float getScrollOffset() const { return scroll_; }

    void setScrollOffset(float offset);
    void scrollBy(float delta);

    // Canvas pixels moved per wheel notch.
    void setScrollSpeed(float pixelsPerNotch) { scrollSpeed_ = pixelsPerNotch; }

    // The panel itself accepts input so the wheel has somewhere to land, even
    // when the cursor is over empty space between rows.
    bool wantsInput() const override { return true; }

    // Consumes the wheel whenever the panel actually scrolled, so an
    // enclosing scroll panel does not also move.
    bool onScroll(float delta) override;

protected:
    void beginChildren(UIRenderer& renderer) const override;
    void endChildren(UIRenderer& renderer) const override;

private:
    float maxScroll() const;

    std::shared_ptr<UIPanel> content_;

    float contentHeight_ = 0.0f;
    float scroll_        = 0.0f;
    float scrollSpeed_   = 0.0f;
};
