#pragma once

#include "Config.h"
#include "Engine.h"
#include "Logger.h"
#include "Window.h"

class Application
{
public:
    Application();
    ~Application();

    bool Initialize();

    void Run();

    void Shutdown();

private:
    Logger m_Logger;
    Config m_Config;
    Window m_Window;
    Engine m_Engine;
    
};