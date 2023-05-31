// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_ENGINE_H
#define ME_ENGINE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/cpp/utils.hpp"
#include "core/platform.h"
#include "core/utils/utility.hpp"
#include "engine/engine_core.h"
#include "renderer/gpu.hpp"

#define IMPLENGINE()             \
    extern engine_core Core;     \
    extern engine_render Render; \
    extern windows Screen;       \
    extern engine_time Time

// Engine functions called from main
int InitEngine(void (*InitCppReflection)());
void EngineUpdate();
void EngineUpdateEnd();
void EndEngine(int errorOcurred);
void DrawSplash();

#endif