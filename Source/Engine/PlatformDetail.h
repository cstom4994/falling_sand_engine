// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_PLATFORMDETAIL_H_
#define _METADOT_PLATFORMDETAIL_H_

#include "Core/Macros.hpp"

#if defined(METADOT_PLATFORM_WINDOWS)
#include <Windows.h>
#include <io.h>

#ifdef _WIN32
//#include <SDL_syswm.h>
#include <shobjidl.h>
#endif

#if defined(WIN32) && defined(_MSC_VER)
#define __func__ __FUNCTION__
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
#include <sys/stat.h>
#include <sys/time.h>
#endif

#include <string>




namespace Platforms {
    const std::string &GetExecutablePath();
}

#endif// !_METADOT_PLATFORMDETAIL_H_