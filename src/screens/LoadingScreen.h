#pragma once

#include <array>
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
 * Progress is driven by real loading stages rather than a single timer, so the
 * bar reflects the work the client is actually doing.
 */
class LoadingScreen : public Screen
{
public:
    explicit LoadingScreen(Engine* engine);
    ~LoadingScreen() override = default;

    void OnEnter() override;
    void Update(float deltaTime) override;

private:
    static constexpr size_t kStageCount = 4;

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

    // True while the "World tiles" stage waits for the server's WorldState
    // confirmation, which NetworkManager records for us.
    bool awaitingServer_ = false;

    float  progress_      = 0.0f;
    float  displayed_     = 0.0f;
    size_t currentStage_  = 0;
    float  stageTimer_    = 0.0f;
    float  elapsed_       = 0.0f;
    float  completeHold_  = 0.0f;
    bool   finished_      = false;
};
