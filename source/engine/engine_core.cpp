// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine/engine_core.h"

#include "core/core.h"
#include "core/cpp/utils.hpp"
#include "core/utility.hpp"

engine_core Core;
windows Screen;
engine_time Time;

// --------------- FPS counter functions ---------------

void ExitGame() {}

void GameExited() {}

int InitCore() { return METADOT_OK; }

bool InitTime() {
    Time.startTime = ME_gettime();
    Time.lastTime = Time.startTime;
    Time.lastTickTime = Time.lastTime;
    Time.lastCheckTime = Time.lastTime;

    Time.mspt = 0;
    Time.maxTps = 30;
    Time.framesPerSecond = 0;
    Time.tickCount = 0;

    return METADOT_OK;
}

bool InitScreen(int windowWidth, int windowHeight, int scale, int maxFPS) {
    Screen.windowWidth = windowWidth;
    Screen.windowHeight = windowHeight;
    Screen.maxFPS = maxFPS;
    Screen.gameScale = scale;
    return METADOT_OK;
}

void UpdateTime() {
    Time.now = ME_gettime();
    Time.deltaTime = Time.now - Time.lastTime;
}

void WaitUntilNextFrame() {}

F32 GetFPS() { return Time.framesPerSecond; }

void InitFPS() {
    // Initialize FPS at 0
    memset(Time.tpsTrace, 0, sizeof(Time.tpsTrace));
    memset(Time.frameTimesTrace, 0, sizeof(Time.frameTimesTrace));
    Time.frameCount = 0;
    Time.framesPerSecond = 0;
}

void ProcessTickTime() {
    Time.frameCount++;
    if (Time.now - Time.lastCheckTime >= 1000.0f) {
        Time.lastCheckTime = Time.now;
        Time.framesPerSecond = Time.frameCount;
        Time.frameCount = 0;

        // calculate "feels like" fps
        F32 sum = 0;
        F32 num = 0.01;

        for (int i = 0; i < TraceTimeNum; i++) {
            F32 weight = Time.frameTimesTrace[i];
            sum += weight * Time.frameTimesTrace[i];
            num += weight;
        }

        Time.feelsLikeFps = 1000 / (sum / num);

        // Update tps trace
        for (int i = 1; i < TraceTimeNum; i++) {
            Time.tpsTrace[i - 1] = Time.tpsTrace[i];
        }
        Time.tpsTrace[TraceTimeNum - 1] = Time.tickCount;

        // Calculate tps
        sum = 0;

        int n = 0;
        for (int i = 0; i < TraceTimeNum; i++)
            if (Time.tpsTrace[i]) {
                sum += Time.tpsTrace[i];
                n++;
            }

        // Time.tps = 1000.0f / (sum / num);
        Time.tps = sum / n;

        Time.tickCount = 0;
    }

    Time.mspt = 1000.0f / Time.tps;

    for (int i = 1; i < TraceTimeNum; i++) {
        Time.frameTimesTrace[i - 1] = Time.frameTimesTrace[i];
    }
    Time.frameTimesTrace[TraceTimeNum - 1] = (U16)(ME_gettime() - Time.now);

    Time.lastTime = Time.now;
}
