// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_PLATFORM_HPP_
#define _METADOT_PLATFORM_HPP_

#include "Core/Macros.hpp"

#include "PlatformDetail.h"
#include "SDLWrapper.hpp"

#if __cplusplus <= 201402L
#error "TODO: fix for this compiler! (at least C++14 is required)"
#endif

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