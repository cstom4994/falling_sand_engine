// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_PLATFORM_HPP_
#define _METADOT_PLATFORM_HPP_

#include "SDLWrapper.hpp"

enum DisplayMode {
    WINDOWED,
    BORDERLESS,
    FULLSCREEN
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
};

#endif