// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine_core.h"

engine_core Core;
engine_screen Screen;
engine_time Time;

// --------------- FPS counter functions ---------------

int exitGame = 0;

void ExitGame() { exitGame = 1; }

int GameExited() { return exitGame; }

int InitCore() {
    exitGame = 0;
    return 1;
}

void InitTime() {}

void InitScreen(int windowWidth, int windowHeight, int scale, int maxFPS) {
    Screen.windowWidth = windowWidth;
    Screen.windowHeight = windowHeight;
    Screen.maxFPS = maxFPS;
    Screen.gameScale = scale;
}

void UpdateTime() {}

void WaitUntilNextFrame() {}

F32 GetFPS() { return Time.framesPerSecond; }

void InitFPS() {
    // Initialize FPS at 0
    memset(Time.frameTimes, 0, sizeof(Time.frameTimes));
    Time.frameCount = 0;
    Time.framesPerSecond = 0;
}

void ProcessFPS() {

}
