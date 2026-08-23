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
#include <string>

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

    // How long the terrain stage waits for chunks before giving up on the
    // server and continuing with whatever arrived.
    constexpr float kChunkTimeout = 15.0f;

    // The server sends every chunk in one burst right after WorldState. When
    // no expected total is known, this much silence after at least one chunk
    // means the burst is over.
    constexpr float kChunkIdleTimeout = 1.5f;

    // Shortest total time on screen, so a fast load does not just flash by.
    constexpr float kMinTotalTime = 0.5f;

    constexpr std::array<const char*, 6> kLoadingTips = {
        "Select a block from your hotbar and click a tile to build - every "
        "placement is stored server-side.",
        "Claim the Strix Core to protect your world and manage who may visit.",
        "Aether technology keeps the Strix Core running - raise its level to "
        "strengthen your world.",
        "World owners can toggle building, breaking and visitor access in the "
        "world settings.",
        "Press Enter to open chat, then press Enter again to send.",
        "Broken blocks regrow on protected worlds - building near your Core is "
        "always safe.",
    };

    // Milestone names, in the order they complete. There are no timed stages:
    // each one finishes when the thing it names has actually happened.
    constexpr std::size_t kStageCount = 3;
    constexpr std::array<const char*, kStageCount> kStageLabels = {
        "World data",
        "Terrain",
        "Player session",
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

    lastChunkCount_  = 0;
    chunkQuietTimer_ = 0.0f;

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
        label->setText(kStageLabels[i]);
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
    case StageWorldData:
        // The server's confirmation (or the timeout) has already been handled
        // by the time this runs. A missing record is normal the first time an
        // account plays, so this is not treated as a failure. It restores
        // which world was last entered, not the world itself - the server
        // owns that.
        if (WorldManager* worlds = engine_ ? engine_->GetWorldManager() : nullptr)
        {
            std::string loadedName;
            if (worlds->LoadLastSession(loadedName))
                LOG_INFO("LoadingScreen: last session was in '" + loadedName + "'");
        }
        return true;

    case StageTerrain:
        // Terrain arrives over the network and is counted by NetworkManager;
        // there is nothing left to do here but acknowledge it.
        return true;

    case StageSession:
        // Player session: record the world so Continue can offer it again.
        if (engine_ && !worldName_.empty())
        {
            if (WorldManager* worlds = engine_->GetWorldManager())
                worlds->SetLastWorld(worldName_, engine_->GetSignedInUser());
        }
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

    // Advance the stages. Each one finishes on a real milestone - there are
    // no fixed durations left; timeouts exist only so a silent server cannot
    // trap the player here forever.
    bool indeterminateTerrain = false;

    if (currentStage_ < kStageCount)
    {
        stageTimer_ += deltaTime;

        const NetworkManager& network = engine_->getNetworkManager();
        const bool worldConfirmed = engine_ && network.isWorldConfirmed();

        // The first stage waits on the server's confirmation, so the bar
        // reflects real progress rather than a fixed delay.
        bool waitingOnServer = currentStage_ == StageWorldData &&
                               awaitingServer_ && !worldConfirmed;

        if (waitingOnServer)
        {
            if (stageTimer_ < kWorldConfirmTimeout)
            {
                if (statusLabel_)
                    statusLabel_->setText("Waiting for the server...");
            }
            else
            {
                LOG_WARN("LoadingScreen: the server did not confirm the world; "
                         "continuing locally.");
                awaitingServer_ = false;
                waitingOnServer = false;
            }
        }

        bool readyToAdvance = !waitingOnServer;

        if (readyToAdvance && currentStage_ == StageTerrain)
        {
            const uint32_t received = network.ChunksReceived();
            const uint32_t expected = network.ChunksExpected();

            // Watch the counter for silence: the server sends every chunk in
            // one burst right after WorldState, so when no expected total was
            // announced, a quiet gap means the burst has ended.
            if (received != lastChunkCount_)
            {
                lastChunkCount_  = received;
                chunkQuietTimer_ = 0.0f;
            }
            else
            {
                chunkQuietTimer_ += deltaTime;
            }

            if (!network.isConnected())
            {
                // A local session has no terrain to receive.
                readyToAdvance = true;
            }
            else if (expected > 0 && received >= expected)
            {
                readyToAdvance = true;
            }
            else if (received > 0 && chunkQuietTimer_ >= kChunkIdleTimeout)
            {
                LOG_INFO(std::format("LoadingScreen: chunk burst ended at {} chunk(s); "
                                     "the server never announced an expected total.",
                                     received));
                readyToAdvance = true;
            }
            else if (stageTimer_ >= kChunkTimeout)
            {
                LOG_WARN(std::format("LoadingScreen: still missing terrain after {:.0f}s "
                                     "({} of {} chunk(s)); continuing.",
                                     kChunkTimeout, received,
                                     expected > 0 ? std::to_string(expected)
                                                  : std::string("unknown")));
                readyToAdvance = true;
            }

            // Without an announced total there is no honest fraction to show,
            // so the bar sweeps instead.
            if (expected == 0)
                indeterminateTerrain = true;
        }

        if (readyToAdvance)
        {
            if (!RunStage(currentStage_))
            {
                LOG_ERROR(std::format("LoadingScreen: stage '{}' failed",
                                      kStageLabels[currentStage_]));

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

            UpdateStageVisuals();

            if (statusLabel_ && currentStage_ < kStageCount)
                statusLabel_->setText(std::string("Loading ") +
                                      kStageLabels[currentStage_] + "...");
        }
    }

    // Overall progress: completed stages plus whatever fraction of the
    // terrain stage has genuinely arrived.
    {
        float stageFraction = 0.0f;

        if (currentStage_ == StageTerrain && engine_)
        {
            const NetworkManager& network = engine_->getNetworkManager();

            if (network.ChunksExpected() > 0)
            {
                stageFraction = std::clamp(
                    static_cast<float>(network.ChunksReceived()) /
                        static_cast<float>(network.ChunksExpected()),
                    0.0f, 1.0f);
            }
        }

        const float overall = (static_cast<float>(currentStage_) + stageFraction) /
                              static_cast<float>(kStageCount);

        progress_ = std::clamp(overall, 0.0f, 1.0f);
    }

    if (indeterminateTerrain)
    {
        // No total to measure against, so sweep the bar back and forth
        // instead of pretending to know a percentage.
        const float sweep = 0.5f + 0.5f * std::sin(elapsed_ * 2.4f);
        displayed_ = 0.08f + 0.42f * sweep;
    }
    else
    {
        // Ease the displayed value towards the real progress so the bar glides.
        displayed_ += (progress_ - displayed_) * std::min(1.0f, deltaTime * 6.0f);
    }

    if (progressBar_)
        progressBar_->setProgress(displayed_);

    if (percentLabel_)
    {
        if (indeterminateTerrain)
            percentLabel_->setText(std::format("{} chunks",
                                               engine_
                                                   ? engine_->getNetworkManager().ChunksReceived()
                                                   : 0u));
        else
            percentLabel_->setText(std::format("{}%",
                                               static_cast<int>(displayed_ * 100.0f + 0.5f)));
    }

    if (currentStage_ >= kStageCount && !finished_)
    {
        if (statusLabel_)
        {
            statusLabel_->setText("Ready");
            statusLabel_->setTextColor(UITheme::Success);
        }

        // Hold at 100% briefly so the completed state is visible, and never
        // leave before the shortest legible time on screen.
        completeHold_ += deltaTime;

        if (completeHold_ >= 0.4f && elapsed_ >= kMinTotalTime && displayed_ > 0.98f)
        {
            finished_ = true;
            RequestScreenChange(ScreenID::Game);
        }
    }
}
