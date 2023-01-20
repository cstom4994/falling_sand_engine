// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine_core.h"

engine_core Core;
engine_screen Screen;
engine_time Time;

// --------------- FPS counter functions ---------------

void ExitGame() {}

void GameExited() {}

int InitCore() { return 1; }

void InitTime() {
    Time.startTime = Time::millis();
    Time.lastTime = Time.startTime;
    Time.lastTick = Time.lastTime;
    Time.lastFPS = Time.lastTime;
    Time.mspt = 33;
    Time.framesPerSecond = 0;
}

void InitScreen(int windowWidth, int windowHeight, int scale, int maxFPS) {
    Screen.windowWidth = windowWidth;
    Screen.windowHeight = windowHeight;
    Screen.maxFPS = maxFPS;
    Screen.gameScale = scale;
}

void UpdateTime() {
    Time.now = Time::millis();
    Time.deltaTime = Time.now - Time.lastTime;
}

void WaitUntilNextFrame() {}

F32 GetFPS() { return Time.framesPerSecond; }

void InitFPS() {
    // Initialize FPS at 0
    memset(Time.frameTimes, 0, sizeof(Time.frameTimes));
    Time.frameCount = 0;
    Time.framesPerSecond = 0;
}

void ProcessFPS() {
    Time.frameCount++;
    if (Time.now - Time.lastFPS >= 1000) {
        Time.lastFPS = Time.now;
        Time.framesPerSecond = Time.frameCount;
        Time.frameCount = 0;

        // calculate "feels like" fps
        F32 sum = 0;
        F32 num = 0.01;

        for (int i = 0; i < FrameTimeNum; i++) {
            F32 weight = Time.frameTimes[i];
            sum += weight * Time.frameTimes[i];
            num += weight;
        }

        Time.feelsLikeFps = 1000 / (sum / num);
    }

    for (int i = 1; i < FrameTimeNum; i++) {
        Time.frameTimes[i - 1] = Time.frameTimes[i];
    }
    Time.frameTimes[FrameTimeNum - 1] = (U16)(Time::millis() - Time.now);

    Time.lastTime = Time.now;
}
