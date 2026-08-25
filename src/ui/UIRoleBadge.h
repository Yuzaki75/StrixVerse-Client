#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "UIElement.h"
#include "../graphics/Color.h"

class Engine;
class UILabel;
class UIPanel;

// -----------------------------------------------------------------------------
// UIRoleBadge
//
// The small role pill for player rows: a rounded chip tinted in the role's
// colour with the role name inside. Colours mirror PlayerListPanel::RoleColor
// exactly so a name reads the same in the roster and anywhere else a badge
// appears.
//
// Decorative by design - it takes no input, so rows stay clickable through it.
// Auto-sizes to its text whenever SetRole is called.
// -----------------------------------------------------------------------------
class UIRoleBadge : public UIElement
{
public:
    explicit UIRoleBadge(Engine* engine);
    ~UIRoleBadge() override = default;

    // Known names colour per PlayerListPanel; anything unknown reads Muted.
    void SetRole(const std::string& roleName);

    // 0 Visitor / 1 Member / 2 Builder / 3 Co-Owner / 4 Owner, as the spawn
    // packets carry it (mirrors Server/src/world/WorldRole.h).
    void SetRole(std::uint8_t worldRole);

    const std::string& getRoleName() const { return roleName_; }

protected:
    // Nothing of its own to draw: the pill child carries the fill and border,
    // the label carries the text. The base class renders both after this.
    void renderSelf(UIRenderer& renderer) const override { (void)renderer; }

private:
    void applyRole(const std::string& roleName, const Color& color);

    Engine* engine_ = nullptr;

    std::shared_ptr<UIPanel> pill_;
    std::shared_ptr<UILabel> label_;

    std::string roleName_;
};
