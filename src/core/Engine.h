#pragma once

class Window;

class Engine
{
public:
    Engine();
    ~Engine();

    bool Initialize(Window* window);

    void Run();

    void Shutdown();

    void Stop();

private:
    void ProcessEvents();
    void Update();
    void Render();

private:
    Window* m_Window = nullptr;
    bool m_Running = false;
};