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
 * Register screen.
 *
 * Implements the style guide's split layout: the purple-accented
 * "YOUR LOOK = YOUR JOURNEY" notice on the left, and the four-field account
 * form on the right.
 *
 * Validation runs per field before the request is sent, and the asynchronous
 * AuthService result drives the success and failure states.
 */
class RegisterScreen : public Screen
{
public:
    explicit RegisterScreen(Engine* engine);
    ~RegisterScreen() override = default;

    void OnEnter() override;
    void Update(float deltaTime) override;
    void OnKeyDown(int key, bool ctrl, bool shift) override;

private:
    void BuildBrandColumn(float columnWidth);
    void BuildForm(float columnX, float columnWidth);

    // Returns an empty string when the form is valid, otherwise the message to
    // show and the field to focus.
    std::string Validate(std::shared_ptr<UITextBox>& fieldToFocus) const;

    void Submit();
    // Refreshes the pre-submit hint line as the player types.
    void UpdateInlineValidation();
    void SetStatus(const std::string& message, const Color& color);
    void SetBusy(bool busy);

    std::shared_ptr<UITextBox>  usernameBox_;
    std::shared_ptr<UITextBox>  emailBox_;
    std::shared_ptr<UITextBox>  passwordBox_;
    std::shared_ptr<UITextBox>  confirmBox_;
    std::shared_ptr<UICheckBox> termsBox_;
    std::shared_ptr<UIButton>   createButton_;
    std::shared_ptr<UILabel>    statusLabel_;
    std::shared_ptr<UILabel>    signInLink_;

    bool submitting_ = false;
};
