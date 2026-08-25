#include "PlayerListPanel.h"

#include "../core/Engine.h"
#include "../graphics/Color.h"
#include "../ui/UIFonts.h"
#include "../ui/UILabel.h"
#include "../ui/UIManager.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"
#include "../ui/UIRoleBadge.h"

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    Font* PanelFont(Engine* engine, UIFonts::Typeface face, unsigned int size)
    {
        UIFonts* fonts = engine ? engine->GetUIFonts() : nullptr;
        return fonts ? fonts->Get(face, size) : nullptr;
    }

    // Panel geometry, in style-guide pixels. Narrower than WorldManagerPanel
    // because it is one column of names, and anchored to the right edge of the
    // canvas so it never covers the chat at the bottom-left.
    constexpr float kPanelWidth   = 150.0f;
    constexpr float kPanelHeight  = 190.0f;
    constexpr float kPanelMargin  = 12.0f;
    constexpr float kPadding      = 10.0f;
    constexpr float kRowHeight    = 14.0f;

    // Semi-transparent dark background: an overlay, not a window. The player
    // must still read the world behind it while it is held open.
    const Color& PanelBackground()
    {
        static const Color color = UITheme::Hex(0x0E1424, 0.80f);
        return color;
    }

    // Role colours. Developer is gold with a violet glow (staff, and it should
    // look like it), Moderator the primary blue, Player plain white.
    const Color& DeveloperColor()
    {
        static const Color color = UITheme::Gold;
        return color;
    }

    const Color& DeveloperGlow()
    {
        static const Color color = UITheme::Secondary;
        return color;
    }

    const Color& ModeratorColor()
    {
        static const Color color = UITheme::Primary;
        return color;
    }

    // World roles. Gold for the owner, echoing UITheme::Gold's use for
    // legendary elsewhere; the accent cyan for a Co-Owner, who acts with the
    // owner's authority; a quieter green for a Builder, who may only build.
    const Color& OwnerColor()
    {
        static const Color c = UITheme::Gold;
        return c;
    }

    const Color& CoOwnerColor()
    {
        static const Color c = UITheme::Accent;
        return c;
    }

    const Color& BuilderColor()
    {
        static const Color c = UITheme::Success;
        return c;
    }

    const Color& PlayerColor()
    {
        static const Color color = UITheme::Text;
        return color;
    }

    // Hover tint for roster rows, matching the world browser's rows: a quiet
    // accent wash that lifts the transparent row without shouting over the
    // role colours.
    const Color& RowHoverTint()
    {
        static const Color color = UITheme::WithAlpha(UITheme::Accent, 0.08f);
        return color;
    }

    // One-line explanations for the row tooltip. A plain "Player" explains
    // itself; giving it a line too would only add noise.
    const char* RoleDescription(const std::string& role)
    {
        if (role == "Developer") return "Developer — server staff";
        if (role == "Moderator") return "Moderator — server staff";

        if (role == "Owner")     return "Owner — full control of this world";
        if (role == "Co-Owner")  return "Co-Owner — acts with the owner's authority";
        if (role == "Builder")   return "Builder — can build and break";
        if (role == "Member")    return "Member — can visit, cannot build";
        if (role == "Visitor")   return "Visitor — can look around";

        return nullptr;
    }
}

PlayerListPanel::PlayerListPanel(Engine* engine, UIManager* uiManager)
    : engine_(engine), uiManager_(uiManager)
{
}

PlayerListPanel::~PlayerListPanel()
{
    // The UIManager outlives this panel on a screen change, so the elements
    // have to be handed back or they keep drawing over whatever comes next.
    if (uiManager_ && root_)
        uiManager_->removeElement(root_);
}

void PlayerListPanel::Build()
{
    if (root_ || !uiManager_)
        return;

    BuildFrame();

    root_->setVisible(false);
}

void PlayerListPanel::BuildFrame()
{
    const float width  = S(kPanelWidth);
    const float height = S(kPanelHeight);

    // Centered-right: flush against the right margin, vertically centred.
    root_ = std::make_shared<UIPanel>();
    root_->setSize(width, height);
    root_->setPosition(UIScale::kDesignWidth - width - S(kPanelMargin),
                       (UIScale::kDesignHeight - height) * 0.5f);
    root_->setBackgroundColor(PanelBackground());
    root_->setBorder(UITheme::SubtleBorder, UITheme::BorderThin);
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
    title->setText("PLAYERS");
    title->setPosition(S(kPadding), S(8.0f));
    title->setSize(width - S(kPadding) * 2.0f, S(13.0f));
    root_->addChild(title);

    // The list body. Rebuilt wholesale on SetPlayers; the roster itself is
    // what changed when it is called.
    list_ = std::make_shared<UIPanel>();
    list_->setSize(width - S(kPadding) * 2.0f, height - S(30.0f));
    list_->setPosition(S(kPadding), S(26.0f));
    list_->setBackgroundColor(UITheme::Hex(0x000000, 0.0f));
    root_->addChild(list_);
}

void PlayerListPanel::Open()
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

    // Roster changes while closed were stashed rather than rendered; apply
    // them now so the panel never opens showing a stale list.
    RebuildRows();

    root_->setVisible(true);
}

void PlayerListPanel::Close()
{
    if (!root_)
        return;

    open_ = false;
    root_->setVisible(false);
}

void PlayerListPanel::Toggle()
{
    if (open_)
        Close();
    else
        Open();
}

void PlayerListPanel::SetPlayers(const std::vector<Entry>& players)
{
    // The roster has no revision counter to key on, so GameScreen pushes it
    // every frame. Without this test that meant tearing down and rebuilding
    // every row sixty times a second - a few thousand UI elements per second
    // for a list that almost never changes, and it ran while the panel was
    // shut as well.
    if (players == players_)
        return;

    players_ = players;

    if (open_)
        RebuildRows();
}

const Color& PlayerListPanel::RoleColor(const std::string& role)
{
    // Two different axes end up in this one string.
    //
    // Developer and Moderator are *server* ranks, from players.rank. Owner
    // through Visitor are *world* roles, from world_members - what you are
    // here, which changes as you walk between worlds. A player can be both,
    // and the rank is the rarer, louder fact, so it wins when it is present.
    //
    // The world roles were added because the roster had been filling every row
    // with the literal "Player": the branches below could not fire, and the
    // spawn packet was already carrying the real role and being discarded one
    // line before it was read.
    if (role == "Developer") return DeveloperColor();
    if (role == "Moderator") return ModeratorColor();

    if (role == "Owner")    return OwnerColor();
    if (role == "Co-Owner") return CoOwnerColor();
    if (role == "Builder")  return BuilderColor();

    // Member and Visitor read plainly: most rows in a busy world are one of
    // the two, and colouring them would leave nothing for the roles that mean
    // something.
    return PlayerColor();
}

void PlayerListPanel::RebuildRows()
{
    if (!list_)
        return;

    list_->clearChildren();

    const float width = list_->getWidth();

    for (const auto& entry : players_)
    {
        auto row = std::make_shared<UIPanel>();
        row->setSize(width, S(kRowHeight));
        row->setBackgroundColor(UITheme::Hex(0x000000, 0.0f));

        // Hoverable so the roster reads as a list of rows rather than a wall
        // of text; the wash is the only feedback, rows are not clickable.
        row->setHoverable(true);
        row->setHoverTint(RowHoverTint());

        if (const char* description = RoleDescription(entry.role))
            row->setTooltipText(description);

        // The role badge sits right-aligned in the row; the name yields the
        // width it needs and keeps its RoleColor treatment.
        auto badge = std::make_shared<UIRoleBadge>(engine_);
        badge->SetRole(entry.role);
        badge->setPosition(width - badge->getWidth() - S(2.0f),
                           (S(kRowHeight) - badge->getHeight()) * 0.5f);

        const float nameWidth = width - S(5.0f) - badge->getWidth() - S(4.0f)
                                - S(2.0f);

        auto name = std::make_shared<UILabel>();
        name->setFont(PanelFont(engine_, UIFonts::Typeface::Body,
                                UITheme::Display::Label));
        name->setTextColor(RoleColor(entry.role));

        // Developers get their colour doubled with a violet glow; everyone
        // else reads plainly.
        if (entry.role == "Developer")
            name->setGlow(DeveloperGlow(), S(2.0f));

        name->setText(entry.name.empty() ? std::string("(unnamed)") : entry.name);
        name->setPosition(S(5.0f), S(3.0f));
        name->setSize(nameWidth, S(11.0f));
        row->addChild(name);
        row->addChild(badge);

        list_->addChild(row);
    }
}

void PlayerListPanel::RaiseToFront()
{
    if (uiManager_ && root_)
    {
        uiManager_->bringToFront(root_);
    }
}
