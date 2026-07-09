#include "Time.h"

#include <chrono>

namespace
{
    // Translation-unit local clock origin (not visible outside this file).
    std::chrono::steady_clock::time_point StartTime()
    {
        static const auto start = std::chrono::steady_clock::now();
        return start;
    }

    // Clamp huge frame spikes (breakpoints, window drags) so the fixed-step
    // accumulator cannot enter a "spiral of death".
    constexpr double kMaxDeltaSeconds = 0.25;
}

float Time::s_DeltaTime = 0.0f;
float Time::s_FPS = 0.0f;
float Time::s_ElapsedTime = 0.0f;
unsigned long long Time::s_FrameCount = 0;
double Time::s_LastTime = 0.0;
float Time::s_FixedTimestep = 1.0f / 60.0f;
double Time::s_Accumulator = 0.0;

void Time::Initialize()
{
    auto now = std::chrono::steady_clock::now();

    s_LastTime =
        std::chrono::duration<double>(now - StartTime()).count();

    s_DeltaTime = 0.0f;
    s_FPS = 0.0f;
    s_ElapsedTime = 0.0f;
    s_FrameCount = 0;
    s_Accumulator = 0.0;
}

void Time::Update()
{
    auto now = std::chrono::steady_clock::now();

    double current =
        std::chrono::duration<double>(now - StartTime()).count();

    double delta = current - s_LastTime;

    if (delta < 0.0)
        delta = 0.0;

    if (delta > kMaxDeltaSeconds)
        delta = kMaxDeltaSeconds;

    s_DeltaTime = static_cast<float>(delta);

    s_LastTime = current;

    s_ElapsedTime = static_cast<float>(current);

    s_FrameCount++;

    s_Accumulator += delta;

    if (s_DeltaTime > 0.0f)
        s_FPS = 1.0f / s_DeltaTime;
}

float Time::GetDeltaTime()
{
    return s_DeltaTime;
}

float Time::GetFPS()
{
    return s_FPS;
}

float Time::GetElapsedTime()
{
    return s_ElapsedTime;
}

unsigned long long Time::GetFrameCount()
{
    return s_FrameCount;
}

void Time::SetFixedTimestep(float seconds)
{
    if (seconds > 0.0f)
        s_FixedTimestep = seconds;
}

float Time::GetFixedTimestep()
{
    return s_FixedTimestep;
}

bool Time::ConsumeFixedStep()
{
    if (s_Accumulator >= static_cast<double>(s_FixedTimestep))
    {
        s_Accumulator -= static_cast<double>(s_FixedTimestep);
        return true;
    }

    return false;
}