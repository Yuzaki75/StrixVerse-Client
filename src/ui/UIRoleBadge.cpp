#include "UIRoleBadge.h"

#include "../core/Engine.h"
#include "../graphics/Font.h"
#include "UIFonts.h"
#include "UILabel.h"
#include "UIPanel.h"
#include "UITheme.h"

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    // Padding between the pill edge and the label, in style-guide pixels.
    constexpr float kPadX = 8.0f;
    constexpr float kPadY = 3.0f;

    // Role colours, mirroring PlayerListPanel::RoleColor: Developer gold,
    // Moderator primary blue, Owner gold, Co-Owner accent cyan, Builder
    // green, Member/Visitor/Player plain white. Anything unknown is Muted.
    const Color& RoleColor(const std::string& role)
    {
        if (role == "Developer") return UITheme::Gold;
        if (role == "Moderator") return UITheme::Primary;

        if (role == "Owner")    return UITheme::Gold;
        if (role == "Co-Owner") return UITheme::Accent;
        if (role == "Builder")  return UITheme::Success;

        if (role == "Member" || role == "Visitor" || role == "Player")
            return UITheme::Text;

        return UITheme::Muted;
    }

    // Mirrors Server/src/world/WorldRole.h, same table GameScreen uses.
    const char* WorldRoleName(std::uint8_t role)
    {
        switch (role)
        {
        case 1:  return "Member";
        case 2:  return "Builder";
        case 3:  return "Co-Owner";
        case 4:  return "Owner";
        default: return "Visitor";
        }
    }
}

UIRoleBadge::UIRoleBadge(Engine* engine)
    : engine_(engine)
{
    pill_ = std::make_shared<UIPanel>();
    pill_->setBorderRadius(UITheme::RadiusChip);

    label_ = std::make_shared<UILabel>();

    pill_->addChild(label_);
    addChild(pill_);
}

void UIRoleBadge::SetRole(const std::string& roleName)
{
    applyRole(roleName, RoleColor(roleName));
}

void UIRoleBadge::SetRole(std::uint8_t worldRole)
{
    const std::string name = WorldRoleName(worldRole);
    applyRole(name, RoleColor(name));
}

void UIRoleBadge::applyRole(const std::string& roleName, const Color& color)
{
    roleName_ = roleName;

    UIFonts* fonts = engine_ ? engine_->GetUIFonts() : nullptr;
    label_->setFont(fonts ? fonts->Get(UIFonts::Typeface::Body, UITheme::Body::Tiny)
                          : nullptr);

    label_->setText(roleName);
    label_->setTextColor(color);

    // Auto-size to the text plus padding so any role name fits its chip.
    Font* face   = label_->getFont();
    float width  = S(kPadX) * 2.0f;
    float height = static_cast<float>(UITheme::Body::Tiny) + S(kPadY) * 2.0f;

    if (face && face->IsLoaded())
    {
        width  += face->MeasureWidth(roleName);
        height  = face->GetLineHeight() + S(kPadY) * 2.0f;
    }

    setSize(width, height);

    // The role colour at low alpha for the fill, full strength for the border
    // and text - the standard tinted-chip treatment.
    pill_->setSize(width, height);
    pill_->setBackgroundColor(UITheme::WithAlpha(color, 0.22f));
    pill_->setBorder(color, UITheme::BorderThin);

    label_->setPosition(S(kPadX), (height - (face ? face->GetLineHeight()
                                                  : static_cast<float>(UITheme::Body::Tiny))) * 0.5f);
    label_->setSize(width - S(kPadX) * 2.0f, height - S(kPadY) * 2.0f);
}
