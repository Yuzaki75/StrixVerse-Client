#include "Screen.h"
#include "../core/Engine.h"
#include "../ui/UIManager.h"
#include "../core/Logger.h"

Screen::Screen(Engine *engine)
    : engine_(engine)
{
    if (engine_)
    {
        uiManager_ = engine_->GetUIManager();
    }
    if (!uiManager_)
    {
        LOG_WARN("Screen: UIManager not available");
    }
}

void Screen::Update(float deltaTime)
{
    // Update the UIManager
    if (uiManager_)
    {
        uiManager_->update(deltaTime);
    }
    // Update game logic (if any)
    UpdateGameLogic();
}

void Screen::Render() const
{
    // Render game (if any)
    RenderGame();
    // Render UI
    if (uiManager_)
    {
        uiManager_->render();
    }
}

void Screen::RequestScreenChange(ScreenID nextScreen)
{
    pendingChange_ = nextScreen;
}