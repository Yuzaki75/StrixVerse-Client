#pragma once

#include <string>
#include <fstream>
#include <mutex>

class Logger
{
public:
    Logger();
    ~Logger();

    bool Initialize();
    void Shutdown();

    static void Info(const std::string& message);
    static void Warning(const std::string& message);
    static void Error(const std::string& message);
    static void Debug(const std::string& message);

private:
    static void Write(
        const std::string& level,
        const std::string& message);

private:
    static std::ofstream s_LogFile;
    static std::mutex s_Mutex;
};