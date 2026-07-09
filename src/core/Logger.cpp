#include "Logger.h"

#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

std::ofstream Logger::s_LogFile;
std::mutex Logger::s_Mutex;

Logger::Logger()
{
}

Logger::~Logger()
{
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
    Write("INFO", message);
}

void Logger::Warning(const std::string& message)
{
    Write("WARNING", message);
}

void Logger::Error(const std::string& message)
{
    Write("ERROR", message);
}

void Logger::Debug(const std::string& message)
{
    Write("DEBUG", message);
}

void Logger::Write(
    const std::string& level,
    const std::string& message)
{
    std::lock_guard<std::mutex> lock(s_Mutex);

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
       << std::put_time(&localTime, "%H:%M:%S")
       << "] "
       << "["
       << level
       << "] "
       << message;

    std::cout << ss.str() << std::endl;

    if (s_LogFile.is_open())
    {
        s_LogFile << ss.str() << std::endl;
    }
}