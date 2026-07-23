#pragma once

// -----------------------------------------------------------------------------
// Timer
//
// Purpose:
//   Central frame-timing service for the engine. Tracks delta time, total
//   elapsed time, FPS and frame count, and provides fixed-timestep support
//   for deterministic simulation updates (physics, networking).
//
// Usage:
//   Timer::Initialize();                 // once, at engine startup
//   Timer::Update();                     // once per frame, at frame start
//   while (Timer::ConsumeFixedStep())    // zero or more fixed updates per frame
//       game.FixedUpdate(Timer::GetFixedTimestep());
//
// Notes:
//   Static-only class: it represents a single, engine-wide clock. Not intended
//   to be instantiated. All state is private and mutated only via Update().
// -----------------------------------------------------------------------------
class Timer
{
public:
    Timer() = delete;

    // Resets all timing state. Call once before the main loop starts.
    static void Initialize();

    // Advances the clock. Call exactly once per frame.
    static void Update();

    // Seconds elapsed between the last two Update() calls.
    static float GetDeltaTime();

    // Instantaneous frames-per-second (1 / delta time).
    static float GetFPS();

    // Seconds elapsed since the process clock started.
    static float GetElapsedTime();

    // Number of Update() calls since Initialize().
    static unsigned long long GetFrameCount();

    // --- Fixed timestep -------------------------------------------------

    // Sets the fixed simulation step in seconds (default 1/60). Values <= 0
    // are ignored.
    static void SetFixedTimestep(float seconds);

    // Current fixed simulation step in seconds.
    static float GetFixedTimestep();

    // Returns true and consumes one fixed step while enough frame time has
    // accumulated. Call in a while-loop each frame to run fixed updates.
    static bool ConsumeFixedStep();

private:
    static float s_DeltaTime;
    static float s_FPS;
    static float s_ElapsedTime;
    static unsigned long long s_FrameCount;
    static double s_LastTime;

    static float s_FixedTimestep;
    static double s_Accumulator;
};