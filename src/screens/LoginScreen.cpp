#include "LoginScreen.h"
#include "../core/Logger.h"
#include "../core/Engine.h"
#include "../core/AuthService.h"
#include "../graphics/Color.h"
#include "../core/Window.h"

LoginScreen::LoginScreen(Engine* engine)
    : Screen(engine)
    , m_Panel(nullptr)
    , m_TitleLabel(nullptr)
    , m_UsernameLabel(nullptr)
    , m_UsernameBox(nullptr)
    , m_PasswordLabel(nullptr)
    , m_PasswordBox(nullptr)
    , m_LoginButton(nullptr)
    , m_RegisterButton(nullptr)
    , m_StatusLabel(nullptr)
{
}

void LoginScreen::OnEnter()
{
    if (!engine_)
    {
        LOG_ERROR("LoginScreen: Engine is null");
        return;
    }

    if (!uiManager_)
    {
        LOG_ERROR("LoginScreen: UIManager not available");
        return;
    }

    // Create a panel to hold the login form
    m_Panel = std::make_shared<UIPanel>();
    int width, height;
    engine_->GetWindow()->GetSize(width, height);
    m_Panel->setSize(400.0f, 300.0f);
    m_Panel->setPosition(
        (static_cast<float>(width) - 400.0f) / 2.0f,
        (static_cast<float>(height) - 300.0f) / 2.0f);
    m_Panel->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.7f}); // Semi-transparent dark
    uiManager_->addElement(m_Panel);

    // Title label
    m_TitleLabel = std::make_shared<UILabel>();
    m_TitleLabel->setText("StrixVerse Login");
    m_TitleLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f}); // White
    m_TitleLabel->setFontSize(32.0f);
    m_TitleLabel->setPosition(200.0f, 30.0f); // Centered horizontally in panel (x=200 for width 400)
    m_TitleLabel->setAlignment(UILabel::Alignment::Center);
    m_Panel->addChild(m_TitleLabel);

    // Username label
    m_UsernameLabel = std::make_shared<UILabel>();
    m_UsernameLabel->setText("Username:");
    m_UsernameLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_UsernameLabel->setFontSize(20.0f);
    m_UsernameLabel->setPosition(50.0f, 80.0f);
    m_Panel->addChild(m_UsernameLabel);

    // Username textbox
    m_UsernameBox = std::make_shared<UITextBox>();
    m_UsernameBox->setSize(300.0f, 40.0f);
    m_UsernameBox->setPosition(50.0f, 110.0f);
    m_UsernameBox->setPlaceholderText("Enter username");
    m_UsernameBox->setBackgroundColor({0.2f, 0.2f, 0.2f, 1.0f});
    m_UsernameBox->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_Panel->addChild(m_UsernameBox);

    // Password label
    m_PasswordLabel = std::make_shared<UILabel>();
    m_PasswordLabel->setText("Password:");
    m_PasswordLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_PasswordLabel->setFontSize(20.0f);
    m_PasswordLabel->setPosition(50.0f, 160.0f);
    m_Panel->addChild(m_PasswordLabel);

    // Password textbox
    m_PasswordBox = std::make_shared<UITextBox>();
    m_PasswordBox->setSize(300.0f, 40.0f);
    m_PasswordBox->setPosition(50.0f, 190.0f);
    m_PasswordBox->setPlaceholderText("Enter password");
    m_PasswordBox->setPasswordMode(true);
    m_PasswordBox->setBackgroundColor({0.2f, 0.2f, 0.2f, 1.0f});
    m_PasswordBox->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_Panel->addChild(m_PasswordBox);

    // Login button
    m_LoginButton = std::make_shared<UIButton>();
    m_LoginButton->setSize(140.0f, 50.0f);
    m_LoginButton->setPosition(50.0f, 240.0f);
    m_LoginButton->setText("Login");
    m_LoginButton->setBackgroundColor({0.0f, 0.6f, 0.0f, 1.0f}); // Green
    m_LoginButton->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_LoginButton->setOnClickCallback([this]() { this->OnLoginButtonClicked(); });
    m_Panel->addChild(m_LoginButton);

    // Register button
    m_RegisterButton = std::make_shared<UIButton>();
    m_RegisterButton->setSize(140.0f, 50.0f);
    m_RegisterButton->setPosition(210.0f, 240.0f);
    m_RegisterButton->setText("Register");
    m_RegisterButton->setBackgroundColor({0.0f, 0.0f, 0.6f, 1.0f}); // Blue
    m_RegisterButton->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_RegisterButton->setOnClickCallback([this]() { this->OnRegisterButtonClicked(); });
    m_Panel->addChild(m_RegisterButton);

    // Status label
    m_StatusLabel = std::make_shared<UILabel>();
    m_StatusLabel->setText("");
    m_StatusLabel->setTextColor({1.0f, 0.3f, 0.3f, 1.0f}); // Light red for errors
    m_StatusLabel->setFontSize(18.0f);
    m_StatusLabel->setPosition(200.0f, 290.0f);
    m_StatusLabel->setAlignment(UILabel::Alignment::Center);
    m_Panel->addChild(m_StatusLabel);
}

void LoginScreen::OnExit()
{
    if (uiManager_ && m_Panel)
    {
        uiManager_->removeElement(m_Panel);
        m_Panel.reset();
        // Children are shared_ptr, so they will be cleaned up when the panel is removed.
        m_TitleLabel.reset();
        m_UsernameLabel.reset();
        m_UsernameBox.reset();
        m_PasswordLabel.reset();
        m_PasswordBox.reset();
        m_LoginButton.reset();
        m_RegisterButton.reset();
        m_StatusLabel.reset();
    }
}

void LoginScreen::Update(float deltaTime)
{
    // Update text box values
    if (m_UsernameBox)
    {
        m_Username = m_UsernameBox->getText();
    }
    if (m_PasswordBox)
    {
        m_Password = m_PasswordBox->getText();
    }
}

void LoginScreen::Render() const
{
    // Nothing to render here; UIManager renders the UI elements.
}

void LoginScreen::OnLoginButtonClicked()
{
    if (!engine_)
        return;

    AuthService* authService = engine_->GetAuthService();
    if (!authService)
    {
        m_StatusMessage = "Auth service not available";
        if (m_StatusLabel)
            m_StatusLabel->setText(m_StatusMessage);
        return;
    }

    if (m_Username.empty() || m_Password.empty())
    {
        m_StatusMessage = "Please enter username and password";
        if (m_StatusLabel)
            m_StatusLabel->setText(m_StatusMessage);
        return;
    }

    bool success = authService->Login(m_Username, m_Password);
    if (success)
    {
        m_StatusMessage = "Login successful!";
        if (m_StatusLabel)
            m_StatusLabel->setText(m_StatusMessage);
        // TODO: After successful login, we should check for a saved world and go to Continue or WorldBrowser
        // For now, we'll just go to WorldBrowser
        RequestScreenChange(ScreenID::WorldBrowser);
    }
    else
    {
        m_StatusMessage = "Login failed. Invalid credentials.";
        if (m_StatusLabel)
            m_StatusLabel->setText(m_StatusMessage);
    }
}

void LoginScreen::OnRegisterButtonClicked()
{
    RequestScreenChange(ScreenID::Register);
}