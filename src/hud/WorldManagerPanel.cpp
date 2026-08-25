#include "WorldManagerPanel.h"

#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../ui/UIButton.h"
#include "../ui/UICheckBox.h"
#include "../ui/UIFonts.h"
#include "../ui/UILabel.h"
#include "../ui/UIManager.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UIScrollPanel.h"
#include "../ui/UITextBox.h"
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

    // Panel geometry, in canvas units. Kept together so the tab bodies and the
    // rows inside them cannot disagree about the usable width.
    // Preview pixels: S() scales these by 1080/460, so 340 is about 800 canvas
    // pixels. Sized against the pause overlay's S(260) x S(170), larger because
    // this one carries two tabs and a scrolling list.
    constexpr float kPanelWidth   = 340.0f;
    constexpr float kPanelHeight  = 235.0f;
    constexpr float kPadding      = 10.0f;
    constexpr float kRowHeight    = 17.0f;
    constexpr float kTabBarHeight = 20.0f;

    // The role values the server uses. Mirrors Server/src/world/WorldRole.h; the
    // client sends these as requests and never interprets them as permission.
    constexpr std::uint8_t kRoleVisitor = 0;
    constexpr std::uint8_t kRoleMember  = 1;
    constexpr std::uint8_t kRoleBuilder = 2;
    constexpr std::uint8_t kRoleCoOwner = 3;
    constexpr std::uint8_t kRoleOwner   = 4;

    const char* RoleName(std::uint8_t role)
    {
        switch (role)
        {
        case kRoleMember:  return "Member";
        case kRoleBuilder: return "Builder";
        case kRoleCoOwner: return "Co-Owner";
        case kRoleOwner:   return "Owner";
        default:           return "Visitor";
        }
    }
}

WorldManagerPanel::WorldManagerPanel(Engine* engine, UIManager* uiManager)
    : engine_(engine), uiManager_(uiManager)
{
}

WorldManagerPanel::~WorldManagerPanel()
{
    // The UIManager outlives this panel on a screen change, so the elements
    // have to be handed back or they keep drawing over whatever comes next.
    if (uiManager_ && root_)
        uiManager_->removeElement(root_);
}

void WorldManagerPanel::Build()
{
    if (root_ || !uiManager_)
        return;

    BuildFrame();
    BuildWorldTab();
    BuildLockTab();

    SelectTab(Tab::World);
    root_->setVisible(false);
}

void WorldManagerPanel::BuildFrame()
{
    const float width  = S(kPanelWidth);
    const float height = S(kPanelHeight);

    root_ = std::make_shared<UIPanel>();
    root_->setSize(width, height);
    root_->setPosition((UIScale::kDesignWidth  - width)  * 0.5f,
                       (UIScale::kDesignHeight - height) * 0.5f);
    root_->setBackgroundColor(UITheme::Hex(0x1E2230, 0.97f));
    root_->setBorder(UITheme::WithAlpha(UITheme::Accent, 0.45f), UITheme::BorderThin);
    root_->setBorderRadius(UITheme::RadiusPanel);

    // A bare UIPanel reports wantsInput() false, which would let every click
    // fall straight through to the world behind it - the same bug as mining
    // through the hotbar. The panel covers live terrain, so it has to swallow.
    root_->setBlocksInput(true);

    // Added to the UIManager rather than to a screen's root so it draws above
    // the HUD; UIManager renders in insertion order.
    uiManager_->addElement(root_);

    title_ = std::make_shared<UILabel>();
    title_->setFont(PanelFont(engine_, UIFonts::Typeface::Display, UITheme::Display::Section));
    title_->setTextColor(UITheme::Text);
    title_->setPosition(S(kPadding), S(8.0f));
    title_->setSize(width - S(kPadding) * 2.0f, S(13.0f));
    root_->addChild(title_);

    closeButton_ = std::make_shared<UIButton>();
    closeButton_->setText("CLOSE");
    closeButton_->setFont(PanelFont(engine_, UIFonts::Typeface::Display, UITheme::Display::Small));
    closeButton_->setVariant(UIButton::Variant::Ghost);
    closeButton_->setSize(S(46.0f), S(15.0f));
    closeButton_->setPosition(width - S(46.0f) - S(kPadding), S(7.0f));
    closeButton_->setOnClick([this]() { Hide(); });
    root_->addChild(closeButton_);

    // Tabs. There is no tab widget, so these are two buttons over two sibling
    // panels toggled with setVisible - the same shape the pause overlay uses to
    // show and hide itself.
    const float tabWidth = S(92.0f);
    const float tabY     = S(26.0f);

    worldTabButton_ = std::make_shared<UIButton>();
    worldTabButton_->setText("WORLD MANAGER");
    worldTabButton_->setFont(PanelFont(engine_, UIFonts::Typeface::Display, UITheme::Display::Small));
    worldTabButton_->setSize(tabWidth, S(kTabBarHeight - 6.0f));
    worldTabButton_->setPosition(S(kPadding), tabY);
    worldTabButton_->setOnClick([this]() { SelectTab(Tab::World); });
    root_->addChild(worldTabButton_);

    lockTabButton_ = std::make_shared<UIButton>();
    lockTabButton_->setText("LOCK MANAGER");
    lockTabButton_->setFont(PanelFont(engine_, UIFonts::Typeface::Display, UITheme::Display::Small));
    lockTabButton_->setSize(tabWidth, S(kTabBarHeight - 6.0f));
    lockTabButton_->setPosition(S(kPadding) + tabWidth + S(8.0f), tabY);
    lockTabButton_->setOnClick([this]() { SelectTab(Tab::Lock); });
    root_->addChild(lockTabButton_);
}

void WorldManagerPanel::BuildWorldTab()
{
    const float width  = S(kPanelWidth) - S(kPadding) * 2.0f;
    const float top    = S(26.0f + kTabBarHeight + 4.0f);
    const float height = S(kPanelHeight) - top - S(kPadding);

    worldTab_ = std::make_shared<UIPanel>();
    worldTab_->setSize(width, height);
    worldTab_->setPosition(S(kPadding), top);
    worldTab_->setBackgroundColor(UITheme::Hex(0x000000, 0.0f));
    root_->addChild(worldTab_);

    const auto caption = [&](std::shared_ptr<UILabel>& out, float y) {
        out = std::make_shared<UILabel>();
        out->setFont(PanelFont(engine_, UIFonts::Typeface::Body, UITheme::Display::Label));
        out->setTextColor(UITheme::Subtext);
        out->setPosition(0.0f, y);
        out->setSize(width, S(11.0f));
        worldTab_->addChild(out);
    };

    caption(ownerLabel_,       S(1.0f));
    caption(levelLabel_,       S(13.0f));
    caption(memberCountLabel_, S(25.0f));

    // The member list. UIScrollPanel already implements onScroll, and
    // UIManager::handleScroll walks up from the element under the cursor until
    // it finds something that consumes it, so this scrolls without any wiring.
    memberList_ = std::make_shared<UIScrollPanel>();
    memberList_->setSize(width, height - S(66.0f));
    memberList_->setPosition(0.0f, S(39.0f));
    memberList_->setBackgroundColor(UITheme::RowBackground);
    memberList_->setBorder(UITheme::SubtleBorder, UITheme::BorderThin);
    memberList_->setBorderRadius(UITheme::RadiusInput);
    memberList_->setScrollSpeed(S(kRowHeight));
    worldTab_->addChild(memberList_);

    const float fieldY = height - S(19.0f);

    inviteField_ = std::make_shared<UITextBox>();
    inviteField_->setPlaceholderText("username");
    inviteField_->setFont(PanelFont(engine_, UIFonts::Typeface::Body, UITheme::Display::Label));
    inviteField_->setTextColor(UITheme::Text);
    inviteField_->setPlaceholderColor(UITheme::Muted);
    inviteField_->setBackgroundColor(UITheme::InputBackground);
    inviteField_->setBorderColor(UITheme::InputBorder);
    inviteField_->setFocusBorderColor(UITheme::Accent);
    inviteField_->setBorderRadius(UITheme::RadiusInput);
    inviteField_->setSize(width - S(104.0f), S(16.0f));
    inviteField_->setPosition(0.0f, fieldY);
    worldTab_->addChild(inviteField_);

    inviteButton_ = std::make_shared<UIButton>();
    inviteButton_->setText("INVITE");
    inviteButton_->setFont(PanelFont(engine_, UIFonts::Typeface::Display, UITheme::Display::Small));
    inviteButton_->setVariant(UIButton::Variant::Success);
    inviteButton_->setSize(S(98.0f), S(16.0f));
    inviteButton_->setPosition(width - S(98.0f), fieldY);
    inviteButton_->setOnClick([this]() {
        if (!engine_ || !inviteField_)
            return;

        const std::string name = inviteField_->getText();
        if (name.empty())
            return;

        // Invited as Member, the lowest standing that is still a membership.
        // Promotion is a separate, deliberate act on the row - so adding
        // someone can never be the same click as trusting them to build.
        engine_->getNetworkManager().sendInviteWorldMember(name, kRoleMember);
        inviteField_->setText(std::string());
    });
    worldTab_->addChild(inviteButton_);

    inviteHint_ = std::make_shared<UILabel>();
    inviteHint_->setFont(PanelFont(engine_, UIFonts::Typeface::Body, UITheme::Display::Small));
    inviteHint_->setTextColor(UITheme::Muted);
    inviteHint_->setText("Invited players join as Member. Promote to Builder to let them build.");
    inviteHint_->setPosition(0.0f, fieldY - S(11.0f));
    inviteHint_->setSize(width, S(9.0f));
    worldTab_->addChild(inviteHint_);
}

void WorldManagerPanel::BuildLockTab()
{
    const float width  = S(kPanelWidth) - S(kPadding) * 2.0f;
    const float top    = S(46.0f + kTabBarHeight + 6.0f);
    const float height = S(kPanelHeight) - top - S(kPadding);

    lockTab_ = std::make_shared<UIPanel>();
    lockTab_->setSize(width, height);
    lockTab_->setPosition(S(kPadding), top);
    lockTab_->setBackgroundColor(UITheme::Hex(0x000000, 0.0f));
    root_->addChild(lockTab_);

    const auto toggle = [&](std::shared_ptr<UICheckBox>& out,
                            const char* label, float y) {
        out = std::make_shared<UICheckBox>();
        out->setLabel(label);
        out->setFont(PanelFont(engine_, UIFonts::Typeface::Body, UITheme::Display::Label));
        out->setLabelColor(UITheme::Text);
        out->setAccentColor(UITheme::Accent);
        out->setBoxSize(S(11.0f));
        out->setPosition(0.0f, y);
        out->setSize(width, S(13.0f));
        out->setOnChanged([this](bool) { SendSettings(); });
        lockTab_->addChild(out);
    };

    toggle(protectionBox_, "Protection on",      S(1.0f));
    toggle(buildingBox_,   "Visitors may build", S(16.0f));
    toggle(breakingBox_,   "Visitors may break", S(31.0f));
    toggle(visitorsBox_,   "Visitors may enter", S(46.0f));

    banList_ = std::make_shared<UIScrollPanel>();
    banList_->setSize(width, height - S(85.0f));
    banList_->setPosition(0.0f, S(63.0f));
    banList_->setBackgroundColor(UITheme::RowBackground);
    banList_->setBorder(UITheme::SubtleBorder, UITheme::BorderThin);
    banList_->setBorderRadius(UITheme::RadiusInput);
    banList_->setScrollSpeed(S(kRowHeight));
    lockTab_->addChild(banList_);

    const float fieldY = height - S(19.0f);

    // Ban duration presets. The cycler sits between the field and BAN and
    // walks the table; 0 is permanent. Seconds are what the wire carries.
    struct BanDuration { const char* label; uint32_t seconds; };
    static constexpr BanDuration kBanDurations[] = {
        { "FOREVER", 0 },
        { "1 HOUR",  3600 },
        { "1 DAY",   86400 },
        { "1 WEEK",  604800 },
    };

    banField_ = std::make_shared<UITextBox>();
    banField_->setPlaceholderText("username");
    banField_->setFont(PanelFont(engine_, UIFonts::Typeface::Body, UITheme::Display::Label));
    banField_->setTextColor(UITheme::Text);
    banField_->setPlaceholderColor(UITheme::Muted);
    banField_->setBackgroundColor(UITheme::InputBackground);
    banField_->setBorderColor(UITheme::InputBorder);
    banField_->setFocusBorderColor(UITheme::Accent);
    banField_->setBorderRadius(UITheme::RadiusInput);
    banField_->setSize(width - S(174.0f), S(16.0f));
    banField_->setPosition(0.0f, fieldY);
    lockTab_->addChild(banField_);

    banDurationButton_ = std::make_shared<UIButton>();
    banDurationButton_->setText(kBanDurations[banDurationIndex_].label);
    banDurationButton_->setFont(PanelFont(engine_, UIFonts::Typeface::Display, UITheme::Display::Small));
    banDurationButton_->setVariant(UIButton::Variant::Ghost);
    banDurationButton_->setSize(S(68.0f), S(16.0f));
    banDurationButton_->setPosition(width - S(166.0f), fieldY);
    banDurationButton_->setOnClick([this]() {
        banDurationIndex_ = (banDurationIndex_ + 1) % 4;
        banDurationButton_->setText(kBanDurations[banDurationIndex_].label);
        if (engine_)
            engine_->GetAudio().PlaySfx("ui_click");
    });
    lockTab_->addChild(banDurationButton_);

    banButton_ = std::make_shared<UIButton>();
    banButton_->setText("BAN");
    banButton_->setFont(PanelFont(engine_, UIFonts::Typeface::Display, UITheme::Display::Small));
    banButton_->setVariant(UIButton::Variant::Danger);
    banButton_->setSize(S(90.0f), S(16.0f));
    banButton_->setPosition(width - S(90.0f), fieldY);
    banButton_->setOnClick([this]() {
        if (!engine_ || !banField_)
            return;

        const std::string name = banField_->getText();
        if (name.empty())
            return;

        const uint32_t duration = kBanDurations[banDurationIndex_].seconds;
        engine_->getNetworkManager().sendBanWorldPlayer(name, true,
                                                        std::string(), duration);
        banField_->setText(std::string());
    });
    lockTab_->addChild(banButton_);
}

void WorldManagerPanel::SelectTab(Tab tab)
{
    tab_ = tab;

    if (worldTab_) worldTab_->setVisible(tab == Tab::World);
    if (lockTab_)  lockTab_->setVisible(tab == Tab::Lock);

    // The selected tab reads as a solid button and the other as a ghost, which
    // is the only affordance available without a tab widget to draw a join.
    if (worldTabButton_)
        worldTabButton_->setVariant(tab == Tab::World ? UIButton::Variant::Primary
                                                      : UIButton::Variant::Ghost);
    if (lockTabButton_)
        lockTabButton_->setVariant(tab == Tab::Lock ? UIButton::Variant::Primary
                                                    : UIButton::Variant::Ghost);
}

void WorldManagerPanel::Show()
{
    if (!root_)
        return;

    open_ = true;

    // UIManager renders in insertion order and the HUD is built after this
    // panel, so on its first show the HUD was drawing over the top of it.
    // Re-inserting moves it to the end of the list, which is what puts a modal
    // above everything rather than merely on top of whatever existed when it
    // was constructed.
    if (uiManager_)
    {
        uiManager_->removeElement(root_);
        uiManager_->addElement(root_);
    }

    root_->setVisible(true);

    // Force a repopulate on open rather than waiting for the next revision
    // change: the info that opened the panel may have arrived before it existed.
    infoRevision_    = 0;
    membersRevision_ = 0;
    Refresh();
}

void WorldManagerPanel::Hide()
{
    if (!root_)
        return;

    open_ = false;
    root_->setVisible(false);
}

void WorldManagerPanel::Refresh()
{
    if (!open_ || !engine_ || !root_)
        return;

    NetworkManager& network = engine_->getNetworkManager();

    const uint32_t info    = network.getWorldInfoRevision();
    const uint32_t members = network.getWorldMembersRevision();

    if (info == infoRevision_ && members == membersRevision_)
        return;

    infoRevision_    = info;
    membersRevision_ = members;

    RefreshWorldTab();
    RefreshLockTab();
}

void WorldManagerPanel::RefreshWorldTab()
{
    const auto& state = engine_->getNetworkManager().getWorldManageState();

    if (title_)
        title_->setText(state.worldName.empty() ? "WORLD" : state.worldName);

    if (ownerLabel_)
    {
        ownerLabel_->setText(state.ownerName.empty()
                                 ? std::string("Unclaimed")
                                 : "Owner: " + state.ownerName);
    }

    if (levelLabel_)
    {
        levelLabel_->setText(std::format("Strix Core level {}  -  protection {}",
                                         state.coreLevel,
                                         state.protectionOn ? "on" : "off"));
    }

    if (memberCountLabel_)
    {
        memberCountLabel_->setText(std::format("{} member(s)  -  you are {}",
                                               state.memberCount,
                                               RoleName(state.viewerRole)));
    }

    // Only a manager gets the invite controls. The server sends no roster at
    // all to anyone else, so for a visitor there is nothing to list either.
    if (inviteField_)  inviteField_->setVisible(state.canManage);
    if (inviteButton_) inviteButton_->setVisible(state.canManage);
    if (inviteHint_)   inviteHint_->setVisible(state.canManage);

    // The Lock tab is management only. Hiding the tab button as well as the
    // body means a visitor is not shown a door they cannot open.
    if (lockTabButton_)
        lockTabButton_->setVisible(state.canManage);

    if (!state.canManage && tab_ == Tab::Lock)
        SelectTab(Tab::World);

    if (!memberList_)
        return;

    memberList_->clearContent();

    const auto& members = engine_->getNetworkManager().getWorldMembers();
    for (const auto& member : members)
        AddMemberRow(member, state.canManage);

    memberList_->refreshContentHeight();
}

void WorldManagerPanel::AddMemberRow(const NetworkManager::WorldRosterEntry& entry,
                                     bool canManage)
{
    const float width = memberList_->getWidth();

    auto row = std::make_shared<UIPanel>();
    row->setSize(width, S(kRowHeight));
    row->setBackgroundColor(UITheme::Hex(0x000000, 0.0f));

    auto name = std::make_shared<UILabel>();
    name->setFont(PanelFont(engine_, UIFonts::Typeface::Body, UITheme::Display::Label));
    name->setTextColor(UITheme::Text);
    name->setText(entry.username + "  (" + RoleName(entry.role) + ")");
    name->setPosition(S(5.0f), S(4.0f));
    name->setSize(width - S(132.0f), S(11.0f));
    row->addChild(name);

    if (canManage)
    {
        const std::string who = entry.username;

        const auto action = [&](const char* text, UIButton::Variant variant,
                                float x, std::function<void()> onClick) {
            auto button = std::make_shared<UIButton>();
            button->setText(text);
            button->setFont(PanelFont(engine_, UIFonts::Typeface::Display,
                                      UITheme::Display::Small));
            button->setVariant(variant);
            button->setSize(S(38.0f), S(12.0f));
            button->setPosition(width - x, S(3.0f));
            button->setOnClick(std::move(onClick));
            row->addChild(button);
        };

        // Promote steps one rung up, demote one down, and both stop at the ends
        // of the range the client may ask for. The server refuses anything at
        // or above the caller's own role regardless, so this only keeps the
        // buttons from sending a request that is certain to be refused.
        const std::uint8_t role = entry.role;

        action("UP", UIButton::Variant::Primary, S(124.0f), [this, who, role]() {
            if (!engine_ || role >= kRoleCoOwner)
                return;
            engine_->getNetworkManager().sendChangeWorldRole(
                who, static_cast<std::uint8_t>(role + 1));
        });

        action("DOWN", UIButton::Variant::Ghost, S(84.0f), [this, who, role]() {
            if (!engine_ || role <= kRoleVisitor)
                return;
            engine_->getNetworkManager().sendChangeWorldRole(
                who, static_cast<std::uint8_t>(role - 1));
        });

        action("REMOVE", UIButton::Variant::Danger, S(43.0f), [this, who]() {
            if (!engine_)
                return;
            engine_->getNetworkManager().sendRemoveWorldMember(who);
        });
    }

    memberList_->addContent(row);
}

void WorldManagerPanel::RefreshLockTab()
{
    const auto& state = engine_->getNetworkManager().getWorldManageState();

    // Writing a checkbox fires its onChanged, which would send a settings
    // packet describing the state the server just told us about - and with four
    // boxes, four of them. The guard makes the write silent.
    applyingSettings_ = true;

    if (protectionBox_) protectionBox_->setChecked(state.protectionOn);
    if (buildingBox_)   buildingBox_->setChecked(state.allowBuilding);
    if (breakingBox_)   breakingBox_->setChecked(state.allowBreaking);
    if (visitorsBox_)   visitorsBox_->setChecked(state.allowVisitors);

    applyingSettings_ = false;

    const bool canManage = state.canManage;
    if (protectionBox_) protectionBox_->setEnabled(canManage);
    if (buildingBox_)   buildingBox_->setEnabled(canManage);
    if (breakingBox_)   breakingBox_->setEnabled(canManage);
    if (visitorsBox_)   visitorsBox_->setEnabled(canManage);
    if (banField_)      banField_->setVisible(canManage);
    if (banButton_)     banButton_->setVisible(canManage);

    if (!banList_)
        return;

    banList_->clearContent();

    for (const auto& ban : engine_->getNetworkManager().getWorldBans())
        AddBanRow(ban, canManage);

    banList_->refreshContentHeight();
}

void WorldManagerPanel::AddBanRow(const NetworkManager::WorldRosterEntry& entry,
                                  bool canManage)
{
    const float width = banList_->getWidth();

    auto row = std::make_shared<UIPanel>();
    row->setSize(width, S(kRowHeight));
    row->setBackgroundColor(UITheme::Hex(0x000000, 0.0f));

    auto name = std::make_shared<UILabel>();
    name->setFont(PanelFont(engine_, UIFonts::Typeface::Body, UITheme::Display::Label));
    name->setTextColor(UITheme::Subtext);
    name->setText(entry.username);
    name->setPosition(S(5.0f), S(4.0f));
    name->setSize(width - S(60.0f), S(11.0f));
    row->addChild(name);

    if (canManage)
    {
        const std::string who = entry.username;

        auto unban = std::make_shared<UIButton>();
        unban->setText("UNBAN");
        unban->setFont(PanelFont(engine_, UIFonts::Typeface::Display, UITheme::Display::Small));
        unban->setVariant(UIButton::Variant::Success);
        unban->setSize(S(44.0f), S(12.0f));
        unban->setPosition(width - S(49.0f), S(3.0f));
        unban->setOnClick([this, who]() {
            if (!engine_)
                return;
            engine_->getNetworkManager().sendBanWorldPlayer(who, false, std::string());
        });
        row->addChild(unban);
    }

    banList_->addContent(row);
}

void WorldManagerPanel::SendSettings()
{
    if (applyingSettings_ || !engine_)
        return;

    if (!protectionBox_ || !buildingBox_ || !breakingBox_ || !visitorsBox_)
        return;

    engine_->getNetworkManager().sendSetWorldSettings(protectionBox_->isChecked(),
                                                      buildingBox_->isChecked(),
                                                      breakingBox_->isChecked(),
                                                      visitorsBox_->isChecked());
}

void WorldManagerPanel::RaiseToFront()
{
    if (uiManager_ && root_)
    {
        uiManager_->bringToFront(root_);
    }
}
