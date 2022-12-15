// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_PLATFORM_H_
#define _METADOT_PLATFORM_H_

#include "core/core.h"
#include "core/macros.h"

#include "platform_detail.h"
#include "sdl_wrapper.h"

typedef enum engine_displaymode {
    WINDOWED,
    BORDERLESS,
    FULLSCREEN
} engine_displaymode;

typedef enum engine_windowflashaction {
    START,
    START_COUNT,
    START_UNTIL_FG,
    STOP
} engine_windowflashaction;

typedef struct engine_platform
{

} engine_platform;

typedef U32 ticks;

int ParseRunArgs(int argc, char *argv[]);
int InitWindow();
void EndWindow();
void SetDisplayMode(engine_displaymode mode);
void SetWindowFlash(engine_windowflashaction action, int count, int period);
void SetVSync(bool vsync);
void SetMinimizeOnLostFocus(bool minimize);
inline ticks GetTime() { return (ticks) SDL_GetTicks(); }

#endif
