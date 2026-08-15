#pragma once

#include <string>

#include "UIElement.h"
#include "../graphics/Color.h"

class Font;

// -----------------------------------------------------------------------------
// UIProgressBar
//
// The design's ".sv-bar" primitive: a dark rounded track with a glowing
// gradient fill and an optional centred label.
//
// Used by World Loading (the main progress bar), World Selection (world
// population) and the connection-failure state (retry progress).
// -----------------------------------------------------------------------------
class UIProgressBar : public UIElement
{
public:
    UIProgressBar();
    ~UIProgressBar() override = default;

    // Clamped to 0..1.
    void setProgress(float progress);
    float getProgress() const { return progress_; }

    void setTrackColor(const Color& color) { trackColor_ = color; }
    void setTrackBorderColor(const Color& color) { trackBorder_ = color; }

    // A single colour gives a flat fill; two give the design's left-to-right
    // gradient (#4F8CFF -> #4DE1FF on the loading screen).
    void setFillColor(const Color& color);
    void setFillGradient(const Color& start, const Color& end);

    void setGlowColor(const Color& color) { glowColor_ = color; }
    void setBorderRadius(float radius) { radius_ = radius; }

    // Optional text drawn centred over the bar.
    void setLabel(const std::string& label) { label_ = label; }
    void setFont(Font* font) { font_ = font; }
    void setLabelColor(const Color& color) { labelColor_ = color; }

protected:
    void renderSelf(UIRenderer& renderer) const override;

private:
    float progress_ = 0.0f;

    Color trackColor_{0.0f, 0.0f, 0.0f, 0.45f};
    Color trackBorder_{0.42f, 0.50f, 0.71f, 0.25f};
    Color fillStart_{0.31f, 0.55f, 1.0f, 1.0f};
    Color fillEnd_{0.30f, 0.88f, 1.0f, 1.0f};
    Color glowColor_{0.30f, 0.88f, 1.0f, 0.65f};
    Color labelColor_{1.0f, 1.0f, 1.0f, 1.0f};

    float radius_ = 0.0f;

    std::string label_;
    Font*       font_ = nullptr;
};
