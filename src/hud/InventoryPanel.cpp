#include "InventoryPanel.h"

#include "../core/Engine.h"
#include "../graphics/Color.h"
#include "../ui/UIFonts.h"
#include "../ui/UILabel.h"
#include "../ui/UIManager.h"
#include "../ui/UIPanel.h"
#include "../ui/UIScale.h"
#include "../ui/UITheme.h"

namespace
{
    constexpr float S(float previewPixels) { return UITheme::Scaled(previewPixels); }

    Font* PanelFont(Engine* engine, UIFonts::Typeface face, unsigned int size)
    {
        UIFonts* fonts = engine ? engine->GetUIFonts() : nullptr;
        return fonts ? fonts->Get(face, size) : nullptr;
    }

    // Panel geometry, in style-guide pixels. Eight columns of slots with room
    // for a title strip and the grid padding.
    constexpr int   kColumns  = 8;
    constexpr float kSlotSize = 26.0f;
    constexpr float kSlotGap  = 4.0f;
    constexpr float kPadding  = 10.0f;

    const Color& PanelBackground()
    {
        static const Color color = UITheme::Hex(0x0E1424, 0.85f);
        return color;
    }
}

InventoryPanel::InventoryPanel(Engine* engine, UIManager* uiManager)
    : engine_(engine), uiManager_(uiManager)
{
}

InventoryPanel::~InventoryPanel()
{
    // The UIManager outlives this panel on a screen change, so the elements
    // have to be handed back or they keep drawing over whatever comes next.
    if (uiManager_ && root_)
        uiManager_->removeElement(root_);
}

void InventoryPanel::Build()
{
    if (root_ || !uiManager_)
        return;

    BuildFrame();

    root_->setVisible(false);
}

void InventoryPanel::BuildFrame()
{
    // Grid width drives the panel width; height fits three rows, the same page
    // size as the hotbar's item range.
    const float gridWidth  = static_cast<float>(kColumns) * S(kSlotSize) +
                             static_cast<float>(kColumns - 1) * S(kSlotGap);
    const float gridHeight = S(kSlotSize) * 3.0f + S(kSlotGap) * 2.0f;
    const float width      = gridWidth + S(kPadding) * 2.0f;
    const float height     = gridHeight + S(30.0f);

    root_ = std::make_shared<UIPanel>();
    root_->setSize(width, height);
    root_->setPosition((UIScale::kDesignWidth  - width)  * 0.5f,
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
    title->setText("INVENTORY");
    title->setPosition(S(kPadding), S(8.0f));
    title->setSize(width - S(kPadding) * 2.0f, S(13.0f));
    root_->addChild(title);

    grid_ = std::make_shared<UIPanel>();
    grid_->setSize(gridWidth, gridHeight);
    grid_->setPosition(S(kPadding), S(26.0f));
    grid_->setBackgroundColor(UITheme::Hex(0x000000, 0.0f));
    root_->addChild(grid_);
}

void InventoryPanel::Open()
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

    // Slot changes while closed were stashed rather than rendered; apply them
    // now so the panel never opens showing a stale grid.
    RebuildGrid();

    root_->setVisible(true);
}

void InventoryPanel::Close()
{
    if (!root_)
        return;

    open_ = false;
    root_->setVisible(false);
}

void InventoryPanel::Toggle()
{
    if (open_)
        Close();
    else
        Open();
}

void InventoryPanel::SetSlots(const std::vector<Slot>& slots)
{
    slots_ = slots;

    // The stored index may no longer point at anything after a resize; clamp
    // rather than highlight nothing silently.
    if (selectedIndex_ >= static_cast<int>(slots_.size()))
        selectedIndex_ = static_cast<int>(slots_.size()) - 1;

    if (open_)
        RebuildGrid();
}

void InventoryPanel::SetSelectedIndex(int index)
{
    if (index == selectedIndex_)
        return;

    selectedIndex_ = index;

    if (open_)
        RebuildGrid();
}

void InventoryPanel::RebuildGrid()
{
    if (!grid_)
        return;

    grid_->clearChildren();

    const float slotSize = S(kSlotSize);

    for (std::size_t i = 0; i < slots_.size(); ++i)
    {
        const Slot& slot = slots_[i];

        const int   col = static_cast<int>(i % kColumns);
        const int   row = static_cast<int>(i / kColumns);
        const float x   = static_cast<float>(col) * (slotSize + S(kSlotGap));
        const float y   = static_cast<float>(row) * (slotSize + S(kSlotGap));

        auto cell = std::make_shared<UIPanel>();
        cell->setSize(slotSize, slotSize);
        cell->setPosition(x, y);
        cell->setBackgroundColor(UITheme::RowBackground);

        // The selected slot reads as selected by its border alone; filling it
        // would hide whatever is inside it. Slot.selected wins when present,
        // otherwise the panel's own index decides.
        const bool highlighted =
            slot.selected ||
            selectedIndex_ == static_cast<int>(i);

        cell->setBorder(highlighted ? UITheme::Accent : UITheme::SubtleBorder,
                        highlighted ? UITheme::BorderThick
                                    : UITheme::BorderThin);
        cell->setBorderRadius(UITheme::RadiusChip);
        grid_->addChild(cell);

        // Icon artwork is optional. Until sprites ship per iconPath, the
        // first letter of the name stands in, so an untextured slot is never
        // blank and two different items never look identical.
        if (!slot.name.empty())
        {
            auto glyph = std::make_shared<UILabel>();
            glyph->setFont(PanelFont(engine_, UIFonts::Typeface::Body,
                                     UITheme::Display::Small));
            glyph->setTextColor(UITheme::Subtext);
            glyph->setText(slot.name.substr(0, 1));
            glyph->setAlignment(UILabel::Alignment::Center);
            glyph->setVerticalAlignment(UILabel::VerticalAlignment::Middle);
            glyph->setPosition(0.0f, S(1.0f));
            glyph->setSize(slotSize, slotSize - S(7.0f));
            cell->addChild(glyph);
        }

        // Quantity, bottom-right. Hidden entirely for single items: a "1" on
        // every block would be noise, not information.
        if (slot.quantity > 1)
        {
            auto count = std::make_shared<UILabel>();
            count->setFont(PanelFont(engine_, UIFonts::Typeface::Data,
                                     UITheme::Display::Micro));
            count->setTextColor(UITheme::Text);
            count->setText(std::to_string(slot.quantity));
            count->setAlignment(UILabel::Alignment::Right);
            count->setPosition(S(2.0f), slotSize - S(9.0f));
            count->setSize(slotSize - S(4.0f), S(8.0f));
            cell->addChild(count);
        }
    }
}

void InventoryPanel::RaiseToFront()
{
    if (uiManager_ && root_)
    {
        uiManager_->bringToFront(root_);
    }
}
