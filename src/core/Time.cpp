#include "Time.h"

#include <chrono>

static std::chrono::high_resolution_clock::time_point g_Start =
    std::chrono::high_resolution_clock::now();

float Time::s_DeltaTime = 0.0f;
float Time::s_FPS = 0.0f;
float Time::s_ElapsedTime = 0.0f;
unsigned long long Time::s_FrameCount = 0;
double Time::s_LastTime = 0.0;

void Time::Initialize()
{
    auto now = std::chrono::high_resolution_clock::now();

    s_LastTime =
        std::chrono::duration<double>(now - g_Start).count();

    s_DeltaTime = 0.0f;
    s_FPS = 0.0f;
    s_ElapsedTime = 0.0f;
    s_FrameCount = 0;
}

void Time::Update()
{
    auto now = std::chrono::high_resolution_clock::now();

    double current =
        std::chrono::duration<double>(now - g_Start).count();

    s_DeltaTime = static_cast<float>(current - s_LastTime);

    s_LastTime = current;

    s_ElapsedTime = static_cast<float>(current);

    s_FrameCount++;

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