#include "UIConfirmDialog.h"

#include "../core/Engine.h"
#include "../graphics/Font.h"
#include "UIButton.h"
#include "UIFonts.h"
#include "UILabel.h"
#include "UIManager.h"
#include "UIPanel.h"
#include "UIScale.h"
#include "UITheme.h"

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    Font* DialogFont(Engine* engine, UIFonts::Typeface face, unsigned int size)
    {
        UIFonts* fonts = engine ? engine->GetUIFonts() : nullptr;
        return fonts ? fonts->Get(face, size) : nullptr;
    }

    // Card geometry, in style-guide pixels. The height is not fixed: Show()
    // grows the card to fit the wrapped message.
    constexpr float kCardWidth    = 280.0f;
    constexpr float kInset        = 20.0f;
    constexpr float kTitleY       = 16.0f;
    constexpr float kTitleHeight  = 22.0f;
    constexpr float kMessageY     = 46.0f;
    constexpr float kLineGap      = 4.0f;
    constexpr float kButtonGap    = 10.0f;
    constexpr float kButtonHeight = 34.0f;

    // Space between the last message line and the button row.
    constexpr float kMessageToButtons = 14.0f;
    constexpr float kBottomPadding    = 18.0f;
}

UIConfirmDialog::UIConfirmDialog(Engine* engine, UIManager* uiManager)
    : engine_(engine), uiManager_(uiManager)
{
}

UIConfirmDialog::~UIConfirmDialog()
{
    // The UIManager outlives this dialog on a screen change, so the elements
    // have to be handed back or they keep drawing over whatever comes next.
    if (uiManager_ && root_)
        uiManager_->removeElement(root_);
}

void UIConfirmDialog::Build()
{
    if (root_ || !uiManager_)
        return;

    BuildFrame();

    root_->setVisible(false);
}

void UIConfirmDialog::BuildFrame()
{
    const float cardWidth = S(kCardWidth);

    // Full-canvas dim that swallows every click while open - the modal rule
    // PauseOverlay established.
    root_ = std::make_shared<UIPanel>();
    root_->setSize(UIScale::kDesignWidth, UIScale::kDesignHeight);
    root_->setPosition(0.0f, 0.0f);
    root_->setBackgroundColor(UITheme::Hex(0x000000, 0.50f));
    root_->setBlocksInput(true);

    // Added to the UIManager rather than to a screen's root so it draws above
    // the HUD; UIManager renders in insertion order.
    uiManager_->addElement(root_);

    card_ = std::make_shared<UIPanel>();
    card_->setSize(cardWidth, S(140.0f));
    card_->setPosition((UIScale::kDesignWidth - cardWidth) * 0.5f,
                       (UIScale::kDesignHeight - S(140.0f)) * 0.5f);
    card_->setBackgroundColor(UITheme::Hex(0x1E2230, 0.97f));
    card_->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.45f), UITheme::BorderThin);
    card_->setBorderRadius(UITheme::RadiusPanel);
    card_->setBlocksInput(true);
    root_->addChild(card_);

    titleLabel_ = std::make_shared<UILabel>();
    titleLabel_->setFont(DialogFont(engine_, UIFonts::Typeface::Display,
                                    UITheme::Display::Label));
    titleLabel_->setTextColor(UITheme::Text);
    titleLabel_->setAlignment(UILabel::Alignment::Center);
    titleLabel_->setPosition(0.0f, S(kTitleY));
    titleLabel_->setSize(cardWidth, S(kTitleHeight));
    card_->addChild(titleLabel_);

    // One transparent container per wrapped line, rebuilt on Show; labels
    // keep simple top-down coordinates inside it.
    messageArea_ = std::make_shared<UIPanel>();
    messageArea_->setPosition(S(kInset), S(kMessageY));
    messageArea_->setSize(cardWidth - S(kInset) * 2.0f, S(10.0f));
    messageArea_->setBackgroundColor(UITheme::Hex(0x000000, 0.0f));
    card_->addChild(messageArea_);

    const float buttonWidth =
        (cardWidth - S(kInset) * 2.0f - S(kButtonGap)) * 0.5f;

    confirmButton_ = std::make_shared<UIButton>();
    confirmButton_->setFont(DialogFont(engine_, UIFonts::Typeface::Display,
                                       UITheme::Display::Button));
    confirmButton_->setVariant(UIButton::Variant::Primary);
    confirmButton_->setSize(buttonWidth, S(kButtonHeight));
    confirmButton_->setPosition(S(kInset), S(90.0f));
    card_->addChild(confirmButton_);

    cancelButton_ = std::make_shared<UIButton>();
    cancelButton_->setFont(DialogFont(engine_, UIFonts::Typeface::Display,
                                      UITheme::Display::Button));
    cancelButton_->setVariant(UIButton::Variant::Ghost);
    cancelButton_->setSize(buttonWidth, S(kButtonHeight));
    cancelButton_->setPosition(S(kInset) + buttonWidth + S(kButtonGap), S(90.0f));
    card_->addChild(cancelButton_);

    // Close first, then hand the result to the owner, so a callback can
    // immediately open another dialog or change screens.
    confirmButton_->setOnClick([this]() {
        Close();
        if (onConfirmed)
            onConfirmed();
    });

    cancelButton_->setOnClick([this]() {
        Close();
        if (onCancelled)
            onCancelled();
    });
}

void UIConfirmDialog::Show(const std::string& title,
                           const std::string& message,
                           const std::string& confirmLabel,
                           const std::string& cancelLabel)
{
    if (!uiManager_)
        return;

    Build();   // no-op once built

    titleLabel_->setText(title);
    confirmButton_->setText(confirmLabel);
    cancelButton_->setText(cancelLabel);

    layoutMessage(message);

    open_ = true;

    // UIManager renders in insertion order, so re-inserting keeps the dialog
    // above anything added after it was constructed.
    uiManager_->removeElement(root_);
    uiManager_->addElement(root_);

    root_->setVisible(true);
}

void UIConfirmDialog::Close()
{
    if (!root_)
        return;

    open_ = false;
    root_->setVisible(false);
}

bool UIConfirmDialog::HandleEscape()
{
    if (!open_)
        return false;

    Close();

    if (onCancelled)
        onCancelled();

    return true;
}

void UIConfirmDialog::layoutMessage(const std::string& message)
{
    messageArea_->clearChildren();

    Font* font = DialogFont(engine_, UIFonts::Typeface::Body,
                            UITheme::Body::Regular);
    if (!font || !font->IsLoaded())
        return;

    const float maxWidth   = messageArea_->getWidth();
    const float lineHeight = font->GetLineHeight();

    float y = 0.0f;

    for (const auto& line : UILabel::WrapText(*font, message, maxWidth))
    {
        auto label = std::make_shared<UILabel>();
        label->setFont(font);
        label->setText(line);
        label->setTextColor(UITheme::Subtext);
        label->setPosition(0.0f, y);
        label->setSize(maxWidth, lineHeight);
        messageArea_->addChild(label);

        y += lineHeight + S(kLineGap);
    }

    // Grow and re-centre the card for this message, then drop the button row
    // to the bottom of it.
    const float inset       = S(kInset);
    const float cardWidth   = card_->getWidth();
    const float buttonsY    = S(kMessageY) + y + S(kMessageToButtons);
    const float cardHeight  = buttonsY + S(kButtonHeight) + S(kBottomPadding);

    card_->setSize(cardWidth, cardHeight);
    card_->setPosition((UIScale::kDesignWidth - cardWidth) * 0.5f,
                       (UIScale::kDesignHeight - cardHeight) * 0.5f);

    confirmButton_->setPosition(inset, buttonsY);
    cancelButton_->setPosition(inset + confirmButton_->getWidth() + S(kButtonGap),
                               buttonsY);
}
