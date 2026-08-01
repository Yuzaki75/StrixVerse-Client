#pragma once

#include <string>
#include <fstream>
#include <mutex>

// Severity levels, ordered from most to least verbose.
enum class LogLevel
{
    Debug = 0,
    Info,
    Warning,
    Error
};

// Convenience macros for formatted logging
#define LOG_DEBUG(msg)   Logger::Debug(msg)
#define LOG_INFO(msg)    Logger::Info(msg)
#define LOG_WARN(msg)    Logger::Warning(msg)
#define LOG_ERROR(msg)   Logger::Error(msg)

// -----------------------------------------------------------------------------
// Logger
//
// Purpose:
//   Thread-safe logging to console and file with timestamps and severity
//   filtering. One instance is owned by Application (RAII: the owning
//   instance opens the file in Initialize and closes it in Shutdown), while
//   the static Info/Warning/Error/Debug API lets any system log without
//   holding a reference.
// -----------------------------------------------------------------------------
class Logger
{
public:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool Initialize();
    void Shutdown();

    static void Info(const std::string& message);
    static void Warning(const std::string& message);
    static void Error(const std::string& message);
    static void Debug(const std::string& message);

    // Messages below this level are discarded. Default: Info in release-style
    // usage; set to LogLevel::Debug during development.
    static void SetLevel(LogLevel level);
    static LogLevel GetLevel();

private:
    static void Write(
        LogLevel level,
        const std::string& levelName,
        const std::string& message);

private:
    static std::ofstream s_LogFile;
    static std::mutex s_Mutex;
    static LogLevel s_Level;
};