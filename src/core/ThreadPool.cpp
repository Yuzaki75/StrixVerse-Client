#include "ThreadPool.h"

ThreadPool::ThreadPool(std::size_t threadCount)
{
    if (threadCount == 0)
        threadCount = 1;

    m_Workers.reserve(threadCount);

    for (std::size_t i = 0; i < threadCount; ++i)
    {
        m_Workers.emplace_back([this]() { WorkerLoop(); });
    }
}

ThreadPool::~ThreadPool()
{
    Shutdown();
}

void ThreadPool::Shutdown()
{
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if (m_Stopping)
            return;

        m_Stopping = true;
    }

    m_Condition.notify_all();

    for (std::thread& worker : m_Workers)
    {
        if (worker.joinable())
            worker.join();
    }

    m_Workers.clear();
}

std::size_t ThreadPool::GetThreadCount() const
{
    return m_Workers.size();
}

std::size_t ThreadPool::GetPendingTaskCount() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Tasks.size();
}

void ThreadPool::WorkerLoop()
{
    for (;;)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_Mutex);

            m_Condition.wait(lock, [this]()
            {
                return m_Stopping || !m_Tasks.empty();
            });

            // Graceful shutdown: drain the queue before exiting.
            if (m_Stopping && m_Tasks.empty())
                return;

            task = std::move(m_Tasks.front());
            m_Tasks.pop();
        }

        // packaged_task captures any exception into the caller's future,
        // so a throwing task can never take down a worker thread.
        task();
    }
}
