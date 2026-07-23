#include "Scheduler.h"

#include <algorithm>
#include <iostream>
#include <sstream>

Scheduler::Scheduler(const std::string& name)
{
    // Note: the thread name is not set for simplicity.
    Start();
}

Scheduler::~Scheduler()
{
    Stop();
}

void Scheduler::Start()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Running)
        return;

    m_Running = true;
    m_StopRequested = false;
    m_Thread = std::thread(&Scheduler::WorkerLoop, this);
}

void Scheduler::Stop()
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (!m_Running)
            return;
        m_StopRequested = true;
    }
    m_Cv.notify_one();
    if (m_Thread.joinable())
        m_Thread.join();
    m_Running = false;
}

bool Scheduler::IsRunning() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Running;
}

uint64_t Scheduler::GenerateId()
{
    return m_NextId++;
}

uint64_t Scheduler::Schedule(std::function<void()> callback, Delay delay, const std::string& name)
{
    auto task = std::make_shared<Task>();
    task->callback = std::move(callback);
    task->triggerTime = Clock::now() + std::chrono::duration_cast<Clock::duration>(delay);
    task->interval = std::nullopt; // one-shot
    task->name = name;
    task->active = true;

    uint64_t id = GenerateId();

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Tasks.emplace(task);
        m_TaskMap[id] = task;
    }

    m_Cv.notify_one();
    return id;
}

uint64_t Scheduler::ScheduleRepeating(std::function<void()> callback, Delay initialDelay, Duration interval, const std::string& name)
{
    auto task = std::make_shared<Task>();
    task->callback = std::move(callback);
    task->triggerTime = Clock::now() + std::chrono::duration_cast<Clock::duration>(initialDelay);
    task->interval = interval; // repeating
    task->name = name;
    task->active = true;

    uint64_t id = GenerateId();

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Tasks.emplace(task);
        m_TaskMap[id] = task;
    }

    m_Cv.notify_one();
    return id;
}

bool Scheduler::Cancel(uint64_t id)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_TaskMap.find(id);
    if (it == m_TaskMap.end())
        return false;

    // Mark the task as inactive. It will be ignored when popped from the queue.
    it->second->active = false;
    m_TaskMap.erase(it);
    // Note: the task remains in the priority queue but will be skipped.
    return true;
}

void Scheduler::WorkerLoop()
{
    while (true)
    {
        TaskPtr task;
        {
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_Cv.wait(lock, [this]()
            {
                return m_StopRequested || !m_Tasks.empty();
            });

            if (m_StopRequested && m_Tasks.empty())
                return;

            // Pop the earliest task.
            task = m_Tasks.top();
            m_Tasks.pop();

            // If the task is inactive, skip it and continue.
            if (!task->active)
                continue;
        }

        // Execute the task.
        try
        {
            task->callback();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Scheduler: exception in task \"" << task->name << "\": " << e.what() << std::endl;
        }
        catch (...)
        {
            std::cerr << "Scheduler: unknown exception in task \"" << task->name << "\"" << std::endl;
        }

        // If the task is repeating, reschedule it.
        if (task->interval.has_value())
        {
            task->triggerTime += task->interval.value();
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                if (task->active) // Check again in case it was cancelled during execution.
                    m_Tasks.emplace(task);
            }
            m_Cv.notify_one();
        }
        // If it's a one-shot task, it's done.
    }
}