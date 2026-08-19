#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "Screen.h"

class UIButton;
class UIIcon;
class UILabel;
class UIPanel;

/**
 * Connecting screen.
 *
 * Runs the sign-in sequence the design specifies - Authenticating, Connecting,
 * Loading Player Data, Checking Last World - showing each step resolve, then
 * branches to Continue (when the account has a last world) or World Selection.
 *
 * The connection step goes through AttemptConnect(), which is the single place
 * that has to change when the real NetworkManager session replaces the local
 * mock; the failure, retry and back-to-login paths are already live.
 */
class ConnectingScreen : public Screen
{
public:
    explicit ConnectingScreen(Engine* engine);
    ~ConnectingScreen() override = default;

    void OnEnter() override;
    void Update(float deltaTime) override;
    void OnKeyDown(int key, bool ctrl, bool shift) override;

private:
    enum class Phase
    {
        Running,    // Working through the steps
        Branch,     // All steps done, waiting for the player's choice
        Failed      // A step failed; retry or return to login
    };

    struct Step
    {
        const char* label;
        float       duration;
    };

    void BuildLayout();
    void BuildSteps(float centreX, float& y);
    void BuildFailureCard(float centreX, float y);

    // Returns false to drive the failure path. This is the seam for the real
    // NetworkManager connection.
    bool AttemptConnect();

    void Retry();
    void ShowBranch();
    void ShowFailure(const std::string& reason);

    static constexpr size_t kStepCount = 4;

    std::array<std::shared_ptr<UIIcon>,  kStepCount> stepIcons_{};
    std::array<std::shared_ptr<UILabel>, kStepCount> stepLabels_{};

    std::shared_ptr<UIPanel> badge_;
    std::shared_ptr<UILabel> headlineLabel_;
    std::shared_ptr<UILabel> statusLabel_;

    std::shared_ptr<UIPanel> failureCard_;
    std::shared_ptr<UILabel> failureReason_;

    Phase  phase_        = Phase::Running;
    size_t currentStep_  = 0;
    float  stepTimer_    = 0.0f;
    float  elapsed_      = 0.0f;
    bool   hasLastWorld_ = false;
};
