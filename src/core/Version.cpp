#include "Version.h"

#include <sstream>

// All values can be overridden from the build system, e.g.:
//   target_compile_definitions(client_core PRIVATE
//       STRIX_CLIENT_VERSION="0.2.0"
//       STRIX_BUILD_VERSION="142"
//       STRIX_GIT_COMMIT="a1b2c3d")

#ifndef STRIX_CLIENT_VERSION
#define STRIX_CLIENT_VERSION "0.1.0"
#endif

#ifndef STRIX_BUILD_VERSION
#define STRIX_BUILD_VERSION "0"
#endif

#ifndef STRIX_GIT_COMMIT
#define STRIX_GIT_COMMIT "unknown"
#endif

const char* Version::GetClientVersion()
{
    return STRIX_CLIENT_VERSION;
}

const char* Version::GetBuildVersion()
{
    return STRIX_BUILD_VERSION;
}

const char* Version::GetGitCommit()
{
    return STRIX_GIT_COMMIT;
}

const char* Version::GetBuildDate()
{
    return __DATE__;
}

const char* Version::GetBuildTime()
{
    return __TIME__;
}

std::string Version::GetFullVersionString()
{
    std::ostringstream ss;

    ss << "StrixVerse " << GetClientVersion()
       << " (build " << GetBuildVersion()
       << ", commit " << GetGitCommit()
       << ", " << GetBuildDate()
       << " " << GetBuildTime()
       << ")";

    return ss.str();
}
