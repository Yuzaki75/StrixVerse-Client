#include "UILabel.h"

#include "../graphics/Font.h"

UILabel::UILabel() = default;

void UILabel::setText(const std::string& text)
{
    text_ = text;
}

void UILabel::setGlow(const Color& color, float radius)
{
    glowColor_  = color;
    glowRadius_ = radius;
}

void UILabel::setShadow(const Color& color, float offsetX, float offsetY)
{
    shadowColor_   = color;
    shadowOffsetX_ = offsetX;
    shadowOffsetY_ = offsetY;
}

float UILabel::measureTextWidth() const
{
    if (!font_ || text_.empty())
        return 0.0f;

    return font_->MeasureWidth(text_, letterSpacing_);
}

void UILabel::sizeToFit()
{
    if (!font_)
        return;

    setSize(measureTextWidth(), font_->GetLineHeight());
}

std::vector<std::string> UILabel::WrapText(const Font& font,
                                           const std::string& text,
                                           float maxWidth,
                                           float letterSpacing)
{
    std::vector<std::string> lines;

    if (text.empty() || maxWidth <= 0.0f || !font.IsLoaded())
    {
        if (!text.empty())
            lines.push_back(text);
        return lines;
    }

    std::string current;
    size_t index = 0;

    while (index <= text.size())
    {
        const size_t space = text.find(' ', index);
        const std::string word = text.substr(index, space == std::string::npos
                                                        ? std::string::npos
                                                        : space - index);

        const std::string candidate = current.empty() ? word : current + " " + word;

        if (!current.empty() && font.MeasureWidth(candidate, letterSpacing) > maxWidth)
        {
            lines.push_back(current);
            current = word;
        }
        else
        {
            current = candidate;
        }

        if (space == std::string::npos)
            break;

        index = space + 1;
    }

    if (!current.empty())
        lines.push_back(current);

    return lines;
}

void UILabel::renderSelf(UIRenderer& renderer) const
{
    if (text_.empty() || !font_ || !font_->IsLoaded())
        return;

    const float textWidth  = font_->MeasureWidth(text_, letterSpacing_);
    const float lineHeight = font_->GetLineHeight();

    float x = getAbsoluteX();
    float y = getAbsoluteY();

    switch (hAlign_)
    {
    case Alignment::Center:
        x += (width_ - textWidth) * 0.5f;
        break;
    case Alignment::Right:
        x += width_ - textWidth;
        break;
    case Alignment::Left:
        break;
    }

    switch (vAlign_)
    {
    case VerticalAlignment::Middle:
        y += (height_ - lineHeight) * 0.5f;
        break;
    case VerticalAlignment::Bottom:
        y += height_ - lineHeight;
        break;
    case VerticalAlignment::Top:
        break;
    }

    // Hard offset shadow first, so it sits behind both glow and fill.
    if (shadowColor_.a > 0.0f && (shadowOffsetX_ != 0.0f || shadowOffsetY_ != 0.0f))
    {
        renderer.DrawText(*font_, text_,
                          x + shadowOffsetX_, y + shadowOffsetY_,
                          shadowColor_, letterSpacing_);
    }

    if (glowRadius_ > 0.0f && glowColor_.a > 0.0f)
    {
        renderer.DrawTextGlow(*font_, text_, x, y,
                              textColor_, glowColor_, glowRadius_, letterSpacing_);
    }
    else
    {
        renderer.DrawText(*font_, text_, x, y, textColor_, letterSpacing_);
    }
}
