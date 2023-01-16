// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_ENGINE_H_
#define _METADOT_ENGINE_H_

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__cplusplus)
extern "C" {
#endif

#include "engine_core.h"
#include "engine_ecs.h"
#include "engine_meta.h"
#include "engine_platform.h"
#include "engine_render.h"
#include "engine_scripting.h"
#include "util.h"

#define IMPLENGINE()             \
    extern engine_core Core;     \
    extern engine_render Render; \
    extern engine_screen Screen; \
    extern engine_ecs ECS;       \
    extern engine_time Time

// Engine functions called from main
int InitEngine(void (*InitCppReflection)());
void EngineUpdate();
void EngineUpdateEnd();
void EndEngine(int errorOcurred);

#if defined(__cplusplus)
}
#endif

#endif