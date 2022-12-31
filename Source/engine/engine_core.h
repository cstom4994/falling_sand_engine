// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_ENGINECORE_H_
#define _METADOT_ENGINECORE_H_

#include <stdlib.h>
#include <time.h>

#include "core/core.h"
#include "libs/lua/host/lauxlib.h"
#include "libs/lua/host/lua.h"
#include "libs/lua/host/lualib.h"
#include "sdl_wrapper.h"
#include "utils.h"

typedef struct engine_core {
    C_Window *window;
    C_GLContext *glContext;
} engine_core;

typedef struct engine_screen {
    // Internal resolution, used in rendering
    //  int gameWidth;
    //  int gameHeight;

    // Window resolution
    int windowWidth;
    int windowHeight;

    // Scale of division, as gameRes = windowRes/gameScale
    int gameScale;

    unsigned maxFPS;
} engine_screen;

typedef struct engine_time {
    int fps;
    int feelsLikeFps;
    long long lastTime;
    long long lastTick;
    long long lastLoadingTick;
    long long now;
    long long startTime;
    long long deltaTime;
    long mspt;
} engine_time;

void ExitGame();
int GameExited();

int InitCore();
void InitTime();
void InitScreen(int windowWidth, int windowHeight, int scale, int maxFPS);

void UpdateTime();
void WaitUntilNextFrame();

#endif