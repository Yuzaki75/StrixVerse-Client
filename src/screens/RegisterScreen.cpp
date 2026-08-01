#include "RegisterScreen.h"
#include "../core/Logger.h"
#include "../core/Engine.h"
#include "../core/AuthService.h"
#include "../graphics/Color.h"
#include "../core/Window.h"

RegisterScreen::RegisterScreen(Engine* engine)
    : Screen(engine)
    , m_Panel(nullptr)
    , m_TitleLabel(nullptr)
    , m_UsernameLabel(nullptr)
    , m_UsernameBox(nullptr)
    , m_PasswordLabel(nullptr)
    , m_PasswordBox(nullptr)
    , m_EmailLabel(nullptr)
    , m_EmailBox(nullptr)
    , m_RegisterButton(nullptr)
    , m_BackButton(nullptr)
    , m_StatusLabel(nullptr)
{
}

void RegisterScreen::OnEnter()
{
    if (!engine_)
    {
        LOG_ERROR("RegisterScreen: Engine is null");
        return;
    }

    if (!uiManager_)
    {
        LOG_ERROR("RegisterScreen: UIManager not available");
        return;
    }

    // Create a panel to hold the registration form
    m_Panel = std::make_shared<UIPanel>();
    int width, height;
    engine_->GetWindow()->GetSize(width, height);
    m_Panel->setSize(400.0f, 350.0f);
    m_Panel->setPosition(
        (static_cast<float>(width) - 400.0f) / 2.0f,
        (static_cast<float>(height) - 350.0f) / 2.0f);
    m_Panel->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.8f}); // semi-transparent dark

    // Title
    m_TitleLabel = std::make_shared<UILabel>();
    m_TitleLabel->setText("Create Account");
    m_TitleLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_TitleLabel->setFontSize(24.0f);
    m_TitleLabel->setPosition(200.0f, 30.0f); // relative to panel? We'll set position relative to panel later? Actually UILabel position is absolute? We'll set relative to panel by adding to panel and then setting position relative to panel? The UI system likely uses absolute positions. We'll set the panel's position and then position children relative to the panel's top-left? Actually, the UIElement's position is in screen space. We'll compute based on panel position.
    // Let's set the panel at (panelX, panelY) and then place children at (panelX + offsetX, panelY + offsetY).
    float panelX = m_Panel->getPosition().x;
    float panelY = m_Panel->getPosition().y;

    m_TitleLabel->setPosition(panelX + 200.0f, panelY + 30.0f);

    // Username
    m_UsernameLabel = std::make_shared<UILabel>();
    m_UsernameLabel->setText("Username:");
    m_UsernameLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_UsernameLabel->setFontSize(18.0f);
    m_UsernameLabel->setPosition(panelX + 50.0f, panelY + 80.0f);

    m_UsernameBox = std::make_shared<UITextBox>();
    m_UsernameBox->setSize(300.0f, 30.0f);
    m_UsernameBox->setPosition(panelX + 50.0f, panelY + 110.0f);
    m_UsernameBox->setBackgroundColor({0.2f, 0.2f, 0.2f, 1.0f});
    m_UsernameBox->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});

    // Password
    m_PasswordLabel = std::make_shared<UILabel>();
    m_PasswordLabel->setText("Password:");
    m_PasswordLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_PasswordLabel->setFontSize(18.0f);
    m_PasswordLabel->setPosition(panelX + 50.0f, panelY + 150.0f);

    m_PasswordBox = std::make_shared<UITextBox>();
    m_PasswordBox->setSize(300.0f, 30.0f);
    m_PasswordBox->setPosition(panelX + 50.0f, panelY + 180.0f);
    m_PasswordBox->setBackgroundColor({0.2f, 0.2f, 0.2f, 1.0f});
    m_PasswordBox->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_PasswordBox->setPasswordMode(true); // assuming UITextBox has a password mode

    // Email
    m_EmailLabel = std::make_shared<UILabel>();
    m_EmailLabel->setText("Email:");
    m_EmailLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_EmailLabel->setFontSize(18.0f);
    m_EmailLabel->setPosition(panelX + 50.0f, panelY + 220.0f);

    m_EmailBox = std::make_shared<UITextBox>();
    m_EmailBox->setSize(300.0f, 30.0f);
    m_EmailBox->setPosition(panelX + 50.0f, panelY + 250.0f);
    m_EmailBox->setBackgroundColor({0.2f, 0.2f, 0.2f, 1.0f});
    m_EmailBox->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});

    // Register button
    m_RegisterButton = std::make_shared<UIButton>();
    m_RegisterButton->setSize(100.0f, 40.0f);
    m_RegisterButton->setPosition(panelX + 150.0f, panelY + 300.0f);
    m_RegisterButton->setText("Register");
    m_RegisterButton->setBackgroundColor({0.0f, 0.5f, 0.0f, 1.0f});
    m_RegisterButton->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_RegisterButton->setOnClickCallback([this]() { OnRegisterButtonClicked(); });

    // Back button
    m_BackButton = std::make_shared<UIButton>();
    m_BackButton->setSize(100.0f, 40.0f);
    m_BackButton->setPosition(panelX + 260.0f, panelY + 300.0f);
    m_BackButton->setText("Back");
    m_BackButton->setBackgroundColor({0.5f, 0.0f, 0.0f, 1.0f});
    m_BackButton->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_BackButton->setOnClickCallback([this]() { OnBackButtonClicked(); });

    // Status label
    m_StatusLabel = std::make_shared<UILabel>();
    m_StatusLabel->setText("");
    m_StatusLabel->setTextColor({1.0f, 0.0f, 0.0f, 1.0f}); // red for error
    m_StatusLabel->setFontSize(16.0f);
    m_StatusLabel->setPosition(panelX + 200.0f, panelY + 350.0f);

    // Add all elements to UIManager
    uiManager_->addElement(m_Panel);
    uiManager_->addElement(m_TitleLabel);
    uiManager_->addElement(m_UsernameLabel);
    uiManager_->addElement(m_UsernameBox);
    uiManager_->addElement(m_PasswordLabel);
    uiManager_->addElement(m_PasswordBox);
    uiManager_->addElement(m_EmailLabel);
    uiManager_->addElement(m_EmailBox);
    uiManager_->addElement(m_RegisterButton);
    uiManager_->addElement(m_BackButton);
    uiManager_->addElement(m_StatusLabel);
}

void RegisterScreen::OnExit()
{
    if (!uiManager_) return;

    uiManager_->removeElement(m_Panel);
    uiManager_->removeElement(m_TitleLabel);
    uiManager_->removeElement(m_UsernameLabel);
    uiManager_->removeElement(m_UsernameBox);
    uiManager_->removeElement(m_PasswordLabel);
    uiManager_->removeElement(m_PasswordBox);
    uiManager_->removeElement(m_EmailLabel);
    uiManager_->removeElement(m_EmailBox);
    uiManager_->removeElement(m_RegisterButton);
    uiManager_->removeElement(m_BackButton);
    uiManager_->removeElement(m_StatusLabel);

    // Reset pointers
    m_Panel.reset();
    m_TitleLabel.reset();
    m_UsernameLabel.reset();
    m_UsernameBox.reset();
    m_PasswordLabel.reset();
    m_PasswordBox.reset();
    m_EmailLabel.reset();
    m_EmailBox.reset();
    m_RegisterButton.reset();
    m_BackButton.reset();
    m_StatusLabel.reset();
}

void RegisterScreen::Update(float deltaTime)
{
    // Update text from UI elements
    if (m_UsernameBox) m_Username = m_UsernameBox->getText();
    if (m_PasswordBox) m_Password = m_PasswordBox->getText();
    if (m_EmailBox) m_Email = m_EmailBox->getText();
}

void RegisterScreen::Render() const
{
    // Rendering is done by UIManager
}

void RegisterScreen::OnRegisterButtonClicked()
{
    // Validate input
    if (m_Username.empty() || m_Password.empty() || m_Email.empty())
    {
        m_StatusMessage = "All fields are required";
        if (m_StatusLabel) m_StatusLabel->setText(m_StatusMessage);
        return;
    }

    // Attempt registration via AuthService
    if (engine_)
    {
        auto authService = engine_->GetAuthService();
        if (authService && authService->Register(m_Username, m_Password, m_Email))
        {
            m_StatusMessage = "Registration successful!";
            if (m_StatusLabel) m_StatusLabel->setText(m_StatusMessage);
            m_StatusLabel->setTextColor({0.0f, 1.0f, 0.0f, 1.0f}); // green for success
            // After successful registration, go to login screen
            RequestScreenChange(ScreenID::Login);
        }
        else
        {
            m_StatusMessage = "Registration failed";
            if (m_StatusLabel) m_StatusLabel->setText(m_StatusMessage);
        }
    }
}

void RegisterScreen::OnBackButtonClicked()
{
    RequestScreenChange(ScreenID::Login);
}