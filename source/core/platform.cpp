
#include "core/platform.h"

#include <string.h>

#include "audio/audio.h"
#include "audio/sound.h"
#include "core/const.h"
#include "core/core.h"
#include "core/global.hpp"
#include "core/io/filesystem.h"
#include "core/io/packer.h"
#include "core/macros.h"
#include "core/sdl_wrapper.h"
#include "engine/engine.h"
#include "libs/glad/glad.h"
#include "renderer/renderer_gpu.h"

IMPLENGINE();

int ParseRunArgs(int argc, char *argv[]) {

    if (argc > 1) {
        char *v1 = argv[1];
        if (v1) {
            if (!strcmp(v1, "packer")) {
                if (argc <= 3) {
                    printf("Incorrect parameters\n");
                    return METADOT_FAILED;
                }

                pack_result result = packFiles(argv[2], argc - 3, (const char **)argv + 3, true);

                if (result != SUCCESS_PACK_RESULT) {
                    printf("\nError: %s.\n", packResultToString(result));
                    return METADOT_FAILED;
                }

                return RUNNER_EXIT;

            } else if (!strcmp(v1, "unpacker")) {
                if (argc != 3) {
                    printf("Incorrect parameters\n");
                    return METADOT_FAILED;
                }

                pack_result result = unpackFiles(argv[2], true);

                if (result != SUCCESS_PACK_RESULT) {
                    printf("\nError: %s.\n", packResultToString(result));
                    return METADOT_FAILED;
                }
                return RUNNER_EXIT;

            } else if (!strcmp(v1, "packinfo")) {
                if (argc != 3) {
                    printf("Incorrect parameters\n");
                    return METADOT_FAILED;
                }

                uint8_t majorVersion;
                uint8_t minorVersion;
                uint8_t patchVersion;
                bool isLittleEndian;
                uint64_t itemCount;

                pack_result result = getPackInfo(argv[2], &majorVersion, &minorVersion, &patchVersion, &isLittleEndian, &itemCount);

                if (result != SUCCESS_PACK_RESULT) {
                    printf("\nError: %s.\n", packResultToString(result));
                    return METADOT_FAILED;
                }

                printf("MetaDot Pack [v%d.%d.%d]\n"
                       "Pack information:\n"
                       " Version: %d.%d.%d.\n"
                       " Little endian: %s.\n"
                       " Item count: %llu.\n\n",
                       PACK_VERSION_MAJOR, PACK_VERSION_MINOR, PACK_VERSION_PATCH, majorVersion, minorVersion, patchVersion, isLittleEndian ? "true" : "false", (long long unsigned int)itemCount);

                pack_reader packReader;

                result = createFilePackReader(argv[2], 0, false, &packReader);

                if (result != SUCCESS_PACK_RESULT) {
                    printf("\nError: %s.\n", packResultToString(result));
                    return METADOT_FAILED;
                }

                itemCount = getPackItemCount(packReader);

                for (uint64_t i = 0; i < itemCount; ++i) {
                    printf("Item %llu:\n"
                           " Path: %s.\n"
                           " Size: %u.\n",
                           (long long unsigned int)i, getPackItemPath(packReader, i), getPackItemDataSize(packReader, i));
                    fflush(stdout);
                }

                return RUNNER_EXIT;
            }
        } else {
        }
    }

    return METADOT_OK;
}

void metadot_gl_get_max_texture_size(int *w, int *h) {
    int max_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);

    if (w) *w = max_size;
    if (h) *h = max_size;
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

    auto metadot_gl_global_init = [](metadot_gl_loader_fn loader_fp) {
        if (NULL == loader_fp) {
            if (!gladLoadGL()) {
                METADOT_ERROR("GLAD GL3 loader failed");
                return -1;
            }
        } else {
            if (!gladLoadGLLoader((GLADloadproc)loader_fp)) {
                METADOT_ERROR("GLAD GL3 loader failed");
                return -1;
            }
        }
        return 0;
    };

    if (metadot_gl_global_init(NULL) == -1) {
        METADOT_ERROR("Failed to initialize OpenGL loader!");
        return METADOT_FAILED;
    }

#ifdef METADOT_DEBUG
    const unsigned char *vendor = glGetString(GL_VENDOR);
    const unsigned char *renderer = glGetString(GL_RENDERER);
    const unsigned char *gl_version = glGetString(GL_VERSION);
    const unsigned char *glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);

    int tex_w, tex_h;
    metadot_gl_get_max_texture_size(&tex_w, &tex_h);

    METADOT_INFO("OpenGL info:");
    METADOT_INFO("Vendor: %s", vendor);
    METADOT_INFO("Renderer: %s", renderer);
    METADOT_INFO("GL Version: %s", gl_version);
    METADOT_INFO("GLSL Version: %s", glsl_version);
    METADOT_INFO("Max texture size: %ix%i", tex_w, tex_h);
#endif

    global.audio.InitAudio();

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
    if (SDL_GetWindowWMInfo(Core.window, &info)) {
        METADOT_ASSERT_E(IsWindow(info.info.win.window));
        // Core.wndh = info.info.win.window;
    } else {
        // Core.wndh = NULL;
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

    return METADOT_OK;
}

void metadot_endwindow() {

    cs_shutdown();

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
        case engine_windowflashaction::START:
            flash.dwFlags = FLASHW_ALL;
            break;
        case engine_windowflashaction::START_COUNT:
            flash.dwFlags = FLASHW_ALL | FLASHW_TIMER;
            break;
        case engine_windowflashaction::START_UNTIL_FG:
            flash.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
            break;
        case engine_windowflashaction::STOP:
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

void metadot_set_minimize_onlostfocus(bool minimize) { SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, minimize ? "1" : "0"); }

void metadot_set_windowtitle(const char *title) { SDL_SetWindowTitle(Core.window, win_title_server); }

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
