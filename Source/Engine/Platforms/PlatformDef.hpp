// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/Macros.hpp"

#if defined(METADOT_PLATFORM_WINDOWS)
#include <Windows.h>
#include <io.h>

#ifdef _WIN32
#include <SDL_syswm.h>
#include <shobjidl.h>
#endif

#undef min
#undef max

#elif defined(METADOT_PLATFORM_LINUX)
#include <limits.h>
#include <sys/io.h>
#elif defined(METADOT_PLATFORM_APPLE)
#include <TargetConditionals.h>
#include <sys/ioctl.h>
#include <mach-o/dyld.h>
#endif

#include <string>

#if __cplusplus <= 201402L
#error "TODO: fix for this compiler! (at least C++14 is required)"
#endif

namespace MetaEngine {
    namespace Platforms {

        const std::string &GetExecutablePath();
    }
}// namespace MetaEngine

