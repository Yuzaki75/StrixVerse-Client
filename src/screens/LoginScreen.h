#pragma once

#include "Screen.h"
#include "../ui/UIPanel.h"
#include "../ui/UILabel.h"
#include "../ui/UITextBox.h"
#include "../ui/UIButton.h"

/**
 * Login screen for entering username and password.
 */
class LoginScreen : public Screen
{
public:
    LoginScreen(Engine* engine);
    ~LoginScreen() override = default;

    // Screen overrides
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;
    void Render() const override;

private:
    // UI elements
    std::shared_ptr<UIPanel> m_Panel;
    std::shared_ptr<UILabel> m_TitleLabel;
    std::shared_ptr<UILabel> m_UsernameLabel;
    std::shared_ptr<UITextBox> m_UsernameBox;
    std::shared_ptr<UILabel> m_PasswordLabel;
    std::shared_ptr<UITextBox> m_PasswordBox;
    std::shared_ptr<UIButton> m_LoginButton;
    std::shared_ptr<UIButton> m_RegisterButton;
    std::shared_ptr<UILabel> m_StatusLabel;

    // UI state
    std::string m_Username;
    std::string m_Password;
    std::string m_StatusMessage;

    // Helper methods
    void OnLoginButtonClicked();
    void OnRegisterButtonClicked();
};