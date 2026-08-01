#include "WorldBrowserScreen.h"
#include "../core/Logger.h"
#include "../core/Engine.h"
#include "../graphics/Color.h"
#include "../core/Window.h"

WorldBrowserScreen::WorldBrowserScreen(Engine* engine)
    : Screen(engine)
    , m_Panel(nullptr)
    , m_TitleLabel(nullptr)
    , m_WorldButtons()
    , m_CreateButton(nullptr)
    , m_StatusLabel(nullptr)
    , m_SelectedWorld("")
{
}

void WorldBrowserScreen::OnEnter()
{
    if (!engine_)
    {
        LOG_ERROR("WorldBrowserScreen: Engine is null");
        return;
    }

    if (!uiManager_)
    {
        LOG_ERROR("WorldBrowserScreen: UIManager not available");
        return;
    }

    // Create a panel to hold the world list
    m_Panel = std::make_shared<UIPanel>();
    int width, height;
    engine_->GetWindow()->GetSize(width, height);
    m_Panel->setSize(500.0f, 400.0f);
    m_Panel->setPosition(
        (static_cast<float>(width) - 500.0f) / 2.0f,
        (static_cast<float>(height) - 400.0f) / 2.0f);
    m_Panel->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.8f}); // Semi-transparent dark
    uiManager_->addElement(m_Panel);

    float panelX = m_Panel->getPosition().x;
    float panelY = m_Panel->getPosition().y;

    // Title
    m_TitleLabel = std::make_shared<UILabel>();
    m_TitleLabel->setText("Select a World");
    m_TitleLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_TitleLabel->setFontSize(28.0f);
    m_TitleLabel->setPosition(panelX + 250.0f, panelY + 30.0f);
    m_TitleLabel->setAlignment(UILabel::Alignment::Center);
    m_Panel->addChild(m_TitleLabel);

    // World buttons (we'll create three for demo)
    CreateWorldButtons();

    // Create new world button
    m_CreateButton = std::make_shared<UIButton>();
    m_CreateButton->setSize(200.0f, 40.0f);
    m_CreateButton->setPosition(panelX + 150.0f, panelY + 340.0f);
    m_CreateButton->setText("Create New World");
    m_CreateButton->setBackgroundColor({0.0f, 0.6f, 0.0f, 1.0f}); // Green
    m_CreateButton->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_CreateButton->setOnClickCallback([this]() { this->OnCreateButtonClicked(); });
    m_Panel->addChild(m_CreateButton);

    // Status label
    m_StatusLabel = std::make_shared<UILabel>();
    m_StatusLabel->setText("");
    m_StatusLabel->setTextColor({1.0f, 1.0f, 0.0f, 1.0f}); // Yellow
    m_StatusLabel->setFontSize(18.0f);
    m_StatusLabel->setPosition(panelX + 250.0f, panelY + 380.0f);
    m_StatusLabel->setAlignment(UILabel::Alignment::Center);
    m_Panel->addChild(m_StatusLabel);
}

void WorldBrowserScreen::OnExit()
{
    if (uiManager_ && m_Panel)
    {
        uiManager_->removeElement(m_Panel);
        m_Panel.reset();
        m_TitleLabel.reset();
        m_WorldButtons.clear();
        m_CreateButton.reset();
        m_StatusLabel.reset();
    }
}

void WorldBrowserScreen::Update(float deltaTime)
{
    // No periodic updates needed
}

void WorldBrowserScreen::Render() const
{
    // UI is rendered by UIManager
}

void WorldBrowserScreen::CreateWorldButtons()
{
    float panelX = m_Panel->getPosition().x;
    float panelY = m_Panel->getPosition().y;

    // Clear any existing buttons
    m_WorldButtons.clear();

    // Hardcoded world names
    std::vector<std::string> worldNames = { "World 1", "World 2", "World 3" };

    float startY = panelY + 80.0f;
    float spacing = 60.0f;

    for (size_t i = 0; i < worldNames.size(); ++i)
    {
        auto button = std::make_shared<UIButton>();
        button->setSize(400.0f, 40.0f);
        button->setPosition(panelX + 50.0f, startY + i * spacing);
        button->setText(worldNames[i]);
        button->setBackgroundColor({0.2f, 0.2f, 0.2f, 1.0f});
        button->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
        // Capture i by value in lambda
        button->setOnClickCallback([this, i]() { this->OnWorldButtonClicked(i); });
        m_Panel->addChild(button);
        m_WorldButtons.push_back(button);
    }
}

void WorldBrowserScreen::OnWorldButtonClicked(size_t index)
{
    // For simplicity, we'll just use the index to determine the world name
    std::vector<std::string> worldNames = { "World 1", "World 2", "World 3" };
    if (index < worldNames.size())
    {
        m_SelectedWorld = worldNames[index];
        // Store selected world in engine for later use (e.g., in LoadingScreen and GameScreen)
        if (engine_)
        {
            engine_->SetSelectedWorldName(m_SelectedWorld);
        }
        m_StatusLabel->setText("Selected: " + m_SelectedWorld);
        // Proceed to loading screen
        RequestScreenChange(ScreenID::Loading);
    }
}

void WorldBrowserScreen::OnCreateButtonClicked()
{
    // For simplicity, we'll create a world with a default name
    m_SelectedWorld = "New World";
    if (engine_)
    {
        engine_->SetSelectedWorldName(m_SelectedWorld);
    }
    m_StatusLabel->setText("Created: " + m_SelectedWorld);
    // Proceed to loading screen
    RequestScreenChange(ScreenID::Loading);
}