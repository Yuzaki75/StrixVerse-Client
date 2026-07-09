#include "Logger.h"

#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

std::ofstream Logger::s_LogFile;
std::mutex Logger::s_Mutex;
LogLevel Logger::s_Level = LogLevel::Debug;

Logger::Logger()
{
}

Logger::~Logger()
{
    // RAII safety net: guarantees the file is flushed and closed even when
    // Shutdown() is skipped (early return, exception during init, ...).
    Shutdown();
}

bool Logger::Initialize()
{
    std::filesystem::create_directories("logs");

    s_LogFile.open(
        "logs/client.log",
        std::ios::out | std::ios::app);

    return s_LogFile.is_open();
}

void Logger::Shutdown()
{
    if (s_LogFile.is_open())
    {
        s_LogFile.close();
    }
}

void Logger::Info(const std::string& message)
{
    Write(LogLevel::Info, "INFO", message);
}

void Logger::Warning(const std::string& message)
{
    Write(LogLevel::Warning, "WARNING", message);
}

void Logger::Error(const std::string& message)
{
    Write(LogLevel::Error, "ERROR", message);
}

void Logger::Debug(const std::string& message)
{
    Write(LogLevel::Debug, "DEBUG", message);
}

void Logger::SetLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    s_Level = level;
}

LogLevel Logger::GetLevel()
{
    std::lock_guard<std::mutex> lock(s_Mutex);
    return s_Level;
}

void Logger::Write(
    LogLevel level,
    const std::string& levelName,
    const std::string& message)
{
    std::lock_guard<std::mutex> lock(s_Mutex);

    if (level < s_Level)
        return;

    auto now =
        std::chrono::system_clock::now();

    auto time =
        std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};

#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif

    std::stringstream ss;

    ss << "["
       << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S")
       << "] "
       << "["
       << levelName
       << "] "
       << message;

    std::cout << ss.str() << std::endl;

    if (s_LogFile.is_open())
    {
        s_LogFile << ss.str() << std::endl;
    }
}