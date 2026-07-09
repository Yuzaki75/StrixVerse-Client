#pragma once

#include "Config.h"
#include "Engine.h"
#include "Logger.h"
#include "Window.h"

// -----------------------------------------------------------------------------
// Application
//
// Purpose:
//   Composition root of the client. Owns every top-level system by value
//   (RAII: construction/destruction order is the declaration order below,
//   so the Logger outlives everything that logs) and drives the lifecycle:
//   Initialize() -> Run() -> Shutdown().
// -----------------------------------------------------------------------------
class Application
{
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Initializes all systems. Returns false on the first failure; systems
    // already initialized are cleaned up by Shutdown()/destructors.
    bool Initialize();

    // Blocks inside the engine main loop until the engine stops.
    void Run();

    // Shuts systems down in reverse initialization order. Safe to call
    // after a failed Initialize().
    void Shutdown();

private:
    // Declaration order == initialization order (Logger must come first).
    Logger m_Logger;
    Config m_Config;
    Window m_Window;
    Engine m_Engine;
};