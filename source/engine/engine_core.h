// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_ENGINECORE_H_
#define _METADOT_ENGINECORE_H_

#include <stdlib.h>
#include <time.h>

#include <string>

#include "core/core.h"
#include "core/cpp/utils.hpp"
#include "sdl_wrapper.h"

typedef struct engine_core {
    C_Window *window;
    C_GLContext *glContext;

    std::string gamepath;

    // Maximum memory that can be used
    U64 max_mem = 4294967296;  // 4096mb
} engine_core;

typedef struct windows {
    // Internal resolution, used in rendering
    //  int gameWidth;
    //  int gameHeight;

    // Window resolution
    int windowWidth;
    int windowHeight;

    I32 gameScale = 4;

    unsigned maxFPS;
} windows;

typedef struct engine_time {
    I32 feelsLikeFps;
    I64 lastTime;
    I64 lastCheckTime;
    I64 lastTickTime;
    I64 lastLoadingTick;
    I64 now;
    I64 startTime;
    I64 deltaTime;

    I32 tickCount;

    F32 mspt;
    I32 tpsTrace[TraceTimeNum];
    F32 tps;
    U32 maxTps;

    U16 frameTimesTrace[TraceTimeNum];
    U32 frameCount;
    F32 framesPerSecond;
} engine_time;

void ExitGame();
void GameExited();

int InitCore();
bool InitTime();
bool InitScreen(int windowWidth, int windowHeight, int scale, int maxFPS);

void UpdateTime();
void WaitUntilNextFrame();

void InitFPS();
void ProcessTickTime();
F32 GetFPS();

#endif