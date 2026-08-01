#pragma once

#include "UIElement.h"
#include "../graphics/Color.h"

/**
 * Label UI element for displaying text
 */
class UILabel : public UIElement {
public:
    UILabel();
    ~UILabel() override = default;

    enum class Alignment {
        Left = 0,
        Center = 1,
        Right = 2
    };

    // Text properties
    void setText(const std::string& text);
    const std::string& getText() const { return text_; }

    void setFontSize(float size);
    float getFontSize() const { return fontSize_; }

    void setTextColor(const Color& color);
    const Color& getTextColor() const { return textColor_; }

    void setHorizontalAlignment(int alignment); // 0=left, 1=center, 2=right
    int getHorizontalAlignment() const { return hAlign_; }

    void setVerticalAlignment(int alignment); // 0=top, 1=middle, 2=bottom
    int getVerticalAlignment() const { return vAlign_; }

    // Convenience setter that sets horizontal alignment from Alignment enum
    void setAlignment(Alignment alignment) { hAlign_ = static_cast<int>(alignment); }

    // UIElement overrides
    void renderSelf(SpriteBatch& spriteBatch, Font& font) const override;

private:
    std::string text_;
    float fontSize_;      // Font size in points
    Color textColor_;
    int hAlign_;          // Horizontal alignment (0=left, 1=center, 2=right)
    int vAlign_;          // Vertical alignment (0=top, 1=middle, 2=bottom)
};