#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include "Screen.h"

class UIIcon;
class UILabel;
class UIPanel;
class UIProgressBar;

/**
 * World Loading screen.
 *
 * Implements the style guide's loading view: the world artwork backdrop, the
 * headline progress bar with its percentage readout, the per-asset checklist,
 * the pulsing pip row and the rotating loading tip.
 *
 * Progress is driven by real milestones rather than a single timer: the world
 * data stage completes when the server confirms the join (or the local save
 * answers), and the terrain stage tracks NetworkManager's chunk counters.
 */
class LoadingScreen : public Screen
{
public:
    explicit LoadingScreen(Engine* engine);
    ~LoadingScreen() override = default;

    void OnEnter() override;
    void Update(float deltaTime) override;

    // Number of milestone stages; also sizes the checklist layout.
    static constexpr size_t kStageCount = 3;

private:
    // Stages as milestone indexes; see kStageLabels in the source file.
    enum Stage
    {
        StageWorldData = 0,
        StageTerrain   = 1,
        StageSession   = 2,
    };

    void BuildLayout();
    void LoadWorldArtwork();

    // Performs the work for one stage. Returns false if the stage failed.
    bool RunStage(size_t index);

    void UpdateStageVisuals();

    std::shared_ptr<UIProgressBar> progressBar_;
    std::shared_ptr<UILabel>       headlineLabel_;
    std::shared_ptr<UILabel>       statusLabel_;
    std::shared_ptr<UILabel>       percentLabel_;
    std::shared_ptr<UILabel>       tipLabel_;
    std::shared_ptr<UILabel>       connectionLabel_;

    std::array<std::shared_ptr<UIIcon>,  kStageCount> stageIcons_{};
    std::array<std::shared_ptr<UILabel>, kStageCount> stageLabels_{};
    std::array<std::shared_ptr<UIPanel>, 5>           pips_{};

    std::string worldName_;

    // True while the "World data" stage waits for the server's WorldState
    // confirmation, which NetworkManager records for us.
    bool awaitingServer_ = false;

    float  progress_        = 0.0f;
    float  displayed_       = 0.0f;
    size_t currentStage_    = 0;
    float  stageTimer_      = 0.0f;
    float  elapsed_         = 0.0f;
    float  completeHold_    = 0.0f;
    bool   finished_        = false;

    // Terrain stage bookkeeping. The quiet timer watches the chunk counter:
    // the server sends the whole world in one burst right after WorldState,
    // so when nothing new arrives for a while the burst is over even though
    // no expected total was ever announced.
    uint32_t lastChunkCount_  = 0;
    float    chunkQuietTimer_ = 0.0f;

    // Last values shown in the labels Update() touches every frame.
    // setText copies its argument, so each avoided rewrite also avoids a
    // per-frame string allocation.
    uint32_t shownChunks_          = 0;
    int      shownPercent_         = -1;
    bool     indeterminateShown_   = false;
    bool     waitingMessageShown_  = false;
    bool     readyMessageShown_    = false;
    uint32_t shownRtt_             = 0;
    bool     shownConnected_       = false;
};
