#include "GameScreen.h"
#include "../core/Logger.h"
#include "../core/Engine.h"
#include "../core/ServiceLocator.h"
#include "../ui/UIManager.h"
#include "../graphics/Color.h"
#include "../core/Window.h"
#include "../ecs/EntityManager.h"
#include "../ecs/ComponentManager.h"
#include "../ecs/SystemManager.h"

GameScreen::GameScreen(Engine *engine)
    : Screen(engine), m_Panel(nullptr), m_TitleLabel(nullptr), m_SettingsButton(nullptr), m_HUD(nullptr), m_World(nullptr), m_TileRenderer(nullptr)
{
}

void GameScreen::OnEnter()
{
    if (!engine_)
    {
        LOG_ERROR("GameScreen: Engine is null");
        return;
    }

    if (!uiManager_)
    {
        LOG_ERROR("GameScreen: UIManager not available");
        return;
    }

    // Initialize UI components
    InitializeUI();

    // Initialize HUD
    InitializeHUD();

    // Initialize the world and tile renderer
    InitializeWorld();
}

void GameScreen::InitializeUI()
{
    int width, height;
    engine_->GetWindow()->GetSize(width, height);

    // Create a panel to hold the game UI (background, etc.)
    m_Panel = std::make_shared<UIPanel>();
    m_Panel->setSize(static_cast<float>(width), static_cast<float>(height));
    m_Panel->setPosition(0.0f, 0.0f);
    m_Panel->setBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f}); // Transparent
    uiManager_->addElement(m_Panel);

    // Title label (for debugging)
    m_TitleLabel = std::make_shared<UILabel>();
    m_TitleLabel->setText("StrixVerse - Gameplay Screen");
    m_TitleLabel->setTextColor({1.0f, 1.0f, 1.0f, 1.0f}); // White
    m_TitleLabel->setFontSize(24.0f);
    m_TitleLabel->setPosition(
        (static_cast<float>(width) / 2.0f),
        50.0f);
    m_TitleLabel->setAlignment(UILabel::Alignment::Center);
    m_Panel->addChild(m_TitleLabel);

    // Settings button (top-right corner)
    m_SettingsButton = std::make_shared<UIButton>();
    m_SettingsButton->setSize(100.0f, 40.0f);
    m_SettingsButton->setPosition(
        static_cast<float>(width) - 110.0f,
        10.0f);
    m_SettingsButton->setText("Settings");
    m_SettingsButton->setBackgroundColor({0.0f, 0.0f, 0.6f, 1.0f}); // Blue
    m_SettingsButton->setTextColor({1.0f, 1.0f, 1.0f, 1.0f});
    m_SettingsButton->setOnClickCallback([this]()
                                         { this->OnSettingsButtonClicked(); });
    m_Panel->addChild(m_SettingsButton);
}

void GameScreen::Update(float deltaTime)
{
    // Update HUD
    if (m_HUD)
    {
        m_HUD->Update(deltaTime);
    }

    // Update tile renderer system
    if (m_TileRenderer)
    {
        // TileRendererSystem update handled by SystemManager
    }

    // Update game logic (placeholder for actual game logic)
    // For now, we just update the UI (which is done by UIManager)
    // The ECS systems are updated in Engine::Update
}

void GameScreen::Render() const
{
    // Render the world (placeholder)
    // In a real game, we would render the world terrain, entities, etc.
    // For now, we rely on the ECS systems and UIManager for rendering
    // The ECS systems are updated and rendered in Engine::Update and Engine::Render
}

void GameScreen::OnSettingsButtonClicked()
{
    RequestScreenChange(ScreenID::Settings);
}

void GameScreen::InitializeWorld()
{
    // Get ECS managers from the service locator
    auto entityManager = ServiceLocator::Get<StrixVerse::ECS::EntityManager>();
    auto componentManager = ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();
    auto systemManager = ServiceLocator::Get<StrixVerse::ECS::SystemManager>();

    if (!entityManager || !componentManager || !systemManager)
    {
        LOG_ERROR("GameScreen: Failed to get ECS managers from ServiceLocator");
        return;
    }

    // Create a new world
    m_World = std::make_unique<StrixVerse::World::World>();

    // Generate a small world for testing (4x4x1 chunks)
    // Each chunk is 16x16x4 tiles, so this creates a 64x64x4 tile world
    m_World->GenerateNewWorld(4, 4, 1);

    LOG_INFO("GameScreen: Initialized world with " +
             std::to_string(m_World->GetWidthInChunks()) + "x" +
             std::to_string(m_World->GetHeightInChunks()) + "x" +
             std::to_string(m_World->GetDepthInChunks()) + " chunks (" +
             std::to_string(m_World->GetWidthInTiles()) + "x" +
             std::to_string(m_World->GetHeightInTiles()) + "x" +
             std::to_string(m_World->GetDepthInTiles()) + " tiles");

    // Example: Modify a few tiles to show different terrain types
    auto grassTile = std::make_shared<StrixVerse::World::Tile>(StrixVerse::World::Tile::Type::Grass);
    auto stoneTile = std::make_shared<StrixVerse::World::Tile>(StrixVerse::World::Tile::Type::Stone);
    auto waterTile = std::make_shared<StrixVerse::World::Tile>(StrixVerse::World::Tile::Type::Water);

    // Set some specific tiles
    m_World->SetTileAt(10, 10, 0, stoneTile); // Stone at position (10, 10, 0)
    m_World->SetTileAt(20, 20, 0, waterTile); // Water at position (20, 20, 0)

    // Create and initialize the tile renderer system
    m_TileRenderer = std::make_unique<StrixVerse::ECS::TileRendererSystem>();
    m_TileRenderer->init(entityManager.get(), componentManager.get());
    m_TileRenderer->SetWorld(m_World.get());

    // Add the tile renderer system to the system manager
    // Note: SystemManager::addSystem expects shared_ptr, but we own a unique_ptr
    // We'll create a shared_ptr and keep the unique_ptr empty
    // This is a workaround - ideally TileRendererSystem would be managed by SystemManager
    auto tileRendererShared = std::shared_ptr<StrixVerse::ECS::System>(m_TileRenderer.release());
    systemManager->addSystem(tileRendererShared);

    // Update HUD with world info
    if (m_HUD)
    {
        m_HUD->AddChatMessage("World initialized: " +
                              std::to_string(m_World->GetWidthInTiles()) + "x" +
                              std::to_string(m_World->GetHeightInTiles()) + "x" +
                              std::to_string(m_World->GetDepthInTiles()) + " tiles");
    }
}

void GameScreen::InitializeHUD()
{
    // Initialize HUD
    m_HUD = std::make_unique<HUD>(engine_);
    m_HUD->Initialize();

    // Set some initial values for demo
    m_HUD->SetHealth(100.0f, 100.0f);
    m_HUD->SetMana(50.0f, 50.0f);
    m_HUD->SetLevel(5);
    m_HUD->SetExperience(350, 500);
    m_HUD->SetCoins(150);
    m_HUD->SetGems(5);
    m_HUD->AddChatMessage("Welcome to StrixVerse!");
    m_HUD->AddChatMessage("Press F1 for help.");
    m_HUD->ShowNotification("Game started!", 3.0f);
}

void GameScreen::OnExit()
{
    if (uiManager_ && m_Panel)
    {
        uiManager_->removeElement(m_Panel);
        m_Panel.reset();
        m_TitleLabel.reset();
        m_SettingsButton.reset();
    }

    // HUD will be cleaned up when its unique_ptr goes out of scope
    m_HUD.reset();

    // Clean up tile renderer system
    if (m_TileRenderer)
    {
        m_TileRenderer.reset();
    }

    // World will be cleaned up when its unique_ptr goes out of scope
    m_World.reset();
}