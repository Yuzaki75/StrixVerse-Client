#pragma once

#include <array>
#include <memory>

#include "Screen.h"

class UIButton;
class UILabel;

/**
 * Main menu.
 *
 * Shown once the splash is dismissed. Not part of the delivered Figma set, so
 * it is built from the same tokens and background treatment as the splash to
 * read as a continuation of it rather than a foreign screen.
 *
 * Play Online is the only entry that leaves the front end; Settings and
 * Credits come back here, and Exit closes the client.
 */
class MainMenuScreen : public Screen
{
public:
    explicit MainMenuScreen(Engine* engine);
    ~MainMenuScreen() override = default;

    void OnEnter() override;
    void Update(float deltaTime) override;
    void OnKeyDown(int key, bool ctrl, bool shift) override;

private:
    void BuildBackground();
    void BuildTitle();
    void BuildMenu();
    void BuildFooter();

    void OnPlayOnline();

    // Not named OnExit: Screen::OnExit is the virtual called when a screen is
    // torn down, so that name would quit the game on every screen change out
    // of the menu.
    void OnQuitGame();

    std::array<std::shared_ptr<UIButton>, 4> buttons_{};

    std::shared_ptr<UILabel> statusLabel_;

    float elapsed_ = 0.0f;
};
