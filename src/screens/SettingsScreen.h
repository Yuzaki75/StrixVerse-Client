#pragma once

#include <memory>

#include "Screen.h"

class UIButton;
class UILabel;

/**
 * Settings screen.
 *
 * Not part of the delivered Figma screen set, so its content is unchanged;
 * it has been ported onto the repaired UI system and restyled with the shared
 * theme tokens so it does not look foreign next to the designed screens.
 */
class SettingsScreen : public Screen
{
public:
    explicit SettingsScreen(Engine* engine);
    ~SettingsScreen() override = default;

    void OnEnter() override;
    void OnKeyDown(int key, bool ctrl, bool shift) override;

private:
    void OnBackButtonClicked();

    // Leaves the world and forgets it, so the next sign-in starts at World
    // Selection instead of offering to continue into it.
    void OnLeaveWorldClicked();

    std::shared_ptr<UILabel>  titleLabel_;
    std::shared_ptr<UIButton> backButton_;
    std::shared_ptr<UIButton> leaveButton_;
};
