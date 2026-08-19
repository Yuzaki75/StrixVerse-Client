#pragma once

#include <functional>
#include <memory>
#include <string>

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

    // --- Settings rows -----------------------------------------------------
    // Built from buttons and labels only. The UI framework has no slider and
    // no checkbox, and inventing both to expose four values would be a larger
    // job than the settings themselves; stepper and toggle buttons say the
    // same thing with widgets that already work and can be replaced later
    // without touching the wiring underneath.
    void BuildSettingsRows(float x, float y, float width);

    // One labelled row: caption on the left, a control group on the right.
    // Returns the y for the next row.
    float BuildStepperRow(float x, float y, float width, const std::string& caption,
                          const std::shared_ptr<UILabel>& value,
                          const std::function<void()>& onDown,
                          const std::function<void()>& onUp);
    float BuildToggleRow(float x, float y, float width, const std::string& caption,
                         const std::shared_ptr<UIButton>& toggle);

    // Pushes the current config values into the row labels.
    // Moves through kResolutions by one step in either direction.
    void StepResolution(int direction);

    void RefreshSettingValues();

    // Config is written on every change and saved immediately, so a crash
    // between here and exit cannot lose the setting.
    void ApplyAndSave();

    // Leaves the world and forgets it, so the next sign-in starts at World
    // Selection instead of offering to continue into it.
    void OnLeaveWorldClicked();

    std::shared_ptr<UILabel>  titleLabel_;
    std::shared_ptr<UIButton> backButton_;
    std::shared_ptr<UIButton> leaveButton_;

    std::shared_ptr<UILabel>  volumeValue_;
    std::shared_ptr<UILabel>  resolutionValue_;
    std::shared_ptr<UIButton> fullscreenToggle_;
    std::shared_ptr<UIButton> vsyncToggle_;

    // Index into the offered resolutions, or -1 when the configured size is
    // not one of them - in which case it is shown but stepping starts from the
    // nearest entry rather than snapping silently.
    int resolutionIndex_ = -1;
};
