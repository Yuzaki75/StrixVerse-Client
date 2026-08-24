#include "CreditsScreen.h"

#include "../core/Engine.h"
#include "../audio/AudioManager.h"
#include "../core/Logger.h"
#include "../core/Version.h"
#include "../ui/UIButton.h"
#include "../ui/UIIcon.h"
#include "../ui/UILabel.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

#include <array>
#include <string>
#include <utility>

namespace
{
    struct Section
    {
        const char* heading;
        const char* lines[6];
    };

    // The third-party list mirrors what CMakeLists actually links and what the
    // repository vendors. Fonts are the three OFL faces in assets/fonts.
    constexpr std::array<Section, 3> kSections = {{
        {"STRIXVERSE STUDIOS", {
            "Design and development",
            "Client, server and world systems",
            nullptr, nullptr, nullptr, nullptr}},
        {"BUILT WITH", {
            "SDL3 - windowing, input and audio",
            "OpenGL and GLAD - rendering",
            "FreeType - font rasterisation",
            "glm - mathematics",
            "stb_image / stb_vorbis - image and audio decoding",
            "SQLite - server persistence"}},
        {"TYPEFACES", {
            "Press Start 2P, VT323, Share Tech Mono",
            "All under the SIL Open Font License",
            nullptr, nullptr, nullptr, nullptr}},
    }};
}

CreditsScreen::CreditsScreen(Engine* engine)
    : Screen(engine)
{
}

void CreditsScreen::OnEnter()
{
    if (!uiManager_)
    {
        LOG_ERROR("CreditsScreen: UIManager not available");
        return;
    }

    CreateRoot();

    AddBackdrop(UITheme::Hex(0x090E1A), UITheme::Hex(0x0F1828), true);

    const float originX = DesignOriginX();
    const float originY = DesignOriginY();

    auto title = std::make_shared<UILabel>();
    title->setText("CREDITS");
    title->setFont(DisplayFont(UITheme::Display::Title));
    title->setTextColor(UITheme::Text);
    title->setAlignment(UILabel::Alignment::Center);
    title->setLetterSpacing(4.0f);
    title->setGlow(UITheme::WithAlpha(UITheme::Accent, 0.45f), 20.0f);
    title->setPosition(originX, originY + 120.0f);
    title->setSize(UIScale::kDesignWidth, 46.0f);
    root_->addChild(title);

    // One panel per section, stacked and centred.
    const float panelWidth = 860.0f;
    const float panelX     = originX + (UIScale::kDesignWidth - panelWidth) * 0.5f;

    float y = originY + 230.0f;

    for (const Section& section : kSections)
    {
        size_t lineCount = 0;
        for (const char* line : section.lines)
        {
            if (!line)
                break;
            ++lineCount;
        }

        const float headerHeight = 52.0f;
        const float lineHeight   = 34.0f;
        const float panelHeight  = headerHeight + static_cast<float>(lineCount) * lineHeight + 22.0f;

        auto panel = std::make_shared<UIPanel>();
        panel->setBackgroundColor(UITheme::Panel);
        panel->setBorder(UITheme::PanelBorder, UITheme::BorderThin);
        panel->setBorderRadius(UITheme::RadiusPanel);
        panel->setPosition(panelX, y);
        panel->setSize(panelWidth, panelHeight);
        root_->addChild(panel);

        auto heading = std::make_shared<UILabel>();
        heading->setText(section.heading);
        heading->setFont(DisplayFont(UITheme::Display::Label));
        heading->setTextColor(UITheme::Accent);
        heading->setLetterSpacing(2.4f);
        heading->setPosition(28.0f, 22.0f);
        heading->setSize(panelWidth - 56.0f, 22.0f);
        panel->addChild(heading);

        for (size_t i = 0; i < lineCount; ++i)
        {
            auto line = std::make_shared<UILabel>();
            line->setText(section.lines[i]);
            line->setFont(BodyFont(UITheme::Body::Regular));
            line->setTextColor(UITheme::Subtext);
            line->setPosition(28.0f, headerHeight + static_cast<float>(i) * lineHeight);
            line->setSize(panelWidth - 56.0f, lineHeight);
            panel->addChild(line);
        }

        y += panelHeight + 20.0f;
    }

    auto version = std::make_shared<UILabel>();
    version->setText(std::string("StrixVerse Client v") + Version::GetClientVersion());
    version->setFont(DataFont(UITheme::Data::Small));
    version->setTextColor(UITheme::Muted);
    version->setAlignment(UILabel::Alignment::Center);
    version->setLetterSpacing(2.4f);
    version->setPosition(originX, y + 8.0f);
    version->setSize(UIScale::kDesignWidth, 26.0f);
    root_->addChild(version);

    const float buttonWidth  = 200.0f;
    const float buttonHeight = 56.0f;

    backButton_ = std::make_shared<UIButton>();
    backButton_->setText("BACK");
    backButton_->setFont(DisplayFont(UITheme::Display::Button));
    backButton_->setVariant(UIButton::Variant::Purple);
    backButton_->setSize(buttonWidth, buttonHeight);
    backButton_->setPosition(originX + (UIScale::kDesignWidth - buttonWidth) * 0.5f,
                             originY + UIScale::kDesignHeight - buttonHeight - 60.0f);
    backButton_->setOnClick([this]() {
        engine_->GetAudio().PlaySfx("ui_click");
        OnBack();
    });
    root_->addChild(backButton_);

    auto backIcon = std::make_shared<UIIcon>(UIIcon::Shape::ArrowLeft);
    backIcon->setColor(UITheme::Text);
    backIcon->setPosition(22.0f, (buttonHeight - 20.0f) * 0.5f);
    backIcon->setSize(20.0f, 20.0f);
    backButton_->addChild(backIcon);
    backButton_->setLabelInset(38.0f, 0.0f);
}

void CreditsScreen::OnBack()
{
    RequestScreenChange(ScreenID::MainMenu);
}

void CreditsScreen::OnKeyDown(int key, bool, bool)
{
    if (key == UIKey::Escape)
        OnBack();
}
