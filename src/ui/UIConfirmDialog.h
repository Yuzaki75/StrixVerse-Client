#pragma once

#include <functional>
#include <memory>
#include <string>

class Engine;
class UIButton;
class UILabel;
class UIManager;
class UIPanel;

// -----------------------------------------------------------------------------
// UIConfirmDialog
//
// A reusable modal confirm/cancel dialog, extracted from PauseOverlay's
// pattern: a full-canvas dim that swallows every click, a centred card, and
// two buttons. Built once against the UIManager, shown and hidden by
// visibility, re-inserted on Show so it draws above anything built after it.
//
// What confirm and cancel do is not decided here - the owner sets
// onConfirmed / onCancelled, because navigation belongs to whoever asked the
// question.
//
// Escape: UIManager routes key presses only to the focused element, and a
// dialog's buttons are not focused by merely opening it, so Escape cannot
// reach this dialog on its own (the same way GameScreen drives PauseOverlay).
// The owner calls HandleEscape() from its own Escape handling first; when it
// returns true the press was consumed by the dialog.
// -----------------------------------------------------------------------------
class UIConfirmDialog
{
public:
    UIConfirmDialog(Engine* engine, UIManager* uiManager);
    ~UIConfirmDialog();

    UIConfirmDialog(const UIConfirmDialog&) = delete;
    UIConfirmDialog& operator=(const UIConfirmDialog&) = delete;

    // Built once and toggled by visibility; rebuilding per show would discard
    // live callbacks for no benefit. No-op after the first call.
    void Build();

    // Opens the dialog with the given copy. The card re-centres and grows to
    // fit the wrapped message. Buttons play no sounds themselves - callers
    // own that, per house convention.
    void Show(const std::string& title,
              const std::string& message,
              const std::string& confirmLabel = "CONFIRM",
              const std::string& cancelLabel  = "CANCEL");

    void Close();
    bool IsOpen() const { return open_; }

    // Cancels and consumes the press when open; false means "not mine" and
    // the owner falls through to its own Escape behaviour.
    bool HandleEscape();

    // Fired after Close() has already run, so a callback may immediately Show
    // again or change screens without fighting the dialog.
    std::function<void()> onConfirmed;
    std::function<void()> onCancelled;

private:
    void BuildFrame();
    void layoutMessage(const std::string& message);

    Engine*    engine_    = nullptr;
    UIManager* uiManager_ = nullptr;

    bool open_ = false;

    // The dim is the root so it swallows every click; the card is a child of
    // it and follows wherever the dim goes.
    std::shared_ptr<UIPanel>  root_;
    std::shared_ptr<UIPanel>  card_;
    std::shared_ptr<UILabel>  titleLabel_;
    std::shared_ptr<UIPanel>  messageArea_;
    std::shared_ptr<UIButton> confirmButton_;
    std::shared_ptr<UIButton> cancelButton_;
};
