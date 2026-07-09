#pragma once

#include "Color.h"

class Renderer
{
public:
    static bool Initialize();

    static void Shutdown();

    static void BeginFrame();

    static void EndFrame();

    static void SetClearColor(
        const Color& color);

    static void SetViewport(
        int width,
        int height);

private:
    static Color s_ClearColor;
};