#include "ContinueScreen.h"

#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../networking/NetworkManager.h"
#include "../ui/UIButton.h"
#include "../ui/UIIcon.h"
#include "../ui/UILabel.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

#include <array>
#include <format>
#include <numbers>

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    // The style guide's continue screen (App.tsx 3280-3331).
    constexpr float kCardWidth   = S(360.0f);
    constexpr float kHeaderPadX  = S(20.0f);
    constexpr float kHeaderPadY  = S(14.0f);
    constexpr float kBodyPadX    = S(20.0f);
    constexpr float kBodyPadY    = S(16.0f);
    constexpr float kRowHeight   = S(18.0f);
    constexpr float kRowGap      = S(8.0f);
    constexpr float kIconSize    = S(40.0f);

    constexpr float kAutoJoinSeconds = 5.0f;

    // Decorative crystal shards: percentage of the design area plus a size.
    struct ShardPlacement
    {
        float leftPercent;
        float topPercent;
        float size;
    };

    constexpr std::array<ShardPlacement, 5> kShards = {{
        {8.0f, 12.0f, 18.0f},
        {88.0f, 8.0f, 14.0f},
        {4.0f, 72.0f, 12.0f},
        {91.0f, 70.0f, 16.0f},
        {50.0f, 88.0f, 10.0f},
    }};

    constexpr float kShardRotation = 30.0f * std::numbers::pi_v<float> / 180.0f;
}

ContinueScreen::ContinueScreen(Engine* engine)
    : Screen(engine)
{
}

void ContinueScreen::OnEnter()
{
    if (!uiManager_)
    {
        LOG_ERROR("ContinueScreen: UIManager not available");
        return;
    }

    joining_ = false;

    hasSession_ = false;
    if (WorldManager* worlds = engine_ ? engine_->GetWorldManager() : nullptr)
        hasSession_ = worlds->GetLastWorld(engine_->GetSignedInUser(), session_);

    if (!hasSession_)
    {
        // Nothing to continue into - the design sends the player to the world
        // list rather than showing an empty card.
        LOG_INFO("ContinueScreen: no saved session; opening world selection");
        RequestScreenChange(ScreenID::WorldBrowser);
        return;
    }

    countdown_   = kAutoJoinSeconds;
    autoJoining_ = true;

    CreateRoot();

    AddBackdrop(UITheme::Hex(0x090E1A), UITheme::Hex(0x0F1828), true);

    BuildShards();

    const float originX = DesignOriginX();
    const float originY = DesignOriginY();
    const float centreX = originX + UIScale::kDesignWidth * 0.5f;

    // Lay the column out top-down and centre the group.
    const float welcomeHeight = S(24.0f);
    const float nameHeight    = S(20.0f);
    const float cardHeight    = kHeaderPadY * 2.0f + kIconSize +
                                kBodyPadY * 2.0f + kRowHeight * 5.0f + kRowGap * 4.0f +
                                S(30.0f) + S(20.0f);
    const float hintHeight    = S(18.0f);

    const float totalHeight = welcomeHeight + nameHeight + S(24.0f) +
                              cardHeight + S(24.0f) + hintHeight;

    float y = originY + (UIScale::kDesignHeight - totalHeight) * 0.5f;

    auto welcome = std::make_shared<UILabel>();
    welcome->setText("Welcome back,");
    welcome->setFont(BodyFont(UITheme::Body::Splash));
    welcome->setTextColor(UITheme::Subtext);
    welcome->setAlignment(UILabel::Alignment::Center);
    welcome->setPosition(originX, y);
    welcome->setSize(UIScale::kDesignWidth, welcomeHeight);
    root_->addChild(welcome);
    y += welcomeHeight;

    const std::string username = engine_ && !engine_->GetSignedInUser().empty()
                                     ? engine_->GetSignedInUser()
                                     : std::string("Player");

    auto name = std::make_shared<UILabel>();
    name->setText(username);
    name->setFont(DisplayFont(UITheme::Display::Title));
    name->setTextColor(UITheme::Text);
    name->setAlignment(UILabel::Alignment::Center);
    name->setGlow(UITheme::WithAlpha(UITheme::Accent, 0.50f), S(9.0f));
    name->setPosition(originX, y + S(4.0f));
    name->setSize(UIScale::kDesignWidth, nameHeight);
    root_->addChild(name);
    y += nameHeight + S(24.0f);

    BuildCard(centreX, y, kCardWidth);
    y += cardHeight + S(24.0f);

    countdownLabel_ = std::make_shared<UILabel>();
    countdownLabel_->setFont(BodyFont(UITheme::Body::Welcome));
    countdownLabel_->setTextColor(UITheme::Muted);
    countdownLabel_->setAlignment(UILabel::Alignment::Center);
    countdownLabel_->setPosition(originX, y);
    countdownLabel_->setSize(UIScale::kDesignWidth, hintHeight);
    root_->addChild(countdownLabel_);
}

void ContinueScreen::BuildShards()
{
    const float originX = DesignOriginX();
    const float originY = DesignOriginY();

    for (const ShardPlacement& placement : kShards)
    {
        const float width  = S(placement.size);
        const float height = width * 2.0f;

        auto shard = std::make_shared<UIPanel>();
        shard->setBackgroundColor(UITheme::WithAlpha(UITheme::Accent, 0.05f));
        shard->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.10f), UITheme::BorderThin);
        shard->setBorderRadius(S(2.0f));
        shard->setRotation(kShardRotation);
        shard->setPosition(originX + UIScale::kDesignWidth * placement.leftPercent * 0.01f,
                           originY + UIScale::kDesignHeight * placement.topPercent * 0.01f);
        shard->setSize(width, height);

        root_->addChild(shard);
    }
}

void ContinueScreen::BuildCard(float centreX, float y, float width)
{
    const float cardX = centreX - width * 0.5f;

    const float headerHeight = kHeaderPadY * 2.0f + kIconSize;
    const float bodyHeight   = kBodyPadY * 2.0f + kRowHeight * 5.0f + kRowGap * 4.0f;
    const float footerHeight = S(30.0f) + S(20.0f);

    auto card = std::make_shared<UIPanel>();
    card->setBackgroundColor(UITheme::Panel);
    card->setBorder(UITheme::PanelBorder, UITheme::BorderThin);
    card->setBorderRadius(UITheme::RadiusPanel);
    card->setGlow(Color(0.0f, 0.0f, 0.0f, 0.55f), S(16.0f));
    card->setPosition(cardX, y);
    card->setSize(width, headerHeight + bodyHeight + footerHeight);
    root_->addChild(card);

    // --- Header: world identity and status -------------------------------
    auto header = std::make_shared<UIPanel>();
    header->setBackgroundGradient(UITheme::Hex(0x4CD964, 0.08f), UITheme::Hex(0x4CD964, 0.0f));
    header->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    header->setBorderRadius(0.0f);
    header->setPosition(0.0f, 0.0f);
    header->setSize(width, headerHeight);
    card->addChild(header);

    auto headerRule = std::make_shared<UIPanel>();
    headerRule->setBackgroundColor(UITheme::Hex(0x4CD964, 0.20f));
    headerRule->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    headerRule->setBorderRadius(0.0f);
    headerRule->setPosition(0.0f, headerHeight - UITheme::BorderThin);
    headerRule->setSize(width, UITheme::BorderThin);
    card->addChild(headerRule);

    auto worldIcon = std::make_shared<UIPanel>();
    worldIcon->setBackgroundGradient(UITheme::Hex(0x4CD964, 0.13f), UITheme::Hex(0x4F8CFF, 0.13f));
    worldIcon->setBorder(UITheme::Hex(0x4CD964, 0.30f), UITheme::BorderThin);
    worldIcon->setBorderRadius(UITheme::RadiusPanel);
    worldIcon->setPosition(kHeaderPadX, kHeaderPadY);
    worldIcon->setSize(kIconSize, kIconSize);
    card->addChild(worldIcon);

    auto worldGlyph = std::make_shared<UIIcon>(UIIcon::Shape::Diamond);
    worldGlyph->setColor(UITheme::Success);
    worldGlyph->setPosition(kIconSize * 0.28f, kIconSize * 0.28f);
    worldGlyph->setSize(kIconSize * 0.44f, kIconSize * 0.44f);
    worldIcon->addChild(worldGlyph);

    const float textX = kHeaderPadX + kIconSize + S(12.0f);

    auto eyebrow = std::make_shared<UILabel>();
    eyebrow->setText("LAST VISITED WORLD");
    eyebrow->setFont(DisplayFont(UITheme::Display::Tiny));
    eyebrow->setTextColor(UITheme::Success);
    eyebrow->setPosition(textX, kHeaderPadY + S(4.0f));
    eyebrow->setSize(width - textX - kHeaderPadX, S(8.0f));
    card->addChild(eyebrow);

    auto worldName = std::make_shared<UILabel>();
    worldName->setText(session_.world.name);
    worldName->setFont(DisplayFont(UITheme::Display::Section));
    worldName->setTextColor(UITheme::Text);
    worldName->setPosition(textX, kHeaderPadY + S(17.0f));
    worldName->setSize(width - textX - kHeaderPadX, S(14.0f));
    card->addChild(worldName);

    // Online indicator, right aligned. It reports the client's own connection,
    // which is the only thing the client can actually vouch for - the world's
    // status needs a server world list.
    const bool  connected   = engine_ && engine_->getNetworkManager().isConnected();
    const Color statusColor = connected ? UITheme::Success : UITheme::Muted;

    const float statusWidth = S(70.0f);
    const float statusX = width - kHeaderPadX - statusWidth;

    auto onlineDot = std::make_shared<UIIcon>(UIIcon::Shape::Dot);
    onlineDot->setColor(statusColor);
    onlineDot->setPosition(statusX, kHeaderPadY + S(15.0f));
    onlineDot->setSize(S(7.0f), S(7.0f));
    card->addChild(onlineDot);

    auto onlineLabel = std::make_shared<UILabel>();
    onlineLabel->setText(connected ? "ONLINE" : "OFFLINE");
    onlineLabel->setFont(BodyFont(UITheme::Body::Welcome));
    onlineLabel->setTextColor(statusColor);
    onlineLabel->setPosition(statusX + S(12.0f), kHeaderPadY + S(11.0f));
    onlineLabel->setSize(statusWidth - S(12.0f), S(16.0f));
    card->addChild(onlineLabel);

    // --- Body: session details -------------------------------------------
    //
    // Everything except the world name and the timestamp depends on the server
    // describing the world, which it does not do yet. Unknown fields render as
    // a dash rather than a plausible-looking placeholder.
    const std::string unknown = "-";

    const std::array<std::pair<std::string, std::string>, 5> rows = {{
        {"World Type",  session_.world.type.empty() ? unknown : session_.world.type},
        {"Owner",       session_.world.owner.empty() ? unknown : session_.world.owner},
        {"Last Played", FormatRelativeTime(session_.lastPlayedUnix)},
        {"Position",    session_.hasPosition
                            ? std::format("X:{} Y:{}", session_.positionX, session_.positionY)
                            : unknown},
        {"Players",     session_.world.HasPopulation()
                            ? std::format("{} / {} online", session_.world.players,
                                          session_.world.maxPlayers)
                            : unknown},
    }};

    float rowY = headerHeight + kBodyPadY;

    for (const auto& [label, value] : rows)
    {
        auto labelText = std::make_shared<UILabel>();
        labelText->setText(label);
        labelText->setFont(BodyFont(UITheme::Body::Regular));
        labelText->setTextColor(UITheme::Muted);
        labelText->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
        labelText->setPosition(kBodyPadX, rowY);
        labelText->setSize((width - kBodyPadX * 2.0f) * 0.5f, kRowHeight);
        card->addChild(labelText);

        auto valueText = std::make_shared<UILabel>();
        valueText->setText(value);
        valueText->setFont(DataFont(UITheme::Data::Regular));
        valueText->setTextColor(UITheme::Subtext);
        valueText->setAlignment(UILabel::Alignment::Right);
        valueText->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
        valueText->setPosition(kBodyPadX + (width - kBodyPadX * 2.0f) * 0.5f, rowY);
        valueText->setSize((width - kBodyPadX * 2.0f) * 0.5f, kRowHeight);
        card->addChild(valueText);

        rowY += kRowHeight + kRowGap;
    }

    // --- Footer: actions --------------------------------------------------
    const float buttonY      = headerHeight + bodyHeight;
    const float buttonHeight = S(30.0f);
    const float gap          = S(10.0f);
    const float changeWidth  = S(120.0f);
    const float continueWidth = width - kBodyPadX * 2.0f - changeWidth - gap;

    auto continueButton = std::make_shared<UIButton>();
    continueButton->setText("CONTINUE");
    continueButton->setFont(DisplayFont(UITheme::Display::Button));
    continueButton->setVariant(UIButton::Variant::Primary);
    continueButton->setPosition(kBodyPadX, buttonY);
    continueButton->setSize(continueWidth, buttonHeight);
    continueButton->setOnClick([this]() { JoinLastWorld(); });
    card->addChild(continueButton);

    auto playIcon = std::make_shared<UIIcon>(UIIcon::Shape::Play);
    playIcon->setColor(UITheme::Text);
    playIcon->setPosition(S(12.0f), (buttonHeight - S(12.0f)) * 0.5f);
    playIcon->setSize(S(12.0f), S(12.0f));
    continueButton->addChild(playIcon);
    continueButton->setLabelInset(S(24.0f), 0.0f);

    auto changeButton = std::make_shared<UIButton>();
    changeButton->setText("CHANGE WORLD");
    changeButton->setFont(DisplayFont(UITheme::Display::Label));
    changeButton->setVariant(UIButton::Variant::Purple);
    changeButton->setPosition(kBodyPadX + continueWidth + gap, buttonY);
    changeButton->setSize(changeWidth, buttonHeight);
    changeButton->setOnClick([this]()
                             {
                                 CancelAutoJoin();
                                 RequestScreenChange(ScreenID::WorldBrowser);
                             });
    card->addChild(changeButton);
}

void ContinueScreen::CancelAutoJoin()
{
    if (!autoJoining_)
        return;

    autoJoining_ = false;

    if (countdownLabel_)
    {
        countdownLabel_->setText("Auto-join cancelled - choose an option above.");
        countdownLabel_->setTextColor(UITheme::Muted);
    }
}

void ContinueScreen::JoinLastWorld()
{
    if (joining_)
        return;

    joining_     = true;
    autoJoining_ = false;

    if (engine_)
    {
        engine_->SetSelectedWorldName(session_.world.name);

        // Re-stamp the save so "Last Played" tracks this session as well.
        if (WorldManager* worlds = engine_->GetWorldManager())
            worlds->SetLastWorld(session_.world.name, engine_->GetSignedInUser());

        // Ask the server to place us in the world. Without this the loading
        // screen waits on a confirmation that was never requested, and only
        // proceeds once its timeout expires.
        NetworkManager& network = engine_->getNetworkManager();

        if (network.isConnected() && !network.sendWorldJoin(session_.world.name))
        {
            joining_ = false;

            if (countdownLabel_)
            {
                countdownLabel_->setText("Could not reach the server.");
                countdownLabel_->setTextColor(UITheme::Danger);
            }

            return;
        }
    }

    RequestScreenChange(ScreenID::Loading);
}

void ContinueScreen::Update(float deltaTime)
{
    if (!autoJoining_ || joining_)
        return;

    countdown_ -= deltaTime;

    if (countdown_ <= 0.0f)
    {
        JoinLastWorld();
        return;
    }

    if (countdownLabel_)
    {
        const int seconds = static_cast<int>(countdown_) + 1;
        countdownLabel_->setText(
            std::format("Auto-joining in {}s - press any key to cancel", seconds));
    }
}

void ContinueScreen::OnKeyDown(int, bool, bool)
{
    CancelAutoJoin();
}

void ContinueScreen::OnMouseDown(float, float)
{
    CancelAutoJoin();
}
