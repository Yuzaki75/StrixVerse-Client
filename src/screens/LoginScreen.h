#pragma once

#include <memory>
#include <string>

#include "Screen.h"

class UIButton;
class UICheckBox;
class UILabel;
class UIPanel;
class UITextBox;

/**
 * Login screen.
 *
 * Implements the split layout from the style guide: a branded left column with
 * the live-service stats and the player-flow card, and a crystal panel on the
 * right holding the form.
 *
 * Submitting starts an asynchronous AuthService request; the screen shows the
 * pending state, then either advances to Connecting or reports the failure.
 */
class LoginScreen : public Screen
{
public:
    explicit LoginScreen(Engine* engine);
    ~LoginScreen() override = default;

    void OnEnter() override;
    void Update(float deltaTime) override;
    void OnKeyDown(int key, bool ctrl, bool shift) override;

private:
    void BuildBrandColumn(float columnWidth);
    void BuildForm(float columnX, float columnWidth);

    void Submit();
    void SetStatus(const std::string& message, const Color& color);
    void SetBusy(bool busy);

    std::shared_ptr<UITextBox>  usernameBox_;
    std::shared_ptr<UITextBox>  passwordBox_;
    std::shared_ptr<UICheckBox> rememberBox_;
    std::shared_ptr<UIButton>   loginButton_;
    std::shared_ptr<UIButton>   registerButton_;
    std::shared_ptr<UILabel>    statusLabel_;

    bool submitting_ = false;
};
