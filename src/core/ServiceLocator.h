#pragma once

#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>

// -----------------------------------------------------------------------------
// ServiceLocator
//
// Purpose:
//   Type-safe registry that decouples systems from each other. A system asks
//   for an interface (e.g. an IAudioService) without knowing which concrete
//   implementation was registered or who created it.
//
// Responsibilities:
//   - Provide<T>() : register or replace a service (shared ownership)
//   - Get<T>()     : type-safe lookup, nullptr when absent
//   - Has<T>()     : existence check
//   - Remove<T>()  : unregister a single service
//   - Clear()      : release everything (call during Application::Shutdown)
//
// Thread safety:
//   Every operation locks an internal mutex, so services may be registered
//   and resolved from worker threads (e.g. ThreadPool tasks).
//
// Notes:
//   State lives in function-local statics (no global objects, deterministic
//   initialization). Prefer constructor injection where practical; use the
//   locator for cross-cutting engine services.
// -----------------------------------------------------------------------------
class ServiceLocator
{
public:
    ServiceLocator() = delete;

    // Registers a service under its type T. Replaces any existing instance
    // registered for T (the old shared_ptr is released).
    template <typename T>
    static void Provide(std::shared_ptr<T> service)
    {
        std::lock_guard<std::mutex> lock(GetMutex());
        GetRegistry()[std::type_index(typeid(T))] = std::move(service);
    }

    // Returns the service registered for T, or nullptr when none exists.
    template <typename T>
    static std::shared_ptr<T> Get()
    {
        std::lock_guard<std::mutex> lock(GetMutex());

        auto& registry = GetRegistry();
        auto it = registry.find(std::type_index(typeid(T)));

        if (it == registry.end())
            return nullptr;

        return std::static_pointer_cast<T>(it->second);
    }

    // True when a service is registered for T.
    template <typename T>
    static bool Has()
    {
        std::lock_guard<std::mutex> lock(GetMutex());
        return GetRegistry().count(std::type_index(typeid(T))) > 0;
    }

    // Removes the service registered for T. Returns true when one existed.
    template <typename T>
    static bool Remove()
    {
        std::lock_guard<std::mutex> lock(GetMutex());
        return GetRegistry().erase(std::type_index(typeid(T))) > 0;
    }

    // Releases every registered service. Call once during shutdown, after
    // all systems that resolve services have stopped.
    static void Clear()
    {
        std::lock_guard<std::mutex> lock(GetMutex());
        GetRegistry().clear();
    }

    // Number of registered services (mainly for tests and diagnostics).
    static std::size_t Count()
    {
        std::lock_guard<std::mutex> lock(GetMutex());
        return GetRegistry().size();
    }

private:
    using Registry =
        std::unordered_map<std::type_index, std::shared_ptr<void>>;

    static Registry& GetRegistry()
    {
        static Registry registry;
        return registry;
    }

    static std::mutex& GetMutex()
    {
        static std::mutex mutex;
        return mutex;
    }
};
