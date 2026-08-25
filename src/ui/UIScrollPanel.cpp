#include "UIScrollPanel.h"

#include "UITheme.h"
#include "../graphics/UIRenderer.h"

#include <algorithm>

namespace
{
    constexpr float kThumbWidth  = UITheme::Scaled(4.0f);
    constexpr float kThumbInset  = UITheme::Scaled(2.0f);
    constexpr float kMinThumb    = UITheme::Scaled(24.0f);
    constexpr float kDefaultStep = UITheme::Scaled(48.0f);
}

UIScrollPanel::UIScrollPanel()
    : scrollSpeed_(kDefaultStep)
{
    // The panel itself is a plain viewport by default; screens style it.
    setBackgroundColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    setBorderRadius(0.0f);

    content_ = std::make_shared<UIPanel>();
    content_->setBackgroundColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    content_->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    content_->setBorderRadius(0.0f);
    content_->setPosition(0.0f, 0.0f);

    addChild(content_);
}

void UIScrollPanel::addContent(std::shared_ptr<UIElement> child)
{
    content_->addChild(std::move(child));
}

void UIScrollPanel::clearContent()
{
    content_->clearChildren();
    contentHeight_ = 0.0f;
    scroll_        = 0.0f;
    content_->setPosition(0.0f, 0.0f);
}

const std::vector<std::shared_ptr<UIElement>>& UIScrollPanel::getContent() const
{
    return content_->getChildren();
}

void UIScrollPanel::refreshContentHeight()
{
    float extent = 0.0f;

    for (const auto& child : content_->getChildren())
    {
        if (!child || !child->isVisible())
            continue;

        extent = std::max(extent, child->getY() + child->getHeight());
    }

    contentHeight_ = extent;
    content_->setSize(width_, contentHeight_);

    // Shrinking content can leave the view scrolled past the end.
    setScrollOffset(scroll_);
}

float UIScrollPanel::maxScroll() const
{
    return std::max(0.0f, contentHeight_ - height_);
}

void UIScrollPanel::setScrollOffset(float offset)
{
    scroll_ = std::clamp(offset, 0.0f, maxScroll());
    content_->setPosition(0.0f, -scroll_);
}

void UIScrollPanel::scrollBy(float delta)
{
    setScrollOffset(scroll_ + delta);
}

bool UIScrollPanel::onScroll(float delta)
{
    if (maxScroll() <= 0.0f)
        return false;   // nothing to scroll; let an ancestor try

    // Positive wheel delta means "scroll up" on every platform SDL reports.
    scrollBy(-delta * scrollSpeed_);

    return true;
}

void UIScrollPanel::beginChildren(UIRenderer& renderer) const
{
    renderer.PushClip(getAbsoluteX(), getAbsoluteY(), width_, height_);
}

void UIScrollPanel::endChildren(UIRenderer& renderer) const
{
    renderer.PopClip();

    const float scrollable = maxScroll();
    if (scrollable <= 0.0f)
        return;

    const float x = getAbsoluteX();
    const float y = getAbsoluteY();

    const float trackX = x + width_ - kThumbWidth - kThumbInset;

    // Track.
    renderer.DrawRect(trackX, y, kThumbWidth, height_,
                      UIQuadStyle::Solid(Color(0.0f, 0.0f, 0.0f, 0.15f), kThumbWidth * 0.5f));

    // Thumb: proportional to how much of the content is visible.
    const float visibleFraction = std::clamp(height_ / contentHeight_, 0.0f, 1.0f);
    const float thumbHeight     = std::max(kMinThumb, height_ * visibleFraction);
    const float travel          = height_ - thumbHeight;
    const float thumbY          = y + travel * (scroll_ / scrollable);

    renderer.DrawRect(trackX, thumbY, kThumbWidth, thumbHeight,
                      UIQuadStyle::Solid(UITheme::WithAlpha(UITheme::Border, 0.45f),
                                         kThumbWidth * 0.5f));
}
