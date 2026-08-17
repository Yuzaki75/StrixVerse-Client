#include "core/Application.h"

#include <SDL3/SDL.h>

#include <windows.h>

namespace
{
    // One client per machine.
    //
    // A named mutex is the standard Windows single-instance lock: the kernel
    // owns the name, so it is released even if the process is killed or
    // crashes, which a lock file on disk would not be. "Local\" scopes it to
    // the current login session, so two different users on the same PC can
    // each run their own client -- the restriction is one client per user
    // session, not one per machine account.
    //
    // The handle is deliberately never closed: it lives for the process
    // lifetime and Windows reclaims it on exit.
    bool ClaimSingleInstance()
    {
        const HANDLE mutex = ::CreateMutexW(nullptr, TRUE, L"Local\\StrixVerseClient.SingleInstance");

        if (mutex == nullptr)
        {
            // The lock could not be created at all. Failing open is the right
            // call here: refusing to start because of an unrelated OS error
            // would be worse than allowing a second window.
            return true;
        }

        return ::GetLastError() != ERROR_ALREADY_EXISTS;
    }
}

int main()
{
    if (!ClaimSingleInstance())
    {
        // Shown rather than logged: the second instance exits before the
        // engine, and therefore the logger, exists.
        ::SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
                                   "StrixVerse",
                                   "StrixVerse is already running.\n\n"
                                   "Only one copy can run at a time on this computer.",
                                   nullptr);
        return 0;
    }

    Application app;

    if (!app.Initialize())
        return -1;

    app.Run();

    app.Shutdown();

    return 0;
}
