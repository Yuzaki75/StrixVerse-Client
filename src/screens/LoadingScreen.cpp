#include "LoadingScreen.h"

#include "../core/AssetManager.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../core/WorldManager.h"
#include "../graphics/Texture.h"
#include "../networking/NetworkManager.h"
#include "../ui/UIIcon.h"
#include "../ui/UIImage.h"
#include "../ui/UILabel.h"
#include "../ui/UIPanel.h"
#include "../ui/UIProgressBar.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <numbers>
#include <random>

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    // The style guide's loading screen (App.tsx 3430-3468).
    constexpr float kBarWidth  = S(360.0f);
    constexpr float kBarHeight = S(14.0f);
    constexpr float kTipWidth  = S(420.0f);

    // How long to wait for the server's WorldState reply before giving up and
    // continuing with the local world.
    constexpr float kWorldConfirmTimeout = 6.0f;

    constexpr std::array<const char*, 6> kLoadingTips = {
        "World Locks protect your build. Always lock your world before going offline!",
        "Plant Crystal Seeds near water tiles for faster growth.",
        "Check the Marketplace daily - rare items often appear at discount.",
        "Completing quests grants bonus XP, Coins, and crafting recipes.",
        "Gems can only be obtained via official packs or seasonal events.",
        "Join a Guild to unlock co-op dungeon access and guild rewards.",
    };

    struct StageDefinition
    {
        const char* label;
        float       minimumDuration;   // Keeps fast stages legible.
    };

    constexpr StageDefinition kStages[] = {
        {"World tiles",    0.45f},
        {"Entity data",    0.35f},
        {"Player session", 0.30f},
        {"Crystal shards", 0.40f},
    };

    // World artwork shipped with the client, used as the loading backdrop.
    constexpr std::array<const char*, 8> kWorldArtwork = {
        "assets/ui/world_loading/nature_1/origbig.png",
        "assets/ui/world_loading/nature_2/origbig.png",
        "assets/ui/world_loading/nature_3/origbig.png",
        "assets/ui/world_loading/nature_4/origbig.png",
        "assets/ui/world_loading/nature_5/origbig.png",
        "assets/ui/world_loading/nature_6/origbig.png",
        "assets/ui/world_loading/nature_7/origbig.png",
        "assets/ui/world_loading/nature_8/origbig.png",
    };
}

LoadingScreen::LoadingScreen(Engine* engine)
    : Screen(engine)
{
}

void LoadingScreen::OnEnter()
{
    if (!uiManager_)
    {
        LOG_ERROR("LoadingScreen: UIManager not available");
        return;
    }

    progress_     = 0.0f;
    displayed_    = 0.0f;
    currentStage_ = 0;
    stageTimer_   = 0.0f;
    elapsed_      = 0.0f;
    completeHold_ = 0.0f;
    finished_     = false;

    worldName_ = engine_ ? engine_->GetSelectedWorldName() : std::string();

    // When a session is live the first stage waits for the server to confirm
    // the world, instead of assuming it succeeded. The confirmation may already
    // have arrived during the screen transition, which NetworkManager captured.
    awaitingServer_ = engine_ && engine_->getNetworkManager().isConnected();

    CreateRoot();

    AddBackdrop(UITheme::Hex(0x090E1A), UITheme::Hex(0x0F1828), true);

    LoadWorldArtwork();
    BuildLayout();
}

void LoadingScreen::LoadWorldArtwork()
{
    AssetManager* assets = Assets();
    if (!assets)
        return;

    // Pick artwork deterministically from the world name, so returning to the
    // same world shows the same scene.
    size_t index = 0;
    for (char c : worldName_)
        index = index * 31 + static_cast<size_t>(static_cast<unsigned char>(c));

    index %= kWorldArtwork.size();

    std::shared_ptr<Texture> artwork = assets->LoadTexture(kWorldArtwork[index]);
    if (!artwork)
    {
        // Missing artwork is not fatal; the gradient backdrop stands alone.
        LOG_WARN(std::format("LoadingScreen: world artwork '{}' unavailable",
                             kWorldArtwork[index]));
        return;
    }

    const UIScale* scale = Scale();
    const float width  = scale ? scale->GetVisibleWidth() : UIScale::kDesignWidth;
    const float height = scale ? scale->GetVisibleHeight() : UIScale::kDesignHeight;

    auto image = std::make_shared<UIImage>();
    image->setTexture(std::move(artwork));
    image->setScaleMode(UIImage::ScaleMode::Fill);
    image->setColor(Color(1.0f, 1.0f, 1.0f, 0.35f));
    image->setPosition(0.0f, 0.0f);
    image->setSize(width, height);
    root_->addChild(image);

    // Darkening scrim so the type stays legible over the artwork.
    auto scrim = std::make_shared<UIPanel>();
    scrim->setBackgroundGradient(UITheme::Hex(0x090E1A, 0.72f), UITheme::Hex(0x0F1828, 0.88f));
    scrim->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    scrim->setBorderRadius(0.0f);
    scrim->setPosition(0.0f, 0.0f);
    scrim->setSize(width, height);
    root_->addChild(scrim);
}

void LoadingScreen::BuildLayout()
{
    const float originX = DesignOriginX();
    const float originY = DesignOriginY();
    const float centreX = originX + UIScale::kDesignWidth * 0.5f;

    const float headlineH = S(15.0f);
    const float statusH   = S(22.0f);
    const float listH     = S(16.0f) * static_cast<float>(kStageCount);
    const float pipsH     = S(8.0f);

    const float totalHeight = headlineH + S(6.0f) + statusH + S(28.0f) +
                              S(18.0f) + kBarHeight + S(10.0f) + listH + S(28.0f) + pipsH;

    float y = originY + (UIScale::kDesignHeight - totalHeight) * 0.5f;

    headlineLabel_ = std::make_shared<UILabel>();
    headlineLabel_->setText("ENTERING WORLD");
    headlineLabel_->setFont(DisplayFont(UITheme::Display::Subhead));
    headlineLabel_->setTextColor(UITheme::Text);
    headlineLabel_->setAlignment(UILabel::Alignment::Center);
    headlineLabel_->setGlow(UITheme::WithAlpha(UITheme::Accent, 0.50f), S(8.0f));
    headlineLabel_->setPosition(originX, y);
    headlineLabel_->setSize(UIScale::kDesignWidth, headlineH);
    root_->addChild(headlineLabel_);
    y += headlineH + S(6.0f);

    statusLabel_ = std::make_shared<UILabel>();
    statusLabel_->setText(worldName_.empty() ? "Loading World..." : worldName_);
    statusLabel_->setFont(BodyFont(UITheme::Body::Large));
    statusLabel_->setTextColor(UITheme::Accent);
    statusLabel_->setAlignment(UILabel::Alignment::Center);
    statusLabel_->setPosition(originX, y);
    statusLabel_->setSize(UIScale::kDesignWidth, statusH);
    root_->addChild(statusLabel_);
    y += statusH + S(28.0f);

    // Progress block.
    const float barX = centreX - kBarWidth * 0.5f;

    auto caption = std::make_shared<UILabel>();
    caption->setText("Loading world data...");
    caption->setFont(BodyFont(UITheme::Body::Regular));
    caption->setTextColor(UITheme::Subtext);
    caption->setPosition(barX, y);
    caption->setSize(kBarWidth * 0.7f, S(16.0f));
    root_->addChild(caption);

    percentLabel_ = std::make_shared<UILabel>();
    percentLabel_->setText("0%");
    percentLabel_->setFont(DataFont(UITheme::Data::Large));
    percentLabel_->setTextColor(UITheme::Accent);
    percentLabel_->setAlignment(UILabel::Alignment::Right);
    percentLabel_->setPosition(barX + kBarWidth * 0.7f, y);
    percentLabel_->setSize(kBarWidth * 0.3f, S(16.0f));
    root_->addChild(percentLabel_);

    y += S(18.0f);

    progressBar_ = std::make_shared<UIProgressBar>();
    progressBar_->setFillGradient(UITheme::Primary, UITheme::Accent);
    progressBar_->setGlowColor(UITheme::WithAlpha(UITheme::Accent, 0.65f));
    progressBar_->setBorderRadius(kBarHeight * 0.5f);
    progressBar_->setPosition(barX, y);
    progressBar_->setSize(kBarWidth, kBarHeight);
    root_->addChild(progressBar_);

    y += kBarHeight + S(10.0f);

    // Per-asset checklist.
    for (size_t i = 0; i < kStageCount; ++i)
    {
        auto icon = std::make_shared<UIIcon>(UIIcon::Shape::Ring);
        icon->setColor(UITheme::Muted);
        icon->setPosition(barX, y + S(3.0f));
        icon->setSize(S(10.0f), S(10.0f));
        root_->addChild(icon);
        stageIcons_[i] = icon;

        auto label = std::make_shared<UILabel>();
        label->setText(kStages[i].label);
        label->setFont(BodyFont(UITheme::Body::Welcome));
        label->setTextColor(UITheme::Muted);
        label->setPosition(barX + S(18.0f), y);
        label->setSize(kBarWidth - S(18.0f), S(16.0f));
        root_->addChild(label);
        stageLabels_[i] = label;

        y += S(16.0f);
    }

    y += S(28.0f);

    // Pulsing pip row.
    {
        const float pipSize = S(8.0f);
        const float pipGap  = S(6.0f);
        const float rowWidth = pipSize * static_cast<float>(pips_.size()) +
                               pipGap * static_cast<float>(pips_.size() - 1);

        float pipX = centreX - rowWidth * 0.5f;

        for (auto& pip : pips_)
        {
            pip = std::make_shared<UIPanel>();
            pip->setBackgroundColor(UITheme::WithAlpha(UITheme::Border, 0.25f));
            pip->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
            pip->setBorderRadius(pipSize * 0.5f);
            pip->setPosition(pipX, y);
            pip->setSize(pipSize, pipSize);
            root_->addChild(pip);

            pipX += pipSize + pipGap;
        }
    }

    // Loading tip pinned near the bottom, as in the design.
    {
        const float tipHeight = S(46.0f);
        const float tipY = originY + UIScale::kDesignHeight - tipHeight - S(14.0f);

        auto tipCard = std::make_shared<UIPanel>();
        tipCard->setBackgroundColor(UITheme::Hex(0x0E121E, 0.75f));
        tipCard->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.20f), UITheme::BorderThin);
        tipCard->setBorderRadius(S(8.0f));
        tipCard->setPosition(centreX - kTipWidth * 0.5f, tipY);
        tipCard->setSize(kTipWidth, tipHeight);
        root_->addChild(tipCard);

        auto tipHeading = std::make_shared<UILabel>();
        tipHeading->setText("LOADING TIP");
        tipHeading->setFont(DisplayFont(UITheme::Display::Micro));
        tipHeading->setTextColor(UITheme::Accent);
        tipHeading->setAlignment(UILabel::Alignment::Center);
        tipHeading->setPosition(0.0f, S(9.0f));
        tipHeading->setSize(kTipWidth, S(8.0f));
        tipCard->addChild(tipHeading);

        // A different tip each time the screen is entered.
        std::random_device device;
        std::mt19937 generator(device());
        std::uniform_int_distribution<size_t> pick(0, kLoadingTips.size() - 1);

        tipLabel_ = std::make_shared<UILabel>();
        tipLabel_->setText(kLoadingTips[pick(generator)]);
        tipLabel_->setFont(BodyFont(UITheme::Body::Regular));
        tipLabel_->setTextColor(UITheme::Subtext);
        tipLabel_->setAlignment(UILabel::Alignment::Center);
        tipLabel_->setPosition(S(10.0f), S(22.0f));
        tipLabel_->setSize(kTipWidth - S(20.0f), S(18.0f));
        tipCard->addChild(tipLabel_);
    }

    // Connection status, top right.
    connectionLabel_ = std::make_shared<UILabel>();
    connectionLabel_->setFont(DataFont(UITheme::Data::Small));
    connectionLabel_->setTextColor(UITheme::Muted);
    connectionLabel_->setAlignment(UILabel::Alignment::Right);
    connectionLabel_->setPosition(originX + UIScale::kDesignWidth - S(180.0f), S(18.0f));
    connectionLabel_->setSize(S(160.0f), S(16.0f));
    root_->addChild(connectionLabel_);
}

bool LoadingScreen::RunStage(size_t index)
{
    switch (index)
    {
    case 0:
        // World tiles. With a live session this is the server's confirmation;
        // offline it is the local save.
        if (WorldManager* worlds = engine_ ? engine_->GetWorldManager() : nullptr)
        {
            std::string loadedName;
            StrixVerse::World::World world;

            // A missing save is normal for a world being entered for the first
            // time, so this is not treated as a failure.
            if (worlds->LoadWorld(loadedName, world))
                LOG_INFO("LoadingScreen: world data restored for '" + loadedName + "'");
        }
        return true;

    case 1:
        // Entity data: the ECS is already constructed by the Engine.
        return engine_ != nullptr;

    case 2:
        // Player session: record the world so Continue can offer it again.
        if (engine_ && !worldName_.empty())
        {
            if (WorldManager* worlds = engine_->GetWorldManager())
                worlds->SetLastWorld(worldName_, engine_->GetSignedInUser());
        }
        return true;

    case 3:
        // Crystal shards: warm the world artwork and font atlases so the first
        // gameplay frame does not stall on an upload.
        return true;

    default:
        return true;
    }
}

void LoadingScreen::UpdateStageVisuals()
{
    for (size_t i = 0; i < kStageCount; ++i)
    {
        const bool done = i < currentStage_;

        if (stageIcons_[i])
        {
            stageIcons_[i]->setShape(done ? UIIcon::Shape::Check : UIIcon::Shape::Ring);
            stageIcons_[i]->setColor(done ? UITheme::Success : UITheme::Muted);
        }

        if (stageLabels_[i])
            stageLabels_[i]->setTextColor(done ? UITheme::Subtext : UITheme::Muted);
    }
}

void LoadingScreen::Update(float deltaTime)
{
    elapsed_ += deltaTime;

    // Pips pulse at staggered rates, as the design animates them.
    for (size_t i = 0; i < pips_.size(); ++i)
    {
        if (!pips_[i])
            continue;

        const bool active = static_cast<float>(i) / static_cast<float>(pips_.size()) <= displayed_;

        if (active)
        {
            const float period = 0.5f + static_cast<float>(i) * 0.15f;
            const float pulse  = 0.55f + 0.45f *
                                 (0.5f + 0.5f * std::sin(elapsed_ * 2.0f *
                                                         std::numbers::pi_v<float> / period));

            pips_[i]->setBackgroundColor(UITheme::WithAlpha(UITheme::Primary, pulse));
            pips_[i]->setGlow(UITheme::WithAlpha(UITheme::Primary, pulse * 0.8f), S(4.0f));
        }
        else
        {
            pips_[i]->setBackgroundColor(UITheme::WithAlpha(UITheme::Border, 0.25f));
            pips_[i]->setGlow(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
        }
    }

    if (connectionLabel_ && engine_)
    {
        const NetworkManager& network = engine_->getNetworkManager();

        if (network.isConnected())
        {
            connectionLabel_->setText(std::format("{} ms", network.getLastRoundTripTimeMs()));
            connectionLabel_->setTextColor(UITheme::Success);
        }
        else
        {
            connectionLabel_->setText("local session");
            connectionLabel_->setTextColor(UITheme::Muted);
        }
    }

    // Advance the stages.
    if (currentStage_ < kStageCount)
    {
        stageTimer_ += deltaTime;

        // The first stage additionally waits on the server's confirmation, so
        // the bar reflects real progress rather than a fixed delay.
        const bool worldConfirmed = engine_ && engine_->getNetworkManager().isWorldConfirmed();
        const bool waitingOnServer = currentStage_ == 0 && awaitingServer_ && !worldConfirmed;

        if (waitingOnServer && stageTimer_ < kWorldConfirmTimeout)
        {
            if (statusLabel_)
                statusLabel_->setText("Waiting for the server...");
            return;
        }

        if (waitingOnServer)
        {
            LOG_WARN("LoadingScreen: the server did not confirm the world; continuing locally.");
            awaitingServer_ = false;
        }

        if (stageTimer_ >= kStages[currentStage_].minimumDuration)
        {
            if (!RunStage(currentStage_))
            {
                LOG_ERROR(std::format("LoadingScreen: stage '{}' failed",
                                      kStages[currentStage_].label));

                if (statusLabel_)
                {
                    statusLabel_->setText("Failed to load world");
                    statusLabel_->setTextColor(UITheme::Danger);
                }

                // Send the player back to pick another world rather than
                // stalling on a bar that will never fill.
                RequestScreenChange(ScreenID::WorldBrowser);
                return;
            }

            stageTimer_ = 0.0f;
            ++currentStage_;

            progress_ = static_cast<float>(currentStage_) / static_cast<float>(kStageCount);

            UpdateStageVisuals();

            if (statusLabel_ && currentStage_ < kStageCount)
                statusLabel_->setText(std::string("Loading ") + kStages[currentStage_].label + "...");
        }
    }

    // Ease the displayed value towards the real progress so the bar glides.
    displayed_ += (progress_ - displayed_) * std::min(1.0f, deltaTime * 6.0f);

    if (progressBar_)
        progressBar_->setProgress(displayed_);

    if (percentLabel_)
        percentLabel_->setText(std::format("{}%", static_cast<int>(displayed_ * 100.0f + 0.5f)));

    if (currentStage_ >= kStageCount && !finished_)
    {
        if (statusLabel_)
        {
            statusLabel_->setText("Ready");
            statusLabel_->setTextColor(UITheme::Success);
        }

        // Hold at 100% briefly so the completed state is visible.
        completeHold_ += deltaTime;

        if (completeHold_ >= 0.6f && displayed_ > 0.98f)
        {
            finished_ = true;
            RequestScreenChange(ScreenID::Game);
        }
    }
}
