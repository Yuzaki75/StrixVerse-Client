#pragma once

#include <functional>
#include <string>

#include "UIElement.h"
#include "../graphics/Color.h"

class Font;

// -----------------------------------------------------------------------------
// UITextBox
//
// The design's ".sv-input" primitive: a rounded field with a placeholder, a
// blinking caret and the cyan focus ring.
//
// Text arrives as UTF-8 through onTextInput (the platform text-input service),
// not as raw key codes, so accented characters and IME composition behave.
// Editing keys - backspace, delete, arrows, home, end - come through onKeyDown.
//
// The caret is tracked as a byte offset into the UTF-8 string and only ever
// moved to a code point boundary.
// -----------------------------------------------------------------------------
class UITextBox : public UIElement
{
public:
    UITextBox();
    ~UITextBox() override = default;

    // --- Content ---------------------------------------------------------
    void setText(const std::string& text);
    const std::string& getText() const { return text_; }

    // Clears the field the next time it gains focus, once.
    //
    // For a value the player did not type -- a remembered username restored at
    // startup -- clicking the field means "I want a different one". Without
    // this there is no selection support to replace it, so typing appends and
    // signing in as someone else produces a mangled name like
    // "olduserNewuser". Touched-once semantics: if the player never focuses
    // the field, the restored value stands.
    void setClearOnNextFocus(bool clear) { clearOnNextFocus_ = clear; }

    void setPlaceholderText(const std::string& text) { placeholder_ = text; }
    const std::string& getPlaceholderText() const { return placeholder_; }

    void setFont(Font* font) { font_ = font; }
    Font* getFont() const { return font_; }

    // --- Appearance -------------------------------------------------------
    void setTextColor(const Color& color) { textColor_ = color; }
    void setPlaceholderColor(const Color& color) { placeholderColor_ = color; }
    void setBackgroundColor(const Color& color) { backgroundColor_ = color; }
    void setBorderColor(const Color& color) { borderColor_ = color; }
    void setFocusBorderColor(const Color& color) { focusBorderColor_ = color; }
    void setBorderWidth(float width) { borderWidth_ = width; }
    void setBorderRadius(float radius) { radius_ = radius; }
    void setPadding(float padding) { padding_ = padding; }

    // --- Behaviour --------------------------------------------------------
    void setPasswordMode(bool enabled) { passwordMode_ = enabled; }
    bool isPasswordMode() const { return passwordMode_; }

    // -1 means unlimited. Counted in code points, not bytes.
    void setMaxLength(int length);
    int getMaxLength() const { return maxLength_; }

    void setOnTextChanged(std::function<void(const std::string&)> callback);
    void setOnEnterPressed(std::function<void()> callback);

    bool hasFocus() const { return hasFocus_; }

    // --- UIElement --------------------------------------------------------
    bool wantsInput() const override { return true; }
    bool isFocusable() const override { return enabled_; }

    void update(float deltaTime) override;
    void onMouseDown(float x, float y) override;
    void onFocusGained() override;
    void onFocusLost() override;
    void onTextInput(const std::string& utf8) override;
    void onKeyDown(int key, bool ctrl, bool shift) override;

protected:
    void renderSelf(UIRenderer& renderer) const override;

private:
    // What is actually drawn: the text, or a run of dots in password mode.
    std::string displayString() const;

    // Byte offsets of the code point boundaries around the caret.
    size_t previousBoundary(size_t index) const;
    size_t nextBoundary(size_t index) const;

    // Counts code points, which is what maxLength_ limits.
    size_t codePointCount() const;

    void notifyChanged();

    std::string text_;
    std::string placeholder_;
    Font*       font_ = nullptr;

    Color textColor_{1.0f, 1.0f, 1.0f, 1.0f};
    Color placeholderColor_{0.42f, 0.50f, 0.71f, 1.0f};
    Color backgroundColor_{0.08f, 0.09f, 0.15f, 0.85f};
    Color borderColor_{0.42f, 0.50f, 0.71f, 0.40f};
    Color focusBorderColor_{0.30f, 0.88f, 1.0f, 1.0f};

    float borderWidth_ = 1.0f;
    float radius_      = 0.0f;
    float padding_     = 0.0f;

    bool passwordMode_ = false;
    int  maxLength_    = -1;

    bool   hasFocus_   = false;
    bool   clearOnNextFocus_ = false;
    size_t caretIndex_ = 0;       // Byte offset into text_.
    float  caretTimer_ = 0.0f;

    // Horizontal scroll applied when the text is wider than the field.
    // Written during rendering and read when mapping a click to a caret
    // position, hence mutable.
    mutable float scrollOffset_ = 0.0f;

    std::function<void(const std::string&)> onTextChanged_;
    std::function<void()>                   onEnterPressed_;
};
