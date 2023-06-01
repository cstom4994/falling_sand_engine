// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_ENGINECORE_H
#define ME_ENGINECORE_H

#include <stdlib.h>
#include <time.h>

#include <string>

#include "core/core.hpp"
#include "core/cpp/utils.hpp"
#include "core/sdl_wrapper.h"

typedef struct engine_core {
    C_Window *window;
    C_GLContext *glContext;

    std::string gamepath;

    // Maximum memory that can be used
    u64 max_mem = 4294967296;  // 4096mb
} engine_core;

typedef struct windows {
    // Internal resolution, used in rendering
    //  int gameWidth;
    //  int gameHeight;

    // Window resolution
    int windowWidth;
    int windowHeight;

    i32 gameScale = 4;

    unsigned maxFPS;
} windows;

typedef struct engine_time {
    i32 feelsLikeFps;
    i64 lastTime;
    i64 lastCheckTime;
    i64 lastTickTime;
    i64 lastLoadingTick;
    i64 now;
    i64 startTime;
    i64 deltaTime;

    i32 tickCount;

    f32 mspt;
    i32 tpsTrace[TraceTimeNum];
    f32 tps;
    u32 maxTps;

    u16 frameTimesTrace[TraceTimeNum];
    u32 frameCount;
    f32 framesPerSecond;
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
f32 GetFPS();

#endif