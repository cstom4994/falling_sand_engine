// Copyright(c) 2022, KaoruXun All rights reserved.

#include "engine_core.h"

engineCore Core;

int exitGame = 0;

void ExitGame() { exitGame = 1; }

int GameExited() { return exitGame; }

int InitCore() {
    exitGame = 0;
    return 1;
}

void InitTime() {}

void InitScreen(int windowWidth, int windowHeight, int scale, int maxFPS) {}

void UpdateTime() {}

void WaitUntilNextFrame() {}
