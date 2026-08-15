#include "WorldBrowserScreen.h"

#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../networking/NetworkManager.h"
#include "../networking/Protocol.h"
#include "../ui/UIButton.h"
#include "../ui/UIIcon.h"
#include "../ui/UILabel.h"
#include "../ui/UIPanel.h"
#include "../ui/UIProgressBar.h"
#include "../ui/UIScale.h"
#include "../ui/UIScrollPanel.h"
#include "../ui/UITextBox.h"
#include "../ui/UITheme.h"

#include <algorithm>
#include <cctype>
#include <format>

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    // The style guide's world selection screen (App.tsx 3334-3427).
    constexpr float kHeaderHeight = S(66.0f);
    constexpr float kFooterHeight = S(42.0f);
    constexpr float kHeaderPadX   = S(20.0f);
    constexpr float kListPadding  = S(14.0f);
    constexpr float kRowHeight    = S(62.0f);
    constexpr float kRowGap       = S(8.0f);
    constexpr float kRowPadX      = S(16.0f);
    constexpr float kRowPadY      = S(12.0f);
    constexpr float kPopWidth     = S(80.0f);

    std::string ToLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    // Population colour thresholds from the design.
    Color PopulationColor(const WorldInfo& world)
    {
        if (world.maxPlayers <= 0)
            return UITheme::Success;

        const float ratio = static_cast<float>(world.players) / static_cast<float>(world.maxPlayers);

        if (ratio >= 1.0f)
            return UITheme::Danger;
        if (ratio >= 0.8f)
            return UITheme::Warning;

        return UITheme::Success;
    }

    Color TypeColor(const std::string& type)
    {
        if (type == "Survival")  return UITheme::Success;
        if (type == "Adventure") return UITheme::Primary;
        if (type == "Creative")  return UITheme::Secondary;
        if (type == "Trading")   return UITheme::Gold;
        if (type == "Event")     return UITheme::Danger;

        return UITheme::Subtext;
    }

    // RECENT / FAVORITE / EVENT badge, matching the design's precedence.
    const char* TagFor(const WorldInfo& world)
    {
        if (world.type == "Event")
            return "EVENT";
        if (world.recent)
            return "RECENT";
        if (world.favourite)
            return "FAVORITE";

        return nullptr;
    }

    Color TagColor(const std::string& tag)
    {
        if (tag == "RECENT")   return UITheme::Primary;
        if (tag == "FAVORITE") return UITheme::Gold;
        if (tag == "EVENT")    return UITheme::Danger;

        return UITheme::Subtext;
    }
}

WorldBrowserScreen::WorldBrowserScreen(Engine* engine)
    : Screen(engine)
{
}

void WorldBrowserScreen::OnEnter()
{
    if (!uiManager_)
    {
        LOG_ERROR("WorldBrowserScreen: UIManager not available");
        return;
    }

    joining_ = false;
    query_.clear();
    filter_ = Filter::All;

    if (WorldManager* worlds = engine_ ? engine_->GetWorldManager() : nullptr)
        worlds_ = worlds->GetAvailableWorlds();

    CreateRoot();

    const float originX = DesignOriginX();
    const float originY = DesignOriginY();

    // Solid screen background with the design's border colour.
    AddBackdrop(UITheme::ScreenBackground, UITheme::ScreenBackground, false);

    BuildHeader(originX, originY, UIScale::kDesignWidth);

    // Scrolling world list between the header and the footer.
    const float listY = originY + kHeaderHeight;
    const float listHeight = UIScale::kDesignHeight - kHeaderHeight - kFooterHeight;

    list_ = std::make_shared<UIScrollPanel>();
    list_->setPosition(originX + kListPadding, listY + kListPadding);
    list_->setSize(UIScale::kDesignWidth - kListPadding * 2.0f, listHeight - kListPadding * 2.0f);
    list_->setScrollSpeed(kRowHeight + kRowGap);
    root_->addChild(list_);

    BuildFooter(originX, originY + UIScale::kDesignHeight - kFooterHeight, UIScale::kDesignWidth);

    RebuildList();
}

void WorldBrowserScreen::BuildHeader(float x, float y, float width)
{
    auto header = std::make_shared<UIPanel>();
    header->setBackgroundColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    header->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    header->setBorderRadius(0.0f);
    header->setPosition(x, y);
    header->setSize(width, kHeaderHeight);
    root_->addChild(header);

    auto rule = std::make_shared<UIPanel>();
    rule->setBackgroundColor(UITheme::WithAlpha(UITheme::Border, 0.20f));
    rule->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    rule->setBorderRadius(0.0f);
    rule->setPosition(0.0f, kHeaderHeight - UITheme::BorderThin);
    rule->setSize(width, UITheme::BorderThin);
    header->addChild(rule);

    auto title = std::make_shared<UILabel>();
    title->setText("WORLD SELECTION");
    title->setFont(DisplayFont(UITheme::Display::Section));
    title->setTextColor(UITheme::Text);
    title->setPosition(kHeaderPadX, S(16.0f));
    title->setSize(S(300.0f), S(14.0f));
    header->addChild(title);

    auto subtitle = std::make_shared<UILabel>();
    subtitle->setText("Choose a world to enter");
    subtitle->setFont(BodyFont(UITheme::Body::Welcome));
    subtitle->setTextColor(UITheme::Subtext);
    subtitle->setPosition(kHeaderPadX, S(34.0f));
    subtitle->setSize(S(300.0f), S(16.0f));
    header->addChild(subtitle);

    // --- Controls, laid out from the right edge --------------------------
    const float controlHeight = S(26.0f);
    const float controlY      = (kHeaderHeight - controlHeight) * 0.5f;

    float cursorX = width - kHeaderPadX;

    auto refresh = std::make_shared<UIButton>();
    refresh->setText("Refresh");
    refresh->setFont(DisplayFont(UITheme::Display::Small));
    refresh->setVariant(UIButton::Variant::Purple);
    refresh->setSize(S(90.0f), controlHeight);
    refresh->setPosition(cursorX - S(90.0f), controlY);
    refresh->setOnClick([this]()
                        {
                            if (WorldManager* worlds = engine_ ? engine_->GetWorldManager() : nullptr)
                                worlds_ = worlds->GetAvailableWorlds();

                            RebuildList();
                        });
    header->addChild(refresh);

    auto refreshIcon = std::make_shared<UIIcon>(UIIcon::Shape::Refresh);
    refreshIcon->setColor(UITheme::Text);
    refreshIcon->setPosition(S(10.0f), (controlHeight - S(11.0f)) * 0.5f);
    refreshIcon->setSize(S(11.0f), S(11.0f));
    refresh->addChild(refreshIcon);
    refresh->setLabelInset(S(21.0f), 0.0f);

    cursorX -= S(90.0f) + S(12.0f);

    // Filter pills.
    struct PillSpec
    {
        const char* label;
        Filter      filter;
        float       width;
    };

    const PillSpec pills[] = {
        {"Favorites", Filter::Favorites, S(96.0f)},
        {"Recent",    Filter::Recent,    S(78.0f)},
        {"All",       Filter::All,       S(54.0f)},
    };

    for (const PillSpec& pill : pills)
    {
        auto button = std::make_shared<UIButton>();
        button->setText(pill.label);
        button->setFont(BodyFont(UITheme::Body::Caption));
        button->setVariant(UIButton::Variant::Ghost);
        button->setBorderRadius(S(5.0f));
        button->setSize(pill.width, controlHeight);
        button->setPosition(cursorX - pill.width, controlY);

        const Filter target = pill.filter;
        button->setOnClick([this, target]() { SetFilter(target); });

        header->addChild(button);

        switch (pill.filter)
        {
        case Filter::All:       allButton_       = button; break;
        case Filter::Recent:    recentButton_    = button; break;
        case Filter::Favorites: favoritesButton_ = button; break;
        }

        cursorX -= pill.width + S(5.0f);
    }

    cursorX -= S(9.0f);

    // Search field.
    const float searchWidth = S(200.0f);

    searchBox_ = std::make_shared<UITextBox>();
    searchBox_->setFont(BodyFont(UITheme::Body::Regular));
    // The field filters the list and names a world to enter, because with no
    // server list there is nothing to filter and typing a name is the only way
    // in. MaxLength matches the protocol's world-name limit.
    searchBox_->setPlaceholderText("Search or type a world name...");
    searchBox_->setMaxLength(static_cast<int>(ProtocolLimits::MaxWorldNameLength));
    searchBox_->setPadding(S(12.0f));
    searchBox_->setSize(searchWidth, controlHeight);
    searchBox_->setPosition(cursorX - searchWidth, controlY);
    searchBox_->setOnTextChanged([this](const std::string& text)
                                 {
                                     query_ = text;
                                     RebuildList();
                                 });
    searchBox_->setOnEnterPressed([this]() { EnterTypedWorld(); });
    header->addChild(searchBox_);

    SetFilter(Filter::All);
}

void WorldBrowserScreen::BuildFooter(float x, float y, float width)
{
    auto footer = std::make_shared<UIPanel>();
    footer->setBackgroundColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    footer->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    footer->setBorderRadius(0.0f);
    footer->setPosition(x, y);
    footer->setSize(width, kFooterHeight);
    root_->addChild(footer);

    auto rule = std::make_shared<UIPanel>();
    rule->setBackgroundColor(UITheme::WithAlpha(UITheme::Border, 0.15f));
    rule->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    rule->setBorderRadius(0.0f);
    rule->setPosition(0.0f, 0.0f);
    rule->setSize(width, UITheme::BorderThin);
    footer->addChild(rule);

    countLabel_ = std::make_shared<UILabel>();
    countLabel_->setFont(BodyFont(UITheme::Body::Welcome));
    countLabel_->setTextColor(UITheme::Muted);
    countLabel_->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    countLabel_->setPosition(kHeaderPadX, 0.0f);
    countLabel_->setSize(S(240.0f), kFooterHeight);
    footer->addChild(countLabel_);

    selectionLabel_ = std::make_shared<UILabel>();
    selectionLabel_->setFont(BodyFont(UITheme::Body::Welcome));
    selectionLabel_->setTextColor(UITheme::Accent);
    selectionLabel_->setAlignment(UILabel::Alignment::Center);
    selectionLabel_->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
    selectionLabel_->setPosition(width * 0.5f - S(150.0f), 0.0f);
    selectionLabel_->setSize(S(300.0f), kFooterHeight);
    footer->addChild(selectionLabel_);

    const float buttonHeight = S(24.0f);
    const float buttonY      = (kFooterHeight - buttonHeight) * 0.5f;

    auto back = std::make_shared<UIButton>();
    back->setText("BACK");
    back->setFont(DisplayFont(UITheme::Display::Small));
    back->setVariant(UIButton::Variant::Purple);
    back->setSize(S(86.0f), buttonHeight);
    back->setPosition(width - kHeaderPadX - S(86.0f), buttonY);
    back->setOnClick([this]() { RequestScreenChange(ScreenID::Continue); });
    footer->addChild(back);

    auto backIcon = std::make_shared<UIIcon>(UIIcon::Shape::ArrowLeft);
    backIcon->setColor(UITheme::Text);
    backIcon->setPosition(S(8.0f), (buttonHeight - S(10.0f)) * 0.5f);
    backIcon->setSize(S(10.0f), S(10.0f));
    back->addChild(backIcon);
    back->setLabelInset(S(18.0f), 0.0f);

    // The world browser has always offered world creation; the design does not
    // draw it, so it is kept here in the footer rather than in the list.
    //
    // It enters the world named in the search field. The server creates the
    // world if it does not exist yet and places the player in it either way,
    // so one control covers both cases.
    createButton_ = std::make_shared<UIButton>();
    createButton_->setText("CREATE WORLD");
    createButton_->setFont(DisplayFont(UITheme::Display::Small));
    createButton_->setVariant(UIButton::Variant::Primary);
    createButton_->setSize(S(130.0f), buttonHeight);
    createButton_->setPosition(width - kHeaderPadX - S(86.0f) - S(10.0f) - S(130.0f), buttonY);
    createButton_->setOnClick([this]() { EnterTypedWorld(); });
    footer->addChild(createButton_);
}

void WorldBrowserScreen::SetFilter(Filter filter)
{
    filter_ = filter;

    // Selected pills take the design's highlighted treatment.
    const auto style = [](const std::shared_ptr<UIButton>& button, bool selected)
    {
        if (!button)
            return;

        if (selected)
        {
            button->setNormalColors(UITheme::WithAlpha(UITheme::Primary, 0.18f),
                                    UITheme::WithAlpha(UITheme::Primary, 0.18f),
                                    UITheme::WithAlpha(UITheme::Primary, 0.50f));
            button->setTextColor(UITheme::Accent);
        }
        else
        {
            button->setNormalColors(Color(0.0f, 0.0f, 0.0f, 0.0f),
                                    Color(0.0f, 0.0f, 0.0f, 0.0f),
                                    UITheme::SubtleBorder);
            button->setTextColor(UITheme::Subtext);
        }
    };

    style(allButton_,       filter_ == Filter::All);
    style(recentButton_,    filter_ == Filter::Recent);
    style(favoritesButton_, filter_ == Filter::Favorites);

    RebuildList();
}

void WorldBrowserScreen::RebuildList()
{
    if (!list_)
        return;

    list_->clearContent();

    const std::string needle = ToLower(query_);
    const float rowWidth = list_->getWidth() - S(8.0f);   // Leave room for the thumb.

    float y = 0.0f;
    size_t shown = 0;

    for (const WorldInfo& world : worlds_)
    {
        if (!needle.empty())
        {
            const bool matches = ToLower(world.name).find(needle) != std::string::npos ||
                                 ToLower(world.owner).find(needle) != std::string::npos;
            if (!matches)
                continue;
        }

        if (filter_ == Filter::Recent && !world.recent)
            continue;

        if (filter_ == Filter::Favorites && !world.favourite)
            continue;

        list_->addContent(BuildWorldRow(world, rowWidth, y));

        y += kRowHeight + kRowGap;
        ++shown;
    }

    if (shown == 0)
    {
        // Two distinct empty states: the design's "no search matches", and the
        // one that applies before any server has sent a world list - which is
        // every session until the world-list packet exists.
        const bool noCatalogue = worlds_.empty();

        auto empty = std::make_shared<UIPanel>();
        empty->setBackgroundColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
        empty->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
        empty->setPosition(0.0f, S(40.0f));
        empty->setSize(rowWidth, noCatalogue ? S(104.0f) : S(80.0f));

        auto icon = std::make_shared<UIIcon>(noCatalogue ? UIIcon::Shape::Diamond
                                                         : UIIcon::Shape::Search);
        icon->setColor(UITheme::Muted);
        icon->setPosition((rowWidth - S(32.0f)) * 0.5f, 0.0f);
        icon->setSize(S(32.0f), S(32.0f));
        empty->addChild(icon);

        auto message = std::make_shared<UILabel>();
        message->setText(noCatalogue ? "No world list from the server."
                                     : "No worlds match your search.");
        message->setFont(BodyFont(UITheme::Body::Medium));
        message->setTextColor(UITheme::Muted);
        message->setAlignment(UILabel::Alignment::Center);
        message->setPosition(0.0f, S(40.0f));
        message->setSize(rowWidth, S(24.0f));
        empty->addChild(message);

        if (noCatalogue)
        {
            auto hint = std::make_shared<UILabel>();
            hint->setText("Type a world name above and press ENTER to enter it.");
            hint->setFont(BodyFont(UITheme::Body::Caption));
            hint->setTextColor(UITheme::WithAlpha(UITheme::Muted, 0.80f));
            hint->setAlignment(UILabel::Alignment::Center);
            hint->setPosition(0.0f, S(66.0f));
            hint->setSize(rowWidth, S(20.0f));
            empty->addChild(hint);
        }

        list_->addContent(empty);
    }

    list_->refreshContentHeight();

    if (countLabel_)
    {
        if (worlds_.empty())
            countLabel_->setText("No worlds listed");
        else if (shown == worlds_.size())
            countLabel_->setText(std::format("{} worlds available", worlds_.size()));
        else
            countLabel_->setText(std::format("{} of {} worlds", shown, worlds_.size()));
    }
}

void WorldBrowserScreen::EnterTypedWorld()
{
    // A name of nothing but spaces is not a name.
    const size_t first = query_.find_first_not_of(" \t");

    if (first == std::string::npos)
    {
        if (selectionLabel_)
        {
            selectionLabel_->setText("Type a world name first.");
            selectionLabel_->setTextColor(UITheme::Warning);
        }

        if (searchBox_ && uiManager_)
            uiManager_->setFocusedElement(searchBox_);

        return;
    }

    const size_t      last = query_.find_last_not_of(" \t");
    const std::string name = query_.substr(first, last - first + 1);

    // A world already in the list carries the server's details; a name typed by
    // hand carries only itself, which is all a join needs.
    const auto known = std::find_if(worlds_.begin(), worlds_.end(),
                                    [&name](const WorldInfo& world)
                                    { return world.name == name; });

    if (known != worlds_.end())
    {
        JoinWorld(*known);
        return;
    }

    WorldInfo world;
    world.name = name;

    JoinWorld(world);
}

std::shared_ptr<UIPanel> WorldBrowserScreen::BuildWorldRow(const WorldInfo& world,
                                                           float width, float y)
{
    const bool selected = world.name == selectedWorld_;

    auto row = std::make_shared<UIPanel>();
    row->setBackgroundColor(world.recent ? UITheme::WithAlpha(UITheme::Primary, 0.06f)
                                         : UITheme::RowBackground);
    row->setBorder(selected ? UITheme::WithAlpha(UITheme::Accent, 0.75f)
                            : (world.recent ? UITheme::WithAlpha(UITheme::Primary, 0.35f)
                                            : UITheme::SubtleBorder),
                   UITheme::BorderThin);
    row->setBorderRadius(UITheme::RadiusCard);
    row->setPosition(0.0f, y);
    row->setSize(width, kRowHeight);
    row->setBlocksInput(true);

    if (selected)
        row->setGlow(UITheme::WithAlpha(UITheme::Accent, 0.30f), S(8.0f));

    // Favourite star.
    auto star = std::make_shared<UIIcon>(UIIcon::Shape::Star);
    star->setColor(world.favourite ? UITheme::Gold : UITheme::WithAlpha(UITheme::Border, 0.25f));
    star->setPosition(kRowPadX, (kRowHeight - S(14.0f)) * 0.5f);
    star->setSize(S(14.0f), S(14.0f));
    row->addChild(star);

    const float infoX = kRowPadX + S(14.0f) + S(14.0f);
    const float infoWidth = width - infoX - kPopWidth - kRowPadX - S(14.0f);

    // Name, tag chip and type chip on one line.
    float chipX = infoX;

    auto name = std::make_shared<UILabel>();
    name->setText(world.name);
    name->setFont(DisplayFont(UITheme::Display::Label));
    name->setTextColor(UITheme::Text);
    name->setPosition(chipX, kRowPadY);
    name->setSize(infoWidth, S(11.0f));
    row->addChild(name);
    name->sizeToFit();
    chipX += name->getWidth() + S(8.0f);

    if (const char* tag = TagFor(world))
    {
        const Color color = TagColor(tag);
        const float chipWidth = S(9.0f) * static_cast<float>(std::string(tag).size()) + S(12.0f);

        auto chip = std::make_shared<UIPanel>();
        chip->setBackgroundColor(UITheme::WithAlpha(color, 0.10f));
        chip->setBorder(UITheme::WithAlpha(color, 0.27f), UITheme::BorderThin);
        chip->setBorderRadius(UITheme::RadiusChip);
        chip->setPosition(chipX, kRowPadY - S(2.0f));
        chip->setSize(chipWidth, S(14.0f));
        row->addChild(chip);

        auto chipLabel = std::make_shared<UILabel>();
        chipLabel->setText(tag);
        chipLabel->setFont(DisplayFont(UITheme::Display::Micro));
        chipLabel->setTextColor(color);
        chipLabel->setAlignment(UILabel::Alignment::Center);
        chipLabel->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
        chipLabel->setPosition(0.0f, 0.0f);
        chipLabel->setSize(chipWidth, S(14.0f));
        chip->addChild(chipLabel);

        chipX += chipWidth + S(8.0f);
    }

    // The type chip is only drawn for a world whose type the server has told
    // us; an unknown type would otherwise render as an empty coloured pill.
    if (!world.type.empty())
    {
        const Color color = TypeColor(world.type);
        const float chipWidth = S(7.0f) * static_cast<float>(world.type.size()) + S(14.0f);

        auto chip = std::make_shared<UIPanel>();
        chip->setBackgroundColor(UITheme::WithAlpha(color, 0.10f));
        chip->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
        chip->setBorderRadius(UITheme::RadiusChip);
        chip->setPosition(chipX, kRowPadY - S(2.0f));
        chip->setSize(chipWidth, S(14.0f));
        row->addChild(chip);

        auto chipLabel = std::make_shared<UILabel>();
        chipLabel->setText(world.type);
        chipLabel->setFont(BodyFont(UITheme::Body::Tiny));
        chipLabel->setTextColor(color);
        chipLabel->setAlignment(UILabel::Alignment::Center);
        chipLabel->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
        chipLabel->setPosition(0.0f, 0.0f);
        chipLabel->setSize(chipWidth, S(14.0f));
        chip->addChild(chipLabel);
    }

    auto owner = std::make_shared<UILabel>();
    owner->setText("by " + world.owner);
    owner->setFont(BodyFont(UITheme::Body::Caption));
    owner->setTextColor(UITheme::Muted);
    owner->setPosition(infoX, kRowPadY + S(15.0f));
    owner->setSize(infoWidth, S(14.0f));
    row->addChild(owner);

    auto description = std::make_shared<UILabel>();
    description->setText(world.description);
    description->setFont(BodyFont(UITheme::Body::Caption));
    description->setTextColor(UITheme::Subtext);
    description->setPosition(infoX, kRowPadY + S(29.0f));
    description->setSize(infoWidth, S(14.0f));
    row->addChild(description);

    // --- Population and join action --------------------------------------
    const float popX = width - kRowPadX - kPopWidth;
    const Color popColor = PopulationColor(world);

    // Population is only shown when the server has reported a capacity.
    // Without one, a count and a bar would be pure invention.
    if (world.HasPopulation())
    {
        auto population = std::make_shared<UILabel>();
        population->setText(std::format("{}/{}", world.players, world.maxPlayers));
        population->setFont(DataFont(UITheme::Data::Regular));
        population->setTextColor(popColor);
        population->setAlignment(UILabel::Alignment::Right);
        population->setPosition(popX, kRowPadY);
        population->setSize(kPopWidth, S(12.0f));
        row->addChild(population);

        auto bar = std::make_shared<UIProgressBar>();
        bar->setProgress(static_cast<float>(world.players) /
                         static_cast<float>(world.maxPlayers));
        bar->setFillColor(popColor);
        bar->setGlowColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
        bar->setBorderRadius(S(2.0f));
        bar->setPosition(popX, kRowPadY + S(16.0f));
        bar->setSize(kPopWidth, S(4.0f));
        row->addChild(bar);
    }

    auto join = std::make_shared<UIButton>();
    join->setText(world.IsFull() ? "FULL" : "JOIN");
    join->setFont(DisplayFont(UITheme::Display::Small));
    join->setVariant(UIButton::Variant::Primary);
    join->setPosition(popX, kRowPadY + S(26.0f));
    join->setSize(kPopWidth, S(20.0f));
    join->setEnabled(!world.IsFull());

    const WorldInfo captured = world;
    join->setOnClick([this, captured]() { JoinWorld(captured); });
    row->addChild(join);

    return row;
}

void WorldBrowserScreen::JoinWorld(const WorldInfo& world)
{
    if (joining_ || world.IsFull())
        return;

    joining_       = true;
    selectedWorld_ = world.name;

    if (selectionLabel_)
        selectionLabel_->setText("Joining " + world.name + "...");

    if (engine_)
    {
        engine_->SetSelectedWorldName(world.name);

        // Remember the choice so Continue can offer it next session.
        if (WorldManager* worlds = engine_->GetWorldManager())
            worlds->SetLastWorld(world.name, engine_->GetSignedInUser());

        // Ask the server to place us in the world. The loading screen waits on
        // the response; offline mode skips straight to the local world.
        NetworkManager& network = engine_->getNetworkManager();

        if (network.isConnected() && !network.sendWorldJoin(world.name))
        {
            joining_ = false;

            if (selectionLabel_)
                selectionLabel_->setText("Could not reach the server.");

            return;
        }
    }

    LOG_INFO("WorldBrowserScreen: joining world '" + world.name + "'");

    RequestScreenChange(ScreenID::Loading);
}

void WorldBrowserScreen::OnKeyDown(int key, bool, bool)
{
    if (key == UIKey::Escape && !joining_)
        RequestScreenChange(ScreenID::Continue);
}
