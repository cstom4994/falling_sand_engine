// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine/engine_core.h"

#include "engine/core/core.hpp"
#include "engine/utils/utility.hpp"
#include "engine/utils/utils.hpp"

EngineData g_engine_data;

// --------------- FPS counter functions ---------------

void ExitGame() {}

void GameExited() {}

int InitCore() { return METADOT_OK; }

bool InitTime() {
    ENGINE()->time.startTime = ME_gettime();
    ENGINE()->time.lastTime = ENGINE()->time.startTime;
    ENGINE()->time.lastTickTime = ENGINE()->time.lastTime;
    ENGINE()->time.lastCheckTime = ENGINE()->time.lastTime;

    ENGINE()->time.mspt = 0;
    ENGINE()->time.maxTps = 30;
    ENGINE()->time.framesPerSecond = 0;
    ENGINE()->time.tickCount = 0;

    return METADOT_OK;
}

bool InitScreen(int windowWidth, int windowHeight, int scale, int maxFPS) {
    ENGINE()->windowWidth = windowWidth;
    ENGINE()->windowHeight = windowHeight;
    ENGINE()->maxFPS = maxFPS;
    ENGINE()->render_scale = scale;
    return METADOT_OK;
}

void UpdateTime() {
    ENGINE()->time.now = ME_gettime();
    ENGINE()->time.deltaTime = ENGINE()->time.now - ENGINE()->time.lastTime;
}

void WaitUntilNextFrame() {}

f32 GetFPS() { return ENGINE()->time.framesPerSecond; }

void InitFPS() {
    // Initialize FPS at 0
    memset(ENGINE()->time.tpsTrace, 0, sizeof(ENGINE()->time.tpsTrace));
    memset(ENGINE()->time.frameTimesTrace, 0, sizeof(ENGINE()->time.frameTimesTrace));
    ENGINE()->time.frameCount = 0;
    ENGINE()->time.framesPerSecond = 0;
}

void ProcessTickTime() {
    ENGINE()->time.frameCount++;
    if (ENGINE()->time.now - ENGINE()->time.lastCheckTime >= 1000.0f) {
        ENGINE()->time.lastCheckTime = ENGINE()->time.now;
        ENGINE()->time.framesPerSecond = ENGINE()->time.frameCount;
        ENGINE()->time.frameCount = 0;

        // calculate "feels like" fps
        f32 sum = 0;
        f32 num = 0.01;

        for (int i = 0; i < TraceTimeNum; i++) {
            f32 weight = ENGINE()->time.frameTimesTrace[i];
            sum += weight * ENGINE()->time.frameTimesTrace[i];
            num += weight;
        }

        ENGINE()->time.feelsLikeFps = 1000 / (sum / num);

        // Update tps trace
        for (int i = 1; i < TraceTimeNum; i++) {
            ENGINE()->time.tpsTrace[i - 1] = ENGINE()->time.tpsTrace[i];
        }
        ENGINE()->time.tpsTrace[TraceTimeNum - 1] = ENGINE()->time.tickCount;

        // Calculate tps
        sum = 0;

        int n = 0;
        for (int i = 0; i < TraceTimeNum; i++)
            if (ENGINE()->time.tpsTrace[i]) {
                sum += ENGINE()->time.tpsTrace[i];
                n++;
            }

        // ENGINE()->time.tps = 1000.0f / (sum / num);
        ENGINE()->time.tps = sum / n;

        ENGINE()->time.tickCount = 0;
    }

    ENGINE()->time.mspt = 1000.0f / ENGINE()->time.tps;

    for (int i = 1; i < TraceTimeNum; i++) {
        ENGINE()->time.frameTimesTrace[i - 1] = ENGINE()->time.frameTimesTrace[i];
    }
    ENGINE()->time.frameTimesTrace[TraceTimeNum - 1] = (u16)(ME_gettime() - ENGINE()->time.now);

    ENGINE()->time.lastTime = ENGINE()->time.now;
}
