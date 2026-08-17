#pragma once

#include <memory>

#include "Screen.h"

class UIButton;
class UILabel;

/**
 * Credits.
 *
 * Lists the people and the third-party work the client is built on. The
 * library list is not decoration: these are the licences the build actually
 * links against, so it is the one place that has to stay accurate as
 * dependencies change.
 */
class CreditsScreen : public Screen
{
public:
    explicit CreditsScreen(Engine* engine);
    ~CreditsScreen() override = default;

    void OnEnter() override;
    void OnKeyDown(int key, bool ctrl, bool shift) override;

private:
    void OnBack();

    std::shared_ptr<UIButton> backButton_;
};
