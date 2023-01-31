// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine_platform.h"

#include <string.h>

#include "audio/audio.h"
#include "audio/sound.h"
#include "core/const.h"
#include "core/core.h"
#include "core/macros.h"
#include "engine.h"
#include "filesystem.h"
#include "renderer/metadot_gl.h"
#include "renderer/renderer_gpu.h"
#include "renderer/renderer_utils.h"
#include "sdl_wrapper.h"

IMPLENGINE();

int ParseRunArgs(int argc, char *argv[]) {

    if (argc > 1) {
        char *v1 = argv[1];
        if (v1) {
            if (!strcmp(v1, "server")) {
                // global.game->GameIsolate_.settings.networkMode = NetworkMode::SERVER;
            } else {
                // global.game->GameIsolate_.settings.networkMode = NetworkMode::HOST;
            }
        } else {
        }
    }

    return METADOT_OK;
}

int metadot_initwindow() {

    // init sdl
    METADOT_INFO("Initializing SDL...");
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        METADOT_ERROR("SDL_Init failed: %s", SDL_GetError());
        return METADOT_FAILED;
    }

    SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "opengl", SDL_HINT_OVERRIDE);

// Rendering on Catalina with High DPI (retina)
// https://github.com/grimfang4/sdl-gpu/issues/201
#if defined(METADOT_ALLOW_HIGHDPI)
    R_WindowFlagEnum SDL_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#else
    R_WindowFlagEnum SDL_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#endif

    // create the window
    METADOT_INFO("Creating game window...");

    Core.window = SDL_CreateWindow(win_game, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Screen.windowWidth, Screen.windowHeight, SDL_flags);

    if (Core.window == R_null) {
        METADOT_ERROR("Could not create SDL_Window: %s", SDL_GetError());
        return METADOT_FAILED;
    }

    // SDL_SetWindowIcon(window, Textures::LoadTexture("data/assets/Icon_32x.png"));

    // create gpu target
    METADOT_INFO("Creating gpu target...");

    R_SetPreInitFlags(R_INIT_DISABLE_VSYNC);
    R_SetInitWindow(SDL_GetWindowID(Core.window));

    Render.target = R_Init(Screen.windowWidth, Screen.windowHeight, SDL_flags);

    if (Render.target == NULL) {
        METADOT_ERROR("Could not create R_Target: %s", SDL_GetError());
        return METADOT_FAILED;
    }

#if defined(METADOT_ALLOW_HIGHDPI)
    R_SetVirtualResolution(RenderTarget_.target, WIDTH * 2, HEIGHT * 2);
#endif

    Render.realTarget = Render.target;

    Core.glContext = (C_GLContext *)Render.target->context->context;

    SDL_GL_MakeCurrent(Core.window, Core.glContext);

    if (metadot_gl_global_init(NULL) == -1) {
        METADOT_ERROR("Failed to initialize OpenGL loader!");
        return METADOT_FAILED;
    }

#ifdef METADOT_DEBUG
    metadot_gl_print_info();
#endif

    AudioEngineInit();

    // METADOT_INFO("Initializing InitFont...");
    // if (!Drawing::InitFont(&gl_context)) {
    //     METADOT_ERROR("InitFont failed");
    //     return METADOT_FAILED;
    // }

    // if (global.game->GameIsolate_.settings.networkMode != NetworkMode::SERVER) {

    //     font64 = Drawing::LoadFont(METADOT_RESLOC("data/assets/fonts/pixel_operator/PixelOperator.ttf"), 64);
    //     font16 = Drawing::LoadFont(METADOT_RESLOC("data/assets/fonts/pixel_operator/PixelOperator.ttf"), 16);
    //     font14 = Drawing::LoadFont(METADOT_RESLOC("data/assets/fonts/pixel_operator/PixelOperator.ttf"), 14);
    // }

#if defined(_WIN32)
    SDL_SysWMinfo info{};
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info)) {
        METADOT_ASSERT_E(IsWindow(info.info.win.window));
        global.HostData.wndh = info.info.win.window;
    } else {
        global.HostData.wndh = NULL;
    }
#elif defined(__linux)
    global.HostData.wndh = 0;
#elif defined(__APPLE__)
    // global.HostData.wndh = 0;
#else
#error "GetWindowWMInfo Error"
#endif
    // this->data->window = window;
    // this->data->imgui_context = m_ImGuiCore->getImGuiCtx();

    // MetaEngine::AnyFunction func1{&IamAfuckingNamespace::func1};
    // MetaEngine::AnyFunction func2{&IamAfuckingNamespace::func2};

    // this->data->Functions.insert(std::make_pair("func1", func1));
    // this->data->Functions.insert(std::make_pair("func2", func2));

    // RegisterFunctions(func_log_info, IamAfuckingNamespace::func_log_info);

    // TODO CppScript

    // initThread.get();

    // global.audioEngine.PlayEvent("event:/Music/Title");
    // global.audioEngine.Update();

    return METADOT_OK;
}

void metadot_endwindow() {

    cs_shutdown();
    metadot_fs_destroy();

    if (NULL != Render.target) R_FreeTarget(Render.target);
    // if (Render.realTarget) R_FreeTarget(Render.realTarget);
    if (Core.window) SDL_DestroyWindow(Core.window);
    R_Quit();
}

void metadot_set_displaymode(engine_displaymode mode) {
    switch (mode) {
        case WINDOWED:
            SDL_SetWindowDisplayMode(Core.window, NULL);
            SDL_SetWindowFullscreen(Core.window, 0);
            // GameUI::OptionsUI::item_current_idx = 0;
            break;
        case BORDERLESS:
            SDL_SetWindowDisplayMode(Core.window, NULL);
            SDL_SetWindowFullscreen(Core.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            // GameUI::OptionsUI::item_current_idx = 1;
            break;
        case FULLSCREEN:
            SDL_MaximizeWindow(Core.window);

            int w;
            int h;
            SDL_GetWindowSize(Core.window, &w, &h);

            SDL_DisplayMode disp;
            SDL_GetWindowDisplayMode(Core.window, &disp);

            disp.w = w;
            disp.h = h;

            SDL_SetWindowDisplayMode(Core.window, &disp);
            SDL_SetWindowFullscreen(Core.window, SDL_WINDOW_FULLSCREEN);
            // GameUI::OptionsUI::item_current_idx = 2;
            break;
    }
}

void metadot_set_windowflash(engine_windowflashaction action, int count, int period) {
    // TODO: look into alternatives for linux/crossplatform
#ifdef METADOT_PLATFORM_WINDOWS

    FLASHWINFO flash;
    flash.cbSize = sizeof(FLASHWINFO);
    // flash.hwnd = hwnd;
    flash.uCount = count;
    flash.dwTimeout = period;

    // pretty sure these flags are supposed to work but they all seem to do the same thing on my machine so idk
    switch (action) {
        case WindowFlashAction::START:
            flash.dwFlags = FLASHW_ALL;
            break;
        case WindowFlashAction::START_COUNT:
            flash.dwFlags = FLASHW_ALL | FLASHW_TIMER;
            break;
        case WindowFlashAction::START_UNTIL_FG:
            flash.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
            break;
        case WindowFlashAction::STOP:
            flash.dwFlags = FLASHW_STOP;
            break;
    }

    FlashWindowEx(&flash);

#endif
}

void metadot_set_VSync(bool vsync) {
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
    // GameUI::OptionsUI::vsync = vsync;
}

void metadot_set_minimize_onlostfocus(bool minimize) {
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, minimize ? "1" : "0");
    // GameUI::OptionsUI::minimizeOnFocus = minimize;
}

void metadot_set_windowtitle(const char *title) { SDL_SetWindowTitle(Core.window, win_title_server); }

R_vec2 metadot_get_mousepos() {
    I32 x, y;
    SDL_GetMouseState(&x, &y);
    return (R_vec2){.x = (float)x, .y = (float)y};
}

char *metadot_clipboard_get() {
    char *text = SDL_GetClipboardText();
    return text;
}

METAENGINE_Result metadot_clipboard_set(const char *string) {
    int ret = SDL_SetClipboardText(string);
    if (ret)
        return metadot_result_error("Unable to set clipboard data.");
    else
        return metadot_result_success();
}
