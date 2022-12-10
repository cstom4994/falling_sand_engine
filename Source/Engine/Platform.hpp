// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_PLATFORM_HPP_
#define _METADOT_PLATFORM_HPP_

#include "Core/Core.hpp"
#include "Core/Macros.hpp"

#include "PlatformDetail.h"
#include "SDLWrapper.hpp"

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

typedef UInt32 ticks;

struct Platform
{
    C_Window *window = nullptr;

    int WIDTH = 1024;
    int HEIGHT = 680;

    int ParseRunArgs(int argc, char *argv[]);
    int InitWindow();
    void EndWindow();
    void SetDisplayMode(DisplayMode mode);
    void HandleWindowSizeChange(int newWidth, int newHeight);
    void SetWindowFlash(WindowFlashAction action, int count, int period);
    void SetVSync(bool vsync);
    void SetMinimizeOnLostFocus(bool minimize);
    ticks GetTime() { return (ticks) SDL_GetTicks(); }
};

#endif