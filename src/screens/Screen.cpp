#include "Screen.h"

#include "../core/AssetManager.h"
#include "../core/Engine.h"
#include "../core/Logger.h"
#include "../graphics/Texture.h"
#include "../ui/UIFonts.h"
#include "../ui/UIManager.h"
#include "../ui/UIPanel.h"
#include "../ui/UIPatterns.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"
#include "../ui/UITiledImage.h"

Screen::Screen(Engine* engine)
    : engine_(engine)
{
    if (engine_)
        uiManager_ = engine_->GetUIManager();

    if (!uiManager_)
        LOG_WARN("Screen: UIManager not available");
}

Screen::~Screen() = default;

void Screen::OnExit()
{
    DestroyRoot();
}

void Screen::Update(float)
{
    // The Engine updates the UIManager once per frame; screens override this
    // for their own logic.
}

void Screen::OnKeyDown(int, bool, bool) {}

void Screen::OnMouseDown(float, float) {}
void Screen::OnRightMouseDown(float, float) {}
void Screen::OnMouseWheel(float, float, float) {}

void Screen::RequestScreenChange(ScreenID nextScreen)
{
    pendingChange_ = nextScreen;
}

std::shared_ptr<UIPanel> Screen::CreateRoot()
{
    DestroyRoot();

    root_ = std::make_shared<UIPanel>();

    // Transparent by default; screens paint their own background.
    root_->setBackgroundColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    root_->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    root_->setBorderRadius(0.0f);

    // Cover everything the window shows, which on a non-16:9 window extends
    // past the 1920x1080 design area.
    if (const UIScale* scale = Scale())
    {
        root_->setPosition(scale->GetVisibleLeft(), scale->GetVisibleTop());
        root_->setSize(scale->GetVisibleWidth(), scale->GetVisibleHeight());
    }
    else
    {
        root_->setPosition(0.0f, 0.0f);
        root_->setSize(UIScale::kDesignWidth, UIScale::kDesignHeight);
    }

    if (uiManager_)
        uiManager_->addElement(root_);

    return root_;
}

void Screen::DestroyRoot()
{
    if (uiManager_ && root_)
        uiManager_->removeElement(root_);

    root_.reset();
}

float Screen::DesignOriginX() const
{
    const UIScale* scale = Scale();
    return scale ? -scale->GetVisibleLeft() : 0.0f;
}

float Screen::DesignOriginY() const
{
    const UIScale* scale = Scale();
    return scale ? -scale->GetVisibleTop() : 0.0f;
}

std::shared_ptr<UIPanel> Screen::AddBackdrop(const Color& top,
                                             const Color& bottom,
                                             bool pixelGrid)
{
    if (!root_)
        return nullptr;

    const UIScale* scale = Scale();
    const float width  = scale ? scale->GetVisibleWidth() : UIScale::kDesignWidth;
    const float height = scale ? scale->GetVisibleHeight() : UIScale::kDesignHeight;

    auto background = std::make_shared<UIPanel>();
    background->setBackgroundGradient(top, bottom);
    background->setBorder(Color(0.0f, 0.0f, 0.0f, 0.0f), 0.0f);
    background->setBorderRadius(0.0f);
    background->setPosition(0.0f, 0.0f);
    background->setSize(width, height);

    root_->addChild(background);

    if (pixelGrid)
    {
        if (AssetManager* assets = Assets())
        {
            if (auto grid = UIPatterns::GetPixelGrid(*assets))
            {
                auto lattice = std::make_shared<UITiledImage>();
                lattice->setTexture(std::move(grid));
                lattice->setTileSize(UIPatterns::kPixelGridTileSize);
                lattice->setColor(UITheme::Hex(0x3A4060, 0.55f));
                lattice->setPosition(0.0f, 0.0f);
                lattice->setSize(width, height);

                root_->addChild(lattice);
            }
        }
    }

    return background;
}

UIFonts* Screen::Fonts() const
{
    return engine_ ? engine_->GetUIFonts() : nullptr;
}

AssetManager* Screen::Assets() const
{
    return engine_ ? engine_->GetAssetManager() : nullptr;
}

const UIScale* Screen::Scale() const
{
    return engine_ ? &engine_->GetUIScale() : nullptr;
}

Font* Screen::DisplayFont(unsigned int pixelSize) const
{
    UIFonts* fonts = Fonts();
    return fonts ? fonts->Get(UIFonts::Typeface::Display, pixelSize) : nullptr;
}

Font* Screen::BodyFont(unsigned int pixelSize) const
{
    UIFonts* fonts = Fonts();
    return fonts ? fonts->Get(UIFonts::Typeface::Body, pixelSize) : nullptr;
}

Font* Screen::DataFont(unsigned int pixelSize) const
{
    UIFonts* fonts = Fonts();
    return fonts ? fonts->Get(UIFonts::Typeface::Data, pixelSize) : nullptr;
}

std::shared_ptr<Texture> Screen::LoadTexture(const std::string& path) const
{
    AssetManager* assets = Assets();
    if (!assets)
        return nullptr;

    // LoadTexture returns the cached instance when it is already resident, so
    // calling this from OnEnter never re-decodes an image.
    return assets->LoadTexture(path);
}
