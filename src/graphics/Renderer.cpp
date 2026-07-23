#include "Renderer.h"

#include <glad/glad.h>

Color Renderer::s_ClearColor = Color(0.1f, 0.1f, 0.15f, 1.0f);

bool Renderer::Initialize()
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // The viewport is set by the Window when it is created or resized.
    return true;
}

void Renderer::Shutdown()
{
}

void Renderer::BeginFrame()
{
    glClearColor(
        s_ClearColor.r,
        s_ClearColor.g,
        s_ClearColor.b,
        s_ClearColor.a);

    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::EndFrame()
{
}

void Renderer::SetClearColor(const Color& color)
{
    s_ClearColor = color;
}

void Renderer::SetViewport(int width, int height)
{
    glViewport(0, 0, width, height);
}