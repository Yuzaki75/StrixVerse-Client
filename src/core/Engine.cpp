#include "Engine.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <chrono>
#include <filesystem>
#include <vector>

#include "AssetManager.h"
#include "Config.h"
#include "Logger.h"
#include "ServiceLocator.h"
#include "Timer.h"
#include "Window.h"

#include "../graphics/Renderer.h"
#include "../graphics/UIRenderer.h"
#include "../ui/UITheme.h"

#include "screens/SplashScreen.h"

// Component and System includes for ECS setup
#include "ecs/Camera2DComponent.h"
#include "ecs/Camera2DSystem.h"
#include "ecs/CharacterComponent.h"
#include "ecs/ColliderComponent.h"
#include "ecs/CollisionSystem.h"
#include "ecs/InputComponent.h"
#include "ecs/InputSystem.h"
#include "ecs/MovementSystem.h"
#include "ecs/NetworkComponent.h"
#include "ecs/NetworkSyncSystem.h"
#include "ecs/PlayerComponent.h"
#include "ecs/PlayerSystem.h"
#include "ecs/RenderSystem.h"
#include "ecs/CharacterRenderSystem.h"
#include "ecs/SpriteComponent.h"
#include "ecs/SystemManager.h"
#include "ecs/TileRendererSystem.h"
#include "ecs/TransformComponent.h"
#include "ecs/VelocityComponent.h"

#include <SDL3/SDL.h>

#include <algorithm>

namespace
{
    // Most fixed simulation steps run for a single rendered frame. Timer
    // already clamps a frame to 0.25s, so this only bites after a stall long
    // enough that catching up would be worse than skipping.
    constexpr int kMaxFixedStepsPerFrame = 8;

    // Translates SDL key codes into the UI's own key enum so nothing under
    // ui/ or screens/ has to include SDL.
    int TranslateKey(SDL_Keycode key)
    {
        switch (key)
        {
        case SDLK_BACKSPACE: return UIKey::Backspace;
        case SDLK_DELETE:    return UIKey::Delete;
        case SDLK_LEFT:      return UIKey::Left;
        case SDLK_RIGHT:     return UIKey::Right;
        case SDLK_UP:        return UIKey::Up;
        case SDLK_DOWN:      return UIKey::Down;
        case SDLK_HOME:      return UIKey::Home;
        case SDLK_END:       return UIKey::End;
        case SDLK_TAB:       return UIKey::Tab;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:  return UIKey::Enter;
        case SDLK_ESCAPE:    return UIKey::Escape;
        case SDLK_0:
        case SDLK_KP_0:      return UIKey::Digit0;
        case SDLK_1:
        case SDLK_KP_1:      return UIKey::Digit1;
        case SDLK_2:
        case SDLK_KP_2:      return UIKey::Digit2;
        case SDLK_3:
        case SDLK_KP_3:      return UIKey::Digit3;
        case SDLK_4:
        case SDLK_KP_4:      return UIKey::Digit4;
        case SDLK_5:
        case SDLK_KP_5:      return UIKey::Digit5;
        case SDLK_6:
        case SDLK_KP_6:      return UIKey::Digit6;
        case SDLK_7:
        case SDLK_KP_7:      return UIKey::Digit7;
        case SDLK_8:
        case SDLK_KP_8:      return UIKey::Digit8;
        case SDLK_9:
        case SDLK_KP_9:      return UIKey::Digit9;
        default:             return UIKey::None;
        }
    }
}

Engine::Engine() = default;

Engine::~Engine()
{
    // RAII safety net for early exits.
    Shutdown();
}

bool Engine::Initialize(Window* window, Config* config)
{
    if (m_State != EngineState::Uninitialized)
    {
        Logger::Warning("Engine: Initialize called twice.");
        return m_State == EngineState::Initialized;
    }

    if (!window)
    {
        Logger::Error("Engine: window is null.");
        return false;
    }

    m_Window = window;
    m_Config = config;

    // The device itself is not opened until a screen asks for a track.
    m_Audio.SetMusicVolume(m_Config
                               ? static_cast<float>(m_Config->GetMusicVolume()) / 100.0f
                               : 0.7f);
    m_Audio.SetSfxVolume(m_Config ? m_Config->GetSfxVolume() : 1.0f);

    Timer::Initialize();

    // Initialize the renderer after the OpenGL context is current.
    if (!Renderer::Initialize())
    {
        Logger::Error("Engine: failed to initialize renderer.");
        return false;
    }

    // Register the engine itself for systems that need to reach back into it.
    ServiceLocator::Provide(std::shared_ptr<Engine>(this, [](Engine*) {}));

    m_AssetManager = std::make_shared<AssetManager>();
    ServiceLocator::Provide(m_AssetManager);

    m_SpriteBatch = std::make_shared<SpriteBatch>();
    ServiceLocator::Provide(m_SpriteBatch);

    m_UIRenderer = std::make_shared<UIRenderer>();
    if (!m_UIRenderer->Initialize(*m_AssetManager))
    {
        Logger::Error("Engine: failed to initialize the UI renderer.");
        return false;
    }
    ServiceLocator::Provide(m_UIRenderer);

    // Rasterise the design's typefaces. A missing font file is logged and the
    // client still starts, just without text in that role.
    m_UIFonts = std::make_shared<UIFonts>();
    m_UIFonts->Initialize(*m_AssetManager);
    ServiceLocator::Provide(m_UIFonts);

    // Establish the canvas mapping before any screen lays itself out.
    HandleResize();

    // Initialize ECS Managers
    m_pEntityManager = std::make_shared<StrixVerse::ECS::EntityManager>();
    m_pComponentManager = std::make_shared<StrixVerse::ECS::ComponentManager>(m_pEntityManager->MAX_ENTITIES);
    m_pSystemManager = std::make_shared<StrixVerse::ECS::SystemManager>(m_pEntityManager.get(), m_pComponentManager.get());

    // Wire up the ECS notification chain
    m_pEntityManager->SetComponentManager(m_pComponentManager.get());
    m_pEntityManager->SetSystemManager(m_pSystemManager.get());
    m_pComponentManager->SetSystemManager(m_pSystemManager.get());

    ServiceLocator::Provide(m_pEntityManager);
    ServiceLocator::Provide(m_pComponentManager);
    ServiceLocator::Provide(m_pSystemManager);

    // Register essential components
    m_pComponentManager->registerComponent<StrixVerse::ECS::Transform>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::SpriteComponent>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::VelocityComponent>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::InputComponent>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::NetworkComponent>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::PlayerComponent>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::Camera2DComponent>();
    m_pComponentManager->registerComponent<StrixVerse::ECS::ColliderComponent>();

    // CharacterRenderSystem is created below and GameScreen gives every player
    // a CharacterComponent, but the type itself was never registered here.
    // ComponentManager::addComponent asserts that the type it is handed has a
    // recorded size, and an unregistered type records zero - so entering any
    // world aborted the Debug build on the frame the player was created. The
    // Release build survived it only because addComponent creates the storage
    // lazily at the correct size, which is luck rather than intent.
    m_pComponentManager->registerComponent<StrixVerse::ECS::CharacterComponent>();

    // Systems run in registration order, for both update and render, so this
    // single order has to satisfy both:
    //
    //   update: sample input -> turn it into velocity -> integrate -> follow
    //   render: tiles first, then the entities standing on them
    //
    // Each system declares its own signature in init(), so none is set here.
    auto inputSystem = m_pSystemManager->createSystem<StrixVerse::ECS::InputSystem>();
    m_pSystemManager->addSystem(inputSystem);

    auto playerSystem = m_pSystemManager->createSystem<StrixVerse::ECS::PlayerSystem>();
    m_pSystemManager->addSystem(playerSystem);

    // Collision clamps the velocity before MovementSystem integrates it.
    auto collisionSystem = m_pSystemManager->createSystem<StrixVerse::ECS::CollisionSystem>();
    m_pSystemManager->addSystem(collisionSystem);

    auto movementSystem = m_pSystemManager->createSystem<StrixVerse::ECS::MovementSystem>();
    m_pSystemManager->addSystem(movementSystem);

    // Remote entities are driven by the network rather than by input, so their
    // transforms are refreshed after local integration and before the camera
    // and renderer read them.
    auto networkSyncSystem = m_pSystemManager->createSystem<StrixVerse::ECS::NetworkSyncSystem>();
    m_pSystemManager->addSystem(networkSyncSystem);

    auto cameraSystem = m_pSystemManager->createSystem<StrixVerse::ECS::Camera2DSystem>();
    m_pSystemManager->addSystem(cameraSystem);

    auto tileRendererSystem = m_pSystemManager->createSystem<StrixVerse::ECS::TileRendererSystem>();
    m_pSystemManager->addSystem(tileRendererSystem);

    auto renderSystem = m_pSystemManager->createSystem<StrixVerse::ECS::RenderSystem>();
    m_pSystemManager->addSystem(renderSystem);

    // After RenderSystem, so characters draw over ordinary sprites. Players
    // carry a CharacterComponent instead of a SpriteComponent, so the two
    // systems never draw the same entity twice.
    auto characterRenderSystem =
        m_pSystemManager->createSystem<StrixVerse::ECS::CharacterRenderSystem>();

        // Cheap, and it runs before anything can depend on the answer.
        StrixVerse::ECS::Camera2DSystem::SelfTest();
    m_pSystemManager->addSystem(characterRenderSystem);

    m_UIManager = std::make_shared<UIManager>();
    ServiceLocator::Provide(m_UIManager);

    m_AuthService = std::make_unique<AuthService>();
    m_WorldManager = std::make_unique<WorldManager>();

    if (!m_NetworkManager.initialize())
    {
        Logger::Error("Engine: failed to initialize network manager.");
        return false;
    }

    // Authentication runs over the session once one exists. In offline mode it
    // resolves locally instead - an explicit choice, never a silent fallback.
    m_AuthService->SetNetworkManager(&m_NetworkManager);
    m_AuthService->SetOfflineMode(IsOfflineMode());

    if (IsOfflineMode())
        Logger::Warning("Engine: offline mode is enabled; the server will not be contacted.");

    // Start on the splash screen.
    m_CurrentScreen = std::make_unique<SplashScreen>(this);
    m_CurrentScreen->OnEnter();

    m_State = EngineState::Initialized;

    Logger::Info("Engine initialized.");

    return true;
}

bool Engine::IsOfflineMode() const
{
    return m_Config != nullptr && m_Config->IsOfflineMode();
}

bool Engine::ConnectToServer()
{
    if (IsOfflineMode())
        return false;

    if (m_NetworkManager.isConnected())
        return true;

    const std::string host = m_Config ? m_Config->GetServerHost() : std::string("127.0.0.1");
    const uint16_t    port = static_cast<uint16_t>(m_Config ? m_Config->GetServerPort() : 17091);

    Logger::Info(std::format("Engine: connecting to {}:{}...", host, port));

    return m_NetworkManager.connect(host, port);
}

bool Engine::BeginConnectToServer()
{
    if (IsOfflineMode())
        return false;

    if (m_NetworkManager.isConnected())
        return true;

    const std::string host = m_Config ? m_Config->GetServerHost() : std::string("127.0.0.1");
    const uint16_t    port = static_cast<uint16_t>(m_Config ? m_Config->GetServerPort() : 17091);

    Logger::Info(std::format("Engine: connecting to {}:{}...", host, port));

    return m_NetworkManager.beginConnect(host, port);
}

Engine::ConnectProgress Engine::PollConnectToServer()
{
    if (m_NetworkManager.isConnected())
        return ConnectProgress::Connected;

    return m_NetworkManager.pollConnect();
}

void Engine::Run()
{
    if (m_State != EngineState::Initialized)
    {
        Logger::Error("Engine: Cannot run engine - not initialized or already running/shutdown.");
        return;
    }

    m_State = EngineState::Running;
    Logger::Info("Engine: Starting main loop");

    while (m_State == EngineState::Running)
    {
        // Real frame time drives the UI, animations and transitions; the ECS
        // keeps its own fixed step for deterministic simulation.
        const float deltaTime = std::min(Timer::GetDeltaTime(), 0.1f);

        ProcessEvents();
        PollScreenshotKey();
        Update(deltaTime);
        Render();

        Timer::Update();
    }

    Logger::Info("Engine: Main loop ended");
}

void Engine::HandleResize()
{
    if (!m_Window)
        return;

    int width = 0;
    int height = 0;
    m_Window->GetSize(width, height);

    if (width <= 0 || height <= 0)
        return;

    Renderer::SetViewport(width, height);

    m_UIScale.Update(width, height);

    // The camera owns the world projection, so it needs the new viewport; the
    // SpriteBatch picks the matrix up per frame in Render().
    m_Camera.SetViewport(static_cast<float>(width), static_cast<float>(height));
}

void Engine::ProcessEvents()
{
    if (!m_Window)
        return;

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            Stop();
            return;

        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            HandleResize();
            break;

        case SDL_EVENT_MOUSE_MOTION:
        {
            const glm::vec2 canvas = m_UIScale.ToCanvas(event.motion.x, event.motion.y);
            if (m_UIManager)
                m_UIManager->handleMouseMove(canvas.x, canvas.y);
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            // Left and right both reach the screen; anything else is ignored.
            // Right-click used to be dropped here, which is why placing a
            // block sent nothing at all -- the screen never heard about it.
            const bool isLeft  = (event.button.button == SDL_BUTTON_LEFT);
            const bool isRight = (event.button.button == SDL_BUTTON_RIGHT);
            if (!isLeft && !isRight)
                break;

            const glm::vec2 canvas = m_UIScale.ToCanvas(event.button.x, event.button.y);

            // The UI is left-click only. Feeding it right-clicks would let a
            // right-click press buttons, which no part of the design expects.
            if (isLeft && m_UIManager)
                m_UIManager->handleMouseDown(canvas.x, canvas.y);

            // A click that landed on the UI is not also a click on the world.
            // Both dispatches used to run for every press, so pressing a hotbar
            // slot selected the item *and* swung the tool at the tile behind
            // the HUD - a hidden block break, and the reason a wrench click on
            // the hotbar was logged as an interaction with a distant tile.
            //
            // Tested against the point rather than against whether the UI acted
            // on it, because the UI takes only left clicks: a right click over
            // the chat panel is still not a right click on the world.
            const bool overUI = m_UIManager && m_UIManager->isPointOverElement(canvas.x, canvas.y);

            // Routed by the button that actually fired the event, so the
            // screen never has to guess which one it was.
            if (m_CurrentScreen && (!overUI || m_CurrentScreen->WantsRawInput()))
            {
                if (isLeft)
                    m_CurrentScreen->OnMouseDown(canvas.x, canvas.y);
                else
                    m_CurrentScreen->OnRightMouseDown(canvas.x, canvas.y);
            }
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            if (event.button.button != SDL_BUTTON_LEFT)
                break;

            const glm::vec2 canvas = m_UIScale.ToCanvas(event.button.x, event.button.y);
            if (m_UIManager)
                m_UIManager->handleMouseUp(canvas.x, canvas.y);
            break;
        }

        case SDL_EVENT_MOUSE_WHEEL:
        {
            float mouseX = 0.0f;
            float mouseY = 0.0f;
            SDL_GetMouseState(&mouseX, &mouseY);

            const glm::vec2 canvas = m_UIScale.ToCanvas(mouseX, mouseY);

            // The UI gets the wheel when the pointer is over it, and the screen
            // gets it otherwise. Splitting on position rather than on whether
            // the UI did anything with it keeps scrolling the chat log from
            // also zooming the world behind it.
            if (m_UIManager && m_UIManager->isPointOverElement(canvas.x, canvas.y))
            {
                m_UIManager->handleScroll(canvas.x, canvas.y, event.wheel.y);
            }

            // Hotbar cycling lives on the gameplay screen, not on a widget;
            // zoom rides along there too, behind a shift/ctrl modifier.
            const bool typing = m_UIManager && m_UIManager->getFocusedElement() != nullptr;
            if (m_CurrentScreen && !typing)
                m_CurrentScreen->OnMouseWheel(canvas.x, canvas.y, event.wheel.y);
            break;
        }

        case SDL_EVENT_TEXT_INPUT:
        {
            // Real text arrives here as UTF-8, already through the platform's
            // keyboard layout and any IME.
            if (m_UIManager && event.text.text)
                m_UIManager->handleTextInput(event.text.text);
            break;
        }

        case SDL_EVENT_KEY_DOWN:
        {
            const SDL_Keymod mods  = event.key.mod;
            const bool       ctrl  = (mods & SDL_KMOD_CTRL) != 0;
            const bool       shift = (mods & SDL_KMOD_SHIFT) != 0;
            const int        key   = TranslateKey(event.key.key);

            const bool uiConsumes = m_UIManager && m_UIManager->isTextInputFocused();

            if (key != UIKey::None && m_UIManager)
                m_UIManager->handleKeyDown(key, ctrl, shift);

            // Screen-level handlers run when nothing has keyboard focus, so a
            // "press any key" screen never eats a keystroke meant for a field.
            if (m_CurrentScreen && (!uiConsumes || m_CurrentScreen->WantsRawInput()))
                m_CurrentScreen->OnKeyDown(key, ctrl, shift);
            break;
        }

        default:
            break;
        }
    }

    // Keep the platform's text input service in step with UI focus.
    if (m_UIManager && m_Window && m_Window->GetSDLWindow())
    {
        const bool wanted = m_UIManager->isTextInputActive();
        const bool active = SDL_TextInputActive(m_Window->GetSDLWindow());

        if (wanted && !active)
            SDL_StartTextInput(m_Window->GetSDLWindow());
        else if (!wanted && active)
            SDL_StopTextInput(m_Window->GetSDLWindow());
    }
}

void Engine::RequestScreenChange(ScreenID id)
{
    if (m_TransitionState != TransitionState::None)
        return;

    m_NextScreen     = id;
    m_HasNextScreen  = true;
    m_TransitionState = TransitionState::FadingOut;
    m_TransitionTimer = 0.0f;
}

void Engine::SwitchScreen(ScreenID id)
{
    if (m_CurrentScreen)
    {
        // Remember where we came from, so a screen reachable from more than
        // one place knows where Back should go.
        m_PreviousScreenId = m_CurrentScreenId;

        m_CurrentScreen->OnExit();
        m_CurrentScreen.reset();
    }

    m_CurrentScreenId = id;

    // Belt and braces: even if a screen forgets to tear down an element, no
    // UI state survives into the next screen.
    if (m_UIManager)
        m_UIManager->clearAllElements();

    m_CurrentScreen = ScreenFactory::CreateScreen(id, this);

    if (!m_CurrentScreen)
    {
        Logger::Error("Engine: ScreenFactory returned no screen; falling back to Splash.");
        m_CurrentScreen = ScreenFactory::CreateScreen(ScreenID::Splash, this);
    }

    if (m_CurrentScreen)
        m_CurrentScreen->OnEnter();
}

void Engine::UpdateTransition(float deltaTime)
{
    // A screen asking to move on starts the fade.
    if (m_TransitionState == TransitionState::None &&
        m_CurrentScreen && m_CurrentScreen->HasPendingChange())
    {
        m_NextScreen = m_CurrentScreen->GetPendingChange();
        m_CurrentScreen->ClearPendingChange();

        m_HasNextScreen   = true;
        m_TransitionState = TransitionState::FadingOut;
        m_TransitionTimer = 0.0f;
    }

    switch (m_TransitionState)
    {
    case TransitionState::None:
        m_FadeAlpha = 0.0f;
        break;

    case TransitionState::FadingOut:
        m_TransitionTimer += deltaTime;
        m_FadeAlpha = std::clamp(m_TransitionTimer / m_TransitionLength, 0.0f, 1.0f);

        if (m_TransitionTimer >= m_TransitionLength)
        {
            m_FadeAlpha = 1.0f;

            if (m_HasNextScreen)
                SwitchScreen(m_NextScreen);

            m_HasNextScreen   = false;
            m_TransitionState = TransitionState::FadingIn;
            m_TransitionTimer = 0.0f;
        }
        break;

    case TransitionState::FadingIn:
        m_TransitionTimer += deltaTime;
        m_FadeAlpha = 1.0f - std::clamp(m_TransitionTimer / m_TransitionLength, 0.0f, 1.0f);

        if (m_TransitionTimer >= m_TransitionLength)
        {
            m_FadeAlpha       = 0.0f;
            m_TransitionState = TransitionState::None;
            m_TransitionTimer = 0.0f;
        }
        break;
    }
}

void Engine::Update(float deltaTime)
{
    if (m_State != EngineState::Running)
        return;

    if (m_pSystemManager)
    {
        // Drain the accumulated frame time a fixed step at a time.
        //
        // This used to run the ECS exactly once per rendered frame, and pass
        // it the fixed timestep as the elapsed time. That is not a fixed
        // timestep - it is a variable one wearing a constant's clothes, and it
        // ties simulation speed directly to frame rate. At 60fps the lie holds
        // because a frame really is 1/60s. Measured here, uncapped, the client
        // runs at 500-900fps with the world on screen, so it was advancing the
        // simulation by a second of game time for every 60 frames and burning
        // through those in a tenth of a second: everything that moved ran an
        // order of magnitude fast, and would run at a different speed again on
        // every machine it was played on.
        //
        // Timer has kept the accumulator and offered ConsumeFixedStep for
        // exactly this since it was written. Neither was ever called.
        const float step = Timer::GetFixedTimestep();

        int steps = 0;
        while (steps < kMaxFixedStepsPerFrame && Timer::ConsumeFixedStep())
        {
            m_pSystemManager->update(step);
            ++steps;
        }

        if (steps == kMaxFixedStepsPerFrame)
            Timer::DiscardOwedFixedSteps();
    }

    // Keeps the music buffer topped up and loops it at the end of the track.
    m_Audio.Update();

    m_NetworkManager.update(deltaTime);

    if (m_UIManager)
        m_UIManager->update(deltaTime);

    if (m_CurrentScreen)
        m_CurrentScreen->Update(deltaTime);

    UpdateTransition(deltaTime);
}

void Engine::Render()
{
    if (m_State != EngineState::Running)
        return;

    Renderer::SetClearColor(UITheme::Background);
    Renderer::BeginFrame();

    // World layer. The camera is followed by Camera2DSystem during update, so
    // its matrix is refreshed here, immediately before anything uses it.
    // Without this the world drew through a fixed window-pixel projection and
    // the camera had no effect on screen at all.
    if (m_SpriteBatch)
        m_SpriteBatch->SetProjection(m_Camera.GetViewProjectionMatrix());

    if (m_CurrentScreen)
        m_CurrentScreen->RenderBackground();

    if (m_pSystemManager)
        m_pSystemManager->render();

    if (m_CurrentScreen)
        m_CurrentScreen->RenderGame();

    // UI layer, drawn in one batched pass over the design canvas.
    if (m_UIRenderer && m_UIRenderer->IsReady())
    {
        m_UIRenderer->Begin(m_UIScale.GetVisibleCanvas(),
                            m_UIScale.GetFramebufferWidth(),
                            m_UIScale.GetFramebufferHeight());

        if (m_UIManager)
            m_UIManager->render(*m_UIRenderer);

        // The transition fade is drawn last so it covers the whole screen.
        if (m_FadeAlpha > 0.0f)
        {
            m_UIRenderer->SetGlobalOpacity(1.0f);
            m_UIRenderer->DrawRect(m_UIScale.GetVisibleLeft(),
                                   m_UIScale.GetVisibleTop(),
                                   m_UIScale.GetVisibleWidth(),
                                   m_UIScale.GetVisibleHeight(),
                                   UIQuadStyle::Solid(Color(0.0f, 0.0f, 0.0f, m_FadeAlpha)));
        }

        m_UIRenderer->End();
    }

    // Before the swap: this is the only moment the finished frame is still
    // readable. After EndFrame the back buffer's contents are undefined.
    if (m_ScreenshotRequested)
    {
        m_ScreenshotRequested = false;
        CaptureScreenshot();
    }

    m_Window->EndFrame();
}

void Engine::PollScreenshotKey()
{
    const bool* keys = SDL_GetKeyboardState(nullptr);
    if (!keys)
        return;

    const bool down = keys[SDL_SCANCODE_F12] != 0;

    // Edge-detected. A held key would otherwise write one file per frame, and
    // at sixty frames a second that fills a directory before you let go.
    if (down && !m_PrevScreenshotKey)
        m_ScreenshotRequested = true;

    m_PrevScreenshotKey = down;
}

void Engine::CaptureScreenshot()
{
    const int width  = m_UIScale.GetFramebufferWidth();
    const int height = m_UIScale.GetFramebufferHeight();

    if (width <= 0 || height <= 0)
    {
        Logger::Warning("Engine: cannot screenshot a zero-sized framebuffer");
        return;
    }

    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4);

    // Tightly packed rows. The default is 4-byte alignment, which silently
    // shears the image whenever the row length is not a multiple of four -- a
    // 1279-pixel-wide window would come out skewed rather than failing.
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL's origin is bottom-left and every image format here is top-down,
    // so the rows come back upside down. The same trap the asset loader
    // documents against stbi_set_flip_vertically_on_load, in the other
    // direction.
    const std::size_t stride = static_cast<std::size_t>(width) * 4;
    std::vector<unsigned char> row(stride);
    for (int y = 0; y < height / 2; ++y)
    {
        unsigned char* top    = pixels.data() + static_cast<std::size_t>(y) * stride;
        unsigned char* bottom = pixels.data() +
                                static_cast<std::size_t>(height - 1 - y) * stride;
        std::copy(top, top + stride, row.begin());
        std::copy(bottom, bottom + stride, top);
        std::copy(row.begin(), row.end(), bottom);
    }

    std::error_code ec;
    std::filesystem::create_directories("screenshots", ec);

    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif

    char stamp[32] = {};
    std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &tm);

    const std::string path = std::string("screenshots/strixverse-") + stamp + ".png";

    if (stbi_write_png(path.c_str(), width, height, 4, pixels.data(),
                       static_cast<int>(stride)) == 0)
    {
        Logger::Error("Engine: failed to write " + path);
        return;
    }

    // Absolute, because the working directory is not obvious from a shortcut
    // and a screenshot nobody can find is not a screenshot.
    Logger::Info("Engine: screenshot written to " +
                 std::filesystem::absolute(path, ec).string());
}

void Engine::Shutdown()
{
    if (m_State == EngineState::Shutdown)
        return;

    Logger::Info("Engine: Shutting down");

    if (m_State == EngineState::Running)
        Stop();

    // Silence before the screens go, so nothing keeps playing over teardown.
    m_Audio.Shutdown();

    if (m_CurrentScreen)
    {
        m_CurrentScreen->OnExit();
        m_CurrentScreen.reset();
    }

    if (m_UIManager)
        m_UIManager->clearAllElements();
    m_UIManager.reset();

    m_pSystemManager.reset();
    m_pComponentManager.reset();
    m_pEntityManager.reset();

    // The UI renderer and fonts own GL objects, so they must go before the
    // asset manager drops the shaders and atlases they reference.
    m_UIFonts.reset();

    if (m_UIRenderer)
        m_UIRenderer->Shutdown();
    m_UIRenderer.reset();

    m_SpriteBatch.reset();

    m_AssetManager.reset();

    m_WorldManager.reset();
    m_AuthService.reset();

    Renderer::Shutdown();

    m_State = EngineState::Shutdown;
    Logger::Info("Engine: Shutdown complete");
}

void Engine::Stop()
{
    if (m_State != EngineState::Running)
        return;

    Logger::Info("Engine: Stopping");
    m_State = EngineState::Stopped;
}

Engine::EngineState Engine::GetState() const
{
    return m_State;
}

bool Engine::IsRunning() const
{
    return m_State == EngineState::Running;
}
