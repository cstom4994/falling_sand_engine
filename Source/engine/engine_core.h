// Copyright(c) 2022-2023, KaoruXun All rights reserved.

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
    I32 feelsLikeFps;
    I64 lastTime;
    I64 lastFPS;
    I64 lastTick;
    I64 lastLoadingTick;
    I64 now;
    I64 startTime;
    I64 deltaTime;

    I32 mspt;

    U16 frameTimes[FrameTimeNum];
    U32 frameCount;
    F32 framesPerSecond;
} engine_time;

void ExitGame();
int GameExited();

int InitCore();
void InitTime();
void InitScreen(int windowWidth, int windowHeight, int scale, int maxFPS);

void UpdateTime();
void WaitUntilNextFrame();

void InitFPS();
void ProcessFPS();
F32 GetFPS();

#endif