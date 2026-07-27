#include "Game.h"

#include "Logger.h"
#include "Core/ServiceLocator.h"
#include "ecs/TransformComponent.h"
#include "ecs/SpriteComponent.h"
#include "ecs/VelocityComponent.h"
#include "ecs/EntityManager.h"
#include "ecs/ComponentManager.h"

namespace
{
    constexpr const char* kStartupScene = "MainMenu";
}

Game::~Game()
{
    // RAII safety net for early exits.
    Shutdown();
}

bool Game::Initialize()
{
    if (m_Initialized)
        return true;

    m_Initialized = true;

    if (!LoadScene(kStartupScene))
    {
        Logger::Error("Game: failed to load startup scene.");
        m_Initialized = false;
        return false;
    }

    // Create test entities to verify ECS is working
    auto entityManager = ::ServiceLocator::Get<StrixVerse::ECS::EntityManager>();
    auto componentManager = ::ServiceLocator::Get<StrixVerse::ECS::ComponentManager>();
    if (entityManager && componentManager)
    {
        // Create first entity with transform and sprite
        m_TestEntity = entityManager->createEntity();

        // Add transform component
        StrixVerse::ECS::Transform transform;
        transform.position = {100.0f, 100.0f};
        transform.rotation = 0.0f;
        transform.scale = {1.0f, 1.0f};
        componentManager->addComponent<StrixVerse::ECS::Transform>(m_TestEntity, transform);

        // Add sprite component (using texture ID 0 for now - would normally load a texture)
        StrixVerse::ECS::SpriteComponent sprite;
        sprite.textureID = 0; // Placeholder
        sprite.r = 1.0f;
        sprite.g = 1.0f;
        sprite.b = 1.0f;
        sprite.a = 1.0f; // White
        sprite.layer = 0;
        componentManager->addComponent<StrixVerse::ECS::SpriteComponent>(m_TestEntity, sprite);

        // Create second entity with transform and velocity
        m_TestEntity2 = entityManager->createEntity();

        // Add transform component
        StrixVerse::ECS::Transform transform2;
        transform2.position = {200.0f, 200.0f};
        transform2.rotation = 0.0f;
        transform2.scale = {1.0f, 1.0f};
        componentManager->addComponent<StrixVerse::ECS::Transform>(m_TestEntity2, transform2);

        // Add velocity component
        StrixVerse::ECS::VelocityComponent velocity;
        velocity.vx = 50.0f; // 50 units per second to the right
        velocity.vy = 0.0f;
        componentManager->addComponent<StrixVerse::ECS::VelocityComponent>(m_TestEntity2, velocity);

        Logger::Info("Game: Created test entities for ECS verification.");
    }
    else
    {
        Logger::Error("Game: Failed to get ECS managers from service locator.");
        m_Initialized = false;
        return false;
    }

    Logger::Info("Game initialized.");

    return true;
}

bool Game::LoadScene(const std::string& sceneName)
{
    if (!m_Initialized)
    {
        Logger::Error("Game: LoadScene called before Initialize.");
        return false;
    }

    if (sceneName.empty())
    {
        Logger::Error("Game: scene name is empty.");
        return false;
    }

    if (!m_ActiveScene.empty())
        UnloadScene();

    // Future World subsystem:
    //
    // if (!SceneManager::Load(sceneName))
    //     return false;

    m_ActiveScene = sceneName;

    Logger::Info("Game: loaded scene '" + m_ActiveScene + "'.");

    return true;
}

void Game::UnloadScene()
{
    if (m_ActiveScene.empty())
        return;

    // Future World subsystem:
    //
    // SceneManager::Unload(m_ActiveScene);

    Logger::Info("Game: unloaded scene '" + m_ActiveScene + "'.");

    m_ActiveScene.clear();
}

void Game::Update(float deltaTime)
{
    if (!m_Initialized || m_ActiveScene.empty())
        return;

    (void)deltaTime;

    // Future gameplay systems:
    //
    // SceneManager::Update(deltaTime);
    // CameraController::Update(deltaTime);
}

void Game::FixedUpdate(float fixedDeltaTime)
{
    if (!m_Initialized || m_ActiveScene.empty())
        return;

    (void)fixedDeltaTime;

    // Future deterministic systems:
    //
    // Physics::Step(fixedDeltaTime);
    // Netcode::Tick(fixedDeltaTime);
}

void Game::Render()
{
    if (!m_Initialized || m_ActiveScene.empty())
        return;

    // Future rendering:
    //
    // SceneManager::Render();
}

void Game::Shutdown()
{
    if (!m_Initialized)
        return;

    // Clean up ECS entities
    auto entityManager = ::ServiceLocator::Get<StrixVerse::ECS::EntityManager>();
    if (entityManager)
    {
        if (m_TestEntity.isValid())
        {
            entityManager->destroyEntity(m_TestEntity);
            m_TestEntity = StrixVerse::ECS::Entity(); // Reset to invalid entity
        }

        if (m_TestEntity2.isValid())
        {
            entityManager->destroyEntity(m_TestEntity2);
            m_TestEntity2 = StrixVerse::ECS::Entity(); // Reset to invalid entity
        }
    }

    UnloadScene();

    m_Initialized = false;

    Logger::Info("Game shutdown.");
}

bool Game::IsInitialized() const
{
    return m_Initialized;
}

bool Game::HasActiveScene() const
{
    return !m_ActiveScene.empty();
}

const std::string& Game::GetActiveSceneName() const
{
    return m_ActiveScene;
}