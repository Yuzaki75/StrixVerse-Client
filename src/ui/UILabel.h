#pragma once

#include <string>
#include <vector>

#include "UIElement.h"
#include "../graphics/Color.h"
#include "../graphics/UIRenderer.h"

class Font;

// -----------------------------------------------------------------------------
// UILabel
//
// Single-line text.
//
// Carries everything the design's typography needs: an explicit face (the three
// typefaces are separate Font instances rasterised per size), a colour, letter
// spacing - the Figma screens track headings heavily - and an optional glow that
// reproduces the crystal text-shadow.
//
// Text is laid out inside the label's own box, so alignment works by giving the
// label the width of its container rather than by guessing at a centre point.
// -----------------------------------------------------------------------------
class UILabel : public UIElement
{
public:
    enum class Alignment
    {
        Left   = 0,
        Center = 1,
        Right  = 2
    };

    enum class VerticalAlignment
    {
        Top    = 0,
        Middle = 1,
        Bottom = 2
    };

    UILabel();
    ~UILabel() override = default;

    void setText(const std::string& text);
    const std::string& getText() const { return text_; }

    // Non-owning: fonts live in the AssetManager cache for the process
    // lifetime, so a raw pointer is safe and avoids per-frame refcounting.
    void setFont(Font* font) { font_ = font; }
    Font* getFont() const { return font_; }

    void setTextColor(const Color& color) { textColor_ = color; }
    const Color& getTextColor() const { return textColor_; }

    void setLetterSpacing(float spacing) { letterSpacing_ = spacing; }
    float getLetterSpacing() const { return letterSpacing_; }

    void setAlignment(Alignment alignment) { hAlign_ = alignment; }
    void setVerticalAlignment(VerticalAlignment alignment) { vAlign_ = alignment; }

    // Text shadow / crystal glow, as used by the splash and screen headings.
    void setGlow(const Color& color, float radius);

    // Hard offset shadow (the splash title's "4px 4px 0 #000").
    void setShadow(const Color& color, float offsetX, float offsetY);

    // Width of the current text, useful for laying out a row of labels.
    float measureTextWidth() const;

    // Resizes the label's box to exactly fit the current text.
    void sizeToFit();

    // Breaks a run into lines that each fit within maxWidth, splitting on
    // spaces. Labels are single-line by design, so paragraphs are built as one
    // label per returned line; this keeps layout explicit and measurable.
    static std::vector<std::string> WrapText(const Font& font,
                                             const std::string& text,
                                             float maxWidth,
                                             float letterSpacing = 0.0f);

protected:
    void renderSelf(UIRenderer& renderer) const override;

private:
    std::string text_;
    Font*       font_ = nullptr;

    Color textColor_{1.0f, 1.0f, 1.0f, 1.0f};
    float letterSpacing_ = 0.0f;

    Alignment         hAlign_ = Alignment::Left;
    VerticalAlignment vAlign_ = VerticalAlignment::Top;

    Color glowColor_{0.0f, 0.0f, 0.0f, 0.0f};
    float glowRadius_ = 0.0f;

    Color shadowColor_{0.0f, 0.0f, 0.0f, 0.0f};
    float shadowOffsetX_ = 0.0f;
    float shadowOffsetY_ = 0.0f;
};
