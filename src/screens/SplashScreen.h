#pragma once

#include "Screen.h"
#include <memory>
#include "../ui/UILabel.h"

/**
 * Splash screen shown at game startup
 */
class SplashScreen : public Screen {
public:
    SplashScreen(Engine* engine);
    ~SplashScreen() override = default;

    // Screen overrides
    void OnEnter() override;
    void OnExit() override;
    void Update(float deltaTime) override;

private:
    std::shared_ptr<UILabel> titleLabel_;
    float timer_; // seconds
};