#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// ThreadPool
//
// Purpose:
//   Fixed-size pool of worker threads executing queued tasks. Used for
//   background work: asset streaming, file IO, network serialization, ...
//
// Responsibilities:
//   - Configurable worker count (defaults to hardware concurrency)
//   - FIFO task queue with std::future results
//   - Exception-safe execution: exceptions thrown inside a task are captured
//     in the returned future and re-thrown on future.get()
//   - Graceful shutdown: queued tasks finish before workers join (RAII, the
//     destructor always shuts down cleanly)
//
// Dependencies: standard library only.
// Ownership: create one instance where it is needed (e.g. owned by Engine)
// or register a shared instance in the ServiceLocator.
// -----------------------------------------------------------------------------
class ThreadPool
{
public:
    explicit ThreadPool(
        std::size_t threadCount = std::thread::hardware_concurrency());

    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // Enqueues a callable with its arguments and returns a future for the
    // result. Throws std::runtime_error when called after Shutdown().
    template <typename F, typename... Args>
    auto Submit(F&& func, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using ReturnType = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [callable = std::forward<F>(func),
             ... captured = std::forward<Args>(args)]() mutable
            {
                return std::invoke(
                    std::move(callable), std::move(captured)...);
            });

        std::future<ReturnType> result = task->get_future();

        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            if (m_Stopping)
            {
                throw std::runtime_error(
                    "ThreadPool::Submit called after shutdown.");
            }

            m_Tasks.emplace([task]() { (*task)(); });
        }

        m_Condition.notify_one();

        return result;
    }

    // Stops accepting new tasks, lets queued tasks finish, joins all
    // workers. Safe to call multiple times; also called by the destructor.
    void Shutdown();

    // Number of worker threads.
    std::size_t GetThreadCount() const;

    // Number of tasks currently waiting in the queue.
    std::size_t GetPendingTaskCount() const;

private:
    void WorkerLoop();

    std::vector<std::thread> m_Workers;
    std::queue<std::function<void()>> m_Tasks;

    mutable std::mutex m_Mutex;
    std::condition_variable m_Condition;
    bool m_Stopping = false;
};
