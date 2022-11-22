// Copyright(c) 2022, KaoruXun All rights reserved.

#pragma once

#include "SDLWrapper.hpp"

struct Platform
{
    C_Window *window = nullptr;

    int WIDTH = 1024;
    int HEIGHT = 720;

    int ParseRunArgs(int argc, char *argv[]);
    int InitWindow();
};