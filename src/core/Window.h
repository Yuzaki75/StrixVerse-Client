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

    // Get the window size.
    void GetSize(int& width, int& height) const;

    // Set the window size (and update viewport if needed).
    void SetSize(int width, int height);

private:
    SDL_Window* m_Window = nullptr;
    SDL_GLContext m_GLContext = nullptr;
    bool m_ShouldClose = false;
};