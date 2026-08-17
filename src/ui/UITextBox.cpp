#include "UITextBox.h"

#include "UITheme.h"
#include "../graphics/Font.h"
#include "../graphics/UIRenderer.h"

#include <algorithm>

namespace
{
    constexpr float kCaretBlinkPeriod = 1.0f;   // Seconds for a full on/off cycle.
    constexpr float kCaretWidth       = UITheme::Scaled(1.5f);

    // A UTF-8 continuation byte is 10xxxxxx; every other byte starts a code point.
    bool IsContinuation(unsigned char byte)
    {
        return (byte & 0xC0) == 0x80;
    }
}

UITextBox::UITextBox()
{
    textColor_        = UITheme::Text;
    placeholderColor_ = UITheme::Muted;
    backgroundColor_  = UITheme::InputBackground;
    borderColor_      = UITheme::InputBorder;
    focusBorderColor_ = UITheme::Accent;

    borderWidth_ = UITheme::BorderThin;
    radius_      = UITheme::RadiusInput;
    padding_     = UITheme::Scaled(12.0f);

    setSize(UITheme::Scaled(200.0f), UITheme::Scaled(34.0f));
}

void UITextBox::setText(const std::string& text)
{
    text_       = text;
    caretIndex_ = text_.size();

    // Trim to the code point limit rather than the byte limit.
    if (maxLength_ > 0)
    {
        while (codePointCount() > static_cast<size_t>(maxLength_))
        {
            text_.erase(previousBoundary(text_.size()));
        }
        caretIndex_ = text_.size();
    }

    notifyChanged();
}

void UITextBox::setMaxLength(int length)
{
    maxLength_ = length;

    if (maxLength_ > 0 && codePointCount() > static_cast<size_t>(maxLength_))
    {
        while (codePointCount() > static_cast<size_t>(maxLength_))
            text_.erase(previousBoundary(text_.size()));

        caretIndex_ = std::min(caretIndex_, text_.size());
        notifyChanged();
    }
}

void UITextBox::setOnTextChanged(std::function<void(const std::string&)> callback)
{
    onTextChanged_ = std::move(callback);
}

void UITextBox::setOnEnterPressed(std::function<void()> callback)
{
    onEnterPressed_ = std::move(callback);
}

void UITextBox::notifyChanged()
{
    if (onTextChanged_)
        onTextChanged_(text_);
}

size_t UITextBox::codePointCount() const
{
    size_t count = 0;
    for (char c : text_)
    {
        if (!IsContinuation(static_cast<unsigned char>(c)))
            ++count;
    }
    return count;
}

size_t UITextBox::previousBoundary(size_t index) const
{
    if (index == 0)
        return 0;

    size_t i = index - 1;
    while (i > 0 && IsContinuation(static_cast<unsigned char>(text_[i])))
        --i;

    return i;
}

size_t UITextBox::nextBoundary(size_t index) const
{
    if (index >= text_.size())
        return text_.size();

    size_t i = index + 1;
    while (i < text_.size() && IsContinuation(static_cast<unsigned char>(text_[i])))
        ++i;

    return i;
}

std::string UITextBox::displayString() const
{
    if (!passwordMode_)
        return text_;

    // One dot per code point, not per byte.
    std::string masked;
    masked.reserve(text_.size());

    for (char c : text_)
    {
        if (!IsContinuation(static_cast<unsigned char>(c)))
            masked += '*';
    }

    return masked;
}

void UITextBox::update(float deltaTime)
{
    UIElement::update(deltaTime);

    if (hasFocus_)
    {
        caretTimer_ += deltaTime;
        if (caretTimer_ >= kCaretBlinkPeriod)
            caretTimer_ -= kCaretBlinkPeriod;
    }
}

void UITextBox::onMouseDown(float x, float)
{
    if (!enabled_ || !font_ || !font_->IsLoaded())
        return;

    // Place the caret at the code point boundary nearest the click, taking the
    // horizontal scroll from the last rendered frame into account.
    const float originX  = getAbsoluteX() + padding_ - scrollOffset_;
    const float relative = x - originX;

    size_t byteIndex = 0;
    float  width     = 0.0f;

    while (byteIndex < text_.size())
    {
        const size_t next = nextBoundary(byteIndex);

        // Password mode advances by a mask character, not the real glyph.
        const std::string piece = passwordMode_
                                      ? std::string("*")
                                      : text_.substr(byteIndex, next - byteIndex);

        const float advance = font_->MeasureWidth(piece, 0.0f);

        if (relative < width + advance * 0.5f)
            break;

        width += advance;
        byteIndex = next;
    }

    caretIndex_ = byteIndex;
    caretTimer_ = 0.0f;
}

void UITextBox::onFocusGained()
{
    hasFocus_   = true;
    caretTimer_ = 0.0f;

    // A restored value the player did not type is replaced rather than
    // appended to. Consumed on use, so this only applies to the first focus.
    if (clearOnNextFocus_)
    {
        clearOnNextFocus_ = false;
        text_.clear();
    }

    caretIndex_ = text_.size();
}

void UITextBox::onFocusLost()
{
    hasFocus_ = false;
}

void UITextBox::onTextInput(const std::string& utf8)
{
    if (!enabled_ || utf8.empty())
        return;

    if (maxLength_ > 0)
    {
        size_t incoming = 0;
        for (char c : utf8)
        {
            if (!IsContinuation(static_cast<unsigned char>(c)))
                ++incoming;
        }

        if (codePointCount() + incoming > static_cast<size_t>(maxLength_))
            return;
    }

    caretIndex_ = std::min(caretIndex_, text_.size());
    text_.insert(caretIndex_, utf8);
    caretIndex_ += utf8.size();
    caretTimer_ = 0.0f;

    notifyChanged();
}

void UITextBox::onKeyDown(int key, bool ctrl, bool)
{
    if (!enabled_)
        return;

    caretIndex_ = std::min(caretIndex_, text_.size());
    caretTimer_ = 0.0f;

    switch (key)
    {
    case UIKey::Backspace:
        if (caretIndex_ > 0)
        {
            const size_t start = ctrl ? 0 : previousBoundary(caretIndex_);
            text_.erase(start, caretIndex_ - start);
            caretIndex_ = start;
            notifyChanged();
        }
        break;

    case UIKey::Delete:
        if (caretIndex_ < text_.size())
        {
            const size_t end = ctrl ? text_.size() : nextBoundary(caretIndex_);
            text_.erase(caretIndex_, end - caretIndex_);
            notifyChanged();
        }
        break;

    case UIKey::Left:
        caretIndex_ = ctrl ? 0 : previousBoundary(caretIndex_);
        break;

    case UIKey::Right:
        caretIndex_ = ctrl ? text_.size() : nextBoundary(caretIndex_);
        break;

    case UIKey::Home:
        caretIndex_ = 0;
        break;

    case UIKey::End:
        caretIndex_ = text_.size();
        break;

    case UIKey::Enter:
        if (onEnterPressed_)
            onEnterPressed_();
        break;

    default:
        break;
    }
}

void UITextBox::renderSelf(UIRenderer& renderer) const
{
    const float x = getAbsoluteX();
    const float y = getAbsoluteY();

    UIQuadStyle style = UIQuadStyle::Solid(backgroundColor_, radius_);

    if (hasFocus_)
    {
        // ".sv-input:focus" - cyan border plus a soft ring.
        style.WithBorder(focusBorderColor_, borderWidth_);
        style.WithGlow(UITheme::WithAlpha(focusBorderColor_, 0.35f), UITheme::Scaled(4.0f));
    }
    else
    {
        style.WithBorder(borderColor_, borderWidth_);
    }

    if (!enabled_)
        style.fillTop = style.fillBottom = UITheme::WithAlpha(backgroundColor_, backgroundColor_.a * 0.5f);

    renderer.DrawRect(x, y, width_, height_, style);

    if (!font_ || !font_->IsLoaded())
        return;

    const std::string display = displayString();
    const bool        showPlaceholder = display.empty() && !placeholder_.empty();
    const std::string drawn = showPlaceholder ? placeholder_ : display;

    const float innerX  = x + padding_;
    const float innerW  = std::max(0.0f, width_ - padding_ * 2.0f);
    const float textY   = y + (height_ - font_->GetLineHeight()) * 0.5f;

    // Keep the caret in view when the content is wider than the field.
    float caretOffset = 0.0f;
    if (!showPlaceholder)
    {
        const std::string beforeCaret = passwordMode_
                                            ? std::string(std::count_if(text_.begin(),
                                                                        text_.begin() + static_cast<ptrdiff_t>(std::min(caretIndex_, text_.size())),
                                                                        [](char c) { return !IsContinuation(static_cast<unsigned char>(c)); }),
                                                          '*')
                                            : text_.substr(0, std::min(caretIndex_, text_.size()));

        caretOffset = font_->MeasureWidth(beforeCaret, 0.0f);
    }

    const float scroll = caretOffset > innerW ? caretOffset - innerW : 0.0f;

    // Remembered so a click can map back to a caret position next frame.
    scrollOffset_ = scroll;

    // The field clips its own content so long values cannot spill over the
    // rounded border.
    renderer.PushClip(innerX, y, innerW, height_);

    renderer.DrawText(*font_, drawn, innerX - scroll, textY,
                      showPlaceholder ? placeholderColor_ : textColor_);

    // Caret: visible for the first half of each blink cycle.
    if (hasFocus_ && caretTimer_ < kCaretBlinkPeriod * 0.5f)
    {
        const float caretX = innerX - scroll + caretOffset;
        const float caretH = font_->GetLineHeight() * 0.8f;
        const float caretY = y + (height_ - caretH) * 0.5f;

        renderer.DrawRect(caretX, caretY, kCaretWidth, caretH,
                          UIQuadStyle::Solid(textColor_));
    }

    renderer.PopClip();
}
