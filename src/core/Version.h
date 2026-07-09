#pragma once

#include <string>

// -----------------------------------------------------------------------------
// Version
//
// Purpose:
//   Single source of truth for client version and build metadata. Values can
//   be overridden from the build system (see CMakeLists.txt), so packaging
//   pipelines can stamp builds without editing source code.
//
// Dependencies: standard library only. Depended on by Application (startup
// banner) and, later, by Networking (protocol/version handshake) and UI.
// -----------------------------------------------------------------------------
class Version
{
public:
    Version() = delete;

    // Semantic client version, e.g. "0.1.0".
    static const char* GetClientVersion();

    // Monotonic build number, e.g. "100". Stamped by CI when available.
    static const char* GetBuildVersion();

    // Short git commit hash, or "unknown" when built outside a repository.
    static const char* GetGitCommit();

    // Compilation date, e.g. "Jul  9 2026".
    static const char* GetBuildDate();

    // Compilation time, e.g. "22:41:05".
    static const char* GetBuildTime();

    // Human-readable summary for logs and the window title, e.g.
    // "StrixVerse 0.1.0 (build 100, commit a1b2c3d, Jul  9 2026 22:41:05)".
    static std::string GetFullVersionString();
};
