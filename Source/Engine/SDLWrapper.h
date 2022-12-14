// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_SDLWRAPPER_H_
#define _METADOT_SDLWRAPPER_H_

// SDL 2.0.5+
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_platform.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>

#if SDL_VERSION_ATLEAST(2, 0, 4) && !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) &&           \
        !(defined(__APPLE__) && TARGET_OS_IOS) && !defined(__amigaos4__)
#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE 1
#else
#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE 0
#endif
#define SDL_HAS_WINDOW_ALPHA SDL_VERSION_ATLEAST(2, 0, 5)
#define SDL_HAS_ALWAYS_ON_TOP SDL_VERSION_ATLEAST(2, 0, 5)
#define SDL_HAS_USABLE_DISPLAY_BOUNDS SDL_VERSION_ATLEAST(2, 0, 5)
#define SDL_HAS_PER_MONITOR_DPI SDL_VERSION_ATLEAST(2, 0, 4)
#define SDL_HAS_VULKAN SDL_VERSION_ATLEAST(2, 0, 6)
#if !SDL_HAS_VULKAN
static const Uint32 SDL_WINDOW_VULKAN = 0x10000000;
#endif

typedef SDL_Surface C_Surface;
typedef SDL_Window C_Window;
typedef SDL_Renderer C_Renderer;
typedef SDL_Rect C_Rect;
typedef SDL_Cursor C_Cursor;
typedef SDL_GLContext C_GLContext;

#endif