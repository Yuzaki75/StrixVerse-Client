#pragma once

#include <memory>

#include "Screen.h"

class UILabel;
class UIPanel;

/**
 * Splash screen.
 *
 * Implements the 1920x1080 Figma frame exactly: the crystal-shard field, the
 * layered brand lockup with its glow, the pulsing prompt and the build/company
 * footers.
 *
 * The design says "PRESS ANY KEY TO CONTINUE", so this screen advances on real
 * input, not on a timer.
 */
class SplashScreen : public Screen
{
public:
    explicit SplashScreen(Engine* engine);
    ~SplashScreen() override = default;

    void OnEnter() override;
    void Update(float deltaTime) override;

    // The splash has no focusable UI, but it must still see every key.
    bool WantsRawInput() const override { return true; }

    void OnKeyDown(int key, bool ctrl, bool shift) override;
    void OnMouseDown(float x, float y) override;

private:
    void BuildBackground();

    // Background artwork plus its darkening scrim, sized to the visible canvas.
    void BuildArtwork(float width, float height);

    void BuildBrand();
    void BuildFooter();

    void Advance();

    std::shared_ptr<UILabel> promptLabel_;

    float elapsed_  = 0.0f;
    bool  advanced_ = false;
};
