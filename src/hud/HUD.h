#pragma once

#include <memory>
#include <vector>
#include <string>

class Engine;
class UIManager;
class UILabel;
class UIImage;
class UIPanel;
class UIButton;

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

    // Update specific elements
    void SetHealth(float current, float maximum);
    void SetMana(float current, float maximum);
    void SetExperience(int current, int requiredForNextLevel);
    void SetLevel(int level);
    void SetCoins(int amount);
    void SetGems(int amount);

    // Add a chat message
    void AddChatMessage(const std::string& message);

    // Show a temporary notification
    void ShowNotification(const std::string& message, float duration = 3.0f);

private:
    Engine* m_Engine;
    UIManager* m_UIManager;

    // HUD elements (owned by UIManager, we just keep references for updating)
    std::shared_ptr<UIPanel> m_HealthPanel;
    std::shared_ptr<UILabel> m_HealthLabel;
    std::shared_ptr<UIImage> m_HealthIcon;

    std::shared_ptr<UIPanel> m_ManaPanel;
    std::shared_ptr<UILabel> m_ManaLabel;
    std::shared_ptr<UIImage> m_ManaIcon;

    std::shared_ptr<UIPanel> m_LevelPanel;
    std::shared_ptr<UILabel> m_LevelLabel;
    std::shared_ptr<UILabel> m_ExperienceLabel;

    std::shared_ptr<UIPanel> m_CoinPanel;
    std::shared_ptr<UILabel> m_CoinLabel;
    std::shared_ptr<UIImage> m_CoinIcon;

    std::shared_ptr<UIPanel> m_GemPanel;
    std::shared_ptr<UILabel> m_GemLabel;
    std::shared_ptr<UIImage> m_GemIcon;

    std::shared_ptr<UIPanel> m_ChatBackground;
    std::shared_ptr<UILabel> m_ChatText;
    std::vector<std::string> m_ChatMessages;
    static const int MAX_CHAT_MESSAGES = 10;

    std::shared_ptr<UIPanel> m_NotificationPanel;
    std::shared_ptr<UILabel> m_NotificationLabel;
    float m_NotificationTimer;
    std::string m_NotificationMessage;
    bool m_NotificationActive;

    // Helper methods to create and position elements
    void CreateHealthSection();
    void CreateManaSection();
    void CreateLevelSection();
    void CreateCurrencySection();
    void CreateChatSection();
    void CreateNotificationSection();
};