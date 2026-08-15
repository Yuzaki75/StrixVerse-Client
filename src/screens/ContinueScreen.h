#pragma once

#include <memory>

#include "Screen.h"
#include "../core/WorldManager.h"

class UIButton;
class UILabel;
class UIPanel;

/**
 * Continue screen.
 *
 * Shows the world the player was last in - type, owner, last played, position
 * and population - with Continue and Change World actions.
 *
 * The design's auto-join countdown is implemented as specified: it runs down
 * and joins on its own, but any key or click cancels it, so the player is never
 * forced through a timer-only transition.
 */
class ContinueScreen : public Screen
{
public:
    explicit ContinueScreen(Engine* engine);
    ~ContinueScreen() override = default;

    void OnEnter() override;
    void Update(float deltaTime) override;

    // The countdown must be cancellable even though buttons hold focus.
    bool WantsRawInput() const override { return true; }

    void OnKeyDown(int key, bool ctrl, bool shift) override;
    void OnMouseDown(float x, float y) override;

private:
    void BuildCard(float centreX, float y, float width);
    void BuildShards();

    void CancelAutoJoin();
    void JoinLastWorld();

    std::shared_ptr<UILabel> countdownLabel_;

    LastWorldSession session_;
    bool             hasSession_ = false;

    float countdown_    = 0.0f;
    bool  autoJoining_  = false;
    bool  joining_      = false;
};
