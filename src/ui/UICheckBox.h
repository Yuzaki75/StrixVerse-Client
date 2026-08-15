#pragma once

#include <functional>
#include <string>
#include <vector>

#include "UIElement.h"
#include "../graphics/Color.h"

class Font;

// -----------------------------------------------------------------------------
// UICheckBox
//
// A square box with a check mark and a label to its right.
//
// Used by Login ("Remember me") and Register (the terms of service consent).
// The whole row - box plus label - is the click target, matching the design's
// wrapping <label> element.
// -----------------------------------------------------------------------------
class UICheckBox : public UIElement
{
public:
    UICheckBox();
    ~UICheckBox() override = default;

    void setChecked(bool checked);
    bool isChecked() const { return checked_; }

    void setLabel(const std::string& label);
    const std::string& getLabel() const { return label_; }

    // Wraps the label to the space left of the element's right edge, so long
    // consent text (the register screen's terms line) stays inside its panel.
    // Call after setFont() and setSize().
    void wrapLabel();

    void setFont(Font* font) { font_ = font; }
    void setLabelColor(const Color& color) { labelColor_ = color; }

    // Accent used for the filled box and the check mark's glow.
    void setAccentColor(const Color& color) { accentColor_ = color; }

    void setBoxSize(float size) { boxSize_ = size; }

    void setOnChanged(std::function<void(bool)> callback) { onChanged_ = std::move(callback); }

    bool wantsInput() const override { return true; }
    bool isFocusable() const override { return enabled_; }

    void onMouseEnter() override { hovered_ = enabled_; }
    void onMouseLeave() override { hovered_ = false; }
    void onClick() override;
    void onKeyDown(int key, bool ctrl, bool shift) override;
    void onFocusGained() override { focused_ = true; }
    void onFocusLost() override { focused_ = false; }

protected:
    void renderSelf(UIRenderer& renderer) const override;

private:
    bool        checked_ = false;
    std::string label_;

    // Rendered lines. Holds exactly one entry unless wrapLabel() split it.
    std::vector<std::string> labelLines_;

    Font* font_ = nullptr;

    Color labelColor_{0.78f, 0.82f, 0.88f, 1.0f};
    Color accentColor_{0.31f, 0.55f, 1.0f, 1.0f};

    float boxSize_ = 0.0f;

    bool hovered_ = false;
    bool focused_ = false;

    std::function<void(bool)> onChanged_;
};
