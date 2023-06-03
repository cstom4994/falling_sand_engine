﻿// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "game.hpp"

#include <stdio.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <regex>
#include <string>
#include <string_view>

#include "background.hpp"
#include "engine/core/const.h"
#include "engine/core/core.hpp"
#include "engine/core/cpp/promise.hpp"
#include "engine/core/cpp/utils.hpp"
#include "engine/core/debug.hpp"
#include "engine/core/global.hpp"
#include "engine/core/io/filesystem.h"
#include "engine/core/job.h"
#include "engine/core/macros.hpp"
#include "engine/core/mathlib.hpp"
#include "engine/core/platform.h"
#include "engine/core/profiler.hpp"
#include "engine/core/sdl_wrapper.h"
#include "engine/core/utils/utility.hpp"
#include "engine/engine.h"
#include "engine/event/applicationevent.hpp"
#include "engine/game_utils/cells.h"
#include "engine/game_utils/mdplot.h"
#include "engine/meta/reflection.hpp"
#include "engine/renderer/gpu.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/scripting.hpp"
#include "game_basic.hpp"
#include "game_datastruct.hpp"
#include "game_resources.hpp"
#include "game_shaders.hpp"
#include "game_ui.hpp"
#include "libs/glad/glad.h"
#include "libs/imgui/imgui.h"
#include "reflectionflat.hpp"
#include "world_generator.cpp"

Global global;

IMPLENGINE();

Game::Game(int argc, char *argv[]) {
    // Initialize promise handle
    MetaEngine::Promise::handleUncaughtException(
            [](MetaEngine::Promise::Promise &d) { d.fail([](long n, int m) { METADOT_BUG("UncaughtException parameters = %d %d", (int)n, m); }).fail([]() { METADOT_ERROR("UncaughtException"); }); });

    // Start memory management including GC
    ME_mem_init(argc, argv);

    ME_job::init();

    ME_profiler_init();
    ME_profiler_register_thread("Application thread");

    // Global game target
    global.game = this;
    METADOT_INFO(std::format("{0} {1}", METADOT_NAME, METADOT_VERSION_TEXT).c_str());
}

Game::~Game() {
    global.game = nullptr;
    ME_profiler_shutdown();

    // Stop memory setting and GC
    ME_mem_end();

#if defined(ME_LEAK_TEST)
    ME_get_leaks_info();
#endif
}

int Game::init(int argc, char *argv[]) {
    // Parse args
    int ret = ParseRunArgs(argc, argv);
    if (ret == METADOT_FAILED) return METADOT_FAILED;
    if (ret == RUNNER_EXIT) return METADOT_OK;

    METADOT_INFO("Starting game...");

    // Initialization of ECSSystem and Engine
    if (InitEngine(init_reflection)) return METADOT_FAILED;

    // Load splash screen
    DrawSplash();

    setEventCallback(ME_BIND_EVENT_FN(onEvent));

    // Initialize Gameplay script system before scripting system initialization
    METADOT_INFO("Loading gameplay script...");

    GameIsolate_.gameplayscript = ME::create_ref<GameplayScriptSystem>(2);
    GameIsolate_.systemList.push_back(GameIsolate_.gameplayscript);

    GameIsolate_.ui = ME::create_ref<UISystem>(4);
    GameIsolate_.systemList.push_back(GameIsolate_.ui);

    GameIsolate_.shaderworker = ME::create_ref<ShaderWorkerSystem>(6, SystemFlags::SystemFlags_Render);
    GameIsolate_.systemList.push_back(GameIsolate_.shaderworker);

    GameIsolate_.backgrounds = ME::create_ref<BackgroundSystem>(8);
    GameIsolate_.systemList.push_back(GameIsolate_.backgrounds);

    // Initialize scripting system
    METADOT_INFO("Loading Script...");
    Scripting::get_singleton_ptr()->Init();

    for (auto &s : GameIsolate_.systemList) {
        s->RegisterLua(Scripting::get_singleton_ptr()->Lua->s_lua);
        s->Create();
        // if (!s->getFlag(SystemFlags::SystemFlags_ImGui)) {
        //     s->Create();
        // }
    }

    // Test aseprite
    GameIsolate_.texturepack->testAse = LoadAsepriteTexture("data/assets/textures/Sprite-0003.ase");

    ME_pack_result pack_result = ME_create_file_pack_reader("data/resources.pack", 0, 0, &GameIsolate_.pack_reader);

    if (pack_result != SUCCESS_PACK_RESULT) {
        METADOT_ERROR("%d", pack_result);
        ME_ASSERT_E(0);
    }

    // Initialize the rng seed
    this->RNG = RNG_Create();
    METADOT_INFO(std::format("SeedRNG {0}", RNG->root_seed).c_str());

    // register & set up materials
    METADOT_INFO("Setting up materials...");
    movingTiles = new u16[global.GameData_.materials_count];
    debugDraw = new DebugDraw(Render.target);

    global.audio.LoadEvent("event:/World/Explode", "explode.ogg");
    global.audio.LoadEvent("event:/Music/Title", "title.ogg");

    // Play sound effects when the game starts
    global.audio.PlayEvent("event:/Music/Title");
    // global.audioEngine.Update();

    // Initialize the world
    METADOT_INFO("Initializing world...");
    GameIsolate_.world = ME::create_scope<World>();
    GameIsolate_.world->noSaveLoad = true;
    GameIsolate_.world->init(METADOT_RESLOC("saves/mainMenu"), (int)ceil(WINDOWS_MAX_WIDTH / RENDER_C_TEST / (f64)CHUNK_W) * CHUNK_W + CHUNK_W * RENDER_C_TEST,
                             (int)ceil(WINDOWS_MAX_HEIGHT / RENDER_C_TEST / (f64)CHUNK_H) * CHUNK_H + CHUNK_H * RENDER_C_TEST, Render.target, &global.audio);

    // set up main menu ui

    METADOT_INFO("Setting up main menu...");
    std::string displayMode = "windowed";

    if (displayMode == "windowed") {
        metadot_set_displaymode(engine_displaymode::WINDOWED);
    } else if (displayMode == "borderless") {
        metadot_set_displaymode(engine_displaymode::BORDERLESS);
    } else if (displayMode == "fullscreen") {
        metadot_set_displaymode(engine_displaymode::FULLSCREEN);
    }

    int w;
    int h;
    SDL_GetWindowSize(Core.window, &w, &h);
    R_SetWindowResolution(w, h);
    R_ResetProjection(Render.realTarget);
    ResolutionChanged(w, h);

    metadot_set_VSync(false);
    metadot_set_minimize_onlostfocus(false);

    fontcache.resize({(float)w, (float)h});

    fontcache.ME_fontcache_init();

    auto ui_font = get_assets(".\\fonts\\fusion-pixel.ttf");
    fontcache.ME_fontcache_load(ui_font.data, ui_font.size);

    // init threadpools
    GameIsolate_.updateDirtyPool = new ME::thread_pool(4);
    GameIsolate_.updateDirtyPool2 = new ME::thread_pool(2);

    return this->run(argc, argv);
}

void Game::createTexture() {

    METADOT_LOG_SCOPE_FUNCTION(INFO);
    METADOT_INFO("Creating world textures...");

    if (TexturePack_.loadingTexture) {
        R_FreeImage(TexturePack_.loadingTexture);
        R_FreeImage(TexturePack_.texture);
        R_FreeImage(TexturePack_.worldTexture);
        R_FreeImage(TexturePack_.lightingTexture);
        R_FreeImage(TexturePack_.emissionTexture);
        R_FreeImage(TexturePack_.textureFire);
        R_FreeImage(TexturePack_.textureFlow);
        R_FreeImage(TexturePack_.texture2Fire);
        R_FreeImage(TexturePack_.textureLayer2);
        R_FreeImage(TexturePack_.textureBackground);
        R_FreeImage(TexturePack_.textureObjects);
        R_FreeImage(TexturePack_.textureObjectsLQ);
        R_FreeImage(TexturePack_.textureObjectsBack);
        R_FreeImage(TexturePack_.textureCells);
        R_FreeImage(TexturePack_.textureEntities);
        R_FreeImage(TexturePack_.textureEntitiesLQ);
        R_FreeImage(TexturePack_.temperatureMap);
        R_FreeImage(TexturePack_.backgroundImage);
    }

    // create textures
    loadingOnColor = 0xFFFFFFFF;
    loadingOffColor = 0x000000FF;

    std::vector<MetaEngine::Promise::Promise> Funcs = {
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "loadingTexture");
                TexturePack_.loadingTexture =
                        R_CreateImage(TexturePack_.loadingScreenW = (Screen.windowWidth / 20), TexturePack_.loadingScreenH = (Screen.windowHeight / 20), R_FormatEnum::R_FORMAT_RGBA);

                R_SetImageFilter(TexturePack_.loadingTexture, R_FILTER_NEAREST);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "texture");
                TexturePack_.texture = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);

                R_SetImageFilter(TexturePack_.texture, R_FILTER_NEAREST);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "worldTexture");
                TexturePack_.worldTexture = R_CreateImage(GameIsolate_.world->width * GameIsolate_.globaldef.hd_objects_size, GameIsolate_.world->height * GameIsolate_.globaldef.hd_objects_size,
                                                          R_FormatEnum::R_FORMAT_RGBA);

                R_SetImageFilter(TexturePack_.worldTexture, R_FILTER_NEAREST);

                R_LoadTarget(TexturePack_.worldTexture);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "lightingTexture");
                TexturePack_.lightingTexture = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.lightingTexture, R_FILTER_NEAREST);
                R_LoadTarget(TexturePack_.lightingTexture);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "emissionTexture");
                TexturePack_.emissionTexture = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.emissionTexture, R_FILTER_NEAREST);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "textureFlow");
                TexturePack_.textureFlow = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureFlow, R_FILTER_NEAREST);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "textureFlowSpead");
                TexturePack_.textureFlowSpead = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureFlowSpead, R_FILTER_NEAREST);
                R_LoadTarget(TexturePack_.textureFlowSpead);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "textureFire");
                TexturePack_.textureFire = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureFire, R_FILTER_NEAREST);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "texture2Fire");
                TexturePack_.texture2Fire = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.texture2Fire, R_FILTER_NEAREST);
                R_LoadTarget(TexturePack_.texture2Fire);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "textureLayer2");
                TexturePack_.textureLayer2 = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureLayer2, R_FILTER_NEAREST);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "textureBackground");
                TexturePack_.textureBackground = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureBackground, R_FILTER_NEAREST);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "textureObjects");
                TexturePack_.textureObjects = R_CreateImage(GameIsolate_.world->width * (GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1),
                                                            GameIsolate_.world->height * (GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1), R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureObjects, R_FILTER_NEAREST);
                R_LoadTarget(TexturePack_.textureObjects);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "textureObjectsLQ");
                TexturePack_.textureObjectsLQ = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureObjectsLQ, R_FILTER_NEAREST);
                R_LoadTarget(TexturePack_.textureObjectsLQ);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "textureObjectsBack");
                TexturePack_.textureObjectsBack =
                        R_CreateImage(GameIsolate_.world->width * (GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1),
                                      GameIsolate_.world->height * (GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1), R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureObjectsBack, R_FILTER_NEAREST);
                R_LoadTarget(TexturePack_.textureObjectsBack);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "textureCells");
                TexturePack_.textureCells = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);

                R_SetImageFilter(TexturePack_.textureCells, R_FILTER_NEAREST);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "textureEntities");
                TexturePack_.textureEntities =
                        R_CreateImage(GameIsolate_.world->width * (GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1),
                                      GameIsolate_.world->height * (GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1), R_FormatEnum::R_FORMAT_RGBA);

                R_LoadTarget(TexturePack_.textureEntities);

                R_SetImageFilter(TexturePack_.textureEntities, R_FILTER_NEAREST);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "textureEntitiesLQ");
                TexturePack_.textureEntitiesLQ = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);

                R_LoadTarget(TexturePack_.textureEntitiesLQ);

                R_SetImageFilter(TexturePack_.textureEntitiesLQ, R_FILTER_NEAREST);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "temperatureMap");
                TexturePack_.temperatureMap = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);

                R_SetImageFilter(TexturePack_.temperatureMap, R_FILTER_NEAREST);
            }),
            MetaEngine::Promise::newPromise([&](MetaEngine::Promise::Defer d) {
                METADOT_LOG_SCOPE_F(INFO, "backgroundImage");
                TexturePack_.backgroundImage = R_CreateImage(Screen.windowWidth, Screen.windowHeight, R_FormatEnum::R_FORMAT_RGBA);

                R_SetImageFilter(TexturePack_.backgroundImage, R_FILTER_NEAREST);

                R_LoadTarget(TexturePack_.backgroundImage);
            })};

    MetaEngine::Promise::all(Funcs);

    auto init_pixels = [&]() {
        // create texture pixel buffers
        TexturePack_.pixels = std::vector<u8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
        TexturePack_.pixels_ar = &TexturePack_.pixels[0];

        TexturePack_.pixelsLayer2 = std::vector<u8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
        TexturePack_.pixelsLayer2_ar = &TexturePack_.pixelsLayer2[0];

        TexturePack_.pixelsBackground = std::vector<u8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
        TexturePack_.pixelsBackground_ar = &TexturePack_.pixelsBackground[0];

        TexturePack_.pixelsObjects = std::vector<u8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, ME_ALPHA_TRANSPARENT);
        TexturePack_.pixelsObjects_ar = &TexturePack_.pixelsObjects[0];

        TexturePack_.pixelsTemp = std::vector<u8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, ME_ALPHA_TRANSPARENT);
        TexturePack_.pixelsTemp_ar = &TexturePack_.pixelsTemp[0];

        TexturePack_.pixelsCells = std::vector<u8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, ME_ALPHA_TRANSPARENT);
        TexturePack_.pixelsCells_ar = &TexturePack_.pixelsCells[0];

        TexturePack_.pixelsLoading = std::vector<u8>(TexturePack_.loadingTexture->w * TexturePack_.loadingTexture->h * 4, ME_ALPHA_TRANSPARENT);
        TexturePack_.pixelsLoading_ar = &TexturePack_.pixelsLoading[0];

        TexturePack_.pixelsFire = std::vector<u8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
        TexturePack_.pixelsFire_ar = &TexturePack_.pixelsFire[0];

        TexturePack_.pixelsFlow = std::vector<u8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
        TexturePack_.pixelsFlow_ar = &TexturePack_.pixelsFlow[0];

        TexturePack_.pixelsEmission = std::vector<u8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
        TexturePack_.pixelsEmission_ar = &TexturePack_.pixelsEmission[0];
    };

    init_pixels();

    METADOT_INFO("Creating world textures done");
}

int Game::run(int argc, char *argv[]) {

    // start loading chunks
    METADOT_INFO("Queueing chunk loading...");
    for (int x = -CHUNK_W * 4; x < GameIsolate_.world->width + CHUNK_W * 4; x += CHUNK_W) {
        for (int y = -CHUNK_H * 3; y < GameIsolate_.world->height + CHUNK_H * 8; y += CHUNK_H) {
            GameIsolate_.world->queueLoadChunk(x / CHUNK_W, y / CHUNK_H, true, true);
        }
    }

    // start game loop
    METADOT_INFO("Starting game loop...");
    global.GameData_.freeCamX = GameIsolate_.world->width / 2.0f - CHUNK_W / 2.0f;
    global.GameData_.freeCamY = GameIsolate_.world->height / 2.0f - (int)(CHUNK_H * 0.75);
    if (GameIsolate_.world->player) {
        // global.GameData_.plPosX = pl_we->x;
        // global.GameData_.plPosY = pl_we->y;
    } else {
        global.GameData_.plPosX = global.GameData_.freeCamX;
        global.GameData_.plPosY = global.GameData_.freeCamY;
    }

    SDL_Event windowEvent;

    Screen.gameScale = 3;
    global.GameData_.ofsX = (int)(-CHUNK_W * 4);
    global.GameData_.ofsY = (int)(-CHUNK_H * 2.5);

    global.GameData_.ofsX = (global.GameData_.ofsX - Screen.windowWidth / 2) / 2 * 3 + Screen.windowWidth / 2;
    global.GameData_.ofsY = (global.GameData_.ofsY - Screen.windowHeight / 2) / 2 * 3 + Screen.windowHeight / 2;

    InitFPS();
    objectDelete = new u8[GameIsolate_.world->width * GameIsolate_.world->height];

    fadeInStart = ME_gettime();
    fadeInLength = 250;
    fadeInWaitFrames = 5;

    // game loop
    while (this->running) {

        ME_profiler_begin_frame();

        EngineUpdate();

#pragma region SDL_Input

        ME_profiler_scope_auto("Loop");

        // handle window events
        while (SDL_PollEvent(&windowEvent)) {

            if (windowEvent.type == SDL_WINDOWEVENT) {
                if (windowEvent.window.event == SDL_WINDOWEVENT_RESIZED) {
                    MetaEngine::WindowResizeEvent event(windowEvent.window.data1, windowEvent.window.data2);
                    EventCallback(event);
                }
            }

            ImGui_ImplSDL2_ProcessEvent(&windowEvent);

            if (ImGui::GetIO().WantCaptureMouse && ImGui::GetIO().WantCaptureKeyboard) {
                if (windowEvent.type == SDL_MOUSEBUTTONDOWN || windowEvent.type == SDL_MOUSEMOTION || windowEvent.type == SDL_MOUSEWHEEL || windowEvent.type == SDL_KEYDOWN ||
                    windowEvent.type == SDL_KEYUP) {
                    continue;
                }
            }

            if (windowEvent.type == SDL_MOUSEWHEEL) {

            } else if (windowEvent.type == SDL_MOUSEMOTION) {

                ControlSystem::mouse_x = windowEvent.motion.x;
                ControlSystem::mouse_y = windowEvent.motion.y;

                if (ControlSystem::DEBUG_DRAW->get() && !GameIsolate_.ui->UIIsMouseOnControls()) {
                    // draw material

                    int x = (int)((windowEvent.motion.x - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
                    int y = (int)((windowEvent.motion.y - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

                    if (lastDrawMX == 0 && lastDrawMY == 0) {
                        lastDrawMX = x;
                        lastDrawMY = y;
                    }

                    GameIsolate_.world->forLine(lastDrawMX, lastDrawMY, x, y, [&](int index) {
                        int lineX = index % GameIsolate_.world->width;
                        int lineY = index / GameIsolate_.world->width;

                        for (int xx = -gameUI.DebugDrawUI__brushSize / 2; xx < (int)(ceil(gameUI.DebugDrawUI__brushSize / 2.0)); xx++) {
                            for (int yy = -gameUI.DebugDrawUI__brushSize / 2; yy < (int)(ceil(gameUI.DebugDrawUI__brushSize / 2.0)); yy++) {
                                if (lineX + xx < 0 || lineY + yy < 0 || lineX + xx >= GameIsolate_.world->width || lineY + yy >= GameIsolate_.world->height) continue;
                                MaterialInstance tp = TilesCreate(gameUI.DebugDrawUI__selectedMaterial, lineX + xx, lineY + yy);
                                GameIsolate_.world->tiles[(lineX + xx) + (lineY + yy) * GameIsolate_.world->width] = tp;
                                GameIsolate_.world->dirty[(lineX + xx) + (lineY + yy) * GameIsolate_.world->width] = true;
                            }
                        }

                        return false;
                    });

                    lastDrawMX = x;
                    lastDrawMY = y;

                } else {
                    lastDrawMX = 0;
                    lastDrawMY = 0;
                }

                if (ControlSystem::mmouse_down && !GameIsolate_.ui->UIIsMouseOnControls()) {
                    // erase material

                    // erase from world
                    int x = (int)((windowEvent.motion.x - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
                    int y = (int)((windowEvent.motion.y - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

                    if (lastEraseMX == 0 && lastEraseMY == 0) {
                        lastEraseMX = x;
                        lastEraseMY = y;
                    }

                    GameIsolate_.world->forLine(lastEraseMX, lastEraseMY, x, y, [&](int index) {
                        int lineX = index % GameIsolate_.world->width;
                        int lineY = index / GameIsolate_.world->width;

                        for (int xx = -gameUI.DebugDrawUI__brushSize / 2; xx < (int)(ceil(gameUI.DebugDrawUI__brushSize / 2.0)); xx++) {
                            for (int yy = -gameUI.DebugDrawUI__brushSize / 2; yy < (int)(ceil(gameUI.DebugDrawUI__brushSize / 2.0)); yy++) {

                                if (abs(xx) + abs(yy) == gameUI.DebugDrawUI__brushSize) continue;
                                if (GameIsolate_.world->getTile(lineX + xx, lineY + yy).mat->physicsType != PhysicsType::AIR) {
                                    GameIsolate_.world->setTile(lineX + xx, lineY + yy, Tiles_NOTHING);
                                    GameIsolate_.world->lastMeshZone.x--;
                                }
                                if (GameIsolate_.world->getTileLayer2(lineX + xx, lineY + yy).mat->physicsType != PhysicsType::AIR) {
                                    GameIsolate_.world->setTileLayer2(lineX + xx, lineY + yy, Tiles_NOTHING);
                                }
                            }
                        }
                        return false;
                    });

                    lastEraseMX = x;
                    lastEraseMY = y;

                    // erase from rigidbodies
                    // this copies the vector
                    std::vector<RigidBody *> *rbs = &GameIsolate_.world->rigidBodies;

                    for (size_t i = 0; i < rbs->size(); i++) {
                        RigidBody *cur = (*rbs)[i];
                        if (!static_cast<bool>(cur->surface)) continue;
                        if (cur->body->IsEnabled()) {
                            f32 s = sin(-cur->body->GetAngle());
                            f32 c = cos(-cur->body->GetAngle());
                            bool upd = false;
                            for (f32 xx = -3; xx <= 3; xx += 0.5) {
                                for (f32 yy = -3; yy <= 3; yy += 0.5) {
                                    if (abs(xx) + abs(yy) == 6) continue;
                                    // rotate point

                                    f32 tx = x + xx - cur->body->GetPosition().x;
                                    f32 ty = y + yy - cur->body->GetPosition().y;

                                    int ntx = (int)(tx * c - ty * s);
                                    int nty = (int)(tx * s + ty * c);

                                    if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                        u32 pixel = R_GET_PIXEL(cur->surface, ntx, nty);
                                        if (((pixel >> 24) & 0xff) != 0x00) {
                                            R_GET_PIXEL(cur->surface, ntx, nty) = 0x00000000;
                                            upd = true;
                                        }
                                    }
                                }
                            }

                            if (upd) {
                                R_FreeImage(cur->texture);
                                cur->texture = R_CopyImageFromSurface(cur->surface);
                                R_SetImageFilter(cur->texture, R_FILTER_NEAREST);
                                // GameIsolate_.world->updateRigidBodyHitbox(cur);
                                cur->needsUpdate = true;
                            }
                        }
                    }

                } else {
                    lastEraseMX = 0;
                    lastEraseMY = 0;
                }

            } else if (windowEvent.type == SDL_KEYDOWN || windowEvent.type == SDL_KEYUP) {
                ControlSystem::KeyEvent(windowEvent.key);
            }

            if (windowEvent.type == SDL_MOUSEBUTTONDOWN) {
                if (windowEvent.button.button == SDL_BUTTON_LEFT) {
                    ControlSystem::lmouse_down = true;
                    ControlSystem::lmouse_up = false;

                    if (GameIsolate_.world->player) {

                        auto [pl_we, pl] = GameIsolate_.world->getHostPlayer();

                        if (!GameIsolate_.ui->UIIsMouseOnControls() && pl && pl->heldItem != NULL) {
                            if (pl->heldItem->getFlag(ItemFlags::ItemFlags_Vacuum)) {
                                pl->holdtype = Vacuum;
                            } else if (pl->heldItem->getFlag(ItemFlags::ItemFlags_Hammer)) {
// #define HAMMER_DEBUG_PHYSICS
#ifdef HAMMER_DEBUG_PHYSICS
                                int x = (int)((windowEvent.button.x - ofsX - camX) / Screen.gameScale);
                                int y = (int)((windowEvent.button.y - ofsY - camY) / Screen.gameScale);

                                GameIsolate_.world->physicsCheck(x, y);
#else
                                mx = windowEvent.button.x;
                                my = windowEvent.button.y;
                                int startInd = getAimSolidSurface(64);

                                if (startInd != -1) {
                                    // pl->hammerX = x;
                                    // pl->hammerY = y;
                                    pl->hammerX = startInd % GameIsolate_.world->width;
                                    pl->hammerY = startInd / GameIsolate_.world->width;
                                    pl->holdtype = Hammer;
                                    // METADOT_BUG("hammer down: {0:d} {0:d} {0:d} {0:d} {0:d}", x, y, startInd, startInd % GameIsolate_.world->width, startInd / GameIsolate_.world->width);
                                    // GameIsolate_.world->setTile(pl->hammerX, pl->hammerY,
                                    // MaterialInstance(&MaterialsList::GENERIC_SOLID, 0x00ff00ff));
                                }
#endif
#undef HAMMER_DEBUG_PHYSICS
                            } else if (pl->heldItem->getFlag(ItemFlags::ItemFlags_Chisel)) {
                                // if hovering rigidbody, open in chisel

                                int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
                                int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

                                std::vector<RigidBody *> rbs = GameIsolate_.world->rigidBodies;  // copy
                                for (size_t i = 0; i < rbs.size(); i++) {
                                    RigidBody *cur = rbs[i];

                                    bool connect = false;
                                    if (cur->body->IsEnabled()) {
                                        f32 s = sin(-cur->body->GetAngle());
                                        f32 c = cos(-cur->body->GetAngle());
                                        bool upd = false;
                                        for (f32 xx = -3; xx <= 3; xx += 0.5) {
                                            for (f32 yy = -3; yy <= 3; yy += 0.5) {
                                                if (abs(xx) + abs(yy) == 6) continue;
                                                // rotate point

                                                f32 tx = x + xx - cur->body->GetPosition().x;
                                                f32 ty = y + yy - cur->body->GetPosition().y;

                                                int ntx = (int)(tx * c - ty * s);
                                                int nty = (int)(tx * s + ty * c);

                                                if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                                    u32 pixel = R_GET_PIXEL(cur->surface, ntx, nty);
                                                    if (((pixel >> 24) & 0xff) != 0x00) {
                                                        connect = true;
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    if (connect) {

                                        // previously: open chisel ui

                                        break;
                                    }
                                }

                            } else if (pl->heldItem->getFlag(ItemFlags::ItemFlags_Tool)) {
                                // break with pickaxe

                                f32 breakSize = pl->heldItem->breakSize;

                                int x = (int)(pl_we->x + pl_we->hw / 2.0f + GameIsolate_.world->loadZone.x + 10 * (f32)cos((pl->holdAngle + 180) * 3.1415f / 180.0f) - breakSize / 2);
                                int y = (int)(pl_we->y + pl_we->hh / 2.0f + GameIsolate_.world->loadZone.y + 10 * (f32)sin((pl->holdAngle + 180) * 3.1415f / 180.0f) - breakSize / 2);

                                C_Surface *tex = SDL_CreateRGBSurfaceWithFormat(0, (int)breakSize, (int)breakSize, 32, SDL_PIXELFORMAT_ARGB8888);

                                int n = 0;
                                for (int xx = 0; xx < breakSize; xx++) {
                                    for (int yy = 0; yy < breakSize; yy++) {
                                        f32 cx = (f32)((xx / breakSize) - 0.5);
                                        f32 cy = (f32)((yy / breakSize) - 0.5);

                                        if (cx * cx + cy * cy > 0.25f) continue;

                                        if (GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width].mat->physicsType == PhysicsType::SOLID) {
                                            R_GET_PIXEL(tex, xx, yy) = GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width].color;
                                            GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width] = Tiles_NOTHING;
                                            GameIsolate_.world->dirty[(x + xx) + (y + yy) * GameIsolate_.world->width] = true;

                                            n++;
                                        }
                                    }
                                }

                                if (n > 0) {
                                    global.audio.PlayEvent("event:/Player/Impact");
                                    b2PolygonShape s;
                                    s.SetAsBox(1, 1);
                                    RigidBody *rb = GameIsolate_.world->makeRigidBody(b2_dynamicBody, (f32)x, (f32)y, 0, s, 1, (f32)0.3, tex);

                                    b2Filter bf = {};
                                    bf.categoryBits = 0x0001;
                                    bf.maskBits = 0xffff;
                                    rb->body->GetFixtureList()[0].SetFilterData(bf);

                                    rb->body->SetLinearVelocity({(f32)((rand() % 100) / 100.0 - 0.5), (f32)((rand() % 100) / 100.0 - 0.5)});

                                    GameIsolate_.world->rigidBodies.push_back(rb);
                                    GameIsolate_.world->updateRigidBodyHitbox(rb);

                                    GameIsolate_.world->lastMeshLoadZone.x--;
                                    GameIsolate_.world->updateWorldMesh();
                                }
                            }
                        }
                    }

                } else if (windowEvent.button.button == SDL_BUTTON_RIGHT) {
                    ControlSystem::rmouse_down = true;
                    ControlSystem::rmouse_up = false;
                    if (GameIsolate_.world->player) {

                        auto [pl_we, pl] = GameIsolate_.world->getHostPlayer();

                        pl->startThrow = ME_gettime();
                    }
                } else if (windowEvent.button.button == SDL_BUTTON_MIDDLE) {
                    ControlSystem::mmouse_down = true;
                    ControlSystem::mmouse_up = false;
                }
            } else if (windowEvent.type == SDL_MOUSEBUTTONUP) {
                if (windowEvent.button.button == SDL_BUTTON_LEFT) {
                    ControlSystem::lmouse_down = false;
                    ControlSystem::lmouse_up = true;

                    if (GameIsolate_.world->player) {

                        auto [pl_we, pl] = GameIsolate_.world->getHostPlayer();

                        if (pl->heldItem) {
                            if (pl->heldItem->getFlag(ItemFlags::ItemFlags_Vacuum)) {
                                if (pl->holdtype == Vacuum) {
                                    pl->holdtype = None;
                                }
                            } else if (pl->heldItem->getFlag(ItemFlags::ItemFlags_Hammer)) {
                                if (pl->holdtype == Hammer) {
                                    int x = (int)((windowEvent.button.x - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
                                    int y = (int)((windowEvent.button.y - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

                                    int dx = pl->hammerX - x;
                                    int dy = pl->hammerY - y;
                                    f32 len = sqrtf(dx * dx + dy * dy);
                                    f32 udx = dx / len;
                                    f32 udy = dy / len;

                                    int ex = pl->hammerX + dx;
                                    int ey = pl->hammerY + dy;
                                    METADOT_BUG("hammer up: %d %d %d %d", ex, ey, dx, dy);
                                    int endInd = -1;

                                    int nSegments = 1 + len / 10;
                                    std::vector<std::tuple<int, int>> points = {};
                                    for (int i = 0; i < nSegments; i++) {
                                        int sx = pl->hammerX + (int)((f32)(dx / nSegments) * (i + 1));
                                        int sy = pl->hammerY + (int)((f32)(dy / nSegments) * (i + 1));
                                        sx += rand() % 3 - 1;
                                        sy += rand() % 3 - 1;
                                        points.push_back(std::tuple<int, int>(sx, sy));
                                    }

                                    int nTilesChanged = 0;
                                    for (size_t i = 0; i < points.size(); i++) {
                                        int segSx = i == 0 ? pl->hammerX : std::get<0>(points[i - 1]);
                                        int segSy = i == 0 ? pl->hammerY : std::get<1>(points[i - 1]);
                                        int segEx = std::get<0>(points[i]);
                                        int segEy = std::get<1>(points[i]);

                                        bool hitSolidYet = false;
                                        bool broke = false;
                                        GameIsolate_.world->forLineCornered(segSx, segSy, segEx, segEy, [&](int index) {
                                            if (GameIsolate_.world->tiles[index].mat->physicsType != PhysicsType::SOLID) {
                                                if (hitSolidYet && (abs((index % GameIsolate_.world->width) - segSx) + (abs((index / GameIsolate_.world->width) - segSy)) > 1)) {
                                                    broke = true;
                                                    return true;
                                                }
                                                return false;
                                            }
                                            hitSolidYet = true;
                                            GameIsolate_.world->tiles[index] =
                                                    MaterialInstance(&MaterialsList::GENERIC_SAND, MetaEngine::Drawing::darkenColor(GameIsolate_.world->tiles[index].color, 0.5f));
                                            GameIsolate_.world->dirty[index] = true;
                                            endInd = index;
                                            nTilesChanged++;
                                            return false;
                                        });

                                        // GameIsolate_.world->setTile(segSx, segSy, MaterialInstance(&MaterialsList::GENERIC_SOLID, 0x00ff00ff));
                                        if (broke) break;
                                    }

                                    // GameIsolate_.world->setTile(ex, ey, MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xff0000ff));

                                    int hx = (pl->hammerX + (endInd % GameIsolate_.world->width)) / 2;
                                    int hy = (pl->hammerY + (endInd / GameIsolate_.world->width)) / 2;

                                    if (GameIsolate_.world->getTile((int)(hx + udy * 2), (int)(hy - udx * 2)).mat->physicsType == PhysicsType::SOLID) {
                                        GameIsolate_.world->physicsCheck((int)(hx + udy * 2), (int)(hy - udx * 2));
                                    }

                                    if (GameIsolate_.world->getTile((int)(hx - udy * 2), (int)(hy + udx * 2)).mat->physicsType == PhysicsType::SOLID) {
                                        GameIsolate_.world->physicsCheck((int)(hx - udy * 2), (int)(hy + udx * 2));
                                    }

                                    if (nTilesChanged > 0) {
                                        global.audio.PlayEvent("event:/Player/Impact");
                                    }

                                    // GameIsolate_.world->setTile((int)(hx), (int)(hy), MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xffffffff));
                                    // GameIsolate_.world->setTile((int)(hx + udy * 6), (int)(hy - udx * 6), MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xffff00ff));
                                    // GameIsolate_.world->setTile((int)(hx - udy * 6), (int)(hy + udx * 6), MaterialInstance(&MaterialsList::GENERIC_SOLID, 0x00ffffff));
                                }
                                pl->holdtype = None;
                            }
                        }
                    }
                } else if (windowEvent.button.button == SDL_BUTTON_RIGHT) {
                    ControlSystem::rmouse_down = false;
                    ControlSystem::rmouse_up = true;
                    // pick up / throw item

                    int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
                    int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

                    bool swapped = false;
                    std::vector<RigidBody *> *rbs = &GameIsolate_.world->rigidBodies;
                    for (size_t i = 0; i < rbs->size(); i++) {
                        RigidBody *cur = (*rbs)[i];

                        bool connect = false;
                        if (!static_cast<bool>(cur->surface)) continue;
                        if (cur->body->IsEnabled()) {
                            f32 s = sin(-cur->body->GetAngle());
                            f32 c = cos(-cur->body->GetAngle());
                            bool upd = false;
                            for (f32 xx = -3; xx <= 3; xx += 0.5) {
                                for (f32 yy = -3; yy <= 3; yy += 0.5) {
                                    if (abs(xx) + abs(yy) == 6) continue;
                                    // rotate point

                                    f32 tx = x + xx - cur->body->GetPosition().x;
                                    f32 ty = y + yy - cur->body->GetPosition().y;

                                    int ntx = (int)(tx * c - ty * s);
                                    int nty = (int)(tx * s + ty * c);

                                    if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                        if (((R_GET_PIXEL(cur->surface, ntx, nty) >> 24) & 0xff) != 0x00) {
                                            connect = true;
                                        }
                                    }
                                }
                            }
                        }

                        if (connect) {
                            if (GameIsolate_.world->player) {

                                auto [pl_we, pl] = GameIsolate_.world->getHostPlayer();

                                pl->setItemInHand(GameIsolate_.world->Reg().find_component<WorldEntity>(GameIsolate_.world->player), Item::makeItem(ItemFlags::ItemFlags_Rigidbody, cur),
                                                  GameIsolate_.world.get());

                                GameIsolate_.world->b2world->DestroyBody(cur->body);
                                GameIsolate_.world->rigidBodies.erase(std::remove(GameIsolate_.world->rigidBodies.begin(), GameIsolate_.world->rigidBodies.end(), cur),
                                                                      GameIsolate_.world->rigidBodies.end());

                                swapped = true;
                            }
                            break;
                        }
                    }

                    if (!swapped) {
                        if (GameIsolate_.world->player) {

                            auto [pl_we, pl] = GameIsolate_.world->getHostPlayer();

                            pl->setItemInHand(GameIsolate_.world->Reg().find_component<WorldEntity>(GameIsolate_.world->player), NULL, GameIsolate_.world.get());
                        }
                    }

                } else if (windowEvent.button.button == SDL_BUTTON_MIDDLE) {
                    ControlSystem::mmouse_down = false;
                    ControlSystem::rmouse_up = true;
                }
            }

            if (windowEvent.type == SDL_MOUSEMOTION) {
                mx = windowEvent.motion.x;
                my = windowEvent.motion.y;
            }

            if (windowEvent.type == SDL_QUIT) {
                running = false;
            }
        }

#pragma endregion SDL_Input

#pragma region GameTick
        ME_profiler_scope_auto("GameTick");

        if (GameIsolate_.globaldef.tick_world) updateFrameEarly();

        while (Time.now - Time.lastTickTime > (1000.0f / Time.maxTps)) {
            Scripting::get_singleton_ptr()->UpdateTick();
            Scripting::get_singleton_ptr()->Update();
            if (GameIsolate_.globaldef.tick_world) {
                tick();
            }
            Render.target = Render.realTarget;
            Time.lastTickTime = Time.now;
            Time.tickCount++;
        }

        if (GameIsolate_.globaldef.tick_world) updateFrameLate();

#pragma endregion GameTick

#pragma region Render
        // render
        ME_profiler_scope_auto("Rendering");

        Render.target = Render.realTarget;
        R_Clear(Render.target);

        ME_profiler_scope_auto("RenderEarly");
        renderEarly();
        Render.target = Render.realTarget;

        ME_profiler_scope_auto("RenderLate");
        renderLate();
        Render.target = Render.realTarget;

        Scripting::get_singleton_ptr()->UpdateRender();

        std::string test_text = "hello";
        fontcache.ME_fontcache_push(test_text, {0.5, 0.5});

        // ME_rect rct{0, 0, 150, 150};
        // RenderTextureRect(GameIsolate_.texturepack->testAse, Render.target, 200, 200, &rct);

        // Update UI
        GameIsolate_.ui->UIRendererUpdate();
        GameIsolate_.ui->UIRendererDraw();

        R_ActivateShaderProgram(0, NULL);
        R_FlushBlitBuffer();

        if (GameIsolate_.globaldef.draw_material_info && !ImGui::GetIO().WantCaptureMouse) {

            int msx = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
            int msy = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

            MaterialInstance tile;

            if (msx >= 0 && msy >= 0 && msx < GameIsolate_.world->width && msy < GameIsolate_.world->height) {
                tile = GameIsolate_.world->tiles[msx + msy * GameIsolate_.world->width];
                // Drawing::drawText(target, tile.mat->name.c_str(), font16, mx + 14, my, 0xff, 0xff, 0xff, ALIGN_LEFT);

                if (tile.mat->id == MaterialsList::GENERIC_AIR.id) {
                    std::vector<RigidBody *> *rbs = &GameIsolate_.world->rigidBodies;

                    for (size_t i = 0; i < rbs->size(); i++) {
                        RigidBody *cur = (*rbs)[i];
                        if (cur->body->IsEnabled() && static_cast<bool>(cur->surface)) {
                            f32 s = sin(-cur->body->GetAngle());
                            f32 c = cos(-cur->body->GetAngle());
                            bool upd = false;

                            f32 tx = msx - cur->body->GetPosition().x;
                            f32 ty = msy - cur->body->GetPosition().y;

                            int ntx = (int)(tx * c - ty * s);
                            int nty = (int)(tx * s + ty * c);

                            if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                tile = cur->tiles[ntx + nty * cur->matWidth];
                            }
                        }
                    }
                }

                // Draw Tooltop window
                if (tile.mat->id != MaterialsList::GENERIC_AIR.id && !GameIsolate_.ui->UIIsMouseOnControls()) {

                    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.11f, 0.11f, 0.11f, 0.4f));
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.00f, 1.00f, 1.00f, 0.2f));
                    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", tile.mat->name.c_str());

                    if (GameIsolate_.globaldef.draw_detailed_material_info) {

                        if (tile.mat->physicsType == PhysicsType::SOUP) {
                            ImGui::Text("fluidAmount = %f", tile.fluidAmount);
                        }

                        ImGui::Auto(*tile.mat);

                        int ln = 0;
                        if (tile.mat->interact) {
                            for (size_t i = 0; i < global.GameData_.materials_container.size(); i++) {
                                if (tile.mat->nInteractions[i] > 0) {
                                    char buff2[40];
                                    snprintf(buff2, sizeof(buff2), "    %s", global.GameData_.materials_container[i]->name.c_str());
                                    // Drawing::drawText(target, buff2, font16, mx + 14, my + 14 * ++ln, 0xff, 0xff, 0xff, ALIGN_LEFT);
                                    ImGui::Text("%s", buff2);

                                    for (int j = 0; j < tile.mat->nInteractions[i]; j++) {
                                        MaterialInteraction inter = tile.mat->interactions[i][j];
                                        char buff1[40];
                                        if (inter.type == INTERACT_TRANSFORM_MATERIAL) {
                                            snprintf(buff1, sizeof(buff1), "        %s %s r=%d x=%d y=%d", "TRANSFORM", global.GameData_.materials_container[inter.data1]->name.c_str(), inter.data2,
                                                     inter.ofsX, inter.ofsY);
                                        } else if (inter.type == INTERACT_SPAWN_MATERIAL) {
                                            snprintf(buff1, sizeof(buff1), "        %s %s r=%d x=%d y=%d", "SPAWN", global.GameData_.materials_container[inter.data1]->name.c_str(), inter.data2,
                                                     inter.ofsX, inter.ofsY);
                                        }
                                        // Drawing::drawText(target, buff1, font16, mx + 14, my + 14 * ++ln, 0xff, 0xff, 0xff, ALIGN_LEFT);
                                        ImGui::Text("%s", buff1);
                                    }
                                }
                            }
                        }
                    }

                    ImGui::EndTooltip();
                    ImGui::PopStyleColor();
                    ImGui::PopStyleColor();
                }
            }
        }

        tickProfiler();

        GameIsolate_.ui->UIRendererDrawImGui();

        fontcache.ME_fontcache_drawcmd();

        // render fade in/out
        if (fadeInWaitFrames > 0) {
            fadeInWaitFrames--;
            fadeInStart = Time.now;
            R_RectangleFilled(Render.target, 0, 0, Screen.windowWidth, Screen.windowHeight, {0, 0, 0, 255});
        } else if (fadeInStart > 0 && fadeInLength > 0) {

            f32 thru = 1 - (f32)(Time.now - fadeInStart) / fadeInLength;

            if (thru >= 0 && thru <= 1) {
                R_RectangleFilled(Render.target, 0, 0, Screen.windowWidth, Screen.windowHeight, {0, 0, 0, (u8)(thru * 255)});
            } else {
                fadeInStart = 0;
                fadeInLength = 0;
            }
        }

        if (fadeOutWaitFrames > 0) {
            fadeOutWaitFrames--;
            fadeOutStart = Time.now;
        } else if (fadeOutStart > 0 && fadeOutLength > 0) {

            f32 thru = (f32)(Time.now - fadeOutStart) / fadeOutLength;

            if (thru >= 0 && thru <= 1) {
                R_RectangleFilled(Render.target, 0, 0, Screen.windowWidth, Screen.windowHeight, {0, 0, 0, (u8)(thru * 255)});
            } else {
                R_RectangleFilled(Render.target, 0, 0, Screen.windowWidth, Screen.windowHeight, {0, 0, 0, 255});
                fadeOutStart = 0;
                fadeOutLength = 0;
                fadeOutCallback.invoke({});
            }
        }

        R_Flip(Render.target);

#pragma endregion Render

        EngineUpdateEnd();
    }

    return exit();
}

int Game::exit() {
    METADOT_INFO("Shutting down...");

    GameIsolate_.world->saveWorld();

    running = false;

    // release resources & shutdown
    std::vector<ME::ref<IGameSystem>>::reverse_iterator backwardIterator;
    for (backwardIterator = GameIsolate_.systemList.rbegin(); backwardIterator != GameIsolate_.systemList.rend(); backwardIterator++) {
        backwardIterator->get()->Destory();
    }

    fontcache.ME_fontcache_end();

    Scripting::get_singleton_ptr()->End();
    Scripting::clean();

    ReleaseGameData();

    ME_destroy_pack_reader(GameIsolate_.pack_reader);

    delete[] objectDelete;

    delete debugDraw;
    delete[] movingTiles;

    delete GameIsolate_.updateDirtyPool;
    delete GameIsolate_.updateDirtyPool2;

    if (GameIsolate_.world.get()) {
        auto *p = GameIsolate_.world.release();
        delete p;
    }

    global.audio.EndAudio();
    metadot_endwindow();

    EndEngine(0);

    METADOT_INFO("Clean done...");

    return METADOT_OK;
}

void Game::updateFrameEarly() {

    // handle controls
    if (ControlSystem::DEBUG_UI->get()) {
        gameUI.visible_debugdraw ^= true;
        GameIsolate_.globaldef.ui_tweak ^= true;
    }

    if (GameIsolate_.globaldef.draw_frame_graph) {
        if (ControlSystem::STATS_DISPLAY->get()) {
            GameIsolate_.globaldef.draw_frame_graph = false;
            GameIsolate_.globaldef.draw_debug_stats = false;
            GameIsolate_.globaldef.draw_chunk_state = false;
            GameIsolate_.globaldef.draw_detailed_material_info = false;
        }
    } else {
        if (ControlSystem::STATS_DISPLAY->get()) {
            GameIsolate_.globaldef.draw_frame_graph = true;
            GameIsolate_.globaldef.draw_debug_stats = true;

            if (ControlSystem::STATS_DISPLAY_DETAILED->get()) {
                GameIsolate_.globaldef.draw_chunk_state = true;
                GameIsolate_.globaldef.draw_detailed_material_info = true;
            }
        }
    }

    if (ControlSystem::DEBUG_REFRESH->get()) {
        for (int x = 0; x < GameIsolate_.world->width; x++) {
            for (int y = 0; y < GameIsolate_.world->height; y++) {
                GameIsolate_.world->dirty[x + y * GameIsolate_.world->width] = true;
                GameIsolate_.world->layer2Dirty[x + y * GameIsolate_.world->width] = true;
                GameIsolate_.world->backgroundDirty[x + y * GameIsolate_.world->width] = true;
            }
        }
    }

    if (ControlSystem::DEBUG_RIGID->get()) {
        for (auto &cur : GameIsolate_.world->rigidBodies) {
            ME_ASSERT_E(cur);
            if (cur->body->IsEnabled()) {
                f32 s = sin(cur->body->GetAngle());
                f32 c = cos(cur->body->GetAngle());
                bool upd = false;

                for (int xx = 0; xx < cur->matWidth; xx++) {
                    for (int yy = 0; yy < cur->matHeight; yy++) {
                        int tx = xx * c - yy * s + cur->body->GetPosition().x;
                        int ty = xx * s + yy * c + cur->body->GetPosition().y;

                        MaterialInstance tt = cur->tiles[xx + yy * cur->matWidth];
                        if (tt.mat->id != MaterialsList::GENERIC_AIR.id) {
                            if (GameIsolate_.world->tiles[tx + ty * GameIsolate_.world->width].mat->id == MaterialsList::GENERIC_AIR.id) {
                                GameIsolate_.world->tiles[tx + ty * GameIsolate_.world->width] = tt;
                                GameIsolate_.world->dirty[tx + ty * GameIsolate_.world->width] = true;
                            } else if (GameIsolate_.world->tiles[(tx + 1) + ty * GameIsolate_.world->width].mat->id == MaterialsList::GENERIC_AIR.id) {
                                GameIsolate_.world->tiles[(tx + 1) + ty * GameIsolate_.world->width] = tt;
                                GameIsolate_.world->dirty[(tx + 1) + ty * GameIsolate_.world->width] = true;
                            } else if (GameIsolate_.world->tiles[(tx - 1) + ty * GameIsolate_.world->width].mat->id == MaterialsList::GENERIC_AIR.id) {
                                GameIsolate_.world->tiles[(tx - 1) + ty * GameIsolate_.world->width] = tt;
                                GameIsolate_.world->dirty[(tx - 1) + ty * GameIsolate_.world->width] = true;
                            } else if (GameIsolate_.world->tiles[tx + (ty + 1) * GameIsolate_.world->width].mat->id == MaterialsList::GENERIC_AIR.id) {
                                GameIsolate_.world->tiles[tx + (ty + 1) * GameIsolate_.world->width] = tt;
                                GameIsolate_.world->dirty[tx + (ty + 1) * GameIsolate_.world->width] = true;
                            } else if (GameIsolate_.world->tiles[tx + (ty - 1) * GameIsolate_.world->width].mat->id == MaterialsList::GENERIC_AIR.id) {
                                GameIsolate_.world->tiles[tx + (ty - 1) * GameIsolate_.world->width] = tt;
                                GameIsolate_.world->dirty[tx + (ty - 1) * GameIsolate_.world->width] = true;
                            } else {
                                GameIsolate_.world->tiles[tx + ty * GameIsolate_.world->width] = TilesCreateObsidian(tx, ty);
                                GameIsolate_.world->dirty[tx + ty * GameIsolate_.world->width] = true;
                            }
                        }
                    }
                }

                if (upd) {
                    R_FreeImage(cur->texture);
                    cur->texture = R_CopyImageFromSurface(cur->surface);
                    R_SetImageFilter(cur->texture, R_FILTER_NEAREST);
                    // GameIsolate_.world->updateRigidBodyHitbox(cur);
                    cur->needsUpdate = true;
                }
            }

            GameIsolate_.world->b2world->DestroyBody(cur->body);
        }
        GameIsolate_.world->rigidBodies.clear();
    }

    if (ControlSystem::DEBUG_UPDATE_WORLD_MESH->get()) {
        GameIsolate_.world->updateWorldMesh();
    }

    if (ControlSystem::DEBUG_EXPLODE->get()) {
        int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
        int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);
        GameIsolate_.world->explosion(x, y, 30);
    }

    if (ControlSystem::DEBUG_CARVE->get()) {
        // carve square

        int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale - 16);
        int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale - 16);

        C_Surface *tex = SDL_CreateRGBSurfaceWithFormat(0, 32, 32, 32, SDL_PIXELFORMAT_ARGB8888);

        int n = 0;
        for (int xx = 0; xx < 32; xx++) {
            for (int yy = 0; yy < 32; yy++) {

                if (GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width].mat->physicsType == PhysicsType::SOLID) {
                    R_GET_PIXEL(tex, xx, yy) = GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width].color;
                    GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width] = Tiles_NOTHING;
                    GameIsolate_.world->dirty[(x + xx) + (y + yy) * GameIsolate_.world->width] = true;
                    n++;
                }
            }
        }
        if (n > 0) {
            b2PolygonShape s;
            s.SetAsBox(1, 1);
            RigidBody *rb = GameIsolate_.world->makeRigidBody(b2_dynamicBody, (f32)x, (f32)y, 0, s, 1, (f32)0.3, tex);
            for (int tx = 0; tx < tex->w; tx++) {
                b2Filter bf = {};
                bf.categoryBits = 0x0002;
                bf.maskBits = 0x0001;
                rb->body->GetFixtureList()[0].SetFilterData(bf);
            }
            GameIsolate_.world->rigidBodies.push_back(rb);
            GameIsolate_.world->updateRigidBodyHitbox(rb);

            GameIsolate_.world->updateWorldMesh();
        }
    }

    if (ControlSystem::DEBUG_BRUSHSIZE_INC->get()) {
        gameUI.DebugDrawUI__brushSize = gameUI.DebugDrawUI__brushSize < 50 ? gameUI.DebugDrawUI__brushSize + 1 : gameUI.DebugDrawUI__brushSize;
    }

    if (ControlSystem::DEBUG_BRUSHSIZE_DEC->get()) {
        gameUI.DebugDrawUI__brushSize = gameUI.DebugDrawUI__brushSize > 1 ? gameUI.DebugDrawUI__brushSize - 1 : gameUI.DebugDrawUI__brushSize;
    }

    if (ControlSystem::DEBUG_TOGGLE_PLAYER->get()) {
        if (GameIsolate_.world->player) {

            auto [pl_we, pl] = GameIsolate_.world->getHostPlayer();

            global.GameData_.freeCamX = pl_we->x + pl_we->hw / 2.0f;
            global.GameData_.freeCamY = pl_we->y - pl_we->hh / 2.0f;
            // GameIsolate_.world->worldEntities.erase(std::remove(GameIsolate_.world->worldEntities.begin(), GameIsolate_.world->worldEntities.end(), GameIsolate_.world->player),
            //                                         GameIsolate_.world->worldEntities.end());
            GameIsolate_.world->b2world->DestroyBody(pl_we->rb->body);
            GameIsolate_.world->Reg().destroy_entity(GameIsolate_.world->player);
            // delete GameIsolate_.world->player;
            GameIsolate_.world->player = 0;
        } else {

            MEvec4 pl_transform{-GameIsolate_.world->loadZone.x + GameIsolate_.world->tickZone.x + GameIsolate_.world->tickZone.w / 2.0f,
                                -GameIsolate_.world->loadZone.y + GameIsolate_.world->tickZone.y + GameIsolate_.world->tickZone.h / 2.0f, 10, 20};

            b2PolygonShape sh;
            sh.SetAsBox(pl_transform.z / 2.0f + 1, pl_transform.w / 2.0f);
            RigidBody *rb =
                    GameIsolate_.world->makeRigidBody(b2BodyType::b2_kinematicBody, pl_transform.x + pl_transform.z / 2.0f - 0.5, pl_transform.y + pl_transform.w / 2.0f - 0.5, 0, sh, 1, 1, NULL);
            rb->body->SetGravityScale(0);
            rb->body->SetLinearDamping(0);
            rb->body->SetAngularDamping(0);

            Item *i3 = new Item();
            i3->setFlag(ItemFlags::ItemFlags_Vacuum);
            // i3->vacuumCells = {};
            i3->surface = LoadTexture("data/assets/objects/testVacuum.png")->surface;
            i3->texture = R_CopyImageFromSurface(i3->surface);
            i3->name = "初始物品";
            R_SetImageFilter(i3->texture, R_FILTER_NEAREST);
            i3->pivotX = 6;

            b2Filter bf = {};
            bf.categoryBits = 0x0001;
            // bf.maskBits = 0x0000;
            rb->body->GetFixtureList()[0].SetFilterData(bf);

            auto player = GameIsolate_.world->Reg().create_entity();
            MetaEngine::ECS::entity_filler(player)
                    .component<Controlable>()
                    .component<WorldEntity>(true, pl_transform.x, pl_transform.y, 0.0f, 0.0f, (int)pl_transform.z, (int)pl_transform.w, rb, std::string("玩家"))
                    .component<Player>();

            auto pl = GameIsolate_.world->Reg().find_component<Player>(player);

            GameIsolate_.world->player = player.id();

            pl->setItemInHand(GameIsolate_.world->Reg().find_component<WorldEntity>(GameIsolate_.world->player), i3, GameIsolate_.world.get());

            // accLoadX = 0;
            // accLoadY = 0;
        }
    }

    if (ControlSystem::PAUSE->get()) {
        if (this->state == EnumGameState::INGAME) {
            gameUI.visible_mainmenu ^= true;
        }
    }

    // global.audioEngine.Update();

    if (state == LOADING) {

    } else {
        global.audio.SetEventParameter("event:/World/Sand", "Sand", 0);
        if (GameIsolate_.world->player) {

            auto [pl_we, pl] = GameIsolate_.world->getHostPlayer();

            if (pl->heldItem != NULL && pl->heldItem->getFlag(ItemFlags::ItemFlags_Fluid_Container)) {
                if (ControlSystem::lmouse_down && pl->heldItem->carry.size() > 0) {
                    // shoot fluid from container

                    int x = (int)(pl_we->x + pl_we->hw / 2.0f + GameIsolate_.world->loadZone.x + 10 * (f32)cos((pl->holdAngle + 180) * 3.1415f / 180.0f));
                    int y = (int)(pl_we->y + pl_we->hh / 2.0f + GameIsolate_.world->loadZone.y + 10 * (f32)sin((pl->holdAngle + 180) * 3.1415f / 180.0f));

                    MaterialInstance mat = pl->heldItem->carry[pl->heldItem->carry.size() - 1];
                    pl->heldItem->carry.pop_back();
                    GameIsolate_.world->addCell(new CellData(mat, (f32)x, (f32)y, (f32)(pl_we->vx / 2 + (rand() % 10 - 5) / 10.0f + 1.5f * (f32)cos((pl->holdAngle + 180) * 3.1415f / 180.0f)),
                                                             (f32)(pl_we->vy / 2 + -(rand() % 5 + 5) / 10.0f + 1.5f * (f32)sin((pl->holdAngle + 180) * 3.1415f / 180.0f)), 0, (f32)0.1));

                    int i = (int)pl->heldItem->carry.size();
                    i = (int)((i / (f32)pl->heldItem->capacity) * pl->heldItem->fill.size());
                    U16Point pt = pl->heldItem->fill[i];
                    R_GET_PIXEL(pl->heldItem->surface, pt.x, pt.y) = 0x00;

                    pl->heldItem->texture = R_CopyImageFromSurface(pl->heldItem->surface);
                    R_SetImageFilter(pl->heldItem->texture, R_FILTER_NEAREST);

                    global.audio.SetEventParameter("event:/World/Sand", "Sand", 1);

                } else {
                    // pick up fluid into container

                    f32 breakSize = pl->heldItem->breakSize;

                    int x = (int)(pl_we->x + pl_we->hw / 2.0f + GameIsolate_.world->loadZone.x + 10 * (f32)cos((pl->holdAngle + 180) * 3.1415f / 180.0f) - breakSize / 2);
                    int y = (int)(pl_we->y + pl_we->hh / 2.0f + GameIsolate_.world->loadZone.y + 10 * (f32)sin((pl->holdAngle + 180) * 3.1415f / 180.0f) - breakSize / 2);

                    int n = 0;
                    for (int xx = 0; xx < breakSize; xx++) {
                        for (int yy = 0; yy < breakSize; yy++) {
                            if (pl->heldItem->capacity == 0 || (pl->heldItem->carry.size() < pl->heldItem->capacity)) {
                                f32 cx = (f32)((xx / breakSize) - 0.5);
                                f32 cy = (f32)((yy / breakSize) - 0.5);

                                if (cx * cx + cy * cy > 0.25f) continue;

                                if (GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width].mat->physicsType == PhysicsType::SAND ||
                                    GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width].mat->physicsType == PhysicsType::SOUP) {
                                    pl->heldItem->carry.push_back(GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width]);

                                    int i = (int)pl->heldItem->carry.size() - 1;
                                    i = (int)((i / (f32)pl->heldItem->capacity) * pl->heldItem->fill.size());
                                    U16Point pt = pl->heldItem->fill[i];
                                    u32 c = GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width].color;
                                    R_GET_PIXEL(pl->heldItem->surface, pt.x, pt.y) = (GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width].mat->alpha << 24) + c;

                                    pl->heldItem->texture = R_CopyImageFromSurface(pl->heldItem->surface);
                                    R_SetImageFilter(pl->heldItem->texture, R_FILTER_NEAREST);

                                    GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width] = Tiles_NOTHING;
                                    GameIsolate_.world->dirty[(x + xx) + (y + yy) * GameIsolate_.world->width] = true;
                                    n++;
                                }
                            }
                        }
                    }

                    if (n > 0) {
                        global.audio.PlayEvent("event:/Player/Impact");
                    }
                }
            }
        }

        // rigidbody hover

        int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
        int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

        bool swapped = false;
        f32 hoverDelta = 10.0 * Time.deltaTime / 1000.0;

        // this copies the vector
        std::vector<RigidBody *> rbs = GameIsolate_.world->rigidBodies;
        for (size_t i = 0; i < rbs.size(); i++) {
            RigidBody *cur = rbs[i];

            ME_ASSERT_E(cur);

            if (swapped) {
                cur->hover = (f32)std::fmax(0, cur->hover - hoverDelta);
                continue;
            }

            bool connect = false;
            if (cur->body->IsEnabled()) {
                f32 s = sin(-cur->body->GetAngle());
                f32 c = cos(-cur->body->GetAngle());
                bool upd = false;
                for (f32 xx = -3; xx <= 3; xx += 0.5) {
                    for (f32 yy = -3; yy <= 3; yy += 0.5) {
                        if (abs(xx) + abs(yy) == 6) continue;
                        // rotate point

                        f32 tx = x + xx - cur->body->GetPosition().x;
                        f32 ty = y + yy - cur->body->GetPosition().y;

                        int ntx = (int)(tx * c - ty * s);
                        int nty = (int)(tx * s + ty * c);

                        if (cur->surface != nullptr) {
                            if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                if (((R_GET_PIXEL(cur->surface, ntx, nty) >> 24) & 0xff) != 0x00) {
                                    connect = true;
                                }
                            }
                        } else {
                            // METADOT_ERROR("cur->surface = nullptr");
                            continue;
                        }
                    }
                }
            }

            if (connect) {
                swapped = true;
                cur->hover = (f32)std::fmin(1, cur->hover + hoverDelta);
            } else {
                cur->hover = (f32)std::fmax(0, cur->hover - hoverDelta);
            }
        }

        // update GameIsolate_.world->tickZone

        GameIsolate_.world->tickZone = {CHUNK_W, CHUNK_H, (float)GameIsolate_.world->width - CHUNK_W * 2, (float)GameIsolate_.world->height - CHUNK_H * 2};
        if (GameIsolate_.world->tickZone.x + GameIsolate_.world->tickZone.w > GameIsolate_.world->width) {
            GameIsolate_.world->tickZone.x = GameIsolate_.world->width - GameIsolate_.world->tickZone.w;
        }

        if (GameIsolate_.world->tickZone.y + GameIsolate_.world->tickZone.h > GameIsolate_.world->height) {
            GameIsolate_.world->tickZone.y = GameIsolate_.world->height - GameIsolate_.world->tickZone.h;
        }
    }
}

void Game::onEvent(MetaEngine::Event &e) {
    MetaEngine::EventDispatcher dispatcher(e);
    dispatcher.Dispatch<MetaEngine::WindowCloseEvent>(ME_BIND_EVENT_FN(onWindowClose));
    dispatcher.Dispatch<MetaEngine::WindowResizeEvent>(ME_BIND_EVENT_FN(onWindowResize));
}

bool Game::onWindowClose(MetaEngine::WindowCloseEvent &e) { return true; }

bool Game::onWindowResize(MetaEngine::WindowResizeEvent &e) {
    R_SetWindowResolution(e.GetWidth(), e.GetHeight());
    R_ResetProjection(Render.realTarget);
    ResolutionChanged(e.GetWidth(), e.GetHeight());
    return true;
}

void Game::tick() {

    // METADOT_BUG("{0:d} {0:d}", accLoadX, accLoadY);
    if (state == LOADING) {
        if (GameIsolate_.world) {
            // tick chunkloading
            GameIsolate_.world->frame();
            if (GameIsolate_.world->readyToMerge.size() == 0 && fadeOutStart == 0) {
                fadeOutStart = Time.now;
                fadeOutLength = 250;
                fadeOutCallback = [&]() {
                    fadeInStart = Time.now;
                    fadeInLength = 500;
                    fadeInWaitFrames = 4;
                    state = stateAfterLoad;
                };

                metadot_set_windowflash(engine_windowflashaction::START_COUNT, 1, 333);
            }
        }
    } else {

        int lastReadyToMergeSize = (int)GameIsolate_.world->readyToMerge.size();

        // check chunk loading
        tickChunkLoading();

        if (GameIsolate_.world->needToTickGeneration) GameIsolate_.world->tickChunkGeneration();

        // clear objects
        R_SetShapeBlendMode(R_BLEND_NORMAL);
        R_Clear(TexturePack_.textureObjects->target);

        R_SetShapeBlendMode(R_BLEND_NORMAL);
        R_Clear(TexturePack_.textureObjectsLQ->target);

        R_SetShapeBlendMode(R_BLEND_NORMAL);
        R_Clear(TexturePack_.textureObjectsBack->target);

        if (GameIsolate_.globaldef.tick_world && GameIsolate_.world->readyToMerge.size() == 0) {
            GameIsolate_.world->tickChunks();
        }

        // render objects

        memset(objectDelete, false, (size_t)GameIsolate_.world->width * GameIsolate_.world->height * sizeof(bool));

        R_SetBlendMode(TexturePack_.textureObjects, R_BLEND_NORMAL);
        R_SetBlendMode(TexturePack_.textureObjectsLQ, R_BLEND_NORMAL);
        R_SetBlendMode(TexturePack_.textureObjectsBack, R_BLEND_NORMAL);

        for (size_t i = 0; i < GameIsolate_.world->rigidBodies.size(); i++) {
            RigidBody *cur = GameIsolate_.world->rigidBodies[i];
            if (cur == nullptr) continue;
            if (cur->surface == nullptr) continue;
            if (!cur->body->IsEnabled()) continue;

            f32 x = cur->body->GetPosition().x;
            f32 y = cur->body->GetPosition().y;

            // draw

            R_Target *tgt = cur->back ? TexturePack_.textureObjectsBack->target : TexturePack_.textureObjects->target;
            R_Target *tgtLQ = cur->back ? TexturePack_.textureObjectsBack->target : TexturePack_.textureObjectsLQ->target;
            int scaleObjTex = GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1;

            ME_rect r = {x * scaleObjTex, y * scaleObjTex, (f32)cur->surface->w * scaleObjTex, (f32)cur->surface->h * scaleObjTex};

            if (cur->texNeedsUpdate) {
                if (cur->texture != nullptr) {
                    R_FreeImage(cur->texture);
                }
                cur->texture = R_CopyImageFromSurface(cur->surface);
                R_SetImageFilter(cur->texture, R_FILTER_NEAREST);
                cur->texNeedsUpdate = false;
            }

            R_BlitRectX(cur->texture, NULL, tgt, &r, cur->body->GetAngle() * 180 / (f32)M_PI, 0, 0, R_FLIP_NONE);

            // draw outline

            u8 outlineAlpha = (u8)(cur->hover * 255);
            if (outlineAlpha > 0) {
                ME_Color col = {0xff, 0xff, 0x80, outlineAlpha};
                R_SetShapeBlendMode(R_BLEND_NORMAL_FACTOR_ALPHA);  // SDL_BLENDMODE_BLEND
                for (auto &l : cur->outline) {
                    MEvec2 *vec = new MEvec2[l.GetNumPoints()];
                    for (int j = 0; j < l.GetNumPoints(); j++) {
                        vec[j] = {(f32)l.GetPoint(j).x / Screen.gameScale, (f32)l.GetPoint(j).y / Screen.gameScale};
                    }
                    MetaEngine::Drawing::drawPolygon(tgtLQ, col, vec, (int)x, (int)y, Screen.gameScale, l.GetNumPoints(), cur->body->GetAngle(), 0, 0);
                    delete[] vec;
                }
                R_SetShapeBlendMode(R_BLEND_NORMAL);  // SDL_BLENDMODE_NONE
            }

            // displace fluids

            f32 s = sin(cur->body->GetAngle());
            f32 c = cos(cur->body->GetAngle());

            std::vector<std::pair<int, int>> checkDirs = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};

            for (int tx = 0; tx < cur->matWidth; tx++) {
                for (int ty = 0; ty < cur->matHeight; ty++) {
                    MaterialInstance rmat = cur->tiles[tx + ty * cur->matWidth];
                    if (rmat.mat->id == MaterialsList::GENERIC_AIR.id) continue;

                    // rotate point
                    int wx = (int)(tx * c - (ty + 1) * s + x);
                    int wy = (int)(tx * s + (ty + 1) * c + y);

                    for (auto &dir : checkDirs) {
                        int wxd = wx + dir.first;
                        int wyd = wy + dir.second;

                        if (wxd < 0 || wyd < 0 || wxd >= GameIsolate_.world->width || wyd >= GameIsolate_.world->height) continue;
                        if (GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width].mat->physicsType == PhysicsType::AIR) {
                            GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width] = rmat;
                            GameIsolate_.world->dirty[wxd + wyd * GameIsolate_.world->width] = true;
                            // objectDelete[wxd + wyd * GameIsolate_.world->width] = true;
                            break;
                        } else if (GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width].mat->physicsType == PhysicsType::SAND) {
                            GameIsolate_.world->addCell(new CellData(GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width], (f32)wxd, (f32)(wyd - 3), (f32)((rand() % 10 - 5) / 10.0f),
                                                                     (f32)(-(rand() % 5 + 5) / 10.0f), 0, (f32)0.1));
                            GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width] = rmat;
                            // objectDelete[wxd + wyd * GameIsolate_.world->width] = true;
                            GameIsolate_.world->dirty[wxd + wyd * GameIsolate_.world->width] = true;
                            cur->body->SetLinearVelocity({cur->body->GetLinearVelocity().x * (f32)0.99, cur->body->GetLinearVelocity().y * (f32)0.99});
                            cur->body->SetAngularVelocity(cur->body->GetAngularVelocity() * (f32)0.98);
                            break;
                        } else if (GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width].mat->physicsType == PhysicsType::SOUP) {
                            GameIsolate_.world->addCell(new CellData(GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width], (f32)wxd, (f32)(wyd - 3), (f32)((rand() % 10 - 5) / 10.0f),
                                                                     (f32)(-(rand() % 5 + 5) / 10.0f), 0, (f32)0.1));
                            GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width] = rmat;
                            // objectDelete[wxd + wyd * GameIsolate_.world->width] = true;
                            GameIsolate_.world->dirty[wxd + wyd * GameIsolate_.world->width] = true;
                            cur->body->SetLinearVelocity({cur->body->GetLinearVelocity().x * (f32)0.998, cur->body->GetLinearVelocity().y * (f32)0.998});
                            cur->body->SetAngularVelocity(cur->body->GetAngularVelocity() * (f32)0.99);
                            break;
                        }
                    }
                }
            }
        }

        // render entities

        if (lastReadyToMergeSize == 0) {
            GameIsolate_.world->tickEntities(TexturePack_.textureEntities->target);

            if (GameIsolate_.world->player) {
                auto [pl_we, pl] = GameIsolate_.world->getHostPlayer();

                if (pl->holdtype == Hammer) {
                    int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
                    int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);
                    R_Line(TexturePack_.textureEntitiesLQ->target, x, y, pl->hammerX, pl->hammerY, {0xff, 0xff, 0x00, 0xff});
                }
            }
        }
        R_SetShapeBlendMode(R_BLEND_NORMAL);  // SDL_BLENDMODE_NONE

        entity_update_event e{this};
        GameIsolate_.world->Reg().process_event(e);

        if ((GameIsolate_.globaldef.tick_world && GameIsolate_.world->readyToMerge.size() == 0) || ControlSystem::DEBUG_TICK->get()) {
            GameIsolate_.world->tick();
        }

        // Tick Cam zoom

        if (state == INGAME && (ControlSystem::ZOOM_IN->get() || ControlSystem::ZOOM_OUT->get())) {
            f32 CamZoomIn = (f32)(ControlSystem::ZOOM_IN->get());
            f32 CamZoomOut = (f32)(ControlSystem::ZOOM_OUT->get());

            f32 deltaScale = CamZoomIn - CamZoomOut;
            int oldScale = Screen.gameScale;
            Screen.gameScale += deltaScale;
            if (Screen.gameScale < 1) Screen.gameScale = 1;

            global.GameData_.ofsX = (global.GameData_.ofsX - Screen.windowWidth / 2) / oldScale * Screen.gameScale + Screen.windowWidth / 2;
            global.GameData_.ofsY = (global.GameData_.ofsY - Screen.windowHeight / 2) / oldScale * Screen.gameScale + Screen.windowHeight / 2;
        } else {
        }

        // player movement
        tickPlayer();

        // update cells, tickObjects, update dirty
        // TODO: this is not entirely thread safe since tickCells changes World::tiles and World::dirty

        bool hadDirty = false;
        bool hadLayer2Dirty = false;
        bool hadBackgroundDirty = false;
        bool hadFire = false;
        bool hadFlow = false;

        // int pitch;
        // void* vdpixels_ar = texture->data;
        // u8* dpixels_ar = (u8*)vdpixels_ar;
        u8 *dpixels_ar = TexturePack_.pixels_ar;
        u8 *dpixelsFire_ar = TexturePack_.pixelsFire_ar;
        u8 *dpixelsFlow_ar = TexturePack_.pixelsFlow_ar;
        u8 *dpixelsEmission_ar = TexturePack_.pixelsEmission_ar;

        std::vector<std::future<void>> results = {};

        int i = 1;

        results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
            // SDL_SetRenderTarget(renderer, textureCells);
            void *cellPixels = TexturePack_.pixelsCells_ar;

            memset(cellPixels, 0, (size_t)GameIsolate_.world->width * GameIsolate_.world->height * 4);

            GameIsolate_.world->renderCells((u8 **)&cellPixels);
            GameIsolate_.world->tickCells();

            // SDL_SetRenderTarget(renderer, NULL);
        }));

        if (GameIsolate_.world->readyToMerge.size() == 0) {
            results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) { GameIsolate_.world->tickObjectBounds(); }));
        }

        for (int i = 0; i < results.size(); i++) {
            results[i].get();
        }

        for (size_t i = 0; i < GameIsolate_.world->rigidBodies.size(); i++) {
            RigidBody *cur = GameIsolate_.world->rigidBodies[i];
            if (cur == nullptr) continue;
            if (cur->surface == nullptr) continue;
            if (!cur->body->IsEnabled()) continue;

            f32 x = cur->body->GetPosition().x;
            f32 y = cur->body->GetPosition().y;

            f32 s = sin(cur->body->GetAngle());
            f32 c = cos(cur->body->GetAngle());

            std::vector<std::pair<int, int>> checkDirs = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (int tx = 0; tx < cur->matWidth; tx++) {
                for (int ty = 0; ty < cur->matHeight; ty++) {
                    MaterialInstance rmat = cur->tiles[tx + ty * cur->matWidth];
                    ME_ASSERT_E(rmat.mat);
                    if (rmat.mat->id == MaterialsList::GENERIC_AIR.id) continue;

                    // rotate point
                    int wx = (int)(tx * c - (ty + 1) * s + x);
                    int wy = (int)(tx * s + (ty + 1) * c + y);

                    bool found = false;
                    for (auto &dir : checkDirs) {
                        int wxd = wx + dir.first;
                        int wyd = wy + dir.second;

                        if (wxd < 0 || wyd < 0 || wxd >= GameIsolate_.world->width || wyd >= GameIsolate_.world->height) continue;
                        if (GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width] == rmat) {
                            cur->tiles[tx + ty * cur->matWidth] = GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width];
                            GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width] = Tiles_NOTHING;
                            GameIsolate_.world->dirty[wxd + wyd * GameIsolate_.world->width] = true;
                            found = true;

                            // for(int dxx = -1; dxx <= 1; dxx++) {
                            //     for(int dyy = -1; dyy <= 1; dyy++) {
                            //         if(GameIsolate_.world->tiles[(wxd + dxx) + (wyd + dyy) * GameIsolate_.world->width].mat->physicsType == PhysicsType::SAND || GameIsolate_.world->tiles[(wxd +
                            //         dxx) + (wyd + dyy) * GameIsolate_.world->width].mat->physicsType == PhysicsType::SOUP) {
                            //             uint32_t color = GameIsolate_.world->tiles[(wxd + dxx) + (wyd + dyy) * GameIsolate_.world->width].color;

                            //            unsigned int offset = ((wxd + dxx) + (wyd + dyy) * GameIsolate_.world->width) * 4;

                            //            dpixels_ar[offset + 2] = ((color >> 0) & 0xff);        // b
                            //            dpixels_ar[offset + 1] = ((color >> 8) & 0xff);        // g
                            //            dpixels_ar[offset + 0] = ((color >> 16) & 0xff);        // r
                            //            dpixels_ar[offset + 3] = GameIsolate_.world->tiles[(wxd + dxx) + (wyd + dyy) * GameIsolate_.world->width].mat->alpha;    // a
                            //        }
                            //    }
                            //}

                            // if(!GameIsolate_.globaldef.draw_load_zones) {
                            //     unsigned int offset = (wxd + wyd * GameIsolate_.world->width) * 4;
                            //     dpixels_ar[offset + 2] = 0;        // b
                            //     dpixels_ar[offset + 1] = 0;        // g
                            //     dpixels_ar[offset + 0] = 0xff;        // r
                            //     dpixels_ar[offset + 3] = 0xff;    // a
                            // }

                            break;
                        }
                    }

                    if (!found) {
                        if (GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width].mat->id == MaterialsList::GENERIC_AIR.id) {
                            cur->tiles[tx + ty * cur->matWidth] = Tiles_NOTHING;
                        }
                    }
                }
            }

            for (int tx = 0; tx < cur->surface->w; tx++) {
                for (int ty = 0; ty < cur->surface->h; ty++) {
                    MaterialInstance mat = cur->tiles[tx + ty * cur->surface->w];
                    if (mat.mat->id == MaterialsList::GENERIC_AIR.id) {
                        R_GET_PIXEL(cur->surface, tx, ty) = 0x00000000;
                    } else {
                        R_GET_PIXEL(cur->surface, tx, ty) = (mat.mat->alpha << 24) + (mat.color & 0x00ffffff);
                    }
                }
            }

            R_FreeImage(cur->texture);
            cur->texture = R_CopyImageFromSurface(cur->surface);
            R_SetImageFilter(cur->texture, R_FILTER_NEAREST);

            cur->needsUpdate = true;
        }

        if (GameIsolate_.world->readyToMerge.size() == 0) {
            if (GameIsolate_.globaldef.tick_box2d) GameIsolate_.world->tickObjects();
        }

        if (Time.tickCount % 10 == 0) GameIsolate_.world->tickObjectsMesh();

        for (int i = 0; i < global.GameData_.materials_count; i++) movingTiles[i] = 0;

        results.clear();
        results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
            for (int i = 0; i < GameIsolate_.world->width * GameIsolate_.world->height; i++) {
                const unsigned int offset = i * 4;

                if (GameIsolate_.world->dirty[i]) {
                    hadDirty = true;
                    movingTiles[GameIsolate_.world->tiles[i].mat->id]++;
                    if (GameIsolate_.world->tiles[i].mat->physicsType == PhysicsType::AIR) {
                        dpixels_ar[offset + 0] = 0;                     // b
                        dpixels_ar[offset + 1] = 0;                     // g
                        dpixels_ar[offset + 2] = 0;                     // r
                        dpixels_ar[offset + 3] = ME_ALPHA_TRANSPARENT;  // a

                        dpixelsFire_ar[offset + 0] = 0;                     // b
                        dpixelsFire_ar[offset + 1] = 0;                     // g
                        dpixelsFire_ar[offset + 2] = 0;                     // r
                        dpixelsFire_ar[offset + 3] = ME_ALPHA_TRANSPARENT;  // a

                        dpixelsEmission_ar[offset + 0] = 0;                     // b
                        dpixelsEmission_ar[offset + 1] = 0;                     // g
                        dpixelsEmission_ar[offset + 2] = 0;                     // r
                        dpixelsEmission_ar[offset + 3] = ME_ALPHA_TRANSPARENT;  // a

                        GameIsolate_.world->flowY[i] = 0;
                        GameIsolate_.world->flowX[i] = 0;
                    } else {
                        u32 color = GameIsolate_.world->tiles[i].color;
                        u32 emit = GameIsolate_.world->tiles[i].mat->emitColor;
                        // f32 br = GameIsolate_.world->light[i];
                        dpixels_ar[offset + 2] = ((color >> 0) & 0xff);                    // b
                        dpixels_ar[offset + 1] = ((color >> 8) & 0xff);                    // g
                        dpixels_ar[offset + 0] = ((color >> 16) & 0xff);                   // r
                        dpixels_ar[offset + 3] = GameIsolate_.world->tiles[i].mat->alpha;  // a

                        dpixelsEmission_ar[offset + 2] = ((emit >> 0) & 0xff);   // b
                        dpixelsEmission_ar[offset + 1] = ((emit >> 8) & 0xff);   // g
                        dpixelsEmission_ar[offset + 0] = ((emit >> 16) & 0xff);  // r
                        dpixelsEmission_ar[offset + 3] = ((emit >> 24) & 0xff);  // a

                        if (GameIsolate_.world->tiles[i].mat->id == MaterialsList::FIRE.id) {
                            dpixelsFire_ar[offset + 2] = ((color >> 0) & 0xff);                    // b
                            dpixelsFire_ar[offset + 1] = ((color >> 8) & 0xff);                    // g
                            dpixelsFire_ar[offset + 0] = ((color >> 16) & 0xff);                   // r
                            dpixelsFire_ar[offset + 3] = GameIsolate_.world->tiles[i].mat->alpha;  // a
                            hadFire = true;
                        }
                        if (GameIsolate_.world->tiles[i].mat->physicsType == PhysicsType::SOUP) {

                            f32 newFlowX = GameIsolate_.world->prevFlowX[i] + (GameIsolate_.world->flowX[i] - GameIsolate_.world->prevFlowX[i]) * 0.25;
                            f32 newFlowY = GameIsolate_.world->prevFlowY[i] + (GameIsolate_.world->flowY[i] - GameIsolate_.world->prevFlowY[i]) * 0.25;
                            if (newFlowY < 0) newFlowY *= 0.5;

                            f64 a;
                            // r g b a
                            dpixelsFlow_ar[offset + 2] = 0;
                            a = newFlowY * (3.0 / GameIsolate_.world->tiles[i].mat->iterations + 0.5) / 4.0 + 0.5;
                            dpixelsFlow_ar[offset + 1] = std::min(std::max(a, 0.0), 1.0) * 255;
                            a = newFlowX * (3.0 / GameIsolate_.world->tiles[i].mat->iterations + 0.5) / 4.0 + 0.5;
                            dpixelsFlow_ar[offset + 0] = std::min(std::max(a, 0.0), 1.0) * 255;
                            dpixelsFlow_ar[offset + 3] = 0xff;

                            hadFlow = true;
                            GameIsolate_.world->prevFlowX[i] = newFlowX;
                            GameIsolate_.world->prevFlowY[i] = newFlowY;
                            GameIsolate_.world->flowY[i] = 0;
                            GameIsolate_.world->flowX[i] = 0;
                        } else {
                            GameIsolate_.world->flowY[i] = 0;
                            GameIsolate_.world->flowX[i] = 0;
                        }
                    }
                }
            }
        }));

        // void* vdpixelsLayer2_ar = textureLayer2->data;
        // u8* dpixelsLayer2_ar = (u8*)vdpixelsLayer2_ar;
        u8 *dpixelsLayer2_ar = TexturePack_.pixelsLayer2_ar;
        results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
            for (int i = 0; i < GameIsolate_.world->width * GameIsolate_.world->height; i++) {
                /*for (int x = 0; x < GameIsolate_.world->width; x++) {
for (int y = 0; y < GameIsolate_.world->height; y++) {*/
                // const unsigned int i = x + y * GameIsolate_.world->width;
                const unsigned int offset = i * 4;
                if (GameIsolate_.world->layer2Dirty[i]) {
                    hadLayer2Dirty = true;
                    if (GameIsolate_.world->layer2[i].mat->physicsType == PhysicsType::AIR) {
                        if (GameIsolate_.globaldef.draw_background_grid) {
                            u32 color = ((i) % 2) == 0 ? 0x888888 : 0x444444;
                            dpixelsLayer2_ar[offset + 2] = (color >> 0) & 0xff;   // b
                            dpixelsLayer2_ar[offset + 1] = (color >> 8) & 0xff;   // g
                            dpixelsLayer2_ar[offset + 0] = (color >> 16) & 0xff;  // r
                            dpixelsLayer2_ar[offset + 3] = SDL_ALPHA_OPAQUE;      // a
                            continue;
                        } else {
                            dpixelsLayer2_ar[offset + 0] = 0;                     // b
                            dpixelsLayer2_ar[offset + 1] = 0;                     // g
                            dpixelsLayer2_ar[offset + 2] = 0;                     // r
                            dpixelsLayer2_ar[offset + 3] = ME_ALPHA_TRANSPARENT;  // a
                            continue;
                        }
                    }
                    u32 color = GameIsolate_.world->layer2[i].color;
                    dpixelsLayer2_ar[offset + 2] = (color >> 0) & 0xff;                       // b
                    dpixelsLayer2_ar[offset + 1] = (color >> 8) & 0xff;                       // g
                    dpixelsLayer2_ar[offset + 0] = (color >> 16) & 0xff;                      // r
                    dpixelsLayer2_ar[offset + 3] = GameIsolate_.world->layer2[i].mat->alpha;  // a
                }
            }
        }));

        // void* vdpixelsBackground_ar = textureBackground->data;
        // u8* dpixelsBackground_ar = (u8*)vdpixelsBackground_ar;
        u8 *dpixelsBackground_ar = TexturePack_.pixelsBackground_ar;
        results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
            for (int i = 0; i < GameIsolate_.world->width * GameIsolate_.world->height; i++) {
                /*for (int x = 0; x < GameIsolate_.world->width; x++) {
for (int y = 0; y < GameIsolate_.world->height; y++) {*/
                // const unsigned int i = x + y * GameIsolate_.world->width;
                const unsigned int offset = i * 4;

                if (GameIsolate_.world->backgroundDirty[i]) {
                    hadBackgroundDirty = true;
                    u32 color = GameIsolate_.world->background[i];
                    dpixelsBackground_ar[offset + 2] = (color >> 0) & 0xff;   // b
                    dpixelsBackground_ar[offset + 1] = (color >> 8) & 0xff;   // g
                    dpixelsBackground_ar[offset + 0] = (color >> 16) & 0xff;  // r
                    dpixelsBackground_ar[offset + 3] = (color >> 24) & 0xff;  // a
                }

                //}
            }
        }));

        for (int i = 0; i < GameIsolate_.world->width * GameIsolate_.world->height; i++) {
            /*for (int x = 0; x < GameIsolate_.world->width; x++) {
for (int y = 0; y < GameIsolate_.world->height; y++) {*/
            // const unsigned int i = x + y * GameIsolate_.world->width;
            const unsigned int offset = i * 4;

            if (objectDelete[i]) {
                GameIsolate_.world->tiles[i] = Tiles_NOTHING;
            }
        }

        // results.push_back(updateDirtyPool->push([&](int id) {

        //}));

        for (int i = 0; i < results.size(); i++) {
            results[i].get();
        }

        updateMaterialSounds();

        R_UpdateImageBytes(TexturePack_.textureCells, NULL, &TexturePack_.pixelsCells_ar[0], GameIsolate_.world->width * 4);

        if (hadDirty) memset(GameIsolate_.world->dirty, false, (size_t)GameIsolate_.world->width * GameIsolate_.world->height);
        if (hadLayer2Dirty) memset(GameIsolate_.world->layer2Dirty, false, (size_t)GameIsolate_.world->width * GameIsolate_.world->height);
        if (hadBackgroundDirty) memset(GameIsolate_.world->backgroundDirty, false, (size_t)GameIsolate_.world->width * GameIsolate_.world->height);

        if (GameIsolate_.globaldef.tick_temperature && Time.tickCount % GameTick == 2) {
            GameIsolate_.world->tickTemperature();
        }
        if (GameIsolate_.globaldef.draw_temperature_map && Time.tickCount % GameTick == 0) {
            renderTemperatureMap(GameIsolate_.world.get());
        }

        if (hadDirty) {
            R_UpdateImageBytes(TexturePack_.texture, NULL, &TexturePack_.pixels[0], GameIsolate_.world->width * 4);

            R_UpdateImageBytes(TexturePack_.emissionTexture, NULL, &TexturePack_.pixelsEmission[0], GameIsolate_.world->width * 4);
        }

        if (hadLayer2Dirty) {
            R_UpdateImageBytes(TexturePack_.textureLayer2, NULL, &TexturePack_.pixelsLayer2[0], GameIsolate_.world->width * 4);
        }

        if (hadBackgroundDirty) {
            R_UpdateImageBytes(TexturePack_.textureBackground, NULL, &TexturePack_.pixelsBackground[0], GameIsolate_.world->width * 4);
        }

        if (hadFlow) {
            R_UpdateImageBytes(TexturePack_.textureFlow, NULL, &TexturePack_.pixelsFlow[0], GameIsolate_.world->width * 4);

            GameIsolate_.shaderworker->waterFlowPassShader->dirty = true;
        }

        if (hadFire) {
            R_UpdateImageBytes(TexturePack_.textureFire, NULL, &TexturePack_.pixelsFire[0], GameIsolate_.world->width * 4);
        }

        if (GameIsolate_.globaldef.draw_temperature_map) {
            R_UpdateImageBytes(TexturePack_.temperatureMap, NULL, &TexturePack_.pixelsTemp[0], GameIsolate_.world->width * 4);
        }

        /*R_UpdateImageBytes(
textureObjects,
NULL,
&pixelsObjects[0],
GameIsolate_.world->width * 4
);*/

        if (GameIsolate_.globaldef.tick_box2d && Time.tickCount % GameTick == 0) GameIsolate_.world->updateWorldMesh();
    }
}

void Game::tickChunkLoading() {

    // if need to load chunks
    if ((abs(accLoadX) > CHUNK_W / 2 || abs(accLoadY) > CHUNK_H / 2)) {
        while (GameIsolate_.world->toLoad.size() > 0) {
            // tick chunkloading
            GameIsolate_.world->frame();
        }

        // iterate

        for (int i = 0; i < GameIsolate_.world->width * GameIsolate_.world->height; i++) {
            const unsigned int offset = i * 4;

#define UCH_SET_PIXEL(pix_ar, ofs, c_r, c_g, c_b, c_a) \
    pix_ar[ofs + 0] = c_b;                             \
    pix_ar[ofs + 1] = c_g;                             \
    pix_ar[ofs + 2] = c_r;                             \
    pix_ar[ofs + 3] = c_a;

            if (GameIsolate_.world->dirty[i]) {
                if (GameIsolate_.world->tiles[i].mat->physicsType == PhysicsType::AIR) {
                    UCH_SET_PIXEL(TexturePack_.pixels_ar, offset, 0, 0, 0, ME_ALPHA_TRANSPARENT);
                } else {
                    u32 color = GameIsolate_.world->tiles[i].color;
                    u32 emit = GameIsolate_.world->tiles[i].mat->emitColor;
                    UCH_SET_PIXEL(TexturePack_.pixels_ar, offset, (color >> 0) & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, GameIsolate_.world->tiles[i].mat->alpha);
                    UCH_SET_PIXEL(TexturePack_.pixelsEmission_ar, offset, (emit >> 0) & 0xff, (emit >> 8) & 0xff, (emit >> 16) & 0xff, (emit >> 24) & 0xff);
                }
            }

            if (GameIsolate_.world->layer2Dirty[i]) {
                if (GameIsolate_.world->layer2[i].mat->physicsType == PhysicsType::AIR) {
                    if (GameIsolate_.globaldef.draw_background_grid) {
                        u32 color = ((i) % 2) == 0 ? 0x888888 : 0x444444;
                        UCH_SET_PIXEL(TexturePack_.pixelsLayer2_ar, offset, (color >> 0) & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, SDL_ALPHA_OPAQUE);
                    } else {
                        UCH_SET_PIXEL(TexturePack_.pixelsLayer2_ar, offset, 0, 0, 0, ME_ALPHA_TRANSPARENT);
                    }
                    continue;
                }
                u32 color = GameIsolate_.world->layer2[i].color;
                UCH_SET_PIXEL(TexturePack_.pixelsLayer2_ar, offset, (color >> 0) & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, GameIsolate_.world->layer2[i].mat->alpha);
            }

            if (GameIsolate_.world->backgroundDirty[i]) {
                u32 color = GameIsolate_.world->background[i];
                UCH_SET_PIXEL(TexturePack_.pixelsBackground_ar, offset, (color >> 0) & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, (color >> 24) & 0xff);
            }
#undef UCH_SET_PIXEL
        }

        memset(GameIsolate_.world->dirty, false, (size_t)GameIsolate_.world->width * GameIsolate_.world->height);
        memset(GameIsolate_.world->layer2Dirty, false, (size_t)GameIsolate_.world->width * GameIsolate_.world->height);
        memset(GameIsolate_.world->backgroundDirty, false, (size_t)GameIsolate_.world->width * GameIsolate_.world->height);

        while ((abs(accLoadX) > CHUNK_W / 2 || abs(accLoadY) > CHUNK_H / 2)) {
            int subX = std::fmax(std::fmin(accLoadX, CHUNK_W / 2), -CHUNK_W / 2);
            if (abs(subX) < CHUNK_W / 2) subX = 0;
            int subY = std::fmax(std::fmin(accLoadY, CHUNK_H / 2), -CHUNK_H / 2);
            if (abs(subY) < CHUNK_H / 2) subY = 0;

            GameIsolate_.world->loadZone.x += subX;
            GameIsolate_.world->loadZone.y += subY;

            int delta = 4 * (subX + subY * GameIsolate_.world->width);

            std::vector<std::future<void>> results = {};
            if (delta > 0) {
                results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixels_ar[0]), &(TexturePack_.pixels_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]) - delta,
                                &(TexturePack_.pixels_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]));
                    // rotate(pixels.begin(), pixels.end() - delta, pixels.end());
                }));
                results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsLayer2_ar[0]), &(TexturePack_.pixelsLayer2_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]) - delta,
                                &(TexturePack_.pixelsLayer2_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]));
                    // rotate(pixelsLayer2.begin(), pixelsLayer2.end() - delta, pixelsLayer2.end());
                }));
                results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsBackground_ar[0]), &(TexturePack_.pixelsBackground_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]) - delta,
                                &(TexturePack_.pixelsBackground_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]));
                    // rotate(pixelsBackground.begin(), pixelsBackground.end() - delta, pixelsBackground.end());
                }));
                results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsFire_ar[0]), &(TexturePack_.pixelsFire_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]) - delta,
                                &(TexturePack_.pixelsFire_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]));
                    // rotate(pixelsFire_ar.begin(), pixelsFire_ar.end() - delta, pixelsFire_ar.end());
                }));
                results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsFlow_ar[0]), &(TexturePack_.pixelsFlow_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]) - delta,
                                &(TexturePack_.pixelsFlow_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]));
                    // rotate(pixelsFlow_ar.begin(), pixelsFlow_ar.end() - delta, pixelsFlow_ar.end());
                }));
                results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsEmission_ar[0]), &(TexturePack_.pixelsEmission_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]) - delta,
                                &(TexturePack_.pixelsEmission_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]));
                    // rotate(pixelsEmission_ar.begin(), pixelsEmission_ar.end() - delta, pixelsEmission_ar.end());
                }));
            } else if (delta < 0) {
                results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixels_ar[0]), &(TexturePack_.pixels_ar[0]) - delta, &(TexturePack_.pixels_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]));
                    // rotate(pixels.begin(), pixels.begin() - delta, pixels.end());
                }));
                results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsLayer2_ar[0]), &(TexturePack_.pixelsLayer2_ar[0]) - delta,
                                &(TexturePack_.pixelsLayer2_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]));
                    // rotate(pixelsLayer2.begin(), pixelsLayer2.begin() - delta, pixelsLayer2.end());
                }));
                results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsBackground_ar[0]), &(TexturePack_.pixelsBackground_ar[0]) - delta,
                                &(TexturePack_.pixelsBackground_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]));
                    // rotate(pixelsBackground.begin(), pixelsBackground.begin() - delta, pixelsBackground.end());
                }));
                results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsFire_ar[0]), &(TexturePack_.pixelsFire_ar[0]) - delta, &(TexturePack_.pixelsFire_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]));
                    // rotate(pixelsFire_ar.begin(), pixelsFire_ar.begin() - delta, pixelsFire_ar.end());
                }));
                results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsFlow_ar[0]), &(TexturePack_.pixelsFlow_ar[0]) - delta, &(TexturePack_.pixelsFlow_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]));
                    // rotate(pixelsFlow_ar.begin(), pixelsFlow_ar.begin() - delta, pixelsFlow_ar.end());
                }));
                results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsEmission_ar[0]), &(TexturePack_.pixelsEmission_ar[0]) - delta,
                                &(TexturePack_.pixelsEmission_ar[GameIsolate_.world->width * GameIsolate_.world->height * 4]));
                    // rotate(pixelsEmission_ar.begin(), pixelsEmission_ar.begin() - delta, pixelsEmission_ar.end());
                }));
            }

            for (auto &v : results) {
                v.get();
            }

#define CLEARPIXEL(pixels, ofs)                                 \
    pixels[ofs + 0] = pixels[ofs + 1] = pixels[ofs + 2] = 0xff; \
    pixels[ofs + 3] = ME_ALPHA_TRANSPARENT

#define CLEARPIXEL_C(offset)                              \
    CLEARPIXEL(TexturePack_.pixels_ar, offset);           \
    CLEARPIXEL(TexturePack_.pixelsLayer2_ar, offset);     \
    CLEARPIXEL(TexturePack_.pixelsObjects_ar, offset);    \
    CLEARPIXEL(TexturePack_.pixelsBackground_ar, offset); \
    CLEARPIXEL(TexturePack_.pixelsFire_ar, offset);       \
    CLEARPIXEL(TexturePack_.pixelsFlow_ar, offset);       \
    CLEARPIXEL(TexturePack_.pixelsEmission_ar, offset)

            for (int x = 0; x < abs(subX); x++) {
                for (int y = 0; y < GameIsolate_.world->height; y++) {
                    const unsigned int offset = (GameIsolate_.world->width * 4 * y) + x * 4;
                    if (offset < GameIsolate_.world->width * GameIsolate_.world->height * 4) {
                        CLEARPIXEL_C(offset);
                    }
                }
            }

            for (int y = 0; y < abs(subY); y++) {
                for (int x = 0; x < GameIsolate_.world->width; x++) {
                    const unsigned int offset = (GameIsolate_.world->width * 4 * y) + x * 4;
                    if (offset < GameIsolate_.world->width * GameIsolate_.world->height * 4) {
                        CLEARPIXEL_C(offset);
                    }
                }
            }

#undef CLEARPIXEL_C
#undef CLEARPIXEL

            accLoadX -= subX;
            accLoadY -= subY;

            global.GameData_.ofsX -= subX * Screen.gameScale;
            global.GameData_.ofsY -= subY * Screen.gameScale;
        }

        GameIsolate_.world->tickChunks();
        GameIsolate_.world->updateWorldMesh();
        GameIsolate_.world->dirty[0] = true;
        GameIsolate_.world->layer2Dirty[0] = true;
        GameIsolate_.world->backgroundDirty[0] = true;

    } else {
        GameIsolate_.world->frame();
    }
}

void Game::tickPlayer() {

    Player *pl = nullptr;
    WorldEntity *pl_we = nullptr;

    if (GameIsolate_.world->player) {
        std::tie(pl_we, pl) = GameIsolate_.world->getHostPlayer();
    }

    if (GameIsolate_.world->player) {

        if (ControlSystem::PLAYER_UP->get() && !ControlSystem::DEBUG_DRAW->get()) {
            if (pl_we->ground) {
                pl_we->vy = -4;
                global.audio.PlayEvent("event:/Player/Jump");
            }
        }

        pl_we->vy += (f32)(((ControlSystem::PLAYER_UP->get() && !ControlSystem::DEBUG_DRAW->get()) ? (pl_we->vy > -1 ? -0.8 : -0.35) : 0) + (ControlSystem::PLAYER_DOWN->get() ? 0.1 : 0));
        if (ControlSystem::PLAYER_UP->get() && !ControlSystem::DEBUG_DRAW->get()) {
            global.audio.SetEventParameter("event:/Player/Fly", "Intensity", 1);
            for (int i = 0; i < 4; i++) {
                CellData *p = new CellData(TilesCreateLava(), (f32)(pl_we->x + GameIsolate_.world->loadZone.x + pl_we->hw / 2 + rand() % 5 - 2 + pl_we->vx),
                                           (f32)(pl_we->y + GameIsolate_.world->loadZone.y + pl_we->hh + pl_we->vy), (f32)((rand() % 10 - 5) / 10.0f + pl_we->vx / 2.0f),
                                           (f32)((rand() % 10) / 10.0f + 1 + pl_we->vy / 2.0f), 0, (f32)0.025);
                p->temporary = true;
                p->lifetime = 120;
                GameIsolate_.world->addCell(p);
            }
        } else {
            global.audio.SetEventParameter("event:/Player/Fly", "Intensity", 0);
        }

        if (pl_we->vy > 0) {
            global.audio.SetEventParameter("event:/Player/Wind", "Wind", (f32)(pl_we->vy / 12.0));
        } else {
            global.audio.SetEventParameter("event:/Player/Wind", "Wind", 0);
        }

        pl_we->vx += (f32)((ControlSystem::PLAYER_LEFT->get() ? (pl_we->vx > 0 ? -0.4 : -0.2) : 0) + (ControlSystem::PLAYER_RIGHT->get() ? (pl_we->vx < 0 ? 0.4 : 0.2) : 0));
        if (!ControlSystem::PLAYER_LEFT->get() && !ControlSystem::PLAYER_RIGHT->get()) pl_we->vx *= (f32)(pl_we->ground ? 0.85 : 0.96);
        if (pl_we->vx > 4.5) pl_we->vx = 4.5;
        if (pl_we->vx < -4.5) pl_we->vx = -4.5;
    } else {
        if (state == INGAME) {
            global.GameData_.freeCamX += (f32)((ControlSystem::PLAYER_LEFT->get() ? -5 : 0) + (ControlSystem::PLAYER_RIGHT->get() ? 5 : 0));
            global.GameData_.freeCamY += (f32)((ControlSystem::PLAYER_UP->get() ? -5 : 0) + (ControlSystem::PLAYER_DOWN->get() ? 5 : 0));
        } else {
        }
    }

    if (GameIsolate_.world->player) {

        global.GameData_.desCamX = (f32)(-(mx - (Screen.windowWidth / 2)) / 4);
        global.GameData_.desCamY = (f32)(-(my - (Screen.windowHeight / 2)) / 4);

        pl->holdAngle = (f32)(atan2(global.GameData_.desCamY, global.GameData_.desCamX) * 180 / (f32)M_PI);

        global.GameData_.desCamX = 0;
        global.GameData_.desCamY = 0;
    } else {
        global.GameData_.desCamX = 0;
        global.GameData_.desCamY = 0;
    }

    if (GameIsolate_.world->player) {

        if (pl->heldItem) {
            if (pl->heldItem->getFlag(ItemFlags::ItemFlags_Vacuum)) {
                if (pl->holdtype == Vacuum) {

                    int wcx = (int)((Screen.windowWidth / 2.0f - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
                    int wcy = (int)((Screen.windowHeight / 2.0f - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

                    int wmx = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
                    int wmy = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

                    int mdx = wmx - wcx;
                    int mdy = wmy - wcy;

                    int distSq = mdx * mdx + mdy * mdy;
                    if (distSq <= 256 * 256) {

                        int sind = -1;
                        bool inObject = true;
                        GameIsolate_.world->forLine(wcx, wcy, wmx, wmy, [&](int ind) {
                            if (GameIsolate_.world->tiles[ind].mat->physicsType == PhysicsType::OBJECT) {
                                if (!inObject) {
                                    sind = ind;
                                    return true;
                                }
                            } else {
                                inObject = false;
                            }

                            if (GameIsolate_.world->tiles[ind].mat->physicsType == PhysicsType::SOLID || GameIsolate_.world->tiles[ind].mat->physicsType == PhysicsType::SAND ||
                                GameIsolate_.world->tiles[ind].mat->physicsType == PhysicsType::SOUP) {
                                sind = ind;
                                return true;
                            }
                            return false;
                        });

                        int x = sind == -1 ? wmx : sind % GameIsolate_.world->width;
                        int y = sind == -1 ? wmy : sind / GameIsolate_.world->width;

                        std::function<void(MaterialInstance, int, int)> makeCell = [&](MaterialInstance tile, int xPos, int yPos) {
                            CellData *par = new CellData(tile, xPos, yPos, 0, 0, 0, (f32)0.01f);
                            par->vx = (rand() % 10 - 5) / 5.0f * 1.0f;
                            par->vy = (rand() % 10 - 5) / 5.0f * 1.0f;
                            par->ax = -par->vx / 10.0f;
                            par->ay = -par->vy / 10.0f;
                            if (par->ay == 0 && par->ax == 0) par->ay = 0.01f;

                            // par->targetX = pl_we->x + pl_we->hw / 2 + GameIsolate_.world->loadZone.x;
                            // par->targetY = pl_we->y + pl_we->hh / 2 + GameIsolate_.world->loadZone.y;
                            // par->targetForce = 0.35f;

                            par->lifetime = 6;

                            par->phase = true;

                            pl->heldItem->vacuumCells.push_back(par);

                            par->killCallback = [&par]() {
                                Player *pl = nullptr;
                                WorldEntity *pl_we = nullptr;

                                if (global.game->GameIsolate_.world->player) {
                                    std::tie(pl_we, pl) = global.game->GameIsolate_.world->getHostPlayer();
                                }

                                if (pl->holdtype != EnumPlayerHoldType::None) {
                                    auto &v = pl->heldItem->vacuumCells;
                                    v.erase(std::remove(v.begin(), v.end(), par), v.end());
                                }
                            };

                            GameIsolate_.world->addCell(par);
                        };

                        int rad = 5;
                        int clipRadSq = rad * rad;
                        clipRadSq += rand() % clipRadSq / 4;
                        for (int xx = -rad; xx <= rad; xx++) {
                            for (int yy = -rad; yy <= rad; yy++) {
                                if (xx * xx + yy * yy > clipRadSq) continue;
                                if ((yy == -rad || yy == rad) && (xx == -rad || xx == rad)) {
                                    continue;
                                }

                                MaterialInstance tile = GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width];
                                if (tile.mat->physicsType == PhysicsType::SOLID || tile.mat->physicsType == PhysicsType::SAND || tile.mat->physicsType == PhysicsType::SOUP) {
                                    makeCell(tile, x + xx, y + yy);
                                    GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width] = Tiles_NOTHING;
                                    // GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width] = TilesCreateFire();
                                    GameIsolate_.world->dirty[(x + xx) + (y + yy) * GameIsolate_.world->width] = true;
                                }
                            }
                        }

                        auto func = [&](CellData *cur) {
                            if (cur->targetForce == 0 && !cur->phase) {
                                int rad = 5;
                                for (int xx = -rad; xx <= rad; xx++) {
                                    for (int yy = -rad; yy <= rad; yy++) {
                                        if ((yy == -rad || yy == rad) && (xx == -rad || x == rad)) continue;

                                        if (((int)(cur->x) == (x + xx)) && ((int)(cur->y) == (y + yy))) {

                                            cur->vx = (rand() % 10 - 5) / 5.0f * 1.0f;
                                            cur->vy = (rand() % 10 - 5) / 5.0f * 1.0f;
                                            cur->ax = -cur->vx / 10.0f;
                                            cur->ay = -cur->vy / 10.0f;
                                            if (cur->ay == 0 && cur->ax == 0) cur->ay = 0.01f;

                                            // par->targetX = pl_we->x + pl_we->hw / 2 + GameIsolate_.world->loadZone.x;
                                            // par->targetY = pl_we->y + pl_we->hh / 2 + GameIsolate_.world->loadZone.y;
                                            // par->targetForce = 0.35f;

                                            cur->lifetime = 6;

                                            cur->phase = true;

                                            pl->heldItem->vacuumCells.push_back(cur);

                                            cur->killCallback = [&cur]() {
                                                Player *pl = nullptr;
                                                WorldEntity *pl_we = nullptr;

                                                if (global.game->GameIsolate_.world->player) {
                                                    std::tie(pl_we, pl) = global.game->GameIsolate_.world->getHostPlayer();
                                                }

                                                if (pl->holdtype != EnumPlayerHoldType::None) {
                                                    auto &v = pl->heldItem->vacuumCells;
                                                    v.erase(std::remove(v.begin(), v.end(), cur), v.end());
                                                }
                                            };

                                            return false;
                                        }
                                    }
                                }
                            }

                            return false;
                        };

                        GameIsolate_.world->cells.erase(std::remove_if(GameIsolate_.world->cells.begin(), GameIsolate_.world->cells.end(), func), GameIsolate_.world->cells.end());

                        std::vector<RigidBody *> *rbs = &GameIsolate_.world->rigidBodies;

                        for (size_t i = 0; i < rbs->size(); i++) {
                            RigidBody *cur = (*rbs)[i];
                            if (!static_cast<bool>(cur->surface)) continue;
                            if (cur->body->IsEnabled()) {
                                f32 s = sin(-cur->body->GetAngle());
                                f32 c = cos(-cur->body->GetAngle());
                                bool upd = false;
                                for (int xx = -rad; xx <= rad; xx++) {
                                    for (int yy = -rad; yy <= rad; yy++) {
                                        if ((yy == -rad || yy == rad) && (xx == -rad || x == rad)) continue;
                                        // rotate point

                                        f32 tx = x + xx - cur->body->GetPosition().x;
                                        f32 ty = y + yy - cur->body->GetPosition().y;

                                        int ntx = (int)(tx * c - ty * s);
                                        int nty = (int)(tx * s + ty * c);

                                        if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                            u32 pixel = R_GET_PIXEL(cur->surface, ntx, nty);
                                            if (((pixel >> 24) & 0xff) != 0x00) {
                                                R_GET_PIXEL(cur->surface, ntx, nty) = 0x00000000;
                                                upd = true;

                                                makeCell(MaterialInstance(&MaterialsList::GENERIC_SOLID, pixel), (x + xx), (y + yy));
                                            }
                                        }
                                    }
                                }

                                if (upd) {
                                    R_FreeImage(cur->texture);
                                    cur->texture = R_CopyImageFromSurface(cur->surface);
                                    R_SetImageFilter(cur->texture, R_FILTER_NEAREST);
                                    // GameIsolate_.world->updateRigidBodyHitbox(cur);
                                    cur->needsUpdate = true;
                                }
                            }
                        }
                    }
                }

                if (pl->heldItem->vacuumCells.size() > 0) {
                    pl->heldItem->vacuumCells.erase(std::remove_if(pl->heldItem->vacuumCells.begin(), pl->heldItem->vacuumCells.end(),
                                                                   [&](CellData *cur) {
                                                                       if (cur->lifetime <= 0) {
                                                                           cur->targetForce = 0.45f;
                                                                           cur->targetX = pl_we->x + pl_we->hw / 2.0f + GameIsolate_.world->loadZone.x;
                                                                           cur->targetY = pl_we->y + pl_we->hh / 2.0f + GameIsolate_.world->loadZone.y;
                                                                           cur->ax = 0;
                                                                           cur->ay = 0.01f;
                                                                       }

                                                                       f32 tdx = cur->targetX - cur->x;
                                                                       f32 tdy = cur->targetY - cur->y;

                                                                       if (tdx * tdx + tdy * tdy < 10 * 10) {
                                                                           cur->temporary = true;
                                                                           cur->lifetime = 0;
                                                                           // METADOT_BUG("vacuum {}", cur->tile.mat->name.c_str());
                                                                           return true;
                                                                       }

                                                                       return false;
                                                                   }),
                                                    pl->heldItem->vacuumCells.end());
                }
            }
        }
    }
}

void Game::tickProfiler() {

    if (global.game->GameIsolate_.globaldef.draw_profiler) {
        static profiler_frame data;
        ME_profiler_get_frame(&data);

        // // if (g_multi) ProfilerDrawFrameNavigation(g_frameInfos.data(), g_frameInfos.size());

        static char buffer[10 * 1024];
        profiler_draw_frame(&data, buffer, 10 * 1024);
        // // ProfilerDrawStats(&data);
    }

    // if (MemCurrentUsageBytes() >= Core.max_mem) {
    // }
}

void Game::updateFrameLate() {

    if (state == LOADING) {

    } else {

        // update camera

        int nofsX;
        int nofsY;

        if (GameIsolate_.world->player) {
            auto [pl_we, pl] = GameIsolate_.world->getHostPlayer();

            if (Time.now - Time.lastTickTime <= Time.mspt) {
                f32 thruTick = (f32)((Time.now - Time.lastTickTime) / Time.mspt);

                global.GameData_.plPosX = pl_we->x + (int)(pl_we->vx * thruTick);
                global.GameData_.plPosY = pl_we->y + (int)(pl_we->vy * thruTick);
            } else {
                global.GameData_.plPosX = pl_we->x;
                global.GameData_.plPosY = pl_we->y;
            }

            // plPosX = (f32)(plPosX + (pl_we->x - plPosX) / 25.0);
            // plPosY = (f32)(plPosY + (pl_we->y - plPosY) / 25.0);

            nofsX = (int)(-((int)global.GameData_.plPosX + pl_we->hw / 2 + GameIsolate_.world->loadZone.x) * Screen.gameScale + Screen.windowWidth / 2);
            nofsY = (int)(-((int)global.GameData_.plPosY + pl_we->hh / 2 + GameIsolate_.world->loadZone.y) * Screen.gameScale + Screen.windowHeight / 2);
        } else {
            global.GameData_.plPosX = (f32)(global.GameData_.plPosX + (global.GameData_.freeCamX - global.GameData_.plPosX) / 50.0f);
            global.GameData_.plPosY = (f32)(global.GameData_.plPosY + (global.GameData_.freeCamY - global.GameData_.plPosY) / 50.0f);

            nofsX = (int)(-(global.GameData_.plPosX + 0 + GameIsolate_.world->loadZone.x) * Screen.gameScale + Screen.windowWidth / 2.0f);
            nofsY = (int)(-(global.GameData_.plPosY + 0 + GameIsolate_.world->loadZone.y) * Screen.gameScale + Screen.windowHeight / 2.0f);
        }

        accLoadX += (nofsX - global.GameData_.ofsX) / (f32)Screen.gameScale;
        accLoadY += (nofsY - global.GameData_.ofsY) / (f32)Screen.gameScale;
        // METADOT_BUG("{0:f} {0:f}", plPosX, plPosY);
        // METADOT_BUG("a {0:d} {0:d}", nofsX, nofsY);
        // METADOT_BUG("{0:d} {0:d}", nofsX - ofsX, nofsY - ofsY);
        global.GameData_.ofsX += (nofsX - global.GameData_.ofsX);
        global.GameData_.ofsY += (nofsY - global.GameData_.ofsY);

        global.GameData_.camX = (f32)(global.GameData_.camX + (global.GameData_.desCamX - global.GameData_.camX) * (Time.now - Time.lastTime) / 250.0f);
        global.GameData_.camY = (f32)(global.GameData_.camY + (global.GameData_.desCamY - global.GameData_.camY) * (Time.now - Time.lastTime) / 250.0f);
    }
}

void Game::renderEarly() {

    GameIsolate_.ui->UIRendererPostUpdate();

    if (state == LOADING) {
        if (Time.now - Time.lastLoadingTick > 20) {
            // render loading screen

            unsigned int *ldPixels = (unsigned int *)TexturePack_.pixelsLoading_ar;
            bool anyFalse = false;
            // int drop  = (sin(now / 250.0) + 1) / 2 * loadingScreenW;
            // int drop2 = (-sin(now / 250.0) + 1) / 2 * loadingScreenW;
            for (int x = 0; x < TexturePack_.loadingScreenW; x++) {
                for (int y = TexturePack_.loadingScreenH - 1; y >= 0; y--) {
                    int i = (x + y * TexturePack_.loadingScreenW);
                    bool state = ldPixels[i] == loadingOnColor;

                    if (!state) anyFalse = true;
                    bool newState = state;
                    // newState = rand() % 2;

                    if (!state && y == 0) {
                        if (rand() % 6 == 0) {
                            newState = true;
                        }
                        // if (x >= drop - 1 && x <= drop + 1) {
                        //     newState = true;
                        // } else if (x >= drop2 - 1 && x <= drop2 + 1) {
                        //     newState = true;
                        // }
                    }

                    if (state && y < TexturePack_.loadingScreenH - 1) {
                        if (ldPixels[(x + (y + 1) * TexturePack_.loadingScreenW)] == loadingOffColor) {
                            ldPixels[(x + (y + 1) * TexturePack_.loadingScreenW)] = loadingOnColor;
                            newState = false;
                        } else {
                            bool canLeft = x > 0 && ldPixels[((x - 1) + (y + 1) * TexturePack_.loadingScreenW)] == loadingOffColor;
                            bool canRight = x < TexturePack_.loadingScreenW - 1 && ldPixels[((x + 1) + (y + 1) * TexturePack_.loadingScreenW)] == loadingOffColor;
                            if (canLeft && !(canRight && (rand() % 2 == 0))) {
                                ldPixels[((x - 1) + (y + 1) * TexturePack_.loadingScreenW)] = loadingOnColor;
                                newState = false;
                            } else if (canRight) {
                                ldPixels[((x + 1) + (y + 1) * TexturePack_.loadingScreenW)] = loadingOnColor;
                                newState = false;
                            }
                        }
                    }

                    ldPixels[(x + y * TexturePack_.loadingScreenW)] = (newState ? loadingOnColor : loadingOffColor);
                    int sx = Screen.windowWidth / TexturePack_.loadingScreenW;
                    int sy = Screen.windowHeight / TexturePack_.loadingScreenH;
                    // R_RectangleFilled(target, x * sx, y * sy, x * sx + sx, y * sy + sy, state ? SDL_Color{ 0xff, 0, 0, 0xff } : SDL_Color{ 0, 0xff, 0, 0xff });
                }
            }
            if (!anyFalse) {
                u32 tmp = loadingOnColor;
                loadingOnColor = loadingOffColor;
                loadingOffColor = tmp;
            }

            R_UpdateImageBytes(TexturePack_.loadingTexture, NULL, &TexturePack_.pixelsLoading_ar[0], TexturePack_.loadingScreenW * 4);

            Time.lastLoadingTick = Time.now;
        } else {
            // #ifdef _WIN32
            //             Sleep(5);
            // #else
            //             sleep(5 / 1000.0f);
            // #endif
        }
        R_ActivateShaderProgram(0, NULL);
        R_BlitRect(TexturePack_.loadingTexture, NULL, Render.target, NULL);

        MetaEngine::Drawing::drawText("Loading...", {255, 255, 255, 255}, Screen.windowWidth / 2, Screen.windowHeight / 2 - 32);

    } else {
        // render entities with LERP

        if (Time.now - Time.lastTickTime <= Time.mspt) {
            R_Clear(TexturePack_.textureEntities->target);
            R_Clear(TexturePack_.textureEntitiesLQ->target);
            if (GameIsolate_.world->player) {
                f32 thruTick = (f32)((Time.now - Time.lastTickTime) / Time.mspt);

                R_SetBlendMode(TexturePack_.textureEntities, R_BLEND_ADD);
                R_SetBlendMode(TexturePack_.textureEntitiesLQ, R_BLEND_ADD);
                int scaleEnt = GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1;

                move_player_event e{1.0f, thruTick, this};
                GameIsolate_.world->Reg().process_event(e);

                if (GameIsolate_.world->player) {
                    auto [pl_we, pl] = GameIsolate_.world->getHostPlayer();

                    if (pl->heldItem != NULL) {
                        if (pl->heldItem->getFlag(ItemFlags::ItemFlags_Hammer)) {
                            if (pl->holdtype == Hammer) {
                                int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
                                int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

                                int dx = x - pl->hammerX;
                                int dy = y - pl->hammerY;
                                f32 len = sqrt(dx * dx + dy * dy);
                                if (len > 40) {
                                    dx = dx / len * 40;
                                    dy = dy / len * 40;
                                }

                                R_Line(TexturePack_.textureEntitiesLQ->target, pl->hammerX + dx, pl->hammerY + dy, pl->hammerX, pl->hammerY, {0xff, 0xff, 0x00, 0xff});
                            } else {
                                int startInd = getAimSolidSurface(64);

                                if (startInd != -1) {
                                    int x = startInd % GameIsolate_.world->width;
                                    int y = startInd / GameIsolate_.world->width;
                                    R_Rectangle(TexturePack_.textureEntitiesLQ->target, x - 1, y - 1, x + 1, y + 1, {0xff, 0xff, 0x00, 0xE0});
                                }
                            }
                        }
                    }
                }
                R_SetBlendMode(TexturePack_.textureEntities, R_BLEND_NORMAL);
                R_SetBlendMode(TexturePack_.textureEntitiesLQ, R_BLEND_NORMAL);
            }
        }

        if (ControlSystem::mmouse_down) {
            int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
            int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);
            R_RectangleFilled(TexturePack_.textureEntitiesLQ->target, x - gameUI.DebugDrawUI__brushSize / 2.0f, y - gameUI.DebugDrawUI__brushSize / 2.0f,
                              x + (int)(ceil(gameUI.DebugDrawUI__brushSize / 2.0)), y + (int)(ceil(gameUI.DebugDrawUI__brushSize / 2.0)), {0xff, 0x40, 0x40, 0x90});
            R_Rectangle(TexturePack_.textureEntitiesLQ->target, x - gameUI.DebugDrawUI__brushSize / 2.0f, y - gameUI.DebugDrawUI__brushSize / 2.0f,
                        x + (int)(ceil(gameUI.DebugDrawUI__brushSize / 2.0)) + 1, y + (int)(ceil(gameUI.DebugDrawUI__brushSize / 2.0)) + 1, {0xff, 0x40, 0x40, 0xE0});
        } else if (ControlSystem::DEBUG_DRAW->get()) {
            int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
            int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);
            R_RectangleFilled(TexturePack_.textureEntitiesLQ->target, x - gameUI.DebugDrawUI__brushSize / 2.0f, y - gameUI.DebugDrawUI__brushSize / 2.0f,
                              x + (int)(ceil(gameUI.DebugDrawUI__brushSize / 2.0)), y + (int)(ceil(gameUI.DebugDrawUI__brushSize / 2.0)), {0x00, 0xff, 0xB0, 0x80});
            R_Rectangle(TexturePack_.textureEntitiesLQ->target, x - gameUI.DebugDrawUI__brushSize / 2.0f, y - gameUI.DebugDrawUI__brushSize / 2.0f,
                        x + (int)(ceil(gameUI.DebugDrawUI__brushSize / 2.0)) + 1, y + (int)(ceil(gameUI.DebugDrawUI__brushSize / 2.0)) + 1, {0x00, 0xff, 0xB0, 0xE0});
        }
    }
}

void Game::renderLate() {

    Render.target = TexturePack_.backgroundImage->target;
    R_Clear(Render.target);

    if (state == LOADING) {

    } else {
        // draw backgrounds

        BackgroundObject *bg = GameIsolate_.backgrounds->Get("TEST_OVERWORLD");
        if (!bg->layers.empty() && GameIsolate_.globaldef.draw_background && Screen.gameScale <= bg->layers[0]->surface.size() && GameIsolate_.world->loadZone.y > -5 * CHUNK_H) {
            R_SetShapeBlendMode(R_BLEND_SET);
            ME_Color col = {static_cast<u8>((bg->solid >> 16) & 0xff), static_cast<u8>((bg->solid >> 8) & 0xff), static_cast<u8>((bg->solid >> 0) & 0xff), 0xff};
            R_ClearColor(Render.target, col);

            ME_rect *dst = new ME_rect;
            ME_rect *src = new ME_rect;

            f32 arX = (f32)Screen.windowWidth / (bg->layers[0]->surface[0]->w);
            f32 arY = (f32)Screen.windowHeight / (bg->layers[0]->surface[0]->h);

            f64 time = ME_gettime() / 1000.0;

            R_SetShapeBlendMode(R_BLEND_NORMAL);

            for (size_t i = 0; i < bg->layers.size(); i++) {
                BackgroundLayer *cur = bg->layers[i].get();

                C_Surface *texture = cur->surface[(size_t)Screen.gameScale - 1];

                R_Image *tex = cur->texture[(size_t)Screen.gameScale - 1];
                R_SetBlendMode(tex, R_BLEND_NORMAL);

                int tw = texture->w;
                int th = texture->h;

                int iter = (int)ceil((f32)Screen.windowWidth / (tw)) + 1;
                for (int n = 0; n < iter; n++) {

                    src->x = 0;
                    src->y = 0;
                    src->w = tw;
                    src->h = th;

                    dst->x = (((global.GameData_.ofsX + global.GameData_.camX) + GameIsolate_.world->loadZone.x * Screen.gameScale) + n * tw / cur->parralaxX) * cur->parralaxX +
                             GameIsolate_.world->width / 2.0f * Screen.gameScale - tw / 2.0f;
                    dst->y = ((global.GameData_.ofsY + global.GameData_.camY) + GameIsolate_.world->loadZone.y * Screen.gameScale) * cur->parralaxY +
                             GameIsolate_.world->height / 2.0f * Screen.gameScale - th / 2.0f - Screen.windowHeight / 3.0f * (Screen.gameScale - 1);
                    dst->w = (f32)tw;
                    dst->h = (f32)th;

                    dst->x += (f32)(Screen.gameScale * fmod(cur->moveX * time, tw));

                    // TODO: optimize
                    while (dst->x >= Screen.windowWidth - 10) dst->x -= (iter * tw);
                    while (dst->x + dst->w < 0) dst->x += (iter * tw - 1);

                    // TODO: optimize
                    if (dst->x < 0) {
                        dst->w += dst->x;
                        src->x -= (int)dst->x;
                        src->w += (int)dst->x;
                        dst->x = 0;
                    }

                    if (dst->y < 0) {
                        dst->h += dst->y;
                        src->y -= (int)dst->y;
                        src->h += (int)dst->y;
                        dst->y = 0;
                    }

                    if (dst->x + dst->w >= Screen.windowWidth) {
                        src->w -= (int)((dst->x + dst->w) - Screen.windowWidth);
                        dst->w += Screen.windowWidth - (dst->x + dst->w);
                    }

                    if (dst->y + dst->h >= Screen.windowHeight) {
                        src->h -= (int)((dst->y + dst->h) - Screen.windowHeight);
                        dst->h += Screen.windowHeight - (dst->y + dst->h);
                    }

                    R_BlitRect(tex, src, Render.target, dst);
                }
            }

            delete dst;
            delete src;
        }

        ME_rect r1 = ME_rect{(f32)(global.GameData_.ofsX + global.GameData_.camX), (f32)(global.GameData_.ofsY + global.GameData_.camY), (f32)(GameIsolate_.world->width * Screen.gameScale),
                             (f32)(GameIsolate_.world->height * Screen.gameScale)};
        R_SetBlendMode(TexturePack_.textureBackground, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureBackground, NULL, Render.target, &r1);

        R_SetBlendMode(TexturePack_.textureLayer2, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureLayer2, NULL, Render.target, &r1);

        R_SetBlendMode(TexturePack_.textureObjectsBack, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureObjectsBack, NULL, Render.target, &r1);

        // shader

        if (GameIsolate_.globaldef.draw_shaders) {

            if (GameIsolate_.shaderworker->waterFlowPassShader->dirty && GameIsolate_.globaldef.water_showFlow) {
                GameIsolate_.shaderworker->waterFlowPassShader->Activate();
                GameIsolate_.shaderworker->waterFlowPassShader->Update(GameIsolate_.world->width, GameIsolate_.world->height);
                R_SetBlendMode(TexturePack_.textureFlow, R_BLEND_SET);
                R_BlitRect(TexturePack_.textureFlow, NULL, TexturePack_.textureFlowSpead->target, NULL);

                GameIsolate_.shaderworker->waterFlowPassShader->dirty = false;
            }

            GameIsolate_.shaderworker->waterShader->Activate();
            f32 t = (Time.now - Time.startTime) / 1000.0;
            GameIsolate_.shaderworker->waterShader->Update(t, Render.target->w * Screen.gameScale, Render.target->h * Screen.gameScale, TexturePack_.texture, r1.x, r1.y, r1.w, r1.h, Screen.gameScale,
                                                           TexturePack_.textureFlowSpead, GameIsolate_.globaldef.water_overlay, GameIsolate_.globaldef.water_showFlow,
                                                           GameIsolate_.globaldef.water_pixelated);
        }

        Render.target = Render.realTarget;

        R_BlitRect(TexturePack_.backgroundImage, NULL, Render.target, NULL);

        R_SetBlendMode(TexturePack_.texture, R_BLEND_NORMAL);
        R_ActivateShaderProgram(0, NULL);

        // done shader

        int lmsx = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
        int lmsy = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

        R_Clear(TexturePack_.worldTexture->target);

        R_BlitRect(TexturePack_.texture, NULL, TexturePack_.worldTexture->target, NULL);

        R_SetBlendMode(TexturePack_.textureObjects, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureObjects, NULL, TexturePack_.worldTexture->target, NULL);
        R_SetBlendMode(TexturePack_.textureObjectsLQ, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureObjectsLQ, NULL, TexturePack_.worldTexture->target, NULL);

        R_SetBlendMode(TexturePack_.textureCells, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureCells, NULL, TexturePack_.worldTexture->target, NULL);

        R_SetBlendMode(TexturePack_.textureEntitiesLQ, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureEntitiesLQ, NULL, TexturePack_.worldTexture->target, NULL);
        R_SetBlendMode(TexturePack_.textureEntities, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureEntities, NULL, TexturePack_.worldTexture->target, NULL);

        if (GameIsolate_.globaldef.draw_shaders) {
            GameIsolate_.shaderworker->newLightingShader->Activate();
            // GameIsolate_.shaderworker->crtShader->Activate();
        }

        // GameIsolate_.shaderworker->crtShader->Update(GameIsolate_.world->width, GameIsolate_.world->height);

        // I use this to only rerender the lighting when a parameter changes or N times per second anyway
        // Doing this massively reduces the GPU load of the shader
        bool needToRerenderLighting = false;

        static long long lastLightingForceRefresh = 0;
        long long now = ME_gettime();
        if (now - lastLightingForceRefresh > 100) {
            lastLightingForceRefresh = now;
            needToRerenderLighting = true;
        }

        if (GameIsolate_.globaldef.draw_shaders && GameIsolate_.world) {
            f32 lightTx;
            f32 lightTy;

            if (GameIsolate_.world->player) {
                auto [pl_we, pl] = GameIsolate_.world->getHostPlayer();

                lightTx = (GameIsolate_.world->loadZone.x + pl_we->x + pl_we->hw / 2.0f) / (f32)GameIsolate_.world->width;
                lightTy = (GameIsolate_.world->loadZone.y + pl_we->y + pl_we->hh / 2.0f) / (f32)GameIsolate_.world->height;
            } else {
                lightTx = lmsx / (f32)GameIsolate_.world->width;
                lightTy = lmsy / (f32)GameIsolate_.world->height;
            }

            if (GameIsolate_.shaderworker->newLightingShader->lastLx != lightTx || GameIsolate_.shaderworker->newLightingShader->lastLy != lightTy) needToRerenderLighting = true;
            GameIsolate_.shaderworker->newLightingShader->Update(TexturePack_.worldTexture, TexturePack_.emissionTexture, lightTx, lightTy);
            if (GameIsolate_.shaderworker->newLightingShader->lastQuality != GameIsolate_.globaldef.lightingQuality) {
                needToRerenderLighting = true;
            }
            GameIsolate_.shaderworker->newLightingShader->SetQuality(GameIsolate_.globaldef.lightingQuality);

            int nBg = 0;
            int range = 64;
            for (int xx = std::max(0, (int)(lightTx * GameIsolate_.world->width) - range); xx <= std::min((int)(lightTx * GameIsolate_.world->width) + range, GameIsolate_.world->width - 1); xx++) {
                for (int yy = std::max(0, (int)(lightTy * GameIsolate_.world->height) - range); yy <= std::min((int)(lightTy * GameIsolate_.world->height) + range, GameIsolate_.world->height - 1);
                     yy++) {
                    if (GameIsolate_.world->background[xx + yy * GameIsolate_.world->width] != 0x00) {
                        nBg++;
                    }
                }
            }

            GameIsolate_.shaderworker->newLightingShader->insideDes = std::min(std::max(0.0f, (f32)nBg / ((range * 2) * (range * 2))), 1.0f);
            GameIsolate_.shaderworker->newLightingShader->insideCur +=
                    (GameIsolate_.shaderworker->newLightingShader->insideDes - GameIsolate_.shaderworker->newLightingShader->insideCur) / 2.0f * (Time.deltaTime / 1000.0f);

            f32 ins = GameIsolate_.shaderworker->newLightingShader->insideCur < 0.05 ? 0.0 : GameIsolate_.shaderworker->newLightingShader->insideCur;
            if (GameIsolate_.shaderworker->newLightingShader->lastInside != ins) needToRerenderLighting = true;
            GameIsolate_.shaderworker->newLightingShader->SetInside(ins);
            GameIsolate_.shaderworker->newLightingShader->SetBounds(GameIsolate_.world->tickZone.x * GameIsolate_.globaldef.hd_objects_size,
                                                                    GameIsolate_.world->tickZone.y * GameIsolate_.globaldef.hd_objects_size,
                                                                    (GameIsolate_.world->tickZone.x + GameIsolate_.world->tickZone.w) * GameIsolate_.globaldef.hd_objects_size,
                                                                    (GameIsolate_.world->tickZone.y + GameIsolate_.world->tickZone.h) * GameIsolate_.globaldef.hd_objects_size);

            if (GameIsolate_.shaderworker->newLightingShader->lastSimpleMode != GameIsolate_.globaldef.simpleLighting) needToRerenderLighting = true;
            GameIsolate_.shaderworker->newLightingShader->SetSimpleMode(GameIsolate_.globaldef.simpleLighting);

            if (GameIsolate_.shaderworker->newLightingShader->lastEmissionEnabled != GameIsolate_.globaldef.lightingEmission) needToRerenderLighting = true;
            GameIsolate_.shaderworker->newLightingShader->SetEmissionEnabled(GameIsolate_.globaldef.lightingEmission);

            if (GameIsolate_.shaderworker->newLightingShader->lastDitheringEnabled != GameIsolate_.globaldef.lightingDithering) needToRerenderLighting = true;
            GameIsolate_.shaderworker->newLightingShader->SetDitheringEnabled(GameIsolate_.globaldef.lightingDithering);
        }

        if (GameIsolate_.globaldef.draw_shaders && needToRerenderLighting) {
            R_Clear(TexturePack_.lightingTexture->target);
            R_BlitRect(TexturePack_.worldTexture, NULL, TexturePack_.lightingTexture->target, NULL);
        }
        if (GameIsolate_.globaldef.draw_shaders) R_ActivateShaderProgram(0, NULL);

        R_BlitRect(TexturePack_.worldTexture, NULL, Render.target, &r1);

        if (GameIsolate_.globaldef.draw_shaders) {
            R_SetBlendMode(TexturePack_.lightingTexture, GameIsolate_.globaldef.draw_light_overlay ? R_BLEND_NORMAL : R_BLEND_MULTIPLY);
            R_BlitRect(TexturePack_.lightingTexture, NULL, Render.target, &r1);
        }

        if (GameIsolate_.globaldef.draw_shaders) {
            R_Clear(TexturePack_.texture2Fire->target);

            GameIsolate_.shaderworker->fireShader->Activate();
            GameIsolate_.shaderworker->fireShader->Update(TexturePack_.textureFire);
            R_BlitRect(TexturePack_.textureFire, NULL, TexturePack_.texture2Fire->target, NULL);
            R_ActivateShaderProgram(0, NULL);

            GameIsolate_.shaderworker->fire2Shader->Activate();
            GameIsolate_.shaderworker->fire2Shader->Update(TexturePack_.texture2Fire);
            R_BlitRect(TexturePack_.texture2Fire, NULL, Render.target, &r1);
            R_ActivateShaderProgram(0, NULL);
        }

        // done light

        renderOverlays();
    }
}

void Game::renderOverlays() {

    char fpsText[50];
    snprintf(fpsText, sizeof(fpsText), "%.1f ms/frame (%.1f(%d) FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate, Time.feelsLikeFps);
    MetaEngine::Drawing::drawText(fpsText, {255, 255, 255, 255}, Screen.windowWidth - ImGui::CalcTextSize(fpsText).x, 0);

    ME_rect r1 = ME_rect{(f32)(global.GameData_.ofsX + global.GameData_.camX), (f32)(global.GameData_.ofsY + global.GameData_.camY), (f32)(GameIsolate_.world->width * Screen.gameScale),
                         (f32)(GameIsolate_.world->height * Screen.gameScale)};
    ME_rect r2 = ME_rect{(f32)(global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->tickZone.x * Screen.gameScale),
                         (f32)(global.GameData_.ofsY + global.GameData_.camY + GameIsolate_.world->tickZone.y * Screen.gameScale), (f32)(GameIsolate_.world->tickZone.w * Screen.gameScale),
                         (f32)(GameIsolate_.world->tickZone.h * Screen.gameScale)};

    if (GameIsolate_.globaldef.draw_temperature_map) {
        R_SetBlendMode(TexturePack_.temperatureMap, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.temperatureMap, NULL, Render.target, &r1);
    }

    if (GameIsolate_.globaldef.draw_load_zones) {
        ME_rect r2m = ME_rect{(f32)(global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->meshZone.x * Screen.gameScale),
                              (f32)(global.GameData_.ofsY + global.GameData_.camY + GameIsolate_.world->meshZone.y * Screen.gameScale), (f32)(GameIsolate_.world->meshZone.w * Screen.gameScale),
                              (f32)(GameIsolate_.world->meshZone.h * Screen.gameScale)};

        R_Rectangle2(Render.target, r2m, {0x00, 0xff, 0xff, 0xff});
        MetaEngine::Drawing::drawTextWithPlate(Render.target, CC("刚体物理更新区域"), {255, 255, 255, 255}, r2m.x + 4, r2m.y + 4);
        R_Rectangle2(Render.target, r2, {0xff, 0x00, 0x00, 0xff});
    }

    if (GameIsolate_.globaldef.draw_load_zones) {

        ME_Color col = {0xff, 0x00, 0x00, 0x20};
        R_SetShapeBlendMode(R_BLEND_NORMAL);

        ME_rect r3 = ME_rect{(f32)(0), (f32)(0), (f32)((global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->tickZone.x * Screen.gameScale)), (f32)(Screen.windowHeight)};
        R_Rectangle2(Render.target, r3, col);

        ME_rect r4 = ME_rect{
                (f32)(global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->tickZone.x * Screen.gameScale + GameIsolate_.world->tickZone.w * Screen.gameScale), (f32)(0),
                (f32)((Screen.windowWidth) - (global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->tickZone.x * Screen.gameScale + GameIsolate_.world->tickZone.w * Screen.gameScale)),
                (f32)(Screen.windowHeight)};
        R_Rectangle2(Render.target, r4, col);

        ME_rect r5 = ME_rect{(f32)(global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->tickZone.x * Screen.gameScale), (f32)(0),
                             (f32)(GameIsolate_.world->tickZone.w * Screen.gameScale), (f32)(global.GameData_.ofsY + global.GameData_.camY + GameIsolate_.world->tickZone.y * Screen.gameScale)};
        R_Rectangle2(Render.target, r5, col);

        ME_rect r6 = ME_rect{
                (f32)(global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->tickZone.x * Screen.gameScale),
                (f32)(global.GameData_.ofsY + global.GameData_.camY + GameIsolate_.world->tickZone.y * Screen.gameScale + GameIsolate_.world->tickZone.h * Screen.gameScale),
                (f32)(GameIsolate_.world->tickZone.w * Screen.gameScale),
                (f32)(Screen.windowHeight - (global.GameData_.ofsY + global.GameData_.camY + GameIsolate_.world->tickZone.y * Screen.gameScale + GameIsolate_.world->tickZone.h * Screen.gameScale))};
        R_Rectangle2(Render.target, r6, col);

        col = {0x00, 0xff, 0x00, 0xff};
        ME_rect r7 = ME_rect{(f32)(global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->width / 2 * Screen.gameScale - (Screen.windowWidth / 3 * Screen.gameScale / 2)),
                             (f32)(global.GameData_.ofsY + global.GameData_.camY + GameIsolate_.world->height / 2 * Screen.gameScale - (Screen.windowHeight / 3 * Screen.gameScale / 2)),
                             (f32)(Screen.windowWidth / 3 * Screen.gameScale), (f32)(Screen.windowHeight / 3 * Screen.gameScale)};
        R_Rectangle2(Render.target, r7, col);
    }

    if (GameIsolate_.globaldef.draw_physics_debug) {
        //
        // for(size_t i = 0; i < GameIsolate_.world->rigidBodies.size(); i++) {
        //    RigidBody cur = *GameIsolate_.world->rigidBodies[i];

        //    f32 x = cur.body->GetPosition().x;
        //    f32 y = cur.body->GetPosition().y;
        //    x = ((x)*Screen.gameScale + ofsX + camX);
        //    y = ((y)*Screen.gameScale + ofsY + camY);

        //    /*SDL_Rect* r = new SDL_Rect{ (int)x, (int)y, cur.surface->w * Screen.gameScale, cur.surface->h * Screen.gameScale };
        //    SDL_RenderCopyEx(renderer, cur.texture, NULL, r, cur.body->GetAngle() * 180 / M_PI, new SDL_Point{ 0, 0 }, SDL_RendererFlip::SDL_FLIP_NONE);
        //    delete r;*/

        //    u32 color = 0x0000ff;

        //    SDL_Color col = {(color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff};

        //    b2Fixture* fix = cur.body->GetFixtureList();
        //    while(fix) {
        //        b2Shape* shape = fix->GetShape();

        //        switch(shape->GetType()) {
        //        case b2Shape::Type::e_polygon:
        //            b2PolygonShape* poly = (b2PolygonShape*)shape;
        //            b2Vec2* verts = poly->m_vertices;

        //            Drawing::drawPolygon(target, col, verts, (int)x, (int)y, Screen.gameScale, poly->m_count, cur.body->GetAngle()/* + fmod((ME_gettime() / 1000.0), 360)*/, 0, 0);

        //            break;
        //        }

        //        fix = fix->GetNext();
        //    }
        //}

        // if(GameIsolate_.world->player) {
        //     RigidBody cur = *pl->rb;

        //    f32 x = cur.body->GetPosition().x;
        //    f32 y = cur.body->GetPosition().y;
        //    x = ((x)*Screen.gameScale + ofsX + camX);
        //    y = ((y)*Screen.gameScale + ofsY + camY);

        //    u32 color = 0x0000ff;

        //    SDL_Color col = {(color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff};

        //    b2Fixture* fix = cur.body->GetFixtureList();
        //    while(fix) {
        //        b2Shape* shape = fix->GetShape();
        //        switch(shape->GetType()) {
        //        case b2Shape::Type::e_polygon:
        //            b2PolygonShape* poly = (b2PolygonShape*)shape;
        //            b2Vec2* verts = poly->m_vertices;

        //            Drawing::drawPolygon(target, col, verts, (int)x, (int)y, Screen.gameScale, poly->m_count, cur.body->GetAngle()/* + fmod((ME_gettime() / 1000.0), 360)*/, 0, 0);

        //            break;
        //        }

        //        fix = fix->GetNext();
        //    }
        //}

        // for(size_t i = 0; i < GameIsolate_.world->worldRigidBodies.size(); i++) {
        //     RigidBody cur = *GameIsolate_.world->worldRigidBodies[i];

        //    f32 x = cur.body->GetPosition().x;
        //    f32 y = cur.body->GetPosition().y;
        //    x = ((x)*Screen.gameScale + ofsX + camX);
        //    y = ((y)*Screen.gameScale + ofsY + camY);

        //    u32 color = 0x00ff00;

        //    SDL_Color col = {(color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff};

        //    b2Fixture* fix = cur.body->GetFixtureList();
        //    while(fix) {
        //        b2Shape* shape = fix->GetShape();
        //        switch(shape->GetType()) {
        //        case b2Shape::Type::e_polygon:
        //            b2PolygonShape* poly = (b2PolygonShape*)shape;
        //            b2Vec2* verts = poly->m_vertices;

        //            Drawing::drawPolygon(target, col, verts, (int)x, (int)y, Screen.gameScale, poly->m_count, cur.body->GetAngle()/* + fmod((ME_gettime() / 1000.0), 360)*/, 0, 0);

        //            break;
        //        }

        //        fix = fix->GetNext();
        //    }

        //    /*color = 0xff0000;
        //    SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff);
        //    SDL_RenderDrawPoint(renderer, x, y);
        //    color = 0x00ff00;
        //    SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff);
        //    SDL_RenderDrawPoint(renderer, cur.body->GetLocalCenter().x * Screen.gameScale + ofsX, cur.body->GetLocalCenter().y * Screen.gameScale + ofsY);*/
        //}

        int minChX = (int)floor((GameIsolate_.world->meshZone.x - GameIsolate_.world->loadZone.x) / CHUNK_W);
        int minChY = (int)floor((GameIsolate_.world->meshZone.y - GameIsolate_.world->loadZone.y) / CHUNK_H);
        int maxChX = (int)ceil((GameIsolate_.world->meshZone.x + GameIsolate_.world->meshZone.w - GameIsolate_.world->loadZone.x) / CHUNK_W);
        int maxChY = (int)ceil((GameIsolate_.world->meshZone.y + GameIsolate_.world->meshZone.h - GameIsolate_.world->loadZone.y) / CHUNK_H);

        for (int cx = minChX; cx <= maxChX; cx++) {
            for (int cy = minChY; cy <= maxChY; cy++) {
                Chunk *ch = GameIsolate_.world->getChunk(cx, cy);
                SDL_Color col = {255, 0, 0, 255};

                f32 x = ((ch->x * CHUNK_W + GameIsolate_.world->loadZone.x) * Screen.gameScale + global.GameData_.ofsX + global.GameData_.camX);
                f32 y = ((ch->y * CHUNK_H + GameIsolate_.world->loadZone.y) * Screen.gameScale + global.GameData_.ofsY + global.GameData_.camY);

                R_Rectangle(Render.target, x, y, x + CHUNK_W * Screen.gameScale, y + CHUNK_H * Screen.gameScale, {50, 50, 0, 255});

                // for(int i = 0; i < ch->polys.size(); i++) {
                //     Drawing::drawPolygon(target, col, ch->polys[i].m_vertices, (int)x, (int)y, Screen.gameScale, ch->polys[i].m_count, 0/* + fmod((ME_gettime() / 1000.0), 360)*/, 0, 0);
                // }
            }
        }

        //

        GameIsolate_.world->b2world->SetDebugDraw(debugDraw);
        debugDraw->scale = Screen.gameScale;
        debugDraw->xOfs = global.GameData_.ofsX + global.GameData_.camX;
        debugDraw->yOfs = global.GameData_.ofsY + global.GameData_.camY;
        debugDraw->SetFlags(0);
        if (GameIsolate_.globaldef.draw_b2d_shape) debugDraw->AppendFlags(DebugDraw::e_shapeBit);
        if (GameIsolate_.globaldef.draw_b2d_joint) debugDraw->AppendFlags(DebugDraw::e_jointBit);
        if (GameIsolate_.globaldef.draw_b2d_aabb) debugDraw->AppendFlags(DebugDraw::e_aabbBit);
        if (GameIsolate_.globaldef.draw_b2d_pair) debugDraw->AppendFlags(DebugDraw::e_pairBit);
        if (GameIsolate_.globaldef.draw_b2d_centerMass) debugDraw->AppendFlags(DebugDraw::e_centerOfMassBit);
        GameIsolate_.world->b2world->DebugDraw();
    }

    // Drawing::drawText("fps",
    //                   std::format("{} FPS\n Feels Like: {} FPS", Time.fps,
    //                               Time.feelsLikeFps),
    //                   Screen.windowWidth, 20);

    if (GameIsolate_.globaldef.draw_chunk_state) {

        int chSize = 10;

        int centerX = Screen.windowWidth / 2;
        int centerY = CHUNK_UNLOAD_DIST * chSize + 10;

        int pposX = global.GameData_.plPosX;
        int pposY = global.GameData_.plPosY;
        int pchx = (int)((pposX / CHUNK_W) * chSize);
        int pchy = (int)((pposY / CHUNK_H) * chSize);
        int pchxf = (int)(((f32)pposX / CHUNK_W) * chSize);
        int pchyf = (int)(((f32)pposY / CHUNK_H) * chSize);

        R_Rectangle(Render.target, centerX - chSize * CHUNK_UNLOAD_DIST + chSize, centerY - chSize * CHUNK_UNLOAD_DIST + chSize, centerX + chSize * CHUNK_UNLOAD_DIST + chSize,
                    centerY + chSize * CHUNK_UNLOAD_DIST + chSize, {0xcc, 0xcc, 0xcc, 0xff});

        ME_rect r = {0, 0, (f32)chSize, (f32)chSize};
        for (auto &p : GameIsolate_.world->chunkCache) {
            if (p.first == INT_MIN) continue;
            int cx = p.first;
            for (auto &p2 : p.second) {
                if (p2.first == INT_MIN) continue;
                int cy = p2.first;
                Chunk *m = p2.second;
                r.x = centerX + m->x * chSize - pchx;
                r.y = centerY + m->y * chSize - pchy;
                ME_Color col;
                if (m->generationPhase == -1) {
                    col = {0x60, 0x60, 0x60, 0xff};
                } else if (m->generationPhase == 0) {
                    col = {0xff, 0x00, 0x00, 0xff};
                } else if (m->generationPhase == 1) {
                    col = {0x00, 0xff, 0x00, 0xff};
                } else if (m->generationPhase == 2) {
                    col = {0x00, 0x00, 0xff, 0xff};
                } else if (m->generationPhase == 3) {
                    col = {0xff, 0xff, 0x00, 0xff};
                } else if (m->generationPhase == 4) {
                    col = {0xff, 0x00, 0xff, 0xff};
                } else if (m->generationPhase == 5) {
                    col = {0x00, 0xff, 0xff, 0xff};
                } else {
                }
                R_Rectangle2(Render.target, r, col);
            }
        }

        int loadx = (int)(((f32)-GameIsolate_.world->loadZone.x / CHUNK_W) * chSize);
        int loady = (int)(((f32)-GameIsolate_.world->loadZone.y / CHUNK_H) * chSize);

        int loadx2 = (int)(((f32)(-GameIsolate_.world->loadZone.x + GameIsolate_.world->loadZone.w) / CHUNK_W) * chSize);
        int loady2 = (int)(((f32)(-GameIsolate_.world->loadZone.y + GameIsolate_.world->loadZone.h) / CHUNK_H) * chSize);
        R_Rectangle(Render.target, centerX - pchx + loadx, centerY - pchy + loady, centerX - pchx + loadx2, centerY - pchy + loady2, {0x00, 0xff, 0xff, 0xff});

        R_Rectangle(Render.target, centerX - pchx + pchxf, centerY - pchy + pchyf, centerX + 1 - pchx + pchxf, centerY + 1 - pchy + pchyf, {0x00, 0xff, 0x00, 0xff});
    }

    if (GameIsolate_.globaldef.draw_debug_stats) {

        int rbCt = 0;
        for (auto &r : GameIsolate_.world->rigidBodies) {
            if (r->body->IsEnabled()) rbCt++;
        }

        int rbTriACt = 0;
        int rbTriCt = 0;
        for (size_t i = 0; i < GameIsolate_.world->rigidBodies.size(); i++) {
            RigidBody cur = *GameIsolate_.world->rigidBodies[i];

            b2Fixture *fix = cur.body->GetFixtureList();
            while (fix) {
                b2Shape *shape = fix->GetShape();

                switch (shape->GetType()) {
                    case b2Shape::Type::e_polygon:
                        rbTriCt++;
                        if (cur.body->IsEnabled()) rbTriACt++;
                        break;
                }

                fix = fix->GetNext();
            }
        }

        int rbTriWCt = 0;

        int minChX = (int)floor((GameIsolate_.world->meshZone.x - GameIsolate_.world->loadZone.x) / CHUNK_W);
        int minChY = (int)floor((GameIsolate_.world->meshZone.y - GameIsolate_.world->loadZone.y) / CHUNK_H);
        int maxChX = (int)ceil((GameIsolate_.world->meshZone.x + GameIsolate_.world->meshZone.w - GameIsolate_.world->loadZone.x) / CHUNK_W);
        int maxChY = (int)ceil((GameIsolate_.world->meshZone.y + GameIsolate_.world->meshZone.h - GameIsolate_.world->loadZone.y) / CHUNK_H);

        for (int cx = minChX; cx <= maxChX; cx++) {
            for (int cy = minChY; cy <= maxChY; cy++) {
                Chunk *ch = GameIsolate_.world->getChunk(cx, cy);
                for (int i = 0; i < ch->polys.size(); i++) {
                    rbTriWCt++;
                }
            }
        }

        int chCt = 0;
        for (auto &p : GameIsolate_.world->chunkCache) {
            if (p.first == INT_MIN) continue;
            int cx = p.first;
            for (auto &p2 : p.second) {
                if (p2.first == INT_MIN) continue;
                chCt++;
            }
        }

        constexpr const char *buffAsStdStr1 = R"(
{0} {1}
XY: {2:.2f} / {3:.2f}
V: {4:.2f} / {5:.2f}
Cells: {6}
Entities: {7}
RigidBodies: {8}/{9} O, {10} W
Tris: {11}/{12} O, {13} W
Cached Chunks: {14}
ReadyToReadyToMerge ({15})
ReadyToMerge ({16})
)";

        // Drawing::drawText(
        //         "info",
        //         std::format(buffAsStdStr1, win_title_client, METADOT_VERSION_TEXT, GameData_.plPosX,
        //                     GameData_.plPosY,
        //                     GameIsolate_.world->player
        //                             ? pl_we->vx
        //                             : 0.0f,
        //                     GameIsolate_.world->player
        //                             ? pl_we->vy
        //                             : 0.0f,
        //                     (int) GameIsolate_.world->cells.size(),
        //                     (int) GameIsolate_.world->worldEntities.size(), rbCt,
        //                     (int) GameIsolate_.world->rigidBodies.size(),
        //                     (int) GameIsolate_.world->worldRigidBodies.size(),
        //                     rbTriACt, rbTriCt, rbTriWCt, chCt,
        //                     (int) GameIsolate_.world->readyToReadyToMerge.size(),
        //                     (int) GameIsolate_.world->readyToMerge.size()),
        //         4, 12);

        float pl_vx = 0.0f;
        float pl_vy = 0.0f;

        if (GameIsolate_.world->player) {
            auto [pl_we, pl] = GameIsolate_.world->getHostPlayer();

            pl_vx = pl_we->vx;
            pl_vy = pl_we->vy;
        }

        auto a = std::format(buffAsStdStr1, win_title_client, METADOT_VERSION_TEXT, global.GameData_.plPosX, global.GameData_.plPosY, pl_vx, pl_vy, (int)GameIsolate_.world->cells.size(),
                             (int)GameIsolate_.world->Reg().entity_count(), rbCt, (int)GameIsolate_.world->rigidBodies.size(), (int)GameIsolate_.world->worldRigidBodies.size(), rbTriACt, rbTriCt,
                             rbTriWCt, chCt, (int)GameIsolate_.world->readyToReadyToMerge.size(), (int)GameIsolate_.world->readyToMerge.size());

        MetaEngine::Drawing::drawText(a, {255, 255, 255, 255}, 10, 0);

        // for (size_t i = 0; i < GameIsolate_.world->readyToReadyToMerge.size(); i++) {
        //     char buff[10];
        //     snprintf(buff, sizeof(buff), "    #%d", (int) i);
        //     std::string buffAsStdStr = buff;
        //     Drawing::drawTextBG(Render.target, buffAsStdStr.c_str(), font16, 4,
        //                         2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff,
        //                         {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);
        // }

        // for (size_t i = 0; i < GameIsolate_.world->readyToMerge.size(); i++) {
        //     char buff[20];
        //     snprintf(buff, sizeof(buff), "    #%d (%d, %d)", (int) i,
        //              GameIsolate_.world->readyToMerge[i]->x,
        //              GameIsolate_.world->readyToMerge[i]->y);
        //     std::string buffAsStdStr = buff;
        //     Drawing::drawTextBG(Render.target, buffAsStdStr.c_str(), font16, 4,
        //                         2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff,
        //                         {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);
        // }
    }

    if (GameIsolate_.globaldef.draw_frame_graph) {

        for (int i = 0; i <= 4; i++) {
            // Drawing::drawText(Render.target, dt_frameGraph[i], Screen.windowWidth - 20,
            //                   Screen.windowHeight - 15 - (i * 25) - 2);
            R_Line(Render.target, Screen.windowWidth - 30 - TraceTimeNum - 5, Screen.windowHeight - 10 - (i * 25), Screen.windowWidth - 25, Screen.windowHeight - 10 - (i * 25),
                   {0xff, 0xff, 0xff, 0xff});
        }

        for (int i = 0; i < TraceTimeNum; i++) {
            int h = Time.frameTimesTrace[i];

            ME_Color col;
            if (h <= (int)(1000 / 144.0)) {
                col = {0x00, 0xff, 0x00, 0xff};
            } else if (h <= (int)(1000 / 60.0)) {
                col = {0xa0, 0xe0, 0x00, 0xff};
            } else if (h <= (int)(1000 / 30.0)) {
                col = {0xff, 0xff, 0x00, 0xff};
            } else if (h <= (int)(1000 / 15.0)) {
                col = {0xff, 0x80, 0x00, 0xff};
            } else {
                col = {0xff, 0x00, 0x00, 0xff};
            }

            R_Line(Render.target, Screen.windowWidth - TraceTimeNum - 30 + i, Screen.windowHeight - 10 - h, Screen.windowWidth - TraceTimeNum - 30 + i, Screen.windowHeight - 10, col);
            // SDL_RenderDrawLine(renderer, WIDTH - TraceTimeNum - 30 + i, HEIGHT - 10 - h, WIDTH - TraceTimeNum - 30 + i, HEIGHT - 10);
        }

        R_Line(Render.target, Screen.windowWidth - 30 - TraceTimeNum - 5, Screen.windowHeight - 10 - (int)(1000.0 / Time.framesPerSecond), Screen.windowWidth - 25,
               Screen.windowHeight - 10 - (int)(1000.0 / Time.framesPerSecond), {0x00, 0xff, 0xff, 0xff});
        R_Line(Render.target, Screen.windowWidth - 30 - TraceTimeNum - 5, Screen.windowHeight - 10 - (int)(1000.0 / Time.feelsLikeFps), Screen.windowWidth - 25,
               Screen.windowHeight - 10 - (int)(1000.0 / Time.feelsLikeFps), {0xff, 0x00, 0xff, 0xff});
    }

    R_SetShapeBlendMode(R_BLEND_NORMAL);

    /*

#ifdef DEVELOPMENT_BUILD
if (dt_versionInfo1.w == -1) {
char buffDevBuild[40];
snprintf(buffDevBuild, sizeof(buffDevBuild), "Development Build");
//if (dt_versionInfo1.t1 != nullptr) R_FreeImage(dt_versionInfo1.t1);
//if (dt_versionInfo1.t2 != nullptr) R_FreeImage(dt_versionInfo1.t2);
dt_versionInfo1 = Drawing::drawTextParams(Render.target, buffDevBuild, font16, 4, Screen.windowHeight - 32 - 13, 0xff, 0xff, 0xff, ALIGN_LEFT);

char buffVersion[40];
snprintf(buffVersion, sizeof(buffVersion), "Version %s - dev", VERSION);
//if (dt_versionInfo2.t1 != nullptr) R_FreeImage(dt_versionInfo2.t1);
//if (dt_versionInfo2.t2 != nullptr) R_FreeImage(dt_versionInfo2.t2);
dt_versionInfo2 = Drawing::drawTextParams(Render.target, buffVersion, font16, 4, Screen.windowHeight - 32, 0xff, 0xff, 0xff, ALIGN_LEFT);

char buffBuildDate[40];
snprintf(buffBuildDate, sizeof(buffBuildDate), "%s : %s", __DATE__, __TIME__);
//if (dt_versionInfo3.t1 != nullptr) R_FreeImage(dt_versionInfo3.t1);
//if (dt_versionInfo3.t2 != nullptr) R_FreeImage(dt_versionInfo3.t2);
dt_versionInfo3 = Drawing::drawTextParams(Render.target, buffBuildDate, font16, 4, Screen.windowHeight - 32 + 13, 0xff, 0xff, 0xff, ALIGN_LEFT);
}

Drawing::drawText(Render.target, dt_versionInfo1, 4, Screen.windowHeight - 32 - 13, ALIGN_LEFT);
Drawing::drawText(Render.target, dt_versionInfo2, 4, Screen.windowHeight - 32, ALIGN_LEFT);
Drawing::drawText(Render.target, dt_versionInfo3, 4, Screen.windowHeight - 32 + 13, ALIGN_LEFT);
#elif defined ALPHA_BUILD
char buffDevBuild[40];
snprintf(buffDevBuild, sizeof(buffDevBuild), "Alpha Build");
Drawing::drawText(target, buffDevBuild, font16, 4, HEIGHT - 32, 0xff, 0xff, 0xff, ALIGN_LEFT);

char buffVersion[40];
snprintf(buffVersion, sizeof(buffVersion), "Version %s - alpha", VERSION);
Drawing::drawText(target, buffVersion, font16, 4, HEIGHT - 32 + 13, 0xff, 0xff, 0xff, ALIGN_LEFT);
#else
char buffVersion[40];
snprintf(buffVersion, sizeof(buffVersion), "Version %s", VERSION);
Drawing::drawText(target, buffVersion, font16, 4, HEIGHT - 32 + 13, 0xff, 0xff, 0xff, ALIGN_LEFT);
#endif

*/
}

void Game::renderTemperatureMap(World *world) {

    for (int x = 0; x < GameIsolate_.world->width; x++) {
        for (int y = 0; y < GameIsolate_.world->height; y++) {
            auto t = GameIsolate_.world->tiles[x + y * GameIsolate_.world->width];
            i32 temp = t.temperature;
            u32 color = (u8)((temp + 1024) / 2048.0f * 255);

            const unsigned int offset = (GameIsolate_.world->width * 4 * y) + x * 4;
            TexturePack_.pixelsTemp_ar[offset + 0] = color;  // b
            TexturePack_.pixelsTemp_ar[offset + 1] = color;  // g
            TexturePack_.pixelsTemp_ar[offset + 2] = color;  // r
            TexturePack_.pixelsTemp_ar[offset + 3] = 0xf0;   // a
        }
    }
}

void Game::ResolutionChanged(int newWidth, int newHeight) {

    int prevWidth = Screen.windowWidth;
    int prevHeight = Screen.windowHeight;

    Screen.windowWidth = newWidth;
    Screen.windowHeight = newHeight;

    global.game->createTexture();

    global.game->accLoadX -= (newWidth - prevWidth) / 2.0f / Screen.gameScale;
    global.game->accLoadY -= (newHeight - prevHeight) / 2.0f / Screen.gameScale;

    METADOT_INFO("Ticking chunk...");
    global.game->tickChunkLoading();
    METADOT_INFO("Ticking chunk done");

    for (int x = 0; x < global.game->GameIsolate_.world->width; x++) {
        for (int y = 0; y < global.game->GameIsolate_.world->height; y++) {
            global.game->GameIsolate_.world->dirty[x + y * global.game->GameIsolate_.world->width] = true;
            global.game->GameIsolate_.world->layer2Dirty[x + y * global.game->GameIsolate_.world->width] = true;
            global.game->GameIsolate_.world->backgroundDirty[x + y * global.game->GameIsolate_.world->width] = true;
        }
    }
}

int Game::getAimSurface(int dist) {
    int dcx = this->mx - Screen.windowWidth / 2;
    int dcy = this->my - Screen.windowHeight / 2;

    f32 len = sqrtf(dcx * dcx + dcy * dcy);
    f32 udx = dcx / len;
    f32 udy = dcy / len;

    int mmx = Screen.windowWidth / 2.0f + udx * dist;
    int mmy = Screen.windowHeight / 2.0f + udy * dist;

    int wcx = (int)((Screen.windowWidth / 2.0f - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
    int wcy = (int)((Screen.windowHeight / 2.0f - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

    int wmx = (int)((mmx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
    int wmy = (int)((mmy - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

    int startInd = -1;
    GameIsolate_.world->forLine(wcx, wcy, wmx, wmy, [&](int ind) {
        if (GameIsolate_.world->tiles[ind].mat->physicsType == PhysicsType::SOLID || GameIsolate_.world->tiles[ind].mat->physicsType == PhysicsType::SAND ||
            GameIsolate_.world->tiles[ind].mat->physicsType == PhysicsType::SOUP) {
            startInd = ind;
            return true;
        }
        return false;
    });

    return startInd;
}

void Game::quitToMainMenu() {

    if (state == LOADING) return;

    GameIsolate_.world->saveWorld();

    std::string worldName = "mainMenu";

    METADOT_INFO("Loading main menu @ %s", METADOT_RESLOC(std::format("saves/{0}", worldName).c_str()));
    gameUI.visible_mainmenu = false;
    state = LOADING;
    stateAfterLoad = MAIN_MENU;

    GameIsolate_.world.reset();

    WorldGenerator *generator = new MaterialTestGenerator();

    std::string wpStr = METADOT_RESLOC(std::format("saves/{0}", worldName).c_str());

    GameIsolate_.world = ME::create_scope<World>();
    GameIsolate_.world->noSaveLoad = true;
    GameIsolate_.world->init(wpStr, (int)ceil(WINDOWS_MAX_WIDTH / RENDER_C_TEST / (f64)CHUNK_W) * CHUNK_W + CHUNK_W * RENDER_C_TEST,
                             (int)ceil(WINDOWS_MAX_HEIGHT / RENDER_C_TEST / (f64)CHUNK_H) * CHUNK_H + CHUNK_H * RENDER_C_TEST, Render.target, &global.audio, generator);

    METADOT_INFO("Queueing chunk loading...");
    for (int x = -CHUNK_W * 4; x < GameIsolate_.world->width + CHUNK_W * 4; x += CHUNK_W) {
        for (int y = -CHUNK_H * 3; y < GameIsolate_.world->height + CHUNK_H * 8; y += CHUNK_H) {
            GameIsolate_.world->queueLoadChunk(x / CHUNK_W, y / CHUNK_H, true, true);
        }
    }

    std::fill(TexturePack_.pixels.begin(), TexturePack_.pixels.end(), 0);
    std::fill(TexturePack_.pixelsBackground.begin(), TexturePack_.pixelsBackground.end(), 0);
    std::fill(TexturePack_.pixelsLayer2.begin(), TexturePack_.pixelsLayer2.end(), 0);
    std::fill(TexturePack_.pixelsFire.begin(), TexturePack_.pixelsFire.end(), 0);
    std::fill(TexturePack_.pixelsFlow.begin(), TexturePack_.pixelsFlow.end(), 0);
    std::fill(TexturePack_.pixelsEmission.begin(), TexturePack_.pixelsEmission.end(), 0);
    std::fill(TexturePack_.pixelsCells.begin(), TexturePack_.pixelsCells.end(), 0);

    R_UpdateImageBytes(TexturePack_.texture, NULL, &TexturePack_.pixels[0], GameIsolate_.world->width * 4);

    R_UpdateImageBytes(TexturePack_.textureBackground, NULL, &TexturePack_.pixelsBackground[0], GameIsolate_.world->width * 4);

    R_UpdateImageBytes(TexturePack_.textureLayer2, NULL, &TexturePack_.pixelsLayer2[0], GameIsolate_.world->width * 4);

    R_UpdateImageBytes(TexturePack_.textureFire, NULL, &TexturePack_.pixelsFire[0], GameIsolate_.world->width * 4);

    R_UpdateImageBytes(TexturePack_.textureFlow, NULL, &TexturePack_.pixelsFlow[0], GameIsolate_.world->width * 4);

    R_UpdateImageBytes(TexturePack_.emissionTexture, NULL, &TexturePack_.pixelsEmission[0], GameIsolate_.world->width * 4);

    R_UpdateImageBytes(TexturePack_.textureCells, NULL, &TexturePack_.pixelsCells[0], GameIsolate_.world->width * 4);

    gameUI.visible_mainmenu = true;
}

ME_assets_handle_t Game::get_assets(std::string path) {
    ME_pack_result pack_result;

    ME_assets_handle_t handle;

    pack_result = ME_read_pack_path_item_data(GameIsolate_.pack_reader, path.c_str(), &handle.data, &handle.size);

    if (pack_result != SUCCESS_PACK_RESULT) {
        ME_destroy_pack_reader(GameIsolate_.pack_reader);
        METADOT_ERROR("%d", pack_result);
        ME_ASSERT_E(0);
    }

    ME_ASSERT_E(handle.data);

    return handle;
}

int Game::getAimSolidSurface(int dist) {
    int dcx = this->mx - Screen.windowWidth / 2;
    int dcy = this->my - Screen.windowHeight / 2;

    f32 len = sqrtf(dcx * dcx + dcy * dcy);
    f32 udx = dcx / len;
    f32 udy = dcy / len;

    int mmx = Screen.windowWidth / 2.0f + udx * dist;
    int mmy = Screen.windowHeight / 2.0f + udy * dist;

    int wcx = (int)((Screen.windowWidth / 2.0f - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
    int wcy = (int)((Screen.windowHeight / 2.0f - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

    int wmx = (int)((mmx - global.GameData_.ofsX - global.GameData_.camX) / Screen.gameScale);
    int wmy = (int)((mmy - global.GameData_.ofsY - global.GameData_.camY) / Screen.gameScale);

    int startInd = -1;
    GameIsolate_.world->forLine(wcx, wcy, wmx, wmy, [&](int ind) {
        if (GameIsolate_.world->tiles[ind].mat->physicsType == PhysicsType::SOLID) {
            startInd = ind;
            return true;
        }
        return false;
    });

    return startInd;
}

void Game::updateMaterialSounds() {
    u16 waterCt = std::min(movingTiles[MaterialsList::WATER.id], (u16)5000);
    f32 water = (f32)waterCt / 3000;
    // METADOT_BUG("{} / {} = {}", waterCt, 3000, water);
    global.audio.SetEventParameter("event:/World/WaterFlow", "FlowIntensity", water);
}