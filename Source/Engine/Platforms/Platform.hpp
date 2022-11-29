// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_PLATFORM_HPP_
#define _METADOT_PLATFORM_HPP_

#include "Core/Macros.hpp"
#include "SDLWrapper.hpp"

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
#include <sys/stat.h>
#include <sys/time.h>
#endif

#include <string>

#if __cplusplus <= 201402L
#error "TODO: fix for this compiler! (at least C++14 is required)"
#endif

namespace Platforms {
    const std::string &GetExecutablePath();
}


enum DisplayMode {
    WINDOWED,
    BORDERLESS,
    FULLSCREEN
};

enum WindowFlashAction {
    START,
    START_COUNT,
    START_UNTIL_FG,
    STOP
};

struct Platform
{
    C_Window *window = nullptr;

    int WIDTH = 1024;
    int HEIGHT = 720;

    int ParseRunArgs(int argc, char *argv[]);
    int InitWindow();
    void SetDisplayMode(DisplayMode mode);
    void HandleWindowSizeChange(int newWidth, int newHeight);
    void SetWindowFlash(WindowFlashAction action, int count, int period);
    void SetVSync(bool vsync);
    void SetMinimizeOnLostFocus(bool minimize);
};

#endif