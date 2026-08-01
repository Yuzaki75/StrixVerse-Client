#include "Screen.h"
#include "../core/Logger.h"

Screen::Screen(std::shared_ptr<Engine> engine)
    : engine_(engine)
{
    if (engine_) {
        uiManager_ = engine_->GetUIManager();
    }
    if (!uiManager_) {
        LOG_WARN("Screen: UIManager not available");
    }
}

void Screen::RequestChange(const std::string& screenName) {
    pendingChange_ = screenName;
}

// Note: ScreenManager would handle checking for pending changes and actually switching screens