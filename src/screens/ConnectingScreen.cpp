#include "ConnectingScreen.h"

#include "../core/AuthService.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../core/WorldManager.h"
#include "../networking/NetworkManager.h"
#include "../ui/UIButton.h"
#include "../ui/UIIcon.h"
#include "../ui/UILabel.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

#include <cmath>
#include <iterator>
#include <numbers>

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    // The style guide's connecting screen (App.tsx 3236-3277).
    constexpr float kBadgeSize   = S(56.0f);
    constexpr float kBadgeRadius = S(16.0f);
    constexpr float kCardWidth   = S(280.0f);
    constexpr float kStepSize    = S(18.0f);
    constexpr float kStepGap     = S(8.0f);
    constexpr float kSectionGap  = S(28.0f);

    // Step labels and their simulated latency, in order. The durations stand
    // in for the network round trips these steps will make.
    struct StepDefinition
    {
        const char* label;
        float       duration;
    };

    constexpr StepDefinition kSteps[] = {
        {"Authenticating",      0.35f},
        {"Connecting",          0.80f},
        {"Loading Player Data", 0.65f},
        {"Checking Last World", 0.55f},
    };

    static_assert(std::size(kSteps) == 4, "kSteps must match ConnectingScreen::kStepCount");
}

ConnectingScreen::ConnectingScreen(Engine* engine)
    : Screen(engine)
{
}

void ConnectingScreen::OnEnter()
{
    if (!uiManager_)
    {
        LOG_ERROR("ConnectingScreen: UIManager not available");
        return;
    }

    phase_       = Phase::Running;
    currentStep_ = 0;
    stepTimer_   = 0.0f;
    elapsed_     = 0.0f;

    // Whether there is a world to return to decides the branch at the end.
    // The saved session is per-account, so a newly registered account - or any
    // account that has not been in a world - takes the World Selection branch.
    hasLastWorld_ = false;

    if (WorldManager* worlds = engine_ ? engine_->GetWorldManager() : nullptr)
        hasLastWorld_ = worlds->HasSavedWorldFor(engine_->GetSignedInUser());

    CreateRoot();

    // linear-gradient(180deg, #090e1a, #0f1828) with the pixel-grid lattice.
    AddBackdrop(UITheme::Hex(0x090E1A), UITheme::Hex(0x0F1828), true);

    BuildLayout();
}

void ConnectingScreen::BuildLayout()
{
    const float originX = DesignOriginX();
    const float originY = DesignOriginY();
    const float centreX = originX + UIScale::kDesignWidth * 0.5f;

    // The column is laid out top-down and then centred as a whole.
    const float stepsHeight = kStepSize * static_cast<float>(kStepCount) +
                              kStepGap * static_cast<float>(kStepCount - 1);
    const float headlineH   = S(16.0f);
    const float statusH     = S(20.0f);
    const float cardHeight  = S(86.0f);

    const float totalHeight = kBadgeSize + kSectionGap +
                              headlineH + S(8.0f) + statusH + kSectionGap +
                              stepsHeight + kSectionGap +
                              cardHeight;

    float y = originY + (UIScale::kDesignHeight - totalHeight) * 0.5f;

    // Pulsing crystal badge.
    badge_ = std::make_shared<UIPanel>();
    badge_->setBackgroundGradient(UITheme::Hex(0x4F8CFF, 0.13f), UITheme::Hex(0x6C5CE7, 0.13f));
    badge_->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.30f), UITheme::BorderThin);
    badge_->setBorderRadius(kBadgeRadius);
    badge_->setPosition(centreX - kBadgeSize * 0.5f, y);
    badge_->setSize(kBadgeSize, kBadgeSize);
    root_->addChild(badge_);

    auto crystal = std::make_shared<UIIcon>(UIIcon::Shape::Diamond);
    crystal->setColor(UITheme::Accent);
    crystal->setPosition(kBadgeSize * 0.25f, kBadgeSize * 0.25f);
    crystal->setSize(kBadgeSize * 0.5f, kBadgeSize * 0.5f);
    badge_->addChild(crystal);

    y += kBadgeSize + kSectionGap;

    headlineLabel_ = std::make_shared<UILabel>();
    headlineLabel_->setText("ENTERING STRIXVERSE");
    headlineLabel_->setFont(DisplayFont(UITheme::Display::Heading));
    headlineLabel_->setTextColor(UITheme::Text);
    headlineLabel_->setAlignment(UILabel::Alignment::Center);
    headlineLabel_->setGlow(UITheme::WithAlpha(UITheme::Accent, 0.50f), S(8.0f));
    headlineLabel_->setPosition(originX, y);
    headlineLabel_->setSize(UIScale::kDesignWidth, headlineH);
    root_->addChild(headlineLabel_);

    y += headlineH + S(8.0f);

    statusLabel_ = std::make_shared<UILabel>();
    statusLabel_->setText("Establishing crystal link...");
    statusLabel_->setFont(BodyFont(UITheme::Body::Medium));
    statusLabel_->setTextColor(UITheme::Accent);
    statusLabel_->setAlignment(UILabel::Alignment::Center);
    statusLabel_->setPosition(originX, y);
    statusLabel_->setSize(UIScale::kDesignWidth, statusH);
    root_->addChild(statusLabel_);

    y += statusH + kSectionGap;

    BuildSteps(centreX, y);

    y += stepsHeight + kSectionGap;

    BuildBranchCard(centreX, y);
    BuildFailureCard(centreX, y);

    // Flavour line pinned near the bottom, as in the design.
    auto flavour = std::make_shared<UILabel>();
    flavour->setText("\"The crystal nexus hums with ancient power...\"");
    flavour->setFont(BodyFont(UITheme::Body::Regular));
    flavour->setTextColor(UITheme::Muted);
    flavour->setAlignment(UILabel::Alignment::Center);
    flavour->setPosition(originX, originY + UIScale::kDesignHeight - S(38.0f));
    flavour->setSize(UIScale::kDesignWidth, S(20.0f));
    root_->addChild(flavour);
}

void ConnectingScreen::BuildSteps(float centreX, float& y)
{
    const float listX = centreX - kCardWidth * 0.5f;
    float rowY = y;

    for (size_t i = 0; i < kStepCount; ++i)
    {
        auto icon = std::make_shared<UIIcon>(UIIcon::Shape::Ring);
        icon->setColor(UITheme::Muted);
        icon->setPosition(listX, rowY);
        icon->setSize(kStepSize, kStepSize);
        root_->addChild(icon);
        stepIcons_[i] = icon;

        auto label = std::make_shared<UILabel>();
        label->setText(kSteps[i].label);
        label->setFont(BodyFont(UITheme::Body::Input));
        label->setTextColor(UITheme::Muted);
        label->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
        label->setPosition(listX + kStepSize + S(12.0f), rowY);
        label->setSize(kCardWidth - kStepSize - S(12.0f), kStepSize);
        root_->addChild(label);
        stepLabels_[i] = label;

        rowY += kStepSize + kStepGap;
    }
}

void ConnectingScreen::BuildBranchCard(float centreX, float y)
{
    const float cardHeight = S(86.0f);

    branchCard_ = std::make_shared<UIPanel>();
    branchCard_->setBackgroundColor(UITheme::WithAlpha(UITheme::Primary, 0.07f));
    branchCard_->setBorder(UITheme::WithAlpha(UITheme::Primary, 0.25f), UITheme::BorderThin);
    branchCard_->setBorderRadius(UITheme::RadiusPanel);
    branchCard_->setPosition(centreX - kCardWidth * 0.5f, y);
    branchCard_->setSize(kCardWidth, cardHeight);
    branchCard_->setVisible(false);
    root_->addChild(branchCard_);

    auto prompt = std::make_shared<UILabel>();
    prompt->setText(hasLastWorld_ ? "Last world found - where would you like to go?"
                                  : "No previous world - choose where to begin.");
    prompt->setFont(BodyFont(UITheme::Body::Regular));
    prompt->setTextColor(UITheme::Accent);
    prompt->setAlignment(UILabel::Alignment::Center);
    prompt->setPosition(S(10.0f), S(14.0f));
    prompt->setSize(kCardWidth - S(20.0f), S(18.0f));
    branchCard_->addChild(prompt);

    const float buttonY      = S(38.0f);
    const float buttonHeight = S(30.0f);
    const float gap          = S(10.0f);
    const float padding      = S(20.0f);

    if (hasLastWorld_)
    {
        const float buttonWidth = (kCardWidth - padding * 2.0f - gap) * 0.5f;

        auto continueButton = std::make_shared<UIButton>();
        continueButton->setText("CONTINUE");
        continueButton->setFont(DisplayFont(UITheme::Display::Label));
        continueButton->setVariant(UIButton::Variant::Primary);
        continueButton->setPosition(padding, buttonY);
        continueButton->setSize(buttonWidth, buttonHeight);
        continueButton->setOnClick([this]() { RequestScreenChange(ScreenID::Continue); });
        branchCard_->addChild(continueButton);

        auto changeButton = std::make_shared<UIButton>();
        changeButton->setText("CHANGE WORLD");
        changeButton->setFont(DisplayFont(UITheme::Display::Small));
        changeButton->setVariant(UIButton::Variant::Purple);
        changeButton->setPosition(padding + buttonWidth + gap, buttonY);
        changeButton->setSize(buttonWidth, buttonHeight);
        changeButton->setOnClick([this]() { RequestScreenChange(ScreenID::WorldBrowser); });
        branchCard_->addChild(changeButton);
    }
    else
    {
        auto selectButton = std::make_shared<UIButton>();
        selectButton->setText("SELECT A WORLD");
        selectButton->setFont(DisplayFont(UITheme::Display::Label));
        selectButton->setVariant(UIButton::Variant::Primary);
        selectButton->setPosition(padding, buttonY);
        selectButton->setSize(kCardWidth - padding * 2.0f, buttonHeight);
        selectButton->setOnClick([this]() { RequestScreenChange(ScreenID::WorldBrowser); });
        branchCard_->addChild(selectButton);
    }
}

void ConnectingScreen::BuildFailureCard(float centreX, float y)
{
    const float cardHeight = S(96.0f);

    failureCard_ = std::make_shared<UIPanel>();
    failureCard_->setBackgroundColor(UITheme::WithAlpha(UITheme::Danger, 0.07f));
    failureCard_->setBorder(UITheme::WithAlpha(UITheme::Danger, 0.30f), UITheme::BorderThin);
    failureCard_->setBorderRadius(UITheme::RadiusPanel);
    failureCard_->setGlow(UITheme::WithAlpha(UITheme::Danger, 0.14f), S(12.0f));
    failureCard_->setPosition(centreX - kCardWidth * 0.5f, y);
    failureCard_->setSize(kCardWidth, cardHeight);
    failureCard_->setVisible(false);
    root_->addChild(failureCard_);

    failureReason_ = std::make_shared<UILabel>();
    failureReason_->setText("Unable to reach the game server.");
    failureReason_->setFont(BodyFont(UITheme::Body::Regular));
    failureReason_->setTextColor(UITheme::Danger);
    failureReason_->setAlignment(UILabel::Alignment::Center);
    failureReason_->setPosition(S(10.0f), S(14.0f));
    failureReason_->setSize(kCardWidth - S(20.0f), S(18.0f));
    failureCard_->addChild(failureReason_);

    const float buttonY      = S(44.0f);
    const float buttonHeight = S(30.0f);
    const float gap          = S(10.0f);
    const float padding      = S(20.0f);
    const float buttonWidth  = (kCardWidth - padding * 2.0f - gap) * 0.5f;

    auto retryButton = std::make_shared<UIButton>();
    retryButton->setText("RETRY");
    retryButton->setFont(DisplayFont(UITheme::Display::Label));
    retryButton->setVariant(UIButton::Variant::Primary);
    retryButton->setPosition(padding, buttonY);
    retryButton->setSize(buttonWidth, buttonHeight);
    retryButton->setOnClick([this]() { Retry(); });
    failureCard_->addChild(retryButton);

    auto backButton = std::make_shared<UIButton>();
    backButton->setText("BACK TO LOGIN");
    backButton->setFont(DisplayFont(UITheme::Display::Small));
    backButton->setVariant(UIButton::Variant::Purple);
    backButton->setPosition(padding + buttonWidth + gap, buttonY);
    backButton->setSize(buttonWidth, buttonHeight);
    backButton->setOnClick([this]() { RequestScreenChange(ScreenID::Login); });
    failureCard_->addChild(backButton);
}

bool ConnectingScreen::AttemptConnect()
{
    if (!engine_)
        return false;

    // Offline mode never contacts the server; the local session stands in.
    if (engine_->IsOfflineMode())
    {
        AuthService* auth = engine_->GetAuthService();
        return auth != nullptr && auth->IsAuthenticated();
    }

    NetworkManager& network = engine_->getNetworkManager();

    // The login screen opened the session; re-establish it if it dropped in
    // between, which is also what the retry button exercises.
    if (!network.isConnected() && !engine_->ConnectToServer())
        return false;

    // The session must also be authenticated - the server rejects world
    // traffic from a connection that has not logged in.
    return network.isConnected() && network.isAuthenticated();
}

void ConnectingScreen::ShowBranch()
{
    phase_ = Phase::Branch;

    if (statusLabel_)
    {
        statusLabel_->setText("Authentication complete");
        statusLabel_->setTextColor(UITheme::Success);
    }

    if (branchCard_)
        branchCard_->setVisible(true);

    if (failureCard_)
        failureCard_->setVisible(false);
}

void ConnectingScreen::ShowFailure(const std::string& reason)
{
    phase_ = Phase::Failed;

    if (statusLabel_)
    {
        statusLabel_->setText("Connection failed");
        statusLabel_->setTextColor(UITheme::Danger);
    }

    if (failureReason_)
        failureReason_->setText(reason);

    if (failureCard_)
        failureCard_->setVisible(true);

    if (branchCard_)
        branchCard_->setVisible(false);

    // Mark the step that failed.
    if (currentStep_ < kStepCount)
    {
        if (stepIcons_[currentStep_])
        {
            stepIcons_[currentStep_]->setShape(UIIcon::Shape::Cross);
            stepIcons_[currentStep_]->setColor(UITheme::Danger);
        }
        if (stepLabels_[currentStep_])
            stepLabels_[currentStep_]->setTextColor(UITheme::Danger);
    }
}

void ConnectingScreen::Retry()
{
    phase_       = Phase::Running;
    currentStep_ = 0;
    stepTimer_   = 0.0f;

    if (failureCard_)
        failureCard_->setVisible(false);

    if (statusLabel_)
    {
        statusLabel_->setText("Retrying...");
        statusLabel_->setTextColor(UITheme::Accent);
    }

    for (size_t i = 0; i < kStepCount; ++i)
    {
        if (stepIcons_[i])
        {
            stepIcons_[i]->setShape(UIIcon::Shape::Ring);
            stepIcons_[i]->setColor(UITheme::Muted);
        }
        if (stepLabels_[i])
            stepLabels_[i]->setTextColor(UITheme::Muted);
    }
}

void ConnectingScreen::Update(float deltaTime)
{
    elapsed_ += deltaTime;

    // crystalPulse: the badge's glow breathes on a 1.6s cycle.
    if (badge_)
    {
        const float phase = 0.5f + 0.5f * std::sin(elapsed_ * 2.0f * std::numbers::pi_v<float> / 1.6f);
        badge_->setGlow(UITheme::WithAlpha(UITheme::Accent, 0.30f + 0.40f * phase),
                        S(8.0f) + S(12.0f) * phase);
    }

    if (phase_ != Phase::Running)
        return;

    stepTimer_ += deltaTime;

    if (currentStep_ >= kStepCount)
    {
        ShowBranch();
        return;
    }

    if (stepTimer_ < kSteps[currentStep_].duration)
        return;

    // The "Connecting" step is the one backed by a real service.
    if (currentStep_ == 1 && !AttemptConnect())
    {
        const std::string reason = engine_ ? engine_->getNetworkManager().getLastError()
                                           : std::string();

        ShowFailure(reason.empty() ? "Unable to reach the game server."
                                   : "Connection failed: " + reason);
        return;
    }

    if (stepIcons_[currentStep_])
    {
        stepIcons_[currentStep_]->setShape(UIIcon::Shape::Check);
        stepIcons_[currentStep_]->setColor(UITheme::Success);
    }

    if (stepLabels_[currentStep_])
        stepLabels_[currentStep_]->setTextColor(UITheme::Subtext);

    stepTimer_ = 0.0f;
    ++currentStep_;

    if (currentStep_ >= kStepCount)
        ShowBranch();
}

void ConnectingScreen::OnKeyDown(int key, bool, bool)
{
    // Escape backs out to login from either the branch or the failure state.
    if (key == UIKey::Escape && phase_ != Phase::Running)
        RequestScreenChange(ScreenID::Login);
}
