#include "CharacterPanel.h"

#include "../core/Engine.h"
#include "../graphics/Color.h"
#include "../ui/UIFonts.h"
#include "../ui/UILabel.h"
#include "../ui/UIManager.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

#include <format>

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    Font* PanelFont(Engine* engine, UIFonts::Typeface face, unsigned int size)
    {
        UIFonts* fonts = engine ? engine->GetUIFonts() : nullptr;
        return fonts ? fonts->Get(face, size) : nullptr;
    }

    // Panel geometry, in style-guide pixels. Narrower than the inventory: a
    // character sheet is a column of short rows.
    constexpr float kPanelWidth     = 170.0f;
    constexpr float kPanelHeight    = 190.0f;
    constexpr float kPadding        = 10.0f;
    constexpr float kRowHeight      = 14.0f;
    constexpr float kHeaderTop      = 26.0f;
    constexpr float kStatsTopOffset = 46.0f;

    const Color& PanelBackground()
    {
        static const Color color = UITheme::Hex(0x1E2230, 0.97f);
        return color;
    }
}

CharacterPanel::CharacterPanel(Engine* engine, UIManager* uiManager)
    : engine_(engine), uiManager_(uiManager)
{
}

CharacterPanel::~CharacterPanel()
{
    // The UIManager outlives this panel on a screen change, so the elements
    // have to be handed back or they keep drawing over whatever comes next.
    if (uiManager_ && root_)
        uiManager_->removeElement(root_);
}

void CharacterPanel::Build()
{
    if (root_ || !uiManager_)
        return;

    BuildFrame();

    root_->setVisible(false);
}

void CharacterPanel::BuildFrame()
{
    const float width  = S(kPanelWidth);
    const float height = S(kPanelHeight);

    root_ = std::make_shared<UIPanel>();
    root_->setSize(width, height);
    root_->setPosition((UIScale::kDesignWidth  - width)  * 0.5f,
                       (UIScale::kDesignHeight - height) * 0.5f);
    root_->setBackgroundColor(PanelBackground());
    root_->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.45f), UITheme::BorderThin);
    root_->setBorderRadius(UITheme::RadiusPanel);

    // An overlay over live terrain swallows clicks for the same reason the
    // world manager does: a bare panel would let every input fall through.
    root_->setBlocksInput(true);

    // Added to the UIManager rather than to a screen's root so it draws above
    // the HUD; UIManager renders in insertion order.
    uiManager_->addElement(root_);

    auto title = std::make_shared<UILabel>();
    title->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                             UITheme::Display::Section));
    title->setTextColor(UITheme::Text);
    title->setText("CHARACTER");
    title->setPosition(S(kPadding), S(8.0f));
    title->setSize(width - S(kPadding) * 2.0f, S(13.0f));
    root_->addChild(title);

    nameLabel_ = std::make_shared<UILabel>();
    nameLabel_->setFont(PanelFont(engine_, UIFonts::Typeface::Body,
                                  UITheme::Body::Medium));
    nameLabel_->setTextColor(UITheme::Text);
    nameLabel_->setPosition(S(kPadding), S(kHeaderTop));
    nameLabel_->setSize(width - S(kPadding) * 2.0f, S(16.0f));
    root_->addChild(nameLabel_);

    roleLabel_ = std::make_shared<UILabel>();
    roleLabel_->setFont(PanelFont(engine_, UIFonts::Typeface::Body,
                                  UITheme::Display::Small));
    roleLabel_->setTextColor(UITheme::Subtext);
    roleLabel_->setPosition(S(kPadding), S(kHeaderTop + 17.0f));
    roleLabel_->setSize(width - S(kPadding) * 2.0f, S(10.0f));
    root_->addChild(roleLabel_);

    levelLabel_ = std::make_shared<UILabel>();
    levelLabel_->setFont(PanelFont(engine_, UIFonts::Typeface::Data,
                                   UITheme::Display::Small));
    levelLabel_->setTextColor(UITheme::Accent);
    levelLabel_->setPosition(S(kPadding), S(kStatsTopOffset - 14.0f));
    levelLabel_->setSize(width - S(kPadding) * 2.0f, S(9.0f));
    root_->addChild(levelLabel_);
}

void CharacterPanel::Open()
{
    if (!root_)
        return;

    open_ = true;

    // UIManager renders in insertion order, so re-inserting keeps the overlay
    // above anything added after it was constructed.
    if (uiManager_)
    {
        uiManager_->removeElement(root_);
        uiManager_->addElement(root_);
    }

    // Character changes while closed were stashed rather than rendered; apply
    // them now so the panel never opens showing a stale sheet.
    Refresh();

    root_->setVisible(true);
}

void CharacterPanel::Close()
{
    if (!root_)
        return;

    open_ = false;
    root_->setVisible(false);
}

void CharacterPanel::Toggle()
{
    if (open_)
        Close();
    else
        Open();
}

void CharacterPanel::SetCharacter(const CharacterInfo& info)
{
    info_ = info;

    if (open_)
        Refresh();
}

void CharacterPanel::Refresh()
{
    if (!root_)
        return;

    const float width = S(kPanelWidth);

    if (nameLabel_)
        nameLabel_->setText(info_.name.empty() ? std::string("(unnamed)")
                                               : info_.name);

    if (roleLabel_)
        roleLabel_->setText(info_.role.empty() ? std::string("Visitor")
                                               : info_.role);

    if (levelLabel_)
        levelLabel_->setText(std::format("Level {}", info_.level));

    for (const auto& row : statRows_)
        root_->removeChild(row);
    statRows_.clear();

    // One label per stat, stacked under the header. The map's ordering is
    // stable between packets, so rows never shuffle under the reader.
    float y = S(kStatsTopOffset);

    for (const auto& [name, value] : info_.stats)
    {
        auto row = std::make_shared<UILabel>();
        row->setFont(PanelFont(engine_, UIFonts::Typeface::Body,
                               UITheme::Display::Label));
        row->setTextColor(UITheme::Subtext);
        row->setText(std::format("{}: {}", name, value));
        row->setPosition(S(kPadding), y);
        row->setSize(width - S(kPadding) * 2.0f, S(kRowHeight - 3.0f));

        root_->addChild(row);
        statRows_.push_back(row);

        y += S(kRowHeight);

        // The panel holds one page of stats. A character with more rows than
        // fit would overflow silently; stop at the edge rather than draw past
        // it until scrolling exists here too.
        if (y > S(kPanelHeight) - S(kPadding))
            break;
    }
}

void CharacterPanel::RaiseToFront()
{
    if (uiManager_ && root_)
    {
        uiManager_->bringToFront(root_);
    }
}
