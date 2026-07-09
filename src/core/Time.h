#pragma once

class Time
{
public:
    static void Initialize();

    static void Update();

    static float GetDeltaTime();

    static float GetFPS();

    static float GetElapsedTime();

    static unsigned long long GetFrameCount();

private:
    static float s_DeltaTime;
    static float s_FPS;
    static float s_ElapsedTime;

    static unsigned long long s_FrameCount;

    static double s_LastTime;
};