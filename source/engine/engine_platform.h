// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_PLATFORM_H_
#define _METADOT_PLATFORM_H_

#include "core/core.h"
#include "core/macros.h"
#include "core/platform.h"
#include "core/stl.h"
#include "renderer/renderer_utils.h"
#include "core/sdl_wrapper.h"

typedef enum engine_displaymode { WINDOWED, BORDERLESS, FULLSCREEN } engine_displaymode;

typedef enum engine_windowflashaction { START, START_COUNT, START_UNTIL_FG, STOP } engine_windowflashaction;

typedef struct engine_platform {

} engine_platform;

#define RUNNER_EXIT 2

int ParseRunArgs(int argc, char* argv[]);
int metadot_initwindow();
void metadot_endwindow();
void metadot_set_displaymode(engine_displaymode mode);
void metadot_set_windowflash(engine_windowflashaction action, int count, int period);
void metadot_set_VSync(bool vsync);
void metadot_set_minimize_onlostfocus(bool minimize);
void metadot_set_windowtitle(const char* title);
void metadot_get_mousepos(int* x, int* y);
char* metadot_clipboard_get();
METAENGINE_Result metadot_clipboard_set(const char* string);

#endif
