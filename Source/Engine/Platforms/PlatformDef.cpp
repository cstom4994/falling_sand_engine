// Copyright(c) 2022, KaoruXun All rights reserved.

#include "PlatformDef.hpp"

#if defined(METADOT_PLATFORM_WINDOWS)
#include <Windows.h>
#elif defined(METADOT_PLATFORM_LINUX)
#include <limits.h>
#elif defined(METADOT_PLATFORM_APPLE)

#endif

namespace MetaEngine::Platforms {

    const std::string &GetExecutablePath() {
        static std::string out;
#if defined(_WIN32)
        if (out.empty()) {
            WCHAR path[260];
            GetModuleFileNameW(NULL, path, 260);
            out = SUtil::ws2s(std::wstring(path));
            cleanPathString(out);
        }
#elif defined(__linux)
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        std::string appPath = std::string(result, (count > 0) ? count : 0);

        std::size_t found = appPath.find_last_of("/\\");
        out = appPath.substr(0, found);
#endif
        return out;
    }
}// namespace MetaEngine::Platforms