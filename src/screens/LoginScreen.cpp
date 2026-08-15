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

    // Remember me / forgot password row.
    rememberBox_ = std::make_shared<UICheckBox>();
    rememberBox_->setLabel("Remember me");
    rememberBox_->setFont(BodyFont(UITheme::Body::Caption));
    rememberBox_->setChecked(true);
    rememberBox_->setAccentColor(UITheme::Primary);
    rememberBox_->setPosition(x, y);
    rememberBox_->setSize(contentWidth * 0.6f, kRowHeight);
    panel->addChild(rememberBox_);

    auto forgot = std::make_shared<UILabel>();
    forgot->setText("Forgot password?");
    forgot->setFont(BodyFont(UITheme::Body::Caption));
    forgot->setTextColor(UITheme::Accent);
    forgot->setAlignment(UILabel::Alignment::Right);
    forgot->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    forgot->setPosition(x + contentWidth * 0.6f, y);
    forgot->setSize(contentWidth * 0.4f, kRowHeight);
    panel->addChild(forgot);

    y += kRowHeight + kFieldGap;

    loginButton_ = std::make_shared<UIButton>();
    loginButton_->setText("ENTER WORLD");
    loginButton_->setFont(DisplayFont(UITheme::Display::Button));
    loginButton_->setVariant(UIButton::Variant::Primary);
    loginButton_->setPosition(x, y);
    loginButton_->setSize(contentWidth, kButtonHeight);
    loginButton_->setOnClick([this]() { Submit(); });
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
                                        RequestScreenChange(ScreenID::Register);
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

    // Start with the username field ready for typing.
    if (uiManager_ && usernameBox_)
        uiManager_->setFocusedElement(usernameBox_);
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
}

void LoginScreen::Submit()
{
    if (submitting_)
        return;

    const std::string username = usernameBox_ ? usernameBox_->getText() : std::string();
    const std::string password = passwordBox_ ? passwordBox_->getText() : std::string();

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

    AuthService* auth = engine_ ? engine_->GetAuthService() : nullptr;
    if (!auth)
    {
        SetStatus("Authentication service unavailable.", UITheme::Danger);
        return;
    }

    if (uiManager_)
        uiManager_->clearFocus();

    SetBusy(true);

    // The session has to exist before credentials can be sent. Connecting is
    // blocking with a short timeout, so report it while it happens.
    if (!engine_->IsOfflineMode() && !engine_->getNetworkManager().isConnected())
    {
        SetStatus("Connecting to server...", UITheme::Accent);

        if (!engine_->ConnectToServer())
        {
            const std::string reason = engine_->getNetworkManager().getLastError();
            SetBusy(false);
            SetStatus(reason.empty() ? "Could not reach the server."
                                     : "Server unreachable: " + reason,
                      UITheme::Danger);
            return;
        }
    }

    SetStatus("Authenticating...", UITheme::Accent);

    auth->BeginLogin(username, password);
}

void LoginScreen::Update(float deltaTime)
{
    AuthService* auth = engine_ ? engine_->GetAuthService() : nullptr;
    if (!auth)
        return;

    auth->Update(deltaTime);

    if (!submitting_)
        return;

    switch (auth->GetStatus())
    {
    case AuthService::Status::Succeeded:
        SetBusy(false);
        SetStatus(auth->GetStatusMessage(), UITheme::Success);

        if (engine_)
            engine_->SetSignedInUser(auth->GetUsername());

        // The Connecting screen owns the rest of the sign-in sequence.
        RequestScreenChange(ScreenID::Connecting);
        break;

    case AuthService::Status::Failed:
        SetBusy(false);
        SetStatus(auth->GetStatusMessage(), UITheme::Danger);
        auth->ResetRequest();

        if (uiManager_ && passwordBox_)
        {
            passwordBox_->setText("");
            uiManager_->setFocusedElement(passwordBox_);
        }
        break;

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
