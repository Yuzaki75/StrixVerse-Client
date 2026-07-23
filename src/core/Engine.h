#include <memory>

class Game;
class Window;
class AssetManager;

// Lifecycle states of the engine. Transitions are strictly forward except
// Running <-> Stopped (Stop() may be called from event handling).
enum class EngineState
{
    Uninitialized,
    Initialized,
    Running,
    Stopped,
    Shutdown
};

// -----------------------------------------------------------------------------
// Engine
//
// Purpose:
//   Owns the main loop and the frame structure (events -> fixed updates ->
//   update -> render) and drives the Game layer. It borrows the Window
//   (non-owning pointer, the Application owns it) and owns the Game.
// -----------------------------------------------------------------------------
class Engine
{
public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool Initialize(Window* window);

    void Run();

    void Shutdown();

    void Stop();

    EngineState GetState() const;
    bool IsRunning() const;

private:
    void ProcessEvents();
    void Update();
    void Render();

private:
    Window* m_Window = nullptr;
    std::unique_ptr<Game> m_Game;
    std::shared_ptr<AssetManager> m_AssetManager;
    EngineState m_State = EngineState::Uninitialized;
};