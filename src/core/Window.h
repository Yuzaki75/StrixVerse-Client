#pragma once

#include <SDL3/SDL.h>
#include <string>

class Window
{
public:
    Window();
    ~Window();

    bool Create(
        int width,
        int height,
        const std::string& title);

    void Destroy();

    void PollEvents();

    bool ShouldClose() const;

    void BeginFrame();

    void EndFrame();

private:
    SDL_Window* m_Window = nullptr;
    bool m_ShouldClose = false;
};