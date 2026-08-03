#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

// -----------------------------------------------------------------------------
// Scheduler
//
// Purpose:
//   Executes callbacks at specific times or after delays. Used for delayed
//   loading, timed events, cooldowns, etc.
//
// Responsibilities:
//   - Schedule one-shot or recurring tasks.
//   - Cancel scheduled tasks.
//   - Thread-safe: safe to call from any thread.
//   - Uses a single background thread to dispatch tasks.
//
// Dependencies: standard library only.
// -----------------------------------------------------------------------------
class Scheduler
{
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = Clock::duration;
    using Milliseconds = std::chrono::milliseconds;
    using Seconds = std::chrono::duration<float>;
    using Delay = std::chrono::duration<float, std::ratio<1>>; // seconds as float

    struct Task
    {
        std::function<void()> callback;
        TimePoint triggerTime;            // When the task should next run.
        std::optional<Duration> interval; // If set, the task repeats every interval.
        std::string name;                 // For debugging.
        bool active = true;

        // For priority queue: earlier triggerTime has higher priority.
        bool operator>(const Task &other) const
        {
            return triggerTime > other.triggerTime;
        }
    };

    explicit Scheduler(const std::string &name = "Scheduler");
    ~Scheduler();

    Scheduler(const Scheduler &) = delete;
    Scheduler &operator=(const Scheduler &) = delete;

    // Schedule a one-shot task to run after the given delay.
    // Returns an ID that can be used to cancel the task.
    uint64_t Schedule(std::function<void()> callback, Delay delay, const std::string &name = "");

    // Schedule a repeating task that runs every interval, starting after the initial delay.
    // Returns an ID that can be used to cancel the task.
    uint64_t ScheduleRepeating(std::function<void()> callback, Delay initialDelay, Duration interval, const std::string &name = "");

    // Cancel a scheduled task by its ID. Returns true if the task was found and removed.
    bool Cancel(uint64_t id);

    // Starts the scheduler's internal thread. Called automatically in the constructor.
    void Start();

    // Stops the scheduler and waits for the thread to finish.
    void Stop();

    // Returns true if the scheduler is running.
    bool IsRunning() const;

private:
    using TaskPtr = std::shared_ptr<Task>;

    void WorkerLoop();

    // Generates a unique ID for a task.
    uint64_t GenerateId();

    std::thread m_Thread;
    mutable std::mutex m_Mutex;
    std::condition_variable m_Cv;
    bool m_Running = false;
    bool m_StopRequested = false;
    uint64_t m_NextId = 1;
    // Priority queue of tasks, ordered by triggerTime (earliest first).
    std::priority_queue<TaskPtr, std::vector<TaskPtr>, std::greater<>> m_Tasks;
    // Map from ID to task for cancellation.
    std::unordered_map<uint64_t, TaskPtr> m_TaskMap;
};