#include "RegisterScreen.h"

#include "../core/AssetManager.h"
#include "../core/AuthService.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../graphics/Font.h"
#include "../ui/UIPatterns.h"
#include "../ui/UITiledImage.h"
#include "../ui/UIButton.h"
#include "../ui/UICheckBox.h"
#include "../ui/UILabel.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITextBox.h"
#include "../ui/UITheme.h"

#include <array>

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    // The style guide's register screen (App.tsx 3180-3233).
    constexpr float kFormColumnWidth = S(320.0f);
    constexpr float kColumnPadding   = S(24.0f);
    constexpr float kPanelPadding    = S(24.0f);

    constexpr float kFieldHeight = S(36.0f);
    constexpr float kLabelHeight = S(9.0f);
    constexpr float kLabelGap    = S(4.0f);
    constexpr float kFieldGap    = S(11.0f);

    constexpr size_t kMinPasswordLength = 8;
    constexpr size_t kMinUsernameLength = 3;

    constexpr std::array<const char*, 4> kJourneyTags = {
        "Equip", "Craft", "Trade", "Quest",
    };

    bool LooksLikeEmail(const std::string& value)
    {
        const size_t at = value.find('@');
        if (at == std::string::npos || at == 0 || at + 1 >= value.size())
            return false;

        return value.find('.', at) != std::string::npos;
    }
}

RegisterScreen::RegisterScreen(Engine* engine)
    : Screen(engine)
{
}

void RegisterScreen::OnEnter()
{
    if (!uiManager_)
    {
        LOG_ERROR("RegisterScreen: UIManager not available");
        return;
    }

    submitting_ = false;

    if (AuthService* auth = engine_ ? engine_->GetAuthService() : nullptr)
        auth->ResetRequest();

    CreateRoot();

    AddBackdrop(UITheme::Hex(0x090E1A), UITheme::Hex(0x101830), false);

    const float originX = DesignOriginX();
    const float brandWidth = UIScale::kDesignWidth - kFormColumnWidth;

    BuildBrandColumn(brandWidth);
    BuildForm(originX + brandWidth, kFormColumnWidth);
}

void RegisterScreen::BuildBrandColumn(float columnWidth)
{
    const float originX = DesignOriginX();
    const float originY = DesignOriginY();

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

    constexpr float titleHeight    = S(30.0f);
    constexpr float subtitleHeight = S(12.0f);
    constexpr float cardWidth      = S(300.0f);
    constexpr float cardPaddingX   = S(18.0f);
    constexpr float cardPaddingY   = S(14.0f);
    constexpr float bodyLineHeight = S(22.0f);
    constexpr float chipHeight     = S(20.0f);

    // The notice copy wraps to the card, so measure it before positioning.
    const float cardContentWidth = cardWidth - cardPaddingX * 2.0f;

    Font* bodyFont = BodyFont(UITheme::Body::Regular);

    std::vector<std::string> firstParagraph;
    std::vector<std::string> secondParagraph;

    if (bodyFont)
    {
        firstParagraph = UILabel::WrapText(
            *bodyFont,
            "There is no character creation screen in StrixVerse.",
            cardContentWidth);

        secondParagraph = UILabel::WrapText(
            *bodyFont,
            "Your appearance evolves as you play - through gear you earn, craft, "
            "trade, and collect from quests and events.",
            cardContentWidth);
    }

    const float cardHeight = cardPaddingY * 2.0f +
                             S(9.0f) + S(10.0f) +
                             bodyLineHeight * static_cast<float>(firstParagraph.size()) +
                             S(6.0f) +
                             bodyLineHeight * static_cast<float>(secondParagraph.size()) +
                             S(12.0f) + chipHeight;

    const float totalHeight = titleHeight + S(16.0f) + subtitleHeight + S(16.0f) + cardHeight;

    float y = (UIScale::kDesignHeight - totalHeight) * 0.5f;

    auto title = std::make_shared<UILabel>();
    title->setText("STRIXVERSE");
    title->setFont(DisplayFont(UITheme::Display::Brand));
    title->setTextColor(UITheme::Text);
    title->setAlignment(UILabel::Alignment::Center);
    title->setShadow(Color(0.0f, 0.0f, 0.0f, 1.0f), S(3.0f), S(3.0f));
    title->setGlow(UITheme::WithAlpha(UITheme::Secondary, 0.80f), S(11.0f));
    title->setPosition(0.0f, y);
    title->setSize(columnWidth, titleHeight);
    column->addChild(title);
    y += titleHeight + S(16.0f);

    auto subtitle = std::make_shared<UILabel>();
    subtitle->setText("JOIN THE CRYSTAL WORLD");
    subtitle->setFont(DisplayFont(UITheme::Display::Label));
    subtitle->setTextColor(UITheme::Secondary);
    subtitle->setLetterSpacing(S(8.0f * 0.14f));
    subtitle->setAlignment(UILabel::Alignment::Center);
    subtitle->setPosition(0.0f, y);
    subtitle->setSize(columnWidth, subtitleHeight);
    column->addChild(subtitle);
    y += subtitleHeight + S(16.0f);

    // "YOUR LOOK = YOUR JOURNEY" notice.
    auto card = std::make_shared<UIPanel>();
    card->setBackgroundColor(UITheme::Hex(0x6C5CE7, 0.10f));
    card->setBorder(UITheme::Hex(0x6C5CE7, 0.35f), UITheme::BorderThin);
    card->setBorderRadius(UITheme::RadiusPanel);
    card->setPosition((columnWidth - cardWidth) * 0.5f, y);
    card->setSize(cardWidth, cardHeight);
    column->addChild(card);

    float cardY = cardPaddingY;

    auto heading = std::make_shared<UILabel>();
    heading->setText("YOUR LOOK = YOUR JOURNEY");
    heading->setFont(DisplayFont(UITheme::Display::Small));
    heading->setTextColor(UITheme::Secondary);
    heading->setPosition(cardPaddingX, cardY);
    heading->setSize(cardContentWidth, S(9.0f));
    card->addChild(heading);
    cardY += S(9.0f) + S(10.0f);

    auto addParagraph = [&](const std::vector<std::string>& lines)
    {
        for (const std::string& line : lines)
        {
            auto text = std::make_shared<UILabel>();
            text->setText(line);
            text->setFont(bodyFont);
            text->setTextColor(UITheme::Subtext);
            text->setPosition(cardPaddingX, cardY);
            text->setSize(cardContentWidth, bodyLineHeight);
            card->addChild(text);

            cardY += bodyLineHeight;
        }
    };

    addParagraph(firstParagraph);
    cardY += S(6.0f);
    addParagraph(secondParagraph);
    cardY += S(12.0f);

    // Journey tag chips.
    {
        const float gap = S(8.0f);
        const float chipWidth =
            (cardContentWidth - gap * static_cast<float>(kJourneyTags.size() - 1)) /
            static_cast<float>(kJourneyTags.size());

        float chipX = cardPaddingX;

        for (const char* tag : kJourneyTags)
        {
            auto chip = std::make_shared<UIPanel>();
            chip->setBackgroundColor(UITheme::Hex(0x6C5CE7, 0.18f));
            chip->setBorder(UITheme::Hex(0x6C5CE7, 0.35f), UITheme::BorderThin);
            chip->setBorderRadius(S(4.0f));
            chip->setPosition(chipX, cardY);
            chip->setSize(chipWidth, chipHeight);
            card->addChild(chip);

            auto label = std::make_shared<UILabel>();
            label->setText(tag);
            label->setFont(BodyFont(UITheme::Body::Tiny));
            label->setTextColor(UITheme::Secondary);
            label->setAlignment(UILabel::Alignment::Center);
            label->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
            label->setPosition(0.0f, 0.0f);
            label->setSize(chipWidth, chipHeight);
            chip->addChild(label);

            chipX += chipWidth + gap;
        }
    }
}

void RegisterScreen::BuildForm(float columnX, float columnWidth)
{
    const float originY = DesignOriginY();

    auto divider = std::make_shared<UIPanel>();
    divider->setBackgroundColor(UITheme::WithAlpha(UITheme::Border, 0.22f));
    divider->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    divider->setBorderRadius(0.0f);
    divider->setPosition(columnX, originY);
    divider->setSize(UITheme::BorderThin, UIScale::kDesignHeight);
    root_->addChild(divider);

    const float panelWidth   = columnWidth - kColumnPadding * 2.0f;
    const float contentWidth = panelWidth - kPanelPadding * 2.0f;

    const float fieldBlock = kLabelHeight + kLabelGap + kFieldHeight + kFieldGap;

    const float contentHeight = S(13.0f) + S(18.0f) +
                                fieldBlock * 4.0f +
                                S(24.0f) + kFieldGap +      // terms row
                                S(31.0f) + kFieldGap +      // create button
                                S(14.0f) + S(8.0f) +        // sign-in link
                                S(14.0f);                    // status line

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

    const float x = kPanelPadding;
    float y = kPanelPadding;

    auto heading = std::make_shared<UILabel>();
    heading->setText("CREATE ACCOUNT");
    heading->setFont(DisplayFont(UITheme::Display::Tagline));
    heading->setTextColor(UITheme::Text);
    heading->setAlignment(UILabel::Alignment::Center);
    heading->setPosition(x, y);
    heading->setSize(contentWidth, S(13.0f));
    panel->addChild(heading);
    y += S(13.0f) + S(18.0f);

    auto addField = [&](const char* caption, const char* placeholder, bool password, int maxLength)
        -> std::shared_ptr<UITextBox>
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
        field->setPlaceholderText(placeholder);
        field->setPasswordMode(password);
        field->setMaxLength(maxLength);
        field->setPosition(x, y + kLabelHeight + kLabelGap);
        field->setSize(contentWidth, kFieldHeight);
        field->setOnEnterPressed([this]() { Submit(); });
        panel->addChild(field);

        y += fieldBlock;
        return field;
    };

    usernameBox_ = addField("USERNAME",         "Choose a unique name", false, 24);
    emailBox_    = addField("EMAIL ADDRESS",    "your@email.com",       false, 96);
    passwordBox_ = addField("PASSWORD",         "Min 8 characters",     true,  64);
    confirmBox_  = addField("CONFIRM PASSWORD", "Repeat password",      true,  64);

    termsBox_ = std::make_shared<UICheckBox>();
    termsBox_->setLabel("I agree to the Terms of Service and Privacy Policy");
    termsBox_->setFont(BodyFont(UITheme::Body::Caption));
    termsBox_->setAccentColor(UITheme::Secondary);
    termsBox_->setChecked(false);
    termsBox_->setPosition(x, y);
    termsBox_->setSize(contentWidth, S(24.0f));
    termsBox_->wrapLabel();
    panel->addChild(termsBox_);
    y += S(24.0f) + kFieldGap;

    createButton_ = std::make_shared<UIButton>();
    createButton_->setText("CREATE ACCOUNT");
    createButton_->setFont(DisplayFont(UITheme::Display::Button));
    createButton_->setVariant(UIButton::Variant::Purple);
    createButton_->setPosition(x, y);
    createButton_->setSize(contentWidth, S(31.0f));
    createButton_->setOnClick([this]() { Submit(); });
    panel->addChild(createButton_);
    y += S(31.0f) + kFieldGap;

    // "Already have an account? Sign in" - a text link, so a ghost button
    // carries the click target without drawing a chrome box.
    auto signIn = std::make_shared<UIButton>();
    signIn->setText("Already have an account? Sign in");
    signIn->setFont(BodyFont(UITheme::Body::Caption));
    signIn->setVariant(UIButton::Variant::Ghost);
    signIn->setNormalColors(Color(0.0f, 0.0f, 0.0f, 0.0f),
                            Color(0.0f, 0.0f, 0.0f, 0.0f),
                            Color(0.0f, 0.0f, 0.0f, 0.0f));
    signIn->setHoverColors(Color(0.0f, 0.0f, 0.0f, 0.0f),
                           Color(0.0f, 0.0f, 0.0f, 0.0f),
                           Color(0.0f, 0.0f, 0.0f, 0.0f));
    signIn->setTextColor(UITheme::Accent);
    signIn->setPosition(x, y);
    signIn->setSize(contentWidth, S(14.0f));
    signIn->setOnClick([this]()
                       {
                           if (!submitting_)
                               RequestScreenChange(ScreenID::Login);
                       });
    panel->addChild(signIn);
    y += S(14.0f) + S(8.0f);

    statusLabel_ = std::make_shared<UILabel>();
    statusLabel_->setFont(BodyFont(UITheme::Body::Caption));
    statusLabel_->setTextColor(UITheme::Danger);
    statusLabel_->setAlignment(UILabel::Alignment::Center);
    statusLabel_->setPosition(x, y);
    statusLabel_->setSize(contentWidth, S(14.0f));
    panel->addChild(statusLabel_);

    if (uiManager_ && usernameBox_)
        uiManager_->setFocusedElement(usernameBox_);
}

std::string RegisterScreen::Validate(std::shared_ptr<UITextBox>& fieldToFocus) const
{
    const std::string username = usernameBox_ ? usernameBox_->getText() : std::string();
    const std::string email    = emailBox_ ? emailBox_->getText() : std::string();
    const std::string password = passwordBox_ ? passwordBox_->getText() : std::string();
    const std::string confirm  = confirmBox_ ? confirmBox_->getText() : std::string();

    if (username.size() < kMinUsernameLength)
    {
        fieldToFocus = usernameBox_;
        return "Username must be at least 3 characters.";
    }

    if (!LooksLikeEmail(email))
    {
        fieldToFocus = emailBox_;
        return "Enter a valid email address.";
    }

    if (password.size() < kMinPasswordLength)
    {
        fieldToFocus = passwordBox_;
        return "Password must be at least 8 characters.";
    }

    if (confirm != password)
    {
        fieldToFocus = confirmBox_;
        return "Passwords do not match.";
    }

    if (termsBox_ && !termsBox_->isChecked())
    {
        fieldToFocus = nullptr;
        return "You must accept the Terms of Service.";
    }

    fieldToFocus = nullptr;
    return {};
}

void RegisterScreen::SetStatus(const std::string& message, const Color& color)
{
    if (!statusLabel_)
        return;

    statusLabel_->setText(message);
    statusLabel_->setTextColor(color);
}

void RegisterScreen::SetBusy(bool busy)
{
    submitting_ = busy;

    if (createButton_)
    {
        createButton_->setEnabled(!busy);
        createButton_->setText(busy ? "CREATING..." : "CREATE ACCOUNT");
    }

    for (auto* field : {&usernameBox_, &emailBox_, &passwordBox_, &confirmBox_})
    {
        if (*field)
            (*field)->setEnabled(!busy);
    }

    if (termsBox_)
        termsBox_->setEnabled(!busy);
}

void RegisterScreen::Submit()
{
    if (submitting_)
        return;

    std::shared_ptr<UITextBox> focusField;
    const std::string error = Validate(focusField);

    if (!error.empty())
    {
        SetStatus(error, UITheme::Danger);

        if (uiManager_ && focusField)
            uiManager_->setFocusedElement(focusField);

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

    SetStatus("Creating account...", UITheme::Accent);

    auth->BeginRegister(usernameBox_->getText(),
                        emailBox_->getText(),
                        passwordBox_->getText());
}

void RegisterScreen::Update(float deltaTime)
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
    {
        SetBusy(false);
        SetStatus(auth->GetStatusMessage(), UITheme::Success);

        // The server signs a new account in as part of registration, so the
        // flow continues into the connecting sequence exactly as after a login.
        if (engine_)
            engine_->SetSignedInUser(auth->GetUsername());

        auth->ResetRequest();
        RequestScreenChange(ScreenID::Connecting);
        break;
    }

    case AuthService::Status::Failed:
        SetBusy(false);
        SetStatus(auth->GetStatusMessage(), UITheme::Danger);
        auth->ResetRequest();
        break;

    case AuthService::Status::Pending:
    case AuthService::Status::Idle:
        break;
    }
}

void RegisterScreen::OnKeyDown(int key, bool, bool)
{
    if (key == UIKey::Escape && !submitting_)
        RequestScreenChange(ScreenID::Login);
    else if (key == UIKey::Enter)
        Submit();
}
