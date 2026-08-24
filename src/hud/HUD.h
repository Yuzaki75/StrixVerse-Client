#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <string>

#include "../graphics/Color.h"

class Engine;
class UIManager;
class UILabel;
class UIImage;
class UIPanel;
class UIButton;
class UIProgressBar;
class UITextBox;
class NetworkManager;

/**
 * Heads-Up Display (HUD) for the game.
 * Displays player stats, inventory, chat, and other gameplay information.
 */
class HUD
{
public:
    HUD(Engine* engine);
    ~HUD();

    // Initialize the HUD (create UI elements)
    void Initialize();

    // Update the HUD (called each frame)
    void Update(float deltaTime);

    // Render the HUD (handled by UIManager, but we might have custom rendering)
    void Render();

    // --- Character stats ---------------------------------------------------
    // Everything shown here comes from the server. `known` false leaves the
    // panels blank rather than displaying a plausible starting figure, which
    // is what the HUD used to do.
    struct Stats
    {
        bool     known                 = false;
        int      level                 = 0;
        int      experience            = 0;
        unsigned experienceToNextLevel = 0;
        unsigned health                = 0;
        unsigned maxHealth             = 0;
    };

    void SetStats(const Stats& stats);

    // Add a chat message
    // What wrote this line. Player speech and the server's own notices share
    // the log, and only the sender can say which is which - the previous
    // version guessed from the wording, because the server had no channel of
    // its own to arrive on.
    //
    // The values follow the notification severity scale the server sends:
    // 0 information, 1 warning, 2 success, 3 error.
    enum class ChatKind
    {
        Player  = -1,
        System  = 0,
        Warning = 1,
        Success = 2,
        Error   = 3,
    };

    void AddChatMessage(const std::string& message, ChatKind kind = ChatKind::Player,
                        const Color& lineColor = Color(0.0f, 0.0f, 0.0f, 0.0f));

    // Pushes a system notice onto the top-centre notification stack. This is
    // the channel for join/left/connection events, kept out of the chat log
    // so conversation and world events never blur together.
    void AddNotification(const std::string& message);

    // Routes server notifications into the stack: registers this HUD as the
    // sink on the given NetworkManager and styles each notice by its severity.
    // Called lazily from Update on the first frame, so GameScreen does not
    // have to wire anything up; calling it explicitly is also fine.
    void BindNotificationSource(NetworkManager* network);

    // --- Inventory ---------------------------------------------------------
    // One entry per occupied slot, in slot order. Passing an empty list draws
    // the bar as entirely empty, which is the honest state before the server
    // has granted the player anything.
    struct InventoryEntry
    {
        uint8_t  slot     = 0;
        uint16_t itemId   = 0;
        uint16_t quantity = 0;
    };

    void SetInventory(const std::vector<InventoryEntry>& entries);

    // --- Tools and slot selection ------------------------------------------
    // The first two hotbar slots are permanent tools rather than inventory.
    // They are client-side only: PUNCH sends the same bare-handed break the
    // server already accepts (tool id 0), and WRENCH is purely a client
    // interaction mode, so neither needs an item definition or a grant.
    enum class Tool
    {
        Punch = 0,   // break blocks
        Wrench = 1,  // inspect players
        Item = 2     // anything from slot 2 upward is a real inventory item
    };

    static constexpr uint8_t kPunchSlot  = 0;
    static constexpr uint8_t kWrenchSlot = 1;
    static constexpr uint8_t kFirstItemSlot = 2;

    uint8_t GetSelectedSlot() const { return m_SelectedSlot; }
    void    SetSelectedSlot(uint8_t slot);

    // Wraps around the bar. Positive delta moves right (slot 0 -> 1).
    void CycleSelectedSlot(int delta);

    Tool GetSelectedTool() const
    {
        if (m_SelectedSlot == kPunchSlot)  return Tool::Punch;
        if (m_SelectedSlot == kWrenchSlot) return Tool::Wrench;
        return Tool::Item;
    }

    void SetOnSlotSelected(std::function<void(uint8_t)> callback)
    {
        m_OnSlotSelected = std::move(callback);
    }

    // --- Chat entry --------------------------------------------------------
    // Called with the typed line when the player submits it. The HUD does not
    // echo it; the owner decides what reaches the log and when.
    void SetChatSubmitHandler(std::function<void(const std::string&)> handler);

    // Puts the caret in the chat field. Movement keys stop reaching the player
    // while it holds focus.
    void FocusChatInput();

    bool IsChatInputFocused() const;

    // Show a temporary notification (routes onto the notification stack)
    void ShowNotification(const std::string& message, float duration = 3.0f);

private:
    Engine* m_Engine;
    UIManager* m_UIManager;

    // HUD elements (owned by UIManager, we just keep references for updating)
    // Mana, coins and gems used to appear here with invented values. The
    // server has no state for any of them, so the panels are gone until it
    // does - the client does not show numbers it cannot source.
    std::shared_ptr<UIPanel>       m_HealthPanel;
    std::shared_ptr<UILabel>       m_HealthLabel;
    std::shared_ptr<UIProgressBar> m_HealthBar;

    std::shared_ptr<UIPanel>       m_LevelPanel;
    std::shared_ptr<UILabel>       m_LevelLabel;
    std::shared_ptr<UILabel>       m_ExperienceLabel;
    std::shared_ptr<UIProgressBar> m_ExperienceBar;

    std::shared_ptr<UIPanel>   m_ChatBackground;
    std::shared_ptr<UITextBox> m_ChatInput;

    std::function<void(const std::string&)> m_OnChatSubmit;

    // One label per visible chat line; UILabel is single-line by design.
    std::vector<std::shared_ptr<UILabel>> m_ChatLines;

    // Each remembered line carries its own colour so the log can keep system
    // notices visually apart from player speech as it scrolls.
    struct ChatMessage
    {
        std::string text;
        Color       color{1.0f, 1.0f, 1.0f, 1.0f};
    };

    std::vector<ChatMessage> m_ChatMessages;
    static const int MAX_CHAT_MESSAGES = 10;

    // True for lines the client itself wrote rather than relayed speech.
    static bool IsSystemChatMessage(const std::string& message);

    // Hotbar: one panel per slot with a quantity label inside it.
    std::shared_ptr<UIPanel>              m_InventoryBar;
    std::vector<std::shared_ptr<UIPanel>> m_InventorySlots;

    // Transparent buttons laid over each slot. UIPanel has no click callback
    // and UIButton does, so the panel keeps the drawing and the button takes
    // the input rather than restyling the whole bar.
    std::vector<std::shared_ptr<UIButton>> m_SlotButtons;

    uint8_t                        m_SelectedSlot = kPunchSlot;
    std::function<void(uint8_t)>   m_OnSlotSelected;

    // Repaints slot borders so the selected one reads as selected.
    void RefreshSlotHighlight();
    std::vector<std::shared_ptr<UILabel>> m_InventoryLabels;

    // Item artwork, one per slot. Shown when the item has a known sprite; the
    // numeric label is the fallback for anything unrecognised, so a new item
    // appears as its id rather than as an empty square.
    std::vector<std::shared_ptr<UIImage>> m_InventoryIcons;

    // Sprite path for an item id, or empty if there is no art for it.
    static std::string IconPathForItem(uint16_t itemId);

    // Draws one of the two permanent tool slots. Falls back to the tool's name
    // if the artwork is missing, so the slot is never silently blank.
    void SetToolSlot(std::size_t barSlot, const std::string& iconPath,
                     const std::string& fallbackText, const Color& fallbackColor);
    std::vector<std::shared_ptr<UILabel>> m_InventoryCounts;

    // --- Notification stack ------------------------------------------------
    // Top-centre system notices, visually distinct from chat: semi-transparent
    // dark card with an Aether-blue/violet accent bar on its left edge. Each
    // entry fades over the last half second of its life; more than five at
    // once and the oldest drops.
    struct Notification
    {
        std::shared_ptr<UIPanel> panel;
        std::shared_ptr<UIPanel> accent;
        std::shared_ptr<UILabel> label;

        Color background{0.0f, 0.0f, 0.0f, 0.0f};
        Color accentColor{0.0f, 0.0f, 0.0f, 0.0f};

        // Alpha the panel's border was styled with, or 0 when it has none;
        // faded in Update alongside everything else.
        float borderAlpha = 0.0f;

        float age      = 0.0f;
        float remaining = 0.0f;
    };

    // Pins the four HUD sections to the window's own edges rather than to
    // the 1920x1080 design canvas.
    //
    // UIScale keeps that canvas centred and lets the axis with spare room see
    // more of the world, which is right for terrain and wrong for a HUD: at
    // 2.45:1 the health panel sat 290 pixels in from the left with empty
    // gutter beside it, because "20 from the left" meant 20 from the canvas,
    // and the canvas no longer started at the window edge.
    //
    // Called on build and again whenever the framebuffer changes shape, which
    // is why the last shape is remembered.
    void LayoutForCanvas();

    int m_LaidOutWidth  = 0;
    int m_LaidOutHeight = 0;

    static constexpr std::size_t kMaxNotifications = 5;

    // Severity: 0 info, 1 warn, 2 success, 3 error - matching
    // WorldNotificationPacket, which is where these arrive from.
    void AddNotification(const std::string& message, float duration, int severity);
    void CloseNotification(std::size_t index);
    void LayoutNotifications();

    std::vector<Notification> m_Notifications;

    // Set once this HUD has registered as the notification sink. Cleared in
    // the destructor so a notice arriving between screen changes queues on
    // the NetworkManager instead of calling into a destroyed HUD.
    bool m_NotificationBound = false;

    // Helper methods to create and position elements
    void CreateHealthSection();
    void CreateLevelSection();
    void CreateChatSection();
    void CreateInventorySection();
};