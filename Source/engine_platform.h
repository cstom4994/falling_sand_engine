// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_PLATFORM_H_
#define _METADOT_PLATFORM_H_

#include "core/core.h"
#include "core/macros.h"
#include "platform_detail.h"
#include "renderer/renderer_utils.h"
#include "sdl_wrapper.h"

typedef enum engine_displaymode { WINDOWED, BORDERLESS, FULLSCREEN } engine_displaymode;

typedef enum engine_windowflashaction { START, START_COUNT, START_UNTIL_FG, STOP } engine_windowflashaction;

typedef struct engine_platform {

} engine_platform;

int ParseRunArgs(int argc, char* argv[]);
int InitWindow();
void EndWindow();
void SetDisplayMode(engine_displaymode mode);
void SetWindowFlash(engine_windowflashaction action, int count, int period);
void SetVSync(bool vsync);
void SetMinimizeOnLostFocus(bool minimize);
void SetWindowTitle(const char* title);
R_vec2 GetMousePos();

#endif