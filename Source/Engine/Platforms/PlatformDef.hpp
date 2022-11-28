// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Core/Macros.hpp"

#if defined(METADOT_PLATFORM_WINDOWS)
#include <Windows.h>
#include <io.h>

#ifdef _WIN32
//#include <SDL_syswm.h>
#include <shobjidl.h>
#endif

#undef min
#undef max

#elif defined(METADOT_PLATFORM_LINUX)
#include <bits/types/struct_tm.h>
#include <bits/types/time_t.h>
#include <limits.h>
#include <sys/io.h>
#include <sys/stat.h>
#elif defined(METADOT_PLATFORM_APPLE)
#include <TargetConditionals.h>
#include <mach-o/dyld.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>
#endif

#include <string>

#if __cplusplus <= 201402L
#error "TODO: fix for this compiler! (at least C++14 is required)"
#endif

namespace Platforms {

    const std::string &GetExecutablePath();
}
