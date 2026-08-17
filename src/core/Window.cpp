#include "Window.h"

#include "Logger.h"
#include <glad/glad.h>

Window::Window()
{
    m_Window = nullptr;
    m_GLContext = nullptr;
    m_ShouldClose = false;
}

Window::~Window()
{
    // RAII safety net: guarantees the context is destroyed and SDL_Quit is called
    // even if Shutdown is skipped.
    Destroy();
}

bool Window::Create(
    int width,
    int height,
    const std::string& title)
{
    // Audio is initialised here alongside video so AudioManager can open a
    // device; SDL requires the subsystem before any audio call.
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        Logger::Error(SDL_GetError());
        return false;
    }

    // Request an OpenGL 4.6 core profile context.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    m_Window =
        SDL_CreateWindow(
            title.c_str(),
            width,
            height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!m_Window)
    {
        Logger::Error(SDL_GetError());
        return false;
    }

    // Create OpenGL context and make it current.
    m_GLContext = SDL_GL_CreateContext(m_Window);
    if (!m_GLContext)
    {
        Logger::Error(SDL_GetError());
        return false;
    }

    int result = SDL_GL_MakeCurrent(m_Window, m_GLContext);
    if (result < 0)
    {
        Logger::Error(SDL_GetError());
        return false;
    }

    // Load OpenGL functions via glad.
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        Logger::Error("Failed to initialize GLAD");
        return false;
    }

    // Set the viewport to match the window size.
    int w = 0, h = 0;
    SDL_GetWindowSize(m_Window, &w, &h);
    glViewport(0, 0, w, h);

    Logger::Info("Window created with OpenGL context.");

    return true;
}

void Window::Destroy()
{
    if (m_GLContext)
    {
        SDL_GL_DestroyContext(m_GLContext);
        m_GLContext = nullptr;
    }

    if (m_Window)
    {
        SDL_DestroyWindow(m_Window);
        m_Window = nullptr;
    }

    SDL_Quit();
}

void Window::PollEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            m_ShouldClose = true;
        }
        else if (event.type == SDL_EVENT_WINDOW_RESIZED)
        {
            // Update the viewport to match the new window size.
            int width = 0, height = 0;
            SDL_GetWindowSize(m_Window, &width, &height);
            glViewport(0, 0, width, height);
        }
    }
}

bool Window::ShouldClose() const
{
    return m_ShouldClose;
}

void Window::BeginFrame()
{
    // Intentionally left empty.
    // The Renderer::BeginFrame will clear the buffer.
}

void Window::EndFrame()
{
    SDL_GL_SwapWindow(m_Window);
}

void Window::GetSize(int& width, int& height) const
{
    SDL_GetWindowSize(m_Window, &width, &height);
}

void Window::SetSize(int width, int height)
{
    SDL_SetWindowSize(m_Window, width, height);
    // Update the viewport to match the new size.
    int w = 0, h = 0;
    SDL_GetWindowSize(m_Window, &w, &h);
    glViewport(0, 0, w, h);
}