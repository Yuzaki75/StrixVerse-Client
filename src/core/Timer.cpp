#include "Timer.h"

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

float Timer::s_DeltaTime = 0.0f;
float Timer::s_FPS = 0.0f;
float Timer::s_ElapsedTime = 0.0f;
unsigned long long Timer::s_FrameCount = 0;
double Timer::s_LastTime = 0.0;
float Timer::s_FixedTimestep = 1.0f / 60.0f;
double Timer::s_Accumulator = 0.0;

void Timer::Initialize()
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

void Timer::Update()
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

float Timer::GetDeltaTime()
{
    return s_DeltaTime;
}

float Timer::GetFPS()
{
    return s_FPS;
}

float Timer::GetElapsedTime()
{
    return s_ElapsedTime;
}

unsigned long long Timer::GetFrameCount()
{
    return s_FrameCount;
}

void Timer::SetFixedTimestep(float seconds)
{
    if (seconds > 0.0f)
        s_FixedTimestep = seconds;
}

float Timer::GetFixedTimestep()
{
    return s_FixedTimestep;
}

void Timer::DiscardOwedFixedSteps()
{
    s_Accumulator = 0.0;
}

bool Timer::ConsumeFixedStep()
{
    if (s_Accumulator >= static_cast<double>(s_FixedTimestep))
    {
        s_Accumulator -= static_cast<double>(s_FixedTimestep);
        return true;
    }

    return false;
}