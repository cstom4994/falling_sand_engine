// Copyright(c) 2022, KaoruXun All rights reserved.

#include "engine_platform.h"
#include "core/const.h"
#include "core/core.h"
#include "engine/engine.h"
#include "engine/renderer/renderer_gpu.h"
#include "engine/sdl_wrapper.h"

#include "libs/glad/glad.h"
#include <string.h>

IMPLENGINE();

int ParseRunArgs(int argc, char *argv[]) {

    if (argc > 1) {
        char *v1 = argv[1];
        if (v1) {
            if (!strcmp(v1, "server")) {
                //global.game->GameIsolate_.settings.networkMode = NetworkMode::SERVER;
            }
        } else {
            //global.game->GameIsolate_.settings.networkMode = NetworkMode::HOST;
        }
    }

    return METADOT_OK;
}

int InitWindow() {

    // init sdl
    METADOT_INFO("Initializing SDL...");
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        METADOT_ERROR("SDL_Init failed: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "opengl", SDL_HINT_OVERRIDE);

// Rendering on Catalina with High DPI (retina)
// https://github.com/grimfang4/sdl-gpu/issues/201
#if defined(METADOT_ALLOW_HIGHDPI)
    R_WindowFlagEnum SDL_flags =
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#else
    R_WindowFlagEnum SDL_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#endif

    // create the window
    METADOT_INFO("Creating game window...");

    Core.window = SDL_CreateWindow(win_game, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   Screen.windowWidth, Screen.windowHeight, SDL_flags);

    if (Core.window == R_null) {
        METADOT_ERROR("Could not create SDL_Window: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    // SDL_SetWindowIcon(window, Textures::LoadTexture("data/assets/Icon_32x.png"));

    // create gpu target
    METADOT_INFO("Creating gpu target...");

    R_SetPreInitFlags(R_INIT_DISABLE_VSYNC);
    R_SetInitWindow(SDL_GetWindowID(Core.window));

    Render.target = R_Init(Screen.windowWidth, Screen.windowHeight, SDL_flags);

    if (Render.target == NULL) {
        METADOT_ERROR("Could not create R_Target: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

#if defined(METADOT_ALLOW_HIGHDPI)
    R_SetVirtualResolution(RenderTarget_.target, WIDTH * 2, HEIGHT * 2);
#endif

    Render.realTarget = Render.target;

    Core.glContext = Render.target->context->context;

    SDL_GL_MakeCurrent(Core.window, Core.glContext);

    if (!gladLoadGL()) {
        METADOT_ERROR("Failed to initialize OpenGL loader!");
        return EXIT_FAILURE;
    }

    // METADOT_INFO("Initializing InitFont...");
    // if (!Drawing::InitFont(&gl_context)) {
    //     METADOT_ERROR("InitFont failed");
    //     return EXIT_FAILURE;
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
    //global.HostData.wndh = 0;
#else
#error "GetWindowWMInfo Error"
#endif
    //this->data->window = window;
    //this->data->imgui_context = m_ImGuiCore->getImGuiCtx();

    // MetaEngine::any_function func1{&IamAfuckingNamespace::func1};
    // MetaEngine::any_function func2{&IamAfuckingNamespace::func2};

    // this->data->Functions.insert(std::make_pair("func1", func1));
    // this->data->Functions.insert(std::make_pair("func2", func2));

    // RegisterFunctions(func_log_info, IamAfuckingNamespace::func_log_info);

    // TODO CppScript

    //initThread.get();

    // global.audioEngine.PlayEvent("event:/Music/Title");
    // global.audioEngine.Update();

    return 1;
}

void EndWindow() { R_Quit(); }

void SetDisplayMode(engine_displaymode mode) {
    switch (mode) {
        case WINDOWED:
            SDL_SetWindowDisplayMode(Core.window, NULL);
            SDL_SetWindowFullscreen(Core.window, 0);
            //GameUI::OptionsUI::item_current_idx = 0;
            break;
        case BORDERLESS:
            SDL_SetWindowDisplayMode(Core.window, NULL);
            SDL_SetWindowFullscreen(Core.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            //GameUI::OptionsUI::item_current_idx = 1;
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
            //GameUI::OptionsUI::item_current_idx = 2;
            break;
    }
}

void SetWindowFlash(engine_windowflashaction action, int count, int period) {
    // TODO: look into alternatives for linux/crossplatform
#ifdef _WIN32

    FLASHWINFO flash;
    flash.cbSize = sizeof(FLASHWINFO);
    //flash.hwnd = hwnd;
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

void SetVSync(bool vsync) {
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
    //GameUI::OptionsUI::vsync = vsync;
}

void SetMinimizeOnLostFocus(bool minimize) {
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, minimize ? "1" : "0");
    //GameUI::OptionsUI::minimizeOnFocus = minimize;
}

void SetWindowTitle(const char *title) { SDL_SetWindowTitle(Core.window, win_title_server); }
