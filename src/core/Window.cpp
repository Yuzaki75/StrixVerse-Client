#include "Window.h"

#include "Logger.h"

Window::Window()
{
    m_Window = nullptr;
    m_ShouldClose = false;
}

Window::~Window()
{
}

bool Window::Create(
    int width,
    int height,
    const std::string& title)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        Logger::Error(
            SDL_GetError());

        return false;
    }

    m_Window =
        SDL_CreateWindow(
            title.c_str(),
            width,
            height,
            SDL_WINDOW_RESIZABLE);

    if (!m_Window)
    {
        Logger::Error(
            SDL_GetError());

        return false;
    }

    Logger::Info(
        "Window created.");

    return true;
}

void Window::Destroy()
{
    if (m_Window)
    {
        SDL_DestroyWindow(
            m_Window);

        m_Window = nullptr;
    }

    SDL_Quit();
}

void Window::PollEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type ==
            SDL_EVENT_QUIT)
        {
            m_ShouldClose = true;
        }
    }
}

bool Window::ShouldClose() const
{
    return m_ShouldClose;
}

void Window::BeginFrame()
{
}

void Window::EndFrame()
{
}