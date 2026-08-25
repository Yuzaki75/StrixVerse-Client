#include "LoginScreen.h"

#include "../core/AssetManager.h"
#include "../core/AuthService.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../ui/UIButton.h"
#include "../ui/UIPatterns.h"
#include "../ui/UITiledImage.h"
#include "../ui/UICheckBox.h"
#include "../ui/UILabel.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITextBox.h"
#include "../ui/UITheme.h"

#include <array>
#include <filesystem>
#include <fstream>

namespace
{
    // Converts a style-guide pixel value onto the 1920x1080 design canvas.
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    // The style guide's login screen (App.tsx 3119-3177).
    constexpr float kFormColumnWidth = S(304.0f);
    constexpr float kColumnPadding   = S(24.0f);
    constexpr float kPanelPadding    = S(24.0f);

    constexpr float kFieldHeight  = S(36.0f);
    constexpr float kLabelHeight  = S(9.0f);
    constexpr float kLabelGap     = S(5.0f);
    constexpr float kFieldGap     = S(12.0f);
    constexpr float kButtonHeight = S(31.0f);
    constexpr float kSmallButton  = S(28.0f);
    constexpr float kRowHeight    = S(16.0f);

    struct Stat
    {
        const char* value;
        const char* label;
    };

    constexpr std::array<Stat, 3> kStats = {{
        {"18K+", "Online"},
        {"500+", "Worlds"},
        {"1M+",  "Players"},
    }};

    constexpr std::array<const char*, 5> kFlowSteps = {
        "Login",
        "Authenticating",
        "Connecting",
        "Continue / World Select",
        "Spawn into World",
    };
}

LoginScreen::LoginScreen(Engine* engine)
    : Screen(engine)
{
}

void LoginScreen::OnEnter()
{
    if (!uiManager_)
    {
        LOG_ERROR("LoginScreen: UIManager not available");
        return;
    }

    submitting_ = false;

    if (AuthService* auth = engine_ ? engine_->GetAuthService() : nullptr)
        auth->ResetRequest();

    CreateRoot();

    // linear-gradient(135deg, #090e1a, #101830)
    AddBackdrop(UITheme::Hex(0x090E1A), UITheme::Hex(0x101830), false);

    const float originX = DesignOriginX();
    const float brandWidth = UIScale::kDesignWidth - kFormColumnWidth;

    BuildBrandColumn(brandWidth);
    BuildForm(originX + brandWidth, kFormColumnWidth);
}

void LoginScreen::BuildBrandColumn(float columnWidth)
{
    const float originX = DesignOriginX();
    const float originY = DesignOriginY();

    // The pixel-grid lattice only covers the brand column, matching the design.
    auto column = std::make_shared<UIPanel>();
    column->setBackgroundColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    column->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    column->setBorderRadius(0.0f);
    column->setPosition(originX, originY);
    column->setSize(columnWidth, UIScale::kDesignHeight);
    root_->addChild(column);

    // ".sv-pixel-grid" covers the brand column only, as in the design.
    if (AssetManager* assets = Assets())
    {
        if (auto grid = UIPatterns::GetPixelGrid(*assets))
        {
            auto lattice = std::make_shared<UITiledImage>();
            lattice->setTexture(std::move(grid));
            lattice->setTileSize(UIPatterns::kPixelGridTileSize);
            lattice->setColor(UITheme::Hex(0x3A4060, 0.55f));
            lattice->setPosition(0.0f, 0.0f);
            lattice->setSize(columnWidth, UIScale::kDesignHeight);
            column->addChild(lattice);
        }
    }

    // Heights are laid out top-down so the whole group ends up centred.
    constexpr float titleHeight   = S(32.0f);
    constexpr float subtitleHeight= S(12.0f);
    constexpr float statsHeight   = S(34.0f);
    constexpr float cardWidth     = S(280.0f);
    constexpr float stepHeight    = S(16.0f);
    constexpr float stepGap       = S(4.0f);
    constexpr float cardPaddingY  = S(12.0f);
    constexpr float cardPaddingX  = S(16.0f);
    constexpr float cardTitle     = S(9.0f);

    const float cardHeight = cardPaddingY * 2.0f + cardTitle + S(8.0f) +
                             stepHeight * static_cast<float>(kFlowSteps.size()) +
                             stepGap * static_cast<float>(kFlowSteps.size() - 1);

    const float totalHeight = titleHeight + S(18.0f) + subtitleHeight + S(18.0f) +
                              statsHeight + S(12.0f) + cardHeight;

    float y = (UIScale::kDesignHeight - totalHeight) * 0.5f;

    // "STRIXVERSE" - Press Start 2P 26px with the crystal glow.
    auto title = std::make_shared<UILabel>();
    title->setText("STRIXVERSE");
    title->setFont(DisplayFont(UITheme::Display::Hero));
    title->setTextColor(UITheme::Text);
    title->setAlignment(UILabel::Alignment::Center);
    title->setShadow(Color(0.0f, 0.0f, 0.0f, 1.0f), S(3.0f), S(3.0f));
    title->setGlow(UITheme::WithAlpha(UITheme::Accent, 0.80f), S(12.0f));
    title->setPosition(0.0f, y);
    title->setSize(columnWidth, titleHeight);
    column->addChild(title);
    y += titleHeight + S(18.0f);

    auto subtitle = std::make_shared<UILabel>();
    subtitle->setText("CRYSTAL \xC2\xB7 FANTASY \xC2\xB7 MMO");
    subtitle->setFont(DisplayFont(UITheme::Display::Label));
    subtitle->setTextColor(UITheme::Accent);
    subtitle->setLetterSpacing(S(8.0f * 0.14f));
    subtitle->setAlignment(UILabel::Alignment::Center);
    subtitle->setPosition(0.0f, y);
    subtitle->setSize(columnWidth, subtitleHeight);
    column->addChild(subtitle);
    y += subtitleHeight + S(18.0f);

    // Live-service stats row.
    {
        constexpr float itemWidth = S(64.0f);
        constexpr float itemGap   = S(28.0f);

        const float rowWidth = itemWidth * static_cast<float>(kStats.size()) +
                               itemGap * static_cast<float>(kStats.size() - 1);
        float itemX = (columnWidth - rowWidth) * 0.5f;

        for (const Stat& stat : kStats)
        {
            auto value = std::make_shared<UILabel>();
            value->setText(stat.value);
            value->setFont(DisplayFont(UITheme::Display::Heading));
            value->setTextColor(UITheme::Accent);
            value->setAlignment(UILabel::Alignment::Center);
            value->setPosition(itemX, y);
            value->setSize(itemWidth, S(15.0f));
            column->addChild(value);

            auto label = std::make_shared<UILabel>();
            label->setText(stat.label);
            label->setFont(BodyFont(UITheme::Body::Welcome));
            label->setTextColor(UITheme::Subtext);
            label->setAlignment(UILabel::Alignment::Center);
            label->setPosition(itemX, y + S(17.0f));
            label->setSize(itemWidth, S(16.0f));
            column->addChild(label);

            itemX += itemWidth + itemGap;
        }
    }
    y += statsHeight + S(12.0f);

    // "PLAYER FLOW" card.
    auto card = std::make_shared<UIPanel>();
    card->setBackgroundColor(UITheme::Hex(0x2C3145, 0.60f));
    card->setBorder(UITheme::WithAlpha(UITheme::Border, 0.25f), UITheme::BorderThin);
    card->setBorderRadius(S(8.0f));
    card->setPosition((columnWidth - cardWidth) * 0.5f, y);
    card->setSize(cardWidth, cardHeight);
    column->addChild(card);

    auto cardTitleLabel = std::make_shared<UILabel>();
    cardTitleLabel->setText("PLAYER FLOW");
    cardTitleLabel->setFont(DisplayFont(UITheme::Display::Small));
    cardTitleLabel->setTextColor(UITheme::Accent);
    cardTitleLabel->setPosition(cardPaddingX, cardPaddingY);
    cardTitleLabel->setSize(cardWidth - cardPaddingX * 2.0f, cardTitle);
    card->addChild(cardTitleLabel);

    float stepY = cardPaddingY + cardTitle + S(8.0f);

    for (size_t i = 0; i < kFlowSteps.size(); ++i)
    {
        auto badge = std::make_shared<UIPanel>();
        badge->setBackgroundColor(UITheme::WithAlpha(UITheme::Primary, 0.20f));
        badge->setBorder(UITheme::WithAlpha(UITheme::Primary, 0.40f), UITheme::BorderThin);
        badge->setBorderRadius(S(4.0f));
        badge->setPosition(cardPaddingX, stepY);
        badge->setSize(stepHeight, stepHeight);
        card->addChild(badge);

        auto index = std::make_shared<UILabel>();
        index->setText(std::to_string(i + 1));
        index->setFont(DisplayFont(UITheme::Display::Tiny));
        index->setTextColor(UITheme::Accent);
        index->setAlignment(UILabel::Alignment::Center);
        index->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
        index->setPosition(0.0f, 0.0f);
        index->setSize(stepHeight, stepHeight);
        badge->addChild(index);

        auto text = std::make_shared<UILabel>();
        text->setText(kFlowSteps[i]);
        text->setFont(BodyFont(UITheme::Body::Welcome));
        text->setTextColor(UITheme::Subtext);
        text->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
        text->setPosition(cardPaddingX + stepHeight + S(8.0f), stepY);
        text->setSize(cardWidth - cardPaddingX * 2.0f - stepHeight - S(8.0f), stepHeight);
        card->addChild(text);

        stepY += stepHeight + stepGap;
    }
}

void LoginScreen::BuildForm(float columnX, float columnWidth)
{
    const float originY = DesignOriginY();

    // The design separates the form column with a hairline border.
    auto divider = std::make_shared<UIPanel>();
    divider->setBackgroundColor(UITheme::WithAlpha(UITheme::Border, 0.22f));
    divider->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    divider->setBorderRadius(0.0f);
    divider->setPosition(columnX, originY);
    divider->setSize(UITheme::BorderThin, UIScale::kDesignHeight);
    root_->addChild(divider);

    const float panelWidth   = columnWidth - kColumnPadding * 2.0f;
    const float contentWidth = panelWidth - kPanelPadding * 2.0f;

    // Stack the contents to find the panel height, then centre it.
    const float contentHeight =
        S(13.0f) + S(20.0f) +                                    // title + gap
        (kLabelHeight + kLabelGap + kFieldHeight) + kFieldGap +   // username
        (kLabelHeight + kLabelGap + kFieldHeight) + kFieldGap +   // password
        kRowHeight + kFieldGap +                                  // options row
        kButtonHeight + kFieldGap +                               // enter world
        S(14.0f) + kFieldGap +                                    // divider row
        kSmallButton + S(10.0f) + S(14.0f);                       // create + status

    const float panelHeight = contentHeight + kPanelPadding * 2.0f;

    auto panel = std::make_shared<UIPanel>();
    panel->setBackgroundColor(UITheme::Panel);
    panel->setBorder(UITheme::PanelBorder, UITheme::BorderThin);
    panel->setBorderRadius(UITheme::RadiusPanel);
    panel->setGlow(Color(0.0f, 0.0f, 0.0f, 0.55f), S(16.0f));
    panel->setPosition(columnX + kColumnPadding,
                       originY + (UIScale::kDesignHeight - panelHeight) * 0.5f);
    panel->setSize(panelWidth, panelHeight);
    root_->addChild(panel);

    float y = kPanelPadding;
    const float x = kPanelPadding;

    auto heading = std::make_shared<UILabel>();
    heading->setText("LOGIN");
    heading->setFont(DisplayFont(UITheme::Display::Tagline));
    heading->setTextColor(UITheme::Text);
    heading->setAlignment(UILabel::Alignment::Center);
    heading->setPosition(x, y);
    heading->setSize(contentWidth, S(13.0f));
    panel->addChild(heading);
    y += S(13.0f) + S(20.0f);

    // Builds one labelled ".sv-input" field and advances the cursor.
    auto addField = [&](const char* caption, bool password) -> std::shared_ptr<UITextBox>
    {
        auto label = std::make_shared<UILabel>();
        label->setText(caption);
        label->setFont(DisplayFont(UITheme::Display::Small));
        label->setTextColor(UITheme::Subtext);
        label->setPosition(x, y);
        label->setSize(contentWidth, kLabelHeight);
        panel->addChild(label);

        auto field = std::make_shared<UITextBox>();
        field->setFont(BodyFont(UITheme::Body::Input));
        field->setPasswordMode(password);
        field->setMaxLength(password ? 64 : 32);
        field->setPosition(x, y + kLabelHeight + kLabelGap);
        field->setSize(contentWidth, kFieldHeight);
        field->setOnEnterPressed([this]() { Submit(); });
        panel->addChild(field);

        y += kLabelHeight + kLabelGap + kFieldHeight + kFieldGap;
        return field;
    };

    usernameBox_ = addField("USERNAME OR EMAIL", false);
    usernameBox_->setPlaceholderText("CrystalMage_42");

    passwordBox_ = addField("PASSWORD", true);
    passwordBox_->setPlaceholderText("Min 8 characters");

    // Remember me / forgot password row. The authenticator-code field shares
    // this exact slot when the server asks for a code - see RevealTotpField()
    // for why it swaps rather than relays the panel out.
    totpBox_ = std::make_shared<UITextBox>();
    totpBox_->setPlaceholderText("6-digit authenticator code");
    totpBox_->setFont(BodyFont(UITheme::Body::Caption));
    totpBox_->setPosition(x, y);
    totpBox_->setSize(contentWidth, kRowHeight);
    totpBox_->setMaxLength(6);
    totpBox_->setVisible(false);
    panel->addChild(totpBox_);

    rememberBox_ = std::make_shared<UICheckBox>();
    rememberBox_->setLabel("Remember me");
    rememberBox_->setFont(BodyFont(UITheme::Body::Caption));
    rememberBox_->setChecked(true);
    rememberBox_->setAccentColor(UITheme::Primary);
    rememberBox_->setPosition(x, y);
    rememberBox_->setSize(contentWidth * 0.6f, kRowHeight);
    panel->addChild(rememberBox_);

    forgotLabel_ = std::make_shared<UILabel>();
    forgotLabel_->setText("Forgot password?");
    forgotLabel_->setFont(BodyFont(UITheme::Body::Caption));
    forgotLabel_->setTextColor(UITheme::Accent);
    forgotLabel_->setAlignment(UILabel::Alignment::Right);
    forgotLabel_->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    forgotLabel_->setPosition(x + contentWidth * 0.6f, y);
    forgotLabel_->setSize(contentWidth * 0.4f, kRowHeight);
    panel->addChild(forgotLabel_);

    y += kRowHeight + kFieldGap;

    loginButton_ = std::make_shared<UIButton>();
    loginButton_->setText("ENTER WORLD");
    loginButton_->setFont(DisplayFont(UITheme::Display::Button));
    loginButton_->setVariant(UIButton::Variant::Primary);
    loginButton_->setPosition(x, y);
    loginButton_->setSize(contentWidth, kButtonHeight);
    loginButton_->setOnClick([this]()
                             {
                                 if (engine_)
                                     engine_->GetAudio().PlaySfx("ui_click");
                                 Submit();
                             });
    panel->addChild(loginButton_);

    y += kButtonHeight + kFieldGap;

    // "new player?" separator.
    {
        const float caption = S(60.0f);
        const float lineWidth = (contentWidth - caption) * 0.5f;
        const float lineY = y + S(7.0f);

        auto left = std::make_shared<UIPanel>();
        left->setBackgroundColor(UITheme::WithAlpha(UITheme::Border, 0.25f));
        left->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
        left->setBorderRadius(0.0f);
        left->setPosition(x, lineY);
        left->setSize(lineWidth, UITheme::BorderThin);
        panel->addChild(left);

        auto caption_label = std::make_shared<UILabel>();
        caption_label->setText("new player?");
        caption_label->setFont(BodyFont(UITheme::Body::Tiny));
        caption_label->setTextColor(UITheme::Muted);
        caption_label->setAlignment(UILabel::Alignment::Center);
        caption_label->setPosition(x + lineWidth, y);
        caption_label->setSize(caption, S(14.0f));
        panel->addChild(caption_label);

        auto right = std::make_shared<UIPanel>();
        right->setBackgroundColor(UITheme::WithAlpha(UITheme::Border, 0.25f));
        right->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
        right->setBorderRadius(0.0f);
        right->setPosition(x + lineWidth + caption, lineY);
        right->setSize(lineWidth, UITheme::BorderThin);
        panel->addChild(right);
    }

    y += S(14.0f) + kFieldGap;

    registerButton_ = std::make_shared<UIButton>();
    registerButton_->setText("CREATE ACCOUNT");
    registerButton_->setFont(DisplayFont(UITheme::Display::Label));
    registerButton_->setVariant(UIButton::Variant::Purple);
    registerButton_->setPosition(x, y);
    registerButton_->setSize(contentWidth, kSmallButton);
    registerButton_->setOnClick([this]()
                                {
                                    if (!submitting_)
                                    {
                                        if (engine_)
                                            engine_->GetAudio().PlaySfx("ui_click");
                                        RequestScreenChange(ScreenID::Register);
                                    }
                                });
    panel->addChild(registerButton_);

    y += kSmallButton + S(10.0f);

    statusLabel_ = std::make_shared<UILabel>();
    statusLabel_->setFont(BodyFont(UITheme::Body::Caption));
    statusLabel_->setTextColor(UITheme::Danger);
    statusLabel_->setAlignment(UILabel::Alignment::Center);
    statusLabel_->setPosition(x, y);
    statusLabel_->setSize(contentWidth, S(14.0f));
    panel->addChild(statusLabel_);

    // Restore the remembered sign-in, if there is one. Only the username comes
    // back; the password is never stored, so focus goes to the password field
    // rather than the username field when one is restored.
    const std::string remembered = LoadRememberedUsername();
    if (!remembered.empty() && usernameBox_)
    {
        usernameBox_->setText(remembered);

        // Clicking the field means the player wants a different account, so
        // the restored name is replaced rather than typed over.
        usernameBox_->setClearOnNextFocus(true);

        if (rememberBox_)
            rememberBox_->setChecked(true);

        if (uiManager_ && passwordBox_)
            uiManager_->setFocusedElement(passwordBox_);

        return;
    }

    // Start with the username field ready for typing.
    if (uiManager_ && usernameBox_)
        uiManager_->setFocusedElement(usernameBox_);
}

// -----------------------------------------------------------------------------
// Remembered sign-in
// -----------------------------------------------------------------------------
// The username only. Storing the password would mean writing a credential to
// disk in plaintext next to the executable, which is worse than the typing it
// saves -- a "stay signed in" that skips the password entirely wants the
// server's session token, not the password itself.

std::string LoginScreen::RememberedLoginPath()
{
    return "saves/remembered_login.txt";
}

std::string LoginScreen::LoadRememberedUsername()
{
    std::ifstream file(RememberedLoginPath());
    if (!file.is_open())
        return {};

    std::string line;
    while (std::getline(file, line))
    {
        constexpr const char* kKey = "username=";
        constexpr std::size_t kKeyLength = 9;

        if (line.rfind(kKey, 0) == 0)
        {
            std::string value = line.substr(kKeyLength);

            // Written on Windows, so a stray carriage return is likely.
            while (!value.empty() && (value.back() == '\r' || value.back() == '\n'))
                value.pop_back();

            return value;
        }
    }

    return {};
}

void LoginScreen::SaveRememberedUsername(const std::string& username)
{
    std::error_code ec;
    std::filesystem::create_directories("saves", ec);
    if (ec)
    {
        LOG_WARN("LoginScreen: could not create the saves directory; sign-in will not be remembered");
        return;
    }

    std::ofstream file(RememberedLoginPath(), std::ios::trunc);
    if (!file.is_open())
    {
        LOG_WARN("LoginScreen: could not write the remembered sign-in");
        return;
    }

    file << "username=" << username << '\n';
}

void LoginScreen::ClearRememberedUsername()
{
    std::error_code ec;
    std::filesystem::remove(RememberedLoginPath(), ec);
}

void LoginScreen::RevealTotpField()
{
    if (totpRequested_)
        return;
    totpRequested_ = true;

    // The code field takes the remember-me row's slot. Remembering the name
    // was already decided by the first submit's checkbox state, and choosing
    // between two fields mid-login is noise - so the row swaps rather than
    // the panel relaying every element below it down a row.
    if (rememberBox_) rememberBox_->setVisible(false);
    if (forgotLabel_) forgotLabel_->setVisible(false);
    if (totpBox_)
    {
        totpBox_->setVisible(true);
        if (uiManager_)
            uiManager_->setFocusedElement(totpBox_);
    }
}

void LoginScreen::SetStatus(const std::string& message, const Color& color)
{
    if (!statusLabel_)
        return;

    statusLabel_->setText(message);
    statusLabel_->setTextColor(color);
}

void LoginScreen::SetBusy(bool busy)
{
    submitting_ = busy;

    if (loginButton_)
    {
        loginButton_->setEnabled(!busy);
        loginButton_->setText(busy ? "AUTHENTICATING" : "ENTER WORLD");
    }

    if (registerButton_)
        registerButton_->setEnabled(!busy);

    if (usernameBox_)
        usernameBox_->setEnabled(!busy);

    if (passwordBox_)
        passwordBox_->setEnabled(!busy);

    if (totpBox_)
        totpBox_->setEnabled(!busy);
}

void LoginScreen::Submit()
{
    if (submitting_)
        return;

    const std::string username = usernameBox_ ? usernameBox_->getText() : std::string();
    const std::string password = passwordBox_ ? passwordBox_->getText() : std::string();
    const std::string totpCode = (totpRequested_ && totpBox_) ? totpBox_->getText()
                                                              : std::string();

    if (username.empty())
    {
        SetStatus("Enter your username or email.", UITheme::Danger);
        if (uiManager_ && usernameBox_)
            uiManager_->setFocusedElement(usernameBox_);
        return;
    }

    if (password.empty())
    {
        SetStatus("Enter your password.", UITheme::Danger);
        if (uiManager_ && passwordBox_)
            uiManager_->setFocusedElement(passwordBox_);
        return;
    }

    // Only demanded once the server has said the account needs one, so a
    // player without 2FA never sees this field at all.
    if (totpRequested_ && totpCode.empty())
    {
        SetStatus("Enter the 6-digit code from your authenticator app.", UITheme::Danger);
        if (uiManager_ && totpBox_)
            uiManager_->setFocusedElement(totpBox_);
        return;
    }

    AuthService* auth = engine_ ? engine_->GetAuthService() : nullptr;
    if (!auth)
    {
        SetStatus("Authentication service unavailable.", UITheme::Danger);
        return;
    }

    if (uiManager_)
        uiManager_->clearFocus();

    SetBusy(true);

    // The session has to exist before credentials can be sent. Connecting no
    // longer blocks: the window keeps drawing and Update() below drives it to
    // completion. Previously this stalled the whole client for the connect
    // timeout, so a wrong host in client.json looked like a frozen window
    // rather than an error.
    if (!engine_->IsOfflineMode() && !engine_->getNetworkManager().isConnected())
    {
        SetStatus("Connecting to server...", UITheme::Accent);

        if (!engine_->BeginConnectToServer())
        {
            const std::string reason = engine_->getNetworkManager().getLastError();
            SetBusy(false);
            SetStatus(reason.empty() ? "Could not reach the server."
                                     : "Server unreachable: " + reason,
                      UITheme::Danger);
            return;
        }

        // Credentials are held until the socket is up. They are not sent to
        // AuthService yet, so nothing leaves the machine until there is a
        // connection to send it on.
        connecting_       = true;
        pendingUsername_  = username;
        pendingPassword_  = password;
        pendingTotpCode_  = totpCode;
        return;
    }

    SetStatus("Authenticating...", UITheme::Accent);

    auth->BeginLogin(username, password, totpCode);
}

void LoginScreen::UpdateConnect()
{
    if (!connecting_ || !engine_)
        return;

    switch (engine_->PollConnectToServer())
    {
    case Engine::ConnectProgress::Pending:
        return;   // still dialling; try again next frame

    case Engine::ConnectProgress::Failed:
    {
        connecting_ = false;
        pendingPassword_.clear();

        const std::string reason = engine_->getNetworkManager().getLastError();
        SetBusy(false);
        SetStatus(reason.empty() ? "Could not reach the server."
                                 : "Server unreachable: " + reason,
                  UITheme::Danger);
        return;
    }

    case Engine::ConnectProgress::Connected:
        break;
    }

    connecting_ = false;

    AuthService* auth = engine_->GetAuthService();
    if (!auth)
    {
        pendingPassword_.clear();
        SetBusy(false);
        SetStatus("Authentication service unavailable.", UITheme::Danger);
        return;
    }

    SetStatus("Authenticating...", UITheme::Accent);
    auth->BeginLogin(pendingUsername_, pendingPassword_, pendingTotpCode_);

    // Not kept a moment longer than needed. Zero the memory before clearing.
    std::fill(pendingPassword_.begin(), pendingPassword_.end(), '\0');
    pendingPassword_.clear();
    std::fill(pendingTotpCode_.begin(), pendingTotpCode_.end(), '\0');
    pendingTotpCode_.clear();
}

void LoginScreen::Update(float deltaTime)
{
    // Advances a pending connect. Runs before the auth check below because
    // until the socket is up there is no login in flight to report on.
    UpdateConnect();

    AuthService* auth = engine_ ? engine_->GetAuthService() : nullptr;
    if (!auth)
        return;

    auth->Update(deltaTime);

    if (!submitting_ || connecting_)
        return;

    switch (auth->GetStatus())
    {
    case AuthService::Status::Succeeded:
        SetBusy(false);
        SetStatus(auth->GetStatusMessage(), UITheme::Success);

        if (engine_)
            engine_->SetSignedInUser(auth->GetUsername());

        // Only persisted on success, so a failed attempt never leaves a
        // remembered name behind. Unchecking the box clears a previously
        // remembered one rather than merely declining to update it.
        if (rememberBox_ && rememberBox_->isChecked())
            SaveRememberedUsername(auth->GetUsername());
        else
            ClearRememberedUsername();

        // The Connecting screen owns the rest of the sign-in sequence.
        RequestScreenChange(ScreenID::Connecting);
        break;

    case AuthService::Status::Failed:
    {
        const std::string& message = auth->GetStatusMessage();

        SetBusy(false);

        // The server names the missing second factor only after a CORRECT
        // password, so this string is the client's cue to show the code field.
        // Kept as a substring match rather than a new protocol field: the
        // wording lives in one server file and one client file, mirrored.
        if (message.find("authenticator") != std::string::npos)
        {
            RevealTotpField();
            SetStatus(message, UITheme::Warning);
        }
        else
        {
            SetStatus(message, UITheme::Danger);

            if (uiManager_ && passwordBox_)
            {
                passwordBox_->setText("");
                uiManager_->setFocusedElement(passwordBox_);
            }
        }

        auth->ResetRequest();
        break;
    }

    case AuthService::Status::Pending:
    case AuthService::Status::Idle:
        break;
    }
}

void LoginScreen::OnKeyDown(int key, bool, bool)
{
    // Enter submits from anywhere on the screen, not only from a focused field.
    if (key == UIKey::Enter)
        Submit();
}
