#include "Application.h"

#include "ServiceLocator.h"
#include "Version.h"

Application::Application()
{
}

Application::~Application()
{
}

bool Application::Initialize()
{
    m_Logger.Initialize();

    Logger::Info("=================================");
    Logger::Info("Starting StrixVerse Client");
    Logger::Info(Version::GetFullVersionString());
    Logger::Info("=================================");

    if (!m_Config.Load())
    {
        Logger::Error("Failed to load configuration.");
        return false;
    }

    if (!m_Window.Create(
            m_Config.GetWidth(),
            m_Config.GetHeight(),
            "StrixVerse"))
    {
        Logger::Error("Failed to create window.");
        return false;
    }

    if (!m_Engine.Initialize(&m_Window, &m_Config))
    {
        Logger::Error("Failed to initialize engine.");
        return false;
    }

    Logger::Info("Client initialized.");

    return true;
}

void Application::Run()
{
    m_Engine.Run();
}

void Application::Shutdown()
{
    Logger::Info("Shutting down client...");

    m_Engine.Shutdown();

    m_Window.Destroy();

    m_Config.Save();

    // Release cross-cutting services after every system has stopped.
    ServiceLocator::Clear();

    m_Logger.Shutdown();
}