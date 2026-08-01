#include "SplashScreen.h"
#include "../ui/UIManager.h"
#include "../core/Logger.h"
#include "../graphics/Color.h"

SplashScreen::SplashScreen(Engine* engine)
    : Screen(engine)
    , titleLabel_(nullptr)
    , timer_(0.0f)
{
}

void SplashScreen::OnEnter()
{
    if (!uiManager_) {
        LOG_ERROR("SplashScreen: UIManager not available");
        return;
    }

    // Create title label
    titleLabel_ = std::make_shared<UILabel>();
    titleLabel_->setText("StrixVerse");
    titleLabel_->setTextColor({1.0f, 1.0f, 1.0f, 1.0f}); // White
    titleLabel_->setFontSize(72.0f);

    // Center the label
    // Note: We'll set position in Update after we know the screen size
    // For now, assume 800x600 and center at (400, 300)
    titleLabel_->setPosition(400.0f, 300.0f);

    uiManager_->addElement(titleLabel_);
}

void SplashScreen::OnExit()
{
    if (uiManager_ && titleLabel_) {
        uiManager_->removeElement(titleLabel_);
        titleLabel_.reset();
    }
}

void SplashScreen::Update(float deltaTime)
{
    // Update timer
    timer_ += deltaTime;

    // Center the label if we have the window size
    // For simplicity, we'll assume 800x600 and just set it once in OnEnter
    // In a real implementation, we'd get the window size from the engine

    // After 3 seconds, request to change to login screen
    if (timer_ >= 3.0f) {
        RequestScreenChange(ScreenID::Login);
    }
}