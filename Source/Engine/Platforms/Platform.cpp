// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Platform.hpp"
#include "Engine/Memory/Memory.hpp"
#include "Engine/Render/RendererGPU.h"
#include "Core/Const.hpp"
#include "Core/Global.hpp"
#include "Game/Legacy/Game.hpp"
#include "Game/Legacy/GameUI.hpp"
#include "Game/Legacy/Networking.hpp"
#include "Game/Settings.hpp"
#include "Libs/structopt.hpp"

#include "glew.h"

struct Options
{
    std::optional<std::string> test;
    std::optional<std::string> networkmode;
    std::vector<std::string> files;
};
STRUCTOPT(Options, test, files);

int Platform::ParseRunArgs(int argc, char *argv[]) {
    try {
        auto options = structopt::app(METADOT_NAME).parse<Options>(argc, argv);

        if (!options.networkmode.value_or("").empty()) {
            if (options.networkmode == "server") Settings::networkMode = NetworkMode::SERVER;
        } else {
            Settings::networkMode = NetworkMode::HOST;
        }

        if (!options.test.value_or("").empty()) {
            if (options.test == "test_") {
                return 0;
            }
        }

    } catch (structopt::exception &e) {
        std::cout << e.what() << "\n";
        std::cout << e.help();
    }
    return 0;
}

int Platform::InitWindow() {

    // init sdl
    METADOT_INFO("Initializing SDL...");
    UInt32 sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
    if (SDL_Init(sdl_init_flags) < 0) {
        METADOT_ERROR("SDL_Init failed: {0}", SDL_GetError());
        return EXIT_FAILURE;
    }

    SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "opengl", SDL_HINT_OVERRIDE);

// Rendering on Catalina with High DPI (retina)
// https://github.com/grimfang4/sdl-gpu/issues/201
#if defined(METADOT_ALLOW_HIGHDPI)
    METAENGINE_Render_WindowFlagEnum SDL_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
#else
    METAENGINE_Render_WindowFlagEnum SDL_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#endif

    if (Settings::networkMode != NetworkMode::SERVER) {
        // create the window
        METADOT_INFO("Creating game window...");

        auto title = Utils::Format("{0} Build {1} - {2}", win_title_client, __DATE__, __TIME__);

        global.platform.window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_flags);

        if (global.platform.window == nullptr) {
            METADOT_ERROR("Could not create SDL_Window: {0}", SDL_GetError());
            return EXIT_FAILURE;
        }

        // SDL_SetWindowIcon(window, Textures::loadTexture("data/assets/Icon_32x.png"));

        // create gpu target
        METADOT_INFO("Creating gpu target...");

        METAENGINE_Render_SetPreInitFlags(METAENGINE_Render_INIT_DISABLE_VSYNC);
        METAENGINE_Render_SetInitWindow(SDL_GetWindowID(global.platform.window));

        global.game->RenderTarget_.target = METAENGINE_Render_Init(WIDTH, HEIGHT, SDL_flags);

        if (global.game->RenderTarget_.target == NULL) {
            METADOT_ERROR("Could not create METAENGINE_Render_Target: {0}", SDL_GetError());
            return EXIT_FAILURE;
        }

#if defined(METADOT_ALLOW_HIGHDPI)
        METAENGINE_Render_SetVirtualResolution(RenderTarget_.target, WIDTH * 2, HEIGHT * 2);
#endif

        global.game->RenderTarget_.realTarget = global.game->RenderTarget_.target;

        C_GLContext &gl_context = global.game->RenderTarget_.target->context->context;


        SDL_GL_MakeCurrent(global.platform.window, gl_context);

        if (GLEW_OK != glewInit()) {
            std::cout << "Failed to initialize OpenGL loader!" << std::endl;
            return EXIT_FAILURE;
        }


        // METADOT_INFO("Initializing InitFont...");
        // if (!Drawing::InitFont(&gl_context)) {
        //     METADOT_ERROR("InitFont failed");
        //     return EXIT_FAILURE;
        // }

        // if (Settings::networkMode != NetworkMode::SERVER) {

        //     font64 = Drawing::LoadFont(METADOT_RESLOC_STR("data/assets/fonts/pixel_operator/PixelOperator.ttf"), 64);
        //     font16 = Drawing::LoadFont(METADOT_RESLOC_STR("data/assets/fonts/pixel_operator/PixelOperator.ttf"), 16);
        //     font14 = Drawing::LoadFont(METADOT_RESLOC_STR("data/assets/fonts/pixel_operator/PixelOperator.ttf"), 14);
        // }

        // load splash screen
        METADOT_INFO("Loading splash screen...");

        METAENGINE_Render_Clear(global.game->RenderTarget_.target);
        METAENGINE_Render_Flip(global.game->RenderTarget_.target);
        C_Surface *splashSurf = Textures::loadTexture("data/assets/title/splash.png");
        METAENGINE_Render_Image *splashImg = METAENGINE_Render_CopyImageFromSurface(splashSurf);
        METAENGINE_Render_SetImageFilter(splashImg, METAENGINE_Render_FILTER_NEAREST);
        METAENGINE_Render_BlitRect(splashImg, NULL, global.game->RenderTarget_.target, NULL);
        METAENGINE_Render_FreeImage(splashImg);
        SDL_FreeSurface(splashSurf);
        METAENGINE_Render_Flip(global.game->RenderTarget_.target);

        // load key bind
        Controls::initKey();

        METADOT_INFO("Loading ImGUI");
        METADOT_NEW(C, global.ImGuiLayer, ImGuiLayer);
        global.ImGuiLayer->Init(global.platform.window, gl_context);

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
        //this->data->imgui_context = m_ImGuiLayer->getImGuiCtx();

        // MetaEngine::any_function func1{&IamAfuckingNamespace::func1};
        // MetaEngine::any_function func2{&IamAfuckingNamespace::func2};

        // this->data->Functions.insert(std::make_pair("func1", func1));
        // this->data->Functions.insert(std::make_pair("func2", func2));

        // RegisterFunctions(func_log_info, IamAfuckingNamespace::func_log_info);

        // TODO CppScript

        //initThread.get();

        // global.audioEngine.PlayEvent("event:/Music/Title");
        // global.audioEngine.Update();
    }

    return 0;
}

void Platform::SetDisplayMode(DisplayMode mode) {
    switch (mode) {
        case DisplayMode::WINDOWED:
            SDL_SetWindowDisplayMode(global.platform.window, NULL);
            SDL_SetWindowFullscreen(global.platform.window, 0);
            GameUI::OptionsUI::item_current_idx = 0;
            break;
        case DisplayMode::BORDERLESS:
            SDL_SetWindowDisplayMode(global.platform.window, NULL);
            SDL_SetWindowFullscreen(global.platform.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            GameUI::OptionsUI::item_current_idx = 1;
            break;
        case DisplayMode::FULLSCREEN:
            SDL_MaximizeWindow(global.platform.window);

            int w;
            int h;
            SDL_GetWindowSize(global.platform.window, &w, &h);

            SDL_DisplayMode disp;
            SDL_GetWindowDisplayMode(global.platform.window, &disp);

            disp.w = w;
            disp.h = h;

            SDL_SetWindowDisplayMode(global.platform.window, &disp);
            SDL_SetWindowFullscreen(global.platform.window, SDL_WINDOW_FULLSCREEN);
            GameUI::OptionsUI::item_current_idx = 2;
            break;
    }

    int w;
    int h;
    SDL_GetWindowSize(global.platform.window, &w, &h);

    METAENGINE_Render_SetWindowResolution(w, h);
    METAENGINE_Render_ResetProjection(global.game->RenderTarget_.realTarget);

    HandleWindowSizeChange(w, h);
}

void Platform::HandleWindowSizeChange(int newWidth, int newHeight) {
    SDL_ShowCursor(Settings::draw_cursor ? SDL_ENABLE : SDL_DISABLE);
    //ImGui::SetMouseCursor(Settings::draw_cursor ? ImGuiMouseCursor_Arrow : ImGuiMouseCursor_None);

    int prevWidth = global.platform.WIDTH;
    int prevHeight = global.platform.HEIGHT;

    global.platform.WIDTH = newWidth;
    global.platform.HEIGHT = newHeight;

    global.game->createTexture();

    global.game->accLoadX -= (newWidth - prevWidth) / 2.0f / global.game->scale;
    global.game->accLoadY -= (newHeight - prevHeight) / 2.0f / global.game->scale;

    METADOT_INFO("Ticking chunk...");
    global.game->tickChunkLoading();
    METADOT_INFO("Ticking chunk done");

    for (int x = 0; x < global.game->getWorld()->width; x++) {
        for (int y = 0; y < global.game->getWorld()->height; y++) {
            global.game->getWorld()->dirty[x + y * global.game->getWorld()->width] = true;
            global.game->getWorld()->layer2Dirty[x + y * global.game->getWorld()->width] = true;
            global.game->getWorld()->backgroundDirty[x + y * global.game->getWorld()->width] = true;
        }
    }
}

void Platform::SetWindowFlash(WindowFlashAction action, int count, int period) {
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

void Platform::SetVSync(bool vsync) {
    SDL_GL_SetSwapInterval(vsync ? 1 : 0);
    GameUI::OptionsUI::vsync = vsync;
}

void Platform::SetMinimizeOnLostFocus(bool minimize) {
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, minimize ? "1" : "0");
    GameUI::OptionsUI::minimizeOnFocus = minimize;
}