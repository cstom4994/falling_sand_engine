// Copyright(c) 2022, KaoruXun All rights reserved.

#include "engine_core.h"

engine_core Core;
engine_screen Screen;

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
