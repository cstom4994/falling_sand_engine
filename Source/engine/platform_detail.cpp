// Copyright(c) 2022, KaoruXun All rights reserved.

#include "platform_detail.h"

#include "game/filesystem.hpp"

namespace Platforms {

    const std::string &GetExecutablePath() {
        static std::string out;
#if defined(METADOT_PLATFORM_WINDOWS)
        if (out.empty()) {
            WCHAR path[260];
            GetModuleFileNameW(NULL, path, 260);
            out = SUtil::ws2s(std::wstring(path));
            FUtil::cleanPathString(out);
        }
#elif defined(METADOT_PLATFORM_LINUX)
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        std::string appPath = std::string(result, (count > 0) ? count : 0);

        std::size_t found = appPath.find_last_of("/\\");
        out = appPath.substr(0, found);
#elif defined(METADOT_PLATFORM_APPLE)
        char buf[PATH_MAX];
        uint32_t bufsize = PATH_MAX;
        if (!_NSGetExecutablePath(buf, &bufsize)) out = buf;
#endif
        return out;
    }
}// namespace Platforms
