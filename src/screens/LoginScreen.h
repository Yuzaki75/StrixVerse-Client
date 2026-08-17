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

    // Drives a non-blocking connect started by Submit(). Called every frame
    // from Update() until the socket is up or has failed.
    void UpdateConnect();

    // True while waiting for the socket. The credentials below are held only
    // for that window: nothing is sent to AuthService until there is a
    // connection, and the password is cleared the moment it is handed over.
    bool        connecting_ = false;
    std::string pendingUsername_;
    std::string pendingPassword_;

    // Remembered sign-in. The username only -- never the password. See the
    // note in LoginScreen.cpp for why.
    static std::string RememberedLoginPath();
    static std::string LoadRememberedUsername();
    static void        SaveRememberedUsername(const std::string& username);
    static void        ClearRememberedUsername();
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
