// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "game.hpp"

#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <regex>
#include <string>
#include <string_view>

#include "core/const.h"
#include "core/core.h"
#include "core/core.hpp"
#include "core/debug_impl.hpp"
#include "core/global.hpp"
#include "core/macros.h"
#include "core/threadpool.hpp"
#include "engine.h"
#include "engine/console.hpp"
#include "engine/engine.h"
#include "engine/filesystem.h"
#include "engine/math.hpp"
#include "engine/memory.hpp"
#include "engine/reflectionflat.hpp"
#include "engine/renderer/gpu.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/scripting.hpp"
#include "engine/sdl_wrapper.h"
#include "engine/utils.hpp"
#include "engine_platform.h"
#include "fonts.h"
#include "game/game_datastruct.hpp"
#include "game/game_resources.hpp"
#include "game/game_shaders.hpp"
#include "game/game_ui.hpp"
#include "libs/glad/glad.h"
#include "physfs/physfs.h"
#include "ui.hpp"
#include "utils.h"
#include "world_generator.cpp"

extern void fuckme();
extern Texture *LoadAseprite(const char *path);

Global global;

IMPLENGINE();

Game::Game(int argc, char *argv[]) {
    METAENGINE_Memory_Init(argc, argv);
    global.game = this;
    METADOT_INFO("%s %s", METADOT_NAME, METADOT_VERSION_TEXT);
}

Game::~Game() {
    global.game = nullptr;
    METAENGINE_Memory_End();
}

int Game::init(int argc, char *argv[]) {

    ParseRunArgs(argc, argv);

    METADOT_INFO("Starting game...");

    InitECS(128);
    if (!InitEngine()) return METADOT_FAILED;

    GameIsolate_.texturepack = (TexturePack *)gc_malloc(&gc, sizeof(TexturePack));

    // load splash screen
    METADOT_INFO("Loading splash screen...");

    R_Clear(Render.target);
    R_Flip(Render.target);
    Texture *splashSurf = LoadTexture("data/assets/title/splash.png");
    R_Image *splashImg = R_CopyImageFromSurface(splashSurf->surface);
    R_SetImageFilter(splashImg, R_FILTER_NEAREST);
    R_BlitRect(splashImg, NULL, Render.target, NULL);
    R_FreeImage(splashImg);
    DestroyTexture(splashSurf);
    R_Flip(Render.target);

    UIRendererInit();

    // scripting system
    METADOT_INFO("Loading Script...");
    METADOT_NEW(C, global.scripts, Scripts);
    global.scripts->Init();

    global.I18N.Init();
    InitGlobalDEF(&global.game->GameIsolate_.globaldef, false);

    global.game->GameSystem_.console.Init();
    GameIsolate_.texturepack->testAse = LoadAseprite("data/assets/textures/Sprite-0001.ase");
    GameIsolate_.backgrounds->Load();

    font = FontCache_CreateFont();
    FontCache_LoadFont(font, METADOT_RESLOC("data/assets/fonts/ark-pixel-12px-monospaced-zh_cn.ttf"), 12, FontCache_MakeColor(255, 255, 255, 255), TTF_STYLE_NORMAL);

    // init the rng
    pcg32_random_t rng;
    pcg32_srandom_r(&rng, Time::millis(), 1);
    unsigned int seed = pcg32_random_r(&rng);
    METADOT_INFO("SeedRNG %d", seed);

    // register & set up materials
    METADOT_INFO("Setting up materials...");

    METADOT_NEW_ARRAY(C, movingTiles, U16, global.GameData_.materials_count);
    METADOT_NEW(C, debugDraw, DebugDraw, Render.target);

    // global.audioEngine.PlayEvent("event:/Music/Background1");
    global.audioEngine.PlayEvent("event:/Music/Title");
    global.audioEngine.Update();

    METADOT_INFO("Initializing world...");
    METADOT_NEW(C, GameIsolate_.world, World);
    GameIsolate_.world->noSaveLoad = true;
    GameIsolate_.world->init(METADOT_RESLOC("saves/mainMenu"), (int)ceil(WINDOWS_MAX_WIDTH / RENDER_C_TEST / (F64)CHUNK_W) * CHUNK_W + CHUNK_W * RENDER_C_TEST,
                             (int)ceil(WINDOWS_MAX_HEIGHT / RENDER_C_TEST / (F64)CHUNK_H) * CHUNK_H + CHUNK_H * RENDER_C_TEST, Render.target, &global.audioEngine);

    {
        // set up main menu ui

        METADOT_INFO("Setting up main menu...");
        std::string displayMode = "windowed";

        if (displayMode == "windowed") {
            SetDisplayMode(engine_displaymode::WINDOWED);
        } else if (displayMode == "borderless") {
            SetDisplayMode(engine_displaymode::BORDERLESS);
        } else if (displayMode == "fullscreen") {
            SetDisplayMode(engine_displaymode::FULLSCREEN);
        }

        int w;
        int h;
        SDL_GetWindowSize(Core.window, &w, &h);
        R_SetWindowResolution(w, h);
        R_ResetProjection(Render.realTarget);
        resolu(w, h);

        SetVSync(true);
        SetMinimizeOnLostFocus(false);
    }

    // init threadpools
    GameIsolate_.updateDirtyPool2 = metadot_thpool_init(2);
    METADOT_NEW(C, GameIsolate_.updateDirtyPool, ThreadPool, 4);

    global.shaderworker = {.newLightingShader_insideDes = 0.0f, .newLightingShader_insideCur = 0.0f};
    LoadShaders(&global.shaderworker);

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
        R_FreeImage(TexturePack_.textureParticles);
        R_FreeImage(TexturePack_.textureEntities);
        R_FreeImage(TexturePack_.textureEntitiesLQ);
        R_FreeImage(TexturePack_.temperatureMap);
        R_FreeImage(TexturePack_.backgroundImage);
    }

    // create textures
    loadingOnColor = 0xFFFFFFFF;
    loadingOffColor = 0x000000FF;

    std::vector<std::function<void(void)>> Funcs = {
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "loadingTexture");
                TexturePack_.loadingTexture =
                        R_CreateImage(TexturePack_.loadingScreenW = (Screen.windowWidth / 20), TexturePack_.loadingScreenH = (Screen.windowHeight / 20), R_FormatEnum::R_FORMAT_RGBA);

                R_SetImageFilter(TexturePack_.loadingTexture, R_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "texture");
                TexturePack_.texture = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);

                R_SetImageFilter(TexturePack_.texture, R_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "worldTexture");
                TexturePack_.worldTexture = R_CreateImage(GameIsolate_.world->width * GameIsolate_.globaldef.hd_objects_size, GameIsolate_.world->height * GameIsolate_.globaldef.hd_objects_size,
                                                          R_FormatEnum::R_FORMAT_RGBA);

                R_SetImageFilter(TexturePack_.worldTexture, R_FILTER_NEAREST);

                R_LoadTarget(TexturePack_.worldTexture);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "lightingTexture");
                TexturePack_.lightingTexture = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.lightingTexture, R_FILTER_NEAREST);
                R_LoadTarget(TexturePack_.lightingTexture);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "emissionTexture");
                TexturePack_.emissionTexture = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.emissionTexture, R_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureFlow");
                TexturePack_.textureFlow = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureFlow, R_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureFlowSpead");
                TexturePack_.textureFlowSpead = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureFlowSpead, R_FILTER_NEAREST);
                R_LoadTarget(TexturePack_.textureFlowSpead);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureFire");
                TexturePack_.textureFire = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureFire, R_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "texture2Fire");
                TexturePack_.texture2Fire = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.texture2Fire, R_FILTER_NEAREST);
                R_LoadTarget(TexturePack_.texture2Fire);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureLayer2");
                TexturePack_.textureLayer2 = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureLayer2, R_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureBackground");
                TexturePack_.textureBackground = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureBackground, R_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureObjects");
                TexturePack_.textureObjects = R_CreateImage(GameIsolate_.world->width * (GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1),
                                                            GameIsolate_.world->height * (GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1), R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureObjects, R_FILTER_NEAREST);
                R_LoadTarget(TexturePack_.textureObjects);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureObjectsLQ");
                TexturePack_.textureObjectsLQ = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureObjectsLQ, R_FILTER_NEAREST);
                R_LoadTarget(TexturePack_.textureObjectsLQ);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureObjectsBack");
                TexturePack_.textureObjectsBack =
                        R_CreateImage(GameIsolate_.world->width * (GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1),
                                      GameIsolate_.world->height * (GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1), R_FormatEnum::R_FORMAT_RGBA);
                R_SetImageFilter(TexturePack_.textureObjectsBack, R_FILTER_NEAREST);
                R_LoadTarget(TexturePack_.textureObjectsBack);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureParticles");
                TexturePack_.textureParticles = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);

                R_SetImageFilter(TexturePack_.textureParticles, R_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureEntities");
                TexturePack_.textureEntities =
                        R_CreateImage(GameIsolate_.world->width * (GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1),
                                      GameIsolate_.world->height * (GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1), R_FormatEnum::R_FORMAT_RGBA);

                R_LoadTarget(TexturePack_.textureEntities);

                R_SetImageFilter(TexturePack_.textureEntities, R_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureEntitiesLQ");
                TexturePack_.textureEntitiesLQ = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);

                R_LoadTarget(TexturePack_.textureEntitiesLQ);

                R_SetImageFilter(TexturePack_.textureEntitiesLQ, R_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "temperatureMap");
                TexturePack_.temperatureMap = R_CreateImage(GameIsolate_.world->width, GameIsolate_.world->height, R_FormatEnum::R_FORMAT_RGBA);

                R_SetImageFilter(TexturePack_.temperatureMap, R_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "backgroundImage");
                TexturePack_.backgroundImage = R_CreateImage(Screen.windowWidth, Screen.windowHeight, R_FormatEnum::R_FORMAT_RGBA);

                R_SetImageFilter(TexturePack_.backgroundImage, R_FILTER_NEAREST);

                R_LoadTarget(TexturePack_.backgroundImage);
            }};

    for (auto f : Funcs) f();

    // create texture pixel buffers

    TexturePack_.pixels = std::vector<U8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
    TexturePack_.pixels_ar = &TexturePack_.pixels[0];

    TexturePack_.pixelsLayer2 = std::vector<U8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
    TexturePack_.pixelsLayer2_ar = &TexturePack_.pixelsLayer2[0];

    TexturePack_.pixelsBackground = std::vector<U8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
    TexturePack_.pixelsBackground_ar = &TexturePack_.pixelsBackground[0];

    TexturePack_.pixelsObjects = std::vector<U8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, METAENGINE_ALPHA_TRANSPARENT);
    TexturePack_.pixelsObjects_ar = &TexturePack_.pixelsObjects[0];

    TexturePack_.pixelsTemp = std::vector<U8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, METAENGINE_ALPHA_TRANSPARENT);
    TexturePack_.pixelsTemp_ar = &TexturePack_.pixelsTemp[0];

    TexturePack_.pixelsParticles = std::vector<U8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, METAENGINE_ALPHA_TRANSPARENT);
    TexturePack_.pixelsParticles_ar = &TexturePack_.pixelsParticles[0];

    TexturePack_.pixelsLoading = std::vector<U8>(TexturePack_.loadingTexture->w * TexturePack_.loadingTexture->h * 4, METAENGINE_ALPHA_TRANSPARENT);
    TexturePack_.pixelsLoading_ar = &TexturePack_.pixelsLoading[0];

    TexturePack_.pixelsFire = std::vector<U8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
    TexturePack_.pixelsFire_ar = &TexturePack_.pixelsFire[0];

    TexturePack_.pixelsFlow = std::vector<U8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
    TexturePack_.pixelsFlow_ar = &TexturePack_.pixelsFlow[0];

    TexturePack_.pixelsEmission = std::vector<U8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
    TexturePack_.pixelsEmission_ar = &TexturePack_.pixelsEmission[0];

    METADOT_INFO("Creating world textures done");
}

int Game::run(int argc, char *argv[]) {
    Time.startTime = Time::millis();

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
    if (GameIsolate_.world->WorldIsolate_.player) {
        global.GameData_.plPosX = GameIsolate_.world->WorldIsolate_.player->x;
        global.GameData_.plPosY = GameIsolate_.world->WorldIsolate_.player->y;
    } else {
        global.GameData_.plPosX = global.GameData_.freeCamX;
        global.GameData_.plPosY = global.GameData_.freeCamY;
    }

    SDL_Event windowEvent;

    I64 lastFPS = Time::millis();
    Time.fps = 0;

    Time.lastTime = Time::millis();
    Time.lastTick = Time.lastTime;
    I64 lastTickPhysics = Time.lastTime;

    Time.mspt = 33;
    I32 msptPhysics = 16;

    scale = 3;
    global.GameData_.ofsX = (int)(-CHUNK_W * 4);
    global.GameData_.ofsY = (int)(-CHUNK_H * 2.5);

    global.GameData_.ofsX = (global.GameData_.ofsX - Screen.windowWidth / 2) / 2 * 3 + Screen.windowWidth / 2;
    global.GameData_.ofsY = (global.GameData_.ofsY - Screen.windowHeight / 2) / 2 * 3 + Screen.windowHeight / 2;

    InitFPS();
    METADOT_NEW_ARRAY(C, objectDelete, U8, GameIsolate_.world->width * GameIsolate_.world->height);

    fadeInStart = Time::millis();
    fadeInLength = 250;
    fadeInWaitFrames = 5;

    // game loop
    while (this->running) {

        Time.now = Time::millis();
        Time.deltaTime = Time.now - Time.lastTime;

        GameIsolate_.profiler.Frame();

        EngineUpdate();

#pragma region SDL_Input

        // handle window events
        GameIsolate_.profiler.Begin(Profiler::Stage::SdlInput);
        while (SDL_PollEvent(&windowEvent)) {

            if (windowEvent.type == SDL_WINDOWEVENT) {
                if (windowEvent.window.event == SDL_WINDOWEVENT_RESIZED) {
                    // METADOT_INFO("Resizing window...");
                    R_SetWindowResolution(windowEvent.window.data1, windowEvent.window.data2);
                    R_ResetProjection(Render.realTarget);
                    resolu(windowEvent.window.data1, windowEvent.window.data2);
                }
            }

            ImGui_ImplSDL2_ProcessEvent(&windowEvent);

            if (ImGui::GetIO().WantCaptureMouse) {
                if (windowEvent.type == SDL_MOUSEBUTTONDOWN || windowEvent.type == SDL_MOUSEBUTTONUP || windowEvent.type == SDL_MOUSEMOTION || windowEvent.type == SDL_MOUSEWHEEL) {
                    continue;
                }
            }

            if (ImGui::GetIO().WantCaptureKeyboard) {
                if (windowEvent.type == SDL_KEYDOWN || windowEvent.type == SDL_KEYUP) {
                    continue;
                }
            }

            if (windowEvent.type == SDL_MOUSEWHEEL) {

            } else if (windowEvent.type == SDL_MOUSEMOTION) {
                if (Controls::DEBUG_DRAW->get()) {
                    // draw material

                    int x = (int)((windowEvent.motion.x - global.GameData_.ofsX - global.GameData_.camX) / scale);
                    int y = (int)((windowEvent.motion.y - global.GameData_.ofsY - global.GameData_.camY) / scale);

                    if (lastDrawMX == 0 && lastDrawMY == 0) {
                        lastDrawMX = x;
                        lastDrawMY = y;
                    }

                    GameIsolate_.world->forLine(lastDrawMX, lastDrawMY, x, y, [&](int index) {
                        int lineX = index % GameIsolate_.world->width;
                        int lineY = index / GameIsolate_.world->width;

                        for (int xx = -GameUI::DebugDrawUI::brushSize / 2; xx < (int)(ceil(GameUI::DebugDrawUI::brushSize / 2.0)); xx++) {
                            for (int yy = -GameUI::DebugDrawUI::brushSize / 2; yy < (int)(ceil(GameUI::DebugDrawUI::brushSize / 2.0)); yy++) {
                                if (lineX + xx < 0 || lineY + yy < 0 || lineX + xx >= GameIsolate_.world->width || lineY + yy >= GameIsolate_.world->height) continue;
                                MaterialInstance tp = TilesCreate(GameUI::DebugDrawUI::selectedMaterial, lineX + xx, lineY + yy);
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

                if (Controls::mmouse) {
                    // erase material

                    // erase from world
                    int x = (int)((windowEvent.motion.x - global.GameData_.ofsX - global.GameData_.camX) / scale);
                    int y = (int)((windowEvent.motion.y - global.GameData_.ofsY - global.GameData_.camY) / scale);

                    if (lastEraseMX == 0 && lastEraseMY == 0) {
                        lastEraseMX = x;
                        lastEraseMY = y;
                    }

                    GameIsolate_.world->forLine(lastEraseMX, lastEraseMY, x, y, [&](int index) {
                        int lineX = index % GameIsolate_.world->width;
                        int lineY = index / GameIsolate_.world->width;

                        for (int xx = -GameUI::DebugDrawUI::brushSize / 2; xx < (int)(ceil(GameUI::DebugDrawUI::brushSize / 2.0)); xx++) {
                            for (int yy = -GameUI::DebugDrawUI::brushSize / 2; yy < (int)(ceil(GameUI::DebugDrawUI::brushSize / 2.0)); yy++) {

                                if (abs(xx) + abs(yy) == GameUI::DebugDrawUI::brushSize) continue;
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
                    std::vector<RigidBody *> *rbs = &GameIsolate_.world->WorldIsolate_.rigidBodies;

                    for (size_t i = 0; i < rbs->size(); i++) {
                        RigidBody *cur = (*rbs)[i];
                        if (!static_cast<bool>(cur->surface)) continue;
                        if (cur->body->IsEnabled()) {
                            F32 s = sin(-cur->body->GetAngle());
                            F32 c = cos(-cur->body->GetAngle());
                            bool upd = false;
                            for (F32 xx = -3; xx <= 3; xx += 0.5) {
                                for (F32 yy = -3; yy <= 3; yy += 0.5) {
                                    if (abs(xx) + abs(yy) == 6) continue;
                                    // rotate point

                                    F32 tx = x + xx - cur->body->GetPosition().x;
                                    F32 ty = y + yy - cur->body->GetPosition().y;

                                    int ntx = (int)(tx * c - ty * s);
                                    int nty = (int)(tx * s + ty * c);

                                    if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                        U32 pixel = R_GET_PIXEL(cur->surface, ntx, nty);
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
            } else if (windowEvent.type == SDL_KEYDOWN) {
                Controls::keyEvent(windowEvent.key);
            } else if (windowEvent.type == SDL_KEYUP) {
                Controls::keyEvent(windowEvent.key);
            }

            if (windowEvent.type == SDL_MOUSEBUTTONDOWN) {
                if (windowEvent.button.button == SDL_BUTTON_LEFT) {
                    Controls::lmouse = true;

                    if (GameIsolate_.world->WorldIsolate_.player && GameIsolate_.world->WorldIsolate_.player->heldItem != NULL) {
                        if (GameIsolate_.world->WorldIsolate_.player->heldItem->getFlag(ItemFlags::VACUUM)) {
                            GameIsolate_.world->WorldIsolate_.player->holdVacuum = true;
                        } else if (GameIsolate_.world->WorldIsolate_.player->heldItem->getFlag(ItemFlags::HAMMER)) {
// #define HAMMER_DEBUG_PHYSICS
#ifdef HAMMER_DEBUG_PHYSICS
                            int x = (int)((windowEvent.button.x - ofsX - camX) / scale);
                            int y = (int)((windowEvent.button.y - ofsY - camY) / scale);

                            GameIsolate_.world->physicsCheck(x, y);
#else
                            mx = windowEvent.button.x;
                            my = windowEvent.button.y;
                            int startInd = getAimSolidSurface(64);

                            if (startInd != -1) {
                                // GameIsolate_.world->WorldIsolate_.player->hammerX = x;
                                // GameIsolate_.world->WorldIsolate_.player->hammerY = y;
                                GameIsolate_.world->WorldIsolate_.player->hammerX = startInd % GameIsolate_.world->width;
                                GameIsolate_.world->WorldIsolate_.player->hammerY = startInd / GameIsolate_.world->width;
                                GameIsolate_.world->WorldIsolate_.player->holdHammer = true;
                                // METADOT_BUG("hammer down: {0:d} {0:d} {0:d} {0:d} {0:d}", x, y, startInd, startInd % GameIsolate_.world->width, startInd / GameIsolate_.world->width);
                                // GameIsolate_.world->setTile(GameIsolate_.world->WorldIsolate_.player->hammerX, GameIsolate_.world->WorldIsolate_.player->hammerY,
                                // MaterialInstance(&MaterialsList::GENERIC_SOLID, 0x00ff00ff));
                            }
#endif
#undef HAMMER_DEBUG_PHYSICS
                        } else if (GameIsolate_.world->WorldIsolate_.player->heldItem->getFlag(ItemFlags::CHISEL)) {
                            // if hovering rigidbody, open in chisel

                            int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / scale);
                            int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / scale);

                            std::vector<RigidBody *> rbs = GameIsolate_.world->WorldIsolate_.rigidBodies;  // copy
                            for (size_t i = 0; i < rbs.size(); i++) {
                                RigidBody *cur = rbs[i];

                                bool connect = false;
                                if (cur->body->IsEnabled()) {
                                    F32 s = sin(-cur->body->GetAngle());
                                    F32 c = cos(-cur->body->GetAngle());
                                    bool upd = false;
                                    for (F32 xx = -3; xx <= 3; xx += 0.5) {
                                        for (F32 yy = -3; yy <= 3; yy += 0.5) {
                                            if (abs(xx) + abs(yy) == 6) continue;
                                            // rotate point

                                            F32 tx = x + xx - cur->body->GetPosition().x;
                                            F32 ty = y + yy - cur->body->GetPosition().y;

                                            int ntx = (int)(tx * c - ty * s);
                                            int nty = (int)(tx * s + ty * c);

                                            if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                                U32 pixel = R_GET_PIXEL(cur->surface, ntx, nty);
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

                        } else if (GameIsolate_.world->WorldIsolate_.player->heldItem->getFlag(ItemFlags::TOOL)) {
                            // break with pickaxe

                            F32 breakSize = GameIsolate_.world->WorldIsolate_.player->heldItem->breakSize;

                            int x = (int)(GameIsolate_.world->WorldIsolate_.player->x + GameIsolate_.world->WorldIsolate_.player->hw / 2.0f + GameIsolate_.world->loadZone.x +
                                          10 * (F32)cos((GameIsolate_.world->WorldIsolate_.player->holdAngle + 180) * 3.1415f / 180.0f) - breakSize / 2);
                            int y = (int)(GameIsolate_.world->WorldIsolate_.player->y + GameIsolate_.world->WorldIsolate_.player->hh / 2.0f + GameIsolate_.world->loadZone.y +
                                          10 * (F32)sin((GameIsolate_.world->WorldIsolate_.player->holdAngle + 180) * 3.1415f / 180.0f) - breakSize / 2);

                            C_Surface *tex = SDL_CreateRGBSurfaceWithFormat(0, (int)breakSize, (int)breakSize, 32, SDL_PIXELFORMAT_ARGB8888);

                            int n = 0;
                            for (int xx = 0; xx < breakSize; xx++) {
                                for (int yy = 0; yy < breakSize; yy++) {
                                    F32 cx = (F32)((xx / breakSize) - 0.5);
                                    F32 cy = (F32)((yy / breakSize) - 0.5);

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
                                global.audioEngine.PlayEvent("event:/Player/Impact");
                                b2PolygonShape s;
                                s.SetAsBox(1, 1);
                                RigidBody *rb = GameIsolate_.world->makeRigidBody(b2_dynamicBody, (F32)x, (F32)y, 0, s, 1, (F32)0.3, tex);

                                b2Filter bf = {};
                                bf.categoryBits = 0x0001;
                                bf.maskBits = 0xffff;
                                rb->body->GetFixtureList()[0].SetFilterData(bf);

                                rb->body->SetLinearVelocity({(F32)((rand() % 100) / 100.0 - 0.5), (F32)((rand() % 100) / 100.0 - 0.5)});

                                GameIsolate_.world->WorldIsolate_.rigidBodies.push_back(rb);
                                GameIsolate_.world->updateRigidBodyHitbox(rb);

                                GameIsolate_.world->lastMeshLoadZone.x--;
                                GameIsolate_.world->updateWorldMesh();
                            }
                        }
                    }

                } else if (windowEvent.button.button == SDL_BUTTON_RIGHT) {
                    Controls::rmouse = true;
                    if (GameIsolate_.world->WorldIsolate_.player) GameIsolate_.world->WorldIsolate_.player->startThrow = Time::millis();
                } else if (windowEvent.button.button == SDL_BUTTON_MIDDLE) {
                    Controls::mmouse = true;
                }
            } else if (windowEvent.type == SDL_MOUSEBUTTONUP) {
                if (windowEvent.button.button == SDL_BUTTON_LEFT) {
                    Controls::lmouse = false;

                    if (GameIsolate_.world->WorldIsolate_.player) {
                        if (GameIsolate_.world->WorldIsolate_.player->heldItem) {
                            if (GameIsolate_.world->WorldIsolate_.player->heldItem->getFlag(ItemFlags::VACUUM)) {
                                if (GameIsolate_.world->WorldIsolate_.player->holdVacuum) {
                                    GameIsolate_.world->WorldIsolate_.player->holdVacuum = false;
                                }
                            } else if (GameIsolate_.world->WorldIsolate_.player->heldItem->getFlag(ItemFlags::HAMMER)) {
                                if (GameIsolate_.world->WorldIsolate_.player->holdHammer) {
                                    int x = (int)((windowEvent.button.x - global.GameData_.ofsX - global.GameData_.camX) / scale);
                                    int y = (int)((windowEvent.button.y - global.GameData_.ofsY - global.GameData_.camY) / scale);

                                    int dx = GameIsolate_.world->WorldIsolate_.player->hammerX - x;
                                    int dy = GameIsolate_.world->WorldIsolate_.player->hammerY - y;
                                    F32 len = sqrtf(dx * dx + dy * dy);
                                    F32 udx = dx / len;
                                    F32 udy = dy / len;

                                    int ex = GameIsolate_.world->WorldIsolate_.player->hammerX + dx;
                                    int ey = GameIsolate_.world->WorldIsolate_.player->hammerY + dy;
                                    METADOT_BUG("hammer up: %d %d %d %d", ex, ey, dx, dy);
                                    int endInd = -1;

                                    int nSegments = 1 + len / 10;
                                    std::vector<std::tuple<int, int>> points = {};
                                    for (int i = 0; i < nSegments; i++) {
                                        int sx = GameIsolate_.world->WorldIsolate_.player->hammerX + (int)((F32)(dx / nSegments) * (i + 1));
                                        int sy = GameIsolate_.world->WorldIsolate_.player->hammerY + (int)((F32)(dy / nSegments) * (i + 1));
                                        sx += rand() % 3 - 1;
                                        sy += rand() % 3 - 1;
                                        points.push_back(std::tuple<int, int>(sx, sy));
                                    }

                                    int nTilesChanged = 0;
                                    for (size_t i = 0; i < points.size(); i++) {
                                        int segSx = i == 0 ? GameIsolate_.world->WorldIsolate_.player->hammerX : std::get<0>(points[i - 1]);
                                        int segSy = i == 0 ? GameIsolate_.world->WorldIsolate_.player->hammerY : std::get<1>(points[i - 1]);
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
                                            GameIsolate_.world->tiles[index] = MaterialInstance(&MaterialsList::GENERIC_SAND, Drawing::darkenColor(GameIsolate_.world->tiles[index].color, 0.5f));
                                            GameIsolate_.world->dirty[index] = true;
                                            endInd = index;
                                            nTilesChanged++;
                                            return false;
                                        });

                                        // GameIsolate_.world->setTile(segSx, segSy, MaterialInstance(&MaterialsList::GENERIC_SOLID, 0x00ff00ff));
                                        if (broke) break;
                                    }

                                    // GameIsolate_.world->setTile(ex, ey, MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xff0000ff));

                                    int hx = (GameIsolate_.world->WorldIsolate_.player->hammerX + (endInd % GameIsolate_.world->width)) / 2;
                                    int hy = (GameIsolate_.world->WorldIsolate_.player->hammerY + (endInd / GameIsolate_.world->width)) / 2;

                                    if (GameIsolate_.world->getTile((int)(hx + udy * 2), (int)(hy - udx * 2)).mat->physicsType == PhysicsType::SOLID) {
                                        GameIsolate_.world->physicsCheck((int)(hx + udy * 2), (int)(hy - udx * 2));
                                    }

                                    if (GameIsolate_.world->getTile((int)(hx - udy * 2), (int)(hy + udx * 2)).mat->physicsType == PhysicsType::SOLID) {
                                        GameIsolate_.world->physicsCheck((int)(hx - udy * 2), (int)(hy + udx * 2));
                                    }

                                    if (nTilesChanged > 0) {
                                        global.audioEngine.PlayEvent("event:/Player/Impact");
                                    }

                                    // GameIsolate_.world->setTile((int)(hx), (int)(hy), MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xffffffff));
                                    // GameIsolate_.world->setTile((int)(hx + udy * 6), (int)(hy - udx * 6), MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xffff00ff));
                                    // GameIsolate_.world->setTile((int)(hx - udy * 6), (int)(hy + udx * 6), MaterialInstance(&MaterialsList::GENERIC_SOLID, 0x00ffffff));
                                }
                                GameIsolate_.world->WorldIsolate_.player->holdHammer = false;
                            }
                        }
                    }
                } else if (windowEvent.button.button == SDL_BUTTON_RIGHT) {
                    Controls::rmouse = false;
                    // pick up / throw item

                    int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / scale);
                    int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / scale);

                    bool swapped = false;
                    std::vector<RigidBody *> *rbs = &GameIsolate_.world->WorldIsolate_.rigidBodies;
                    for (size_t i = 0; i < rbs->size(); i++) {
                        RigidBody *cur = (*rbs)[i];

                        bool connect = false;
                        if (!static_cast<bool>(cur->surface)) continue;
                        if (cur->body->IsEnabled()) {
                            F32 s = sin(-cur->body->GetAngle());
                            F32 c = cos(-cur->body->GetAngle());
                            bool upd = false;
                            for (F32 xx = -3; xx <= 3; xx += 0.5) {
                                for (F32 yy = -3; yy <= 3; yy += 0.5) {
                                    if (abs(xx) + abs(yy) == 6) continue;
                                    // rotate point

                                    F32 tx = x + xx - cur->body->GetPosition().x;
                                    F32 ty = y + yy - cur->body->GetPosition().y;

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
                            if (GameIsolate_.world->WorldIsolate_.player) {
                                GameIsolate_.world->WorldIsolate_.player->setItemInHand(Item::makeItem(ItemFlags::RIGIDBODY, cur), GameIsolate_.world);

                                GameIsolate_.world->b2world->DestroyBody(cur->body);
                                GameIsolate_.world->WorldIsolate_.rigidBodies.erase(
                                        std::remove(GameIsolate_.world->WorldIsolate_.rigidBodies.begin(), GameIsolate_.world->WorldIsolate_.rigidBodies.end(), cur),
                                        GameIsolate_.world->WorldIsolate_.rigidBodies.end());

                                swapped = true;
                            }
                            break;
                        }
                    }

                    if (!swapped) {
                        if (GameIsolate_.world->WorldIsolate_.player) GameIsolate_.world->WorldIsolate_.player->setItemInHand(NULL, GameIsolate_.world);
                    }

                } else if (windowEvent.button.button == SDL_BUTTON_MIDDLE) {
                    Controls::mmouse = false;
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
        GameIsolate_.profiler.End(Profiler::Stage::SdlInput);

#pragma endregion SDL_Input

#pragma region GameTick
        GameIsolate_.profiler.Begin(Profiler::Stage::GameTick);

        // if(GameIsolate_.globaldef.tick_world)
        updateFrameEarly();
        global.scripts->UpdateTick();

        while (Time.now - Time.lastTick > Time.mspt) {
            if (GameIsolate_.globaldef.tick_world) tick();
            Render.target = Render.realTarget;
            Time.lastTick = Time.now;
            tickTime++;
        }

        if (GameIsolate_.globaldef.tick_world) updateFrameLate();
        GameIsolate_.profiler.End(Profiler::Stage::GameTick);
#pragma endregion GameTick

#pragma region Render
        // render
        GameIsolate_.profiler.Begin(Profiler::Stage::Rendering);

        Render.target = Render.realTarget;
        R_Clear(Render.target);

        GameIsolate_.profiler.Begin(Profiler::Stage::RenderEarly);
        renderEarly();
        Render.target = Render.realTarget;
        GameIsolate_.profiler.End(Profiler::Stage::RenderEarly);

        GameIsolate_.profiler.Begin(Profiler::Stage::RenderLate);
        renderLate();
        Render.target = Render.realTarget;
        GameIsolate_.profiler.End(Profiler::Stage::RenderLate);

        global.scripts->UpdateRender();

        // auto image2 = R_CopyImageFromSurface(GameIsolate_.texturepack->testAse);
        // METADOT_ASSERT_E(image2);
        // R_BlitScale(image2, NULL, Render.target, 200, 200, 1.0f, 1.0f);

        // FontCache_Rect rightHalf = {0, 0, Screen.windowWidth / 4.0f, Screen.windowWidth / 1.0f};
        // R_RectangleFilled(Render.target, rightHalf.x, rightHalf.y, rightHalf.x + rightHalf.w, rightHalf.y + rightHalf.h, {255, 255, 255, 255});

        // METADOT_ASSERT_E(font);
        // FontCache_DrawColor(font, Render.target, 200, 200, {255, 144, 255, 255}, "This is %s.\n It works.", "example text");

        R_ActivateShaderProgram(0, NULL);
        R_FlushBlitBuffer();

        // render ImGui
        UIRendererUpdate();

        if (GameIsolate_.globaldef.draw_material_info && !ImGui::GetIO().WantCaptureMouse) {

            int msx = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / scale);
            int msy = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / scale);

            MaterialInstance tile;

            if (msx >= 0 && msy >= 0 && msx < GameIsolate_.world->width && msy < GameIsolate_.world->height) {
                tile = GameIsolate_.world->tiles[msx + msy * GameIsolate_.world->width];
                // Drawing::drawText(target, tile.mat->name.c_str(), font16, mx + 14, my, 0xff, 0xff, 0xff, ALIGN_LEFT);

                if (tile.mat->id == MaterialsList::GENERIC_AIR.id) {
                    std::vector<RigidBody *> *rbs = &GameIsolate_.world->WorldIsolate_.rigidBodies;

                    for (size_t i = 0; i < rbs->size(); i++) {
                        RigidBody *cur = (*rbs)[i];
                        if (cur->body->IsEnabled() && static_cast<bool>(cur->surface)) {
                            F32 s = sin(-cur->body->GetAngle());
                            F32 c = cos(-cur->body->GetAngle());
                            bool upd = false;

                            F32 tx = msx - cur->body->GetPosition().x;
                            F32 ty = msy - cur->body->GetPosition().y;

                            int ntx = (int)(tx * c - ty * s);
                            int nty = (int)(tx * s + ty * c);

                            if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                tile = cur->tiles[ntx + nty * cur->matWidth];
                            }
                        }
                    }
                }

                if (tile.mat->id != MaterialsList::GENERIC_AIR.id) {

                    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.11f, 0.11f, 0.11f, 0.4f));
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.00f, 1.00f, 1.00f, 0.2f));
                    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", tile.mat->name.c_str());

                    if (GameIsolate_.globaldef.draw_detailed_material_info) {

                        if (tile.mat->physicsType == PhysicsType::SOUP) {
                            ImGui::Text("fluidAmount = %f", tile.fluidAmount);
                        }

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

        UIRendererDraw();

        // render fade in/out
        if (fadeInWaitFrames > 0) {
            fadeInWaitFrames--;
            fadeInStart = Time.now;
            R_RectangleFilled(Render.target, 0, 0, Screen.windowWidth, Screen.windowHeight, {0, 0, 0, 255});
        } else if (fadeInStart > 0 && fadeInLength > 0) {

            F32 thru = 1 - (F32)(Time.now - fadeInStart) / fadeInLength;

            if (thru >= 0 && thru <= 1) {
                R_RectangleFilled(Render.target, 0, 0, Screen.windowWidth, Screen.windowHeight, {0, 0, 0, (U8)(thru * 255)});
            } else {
                fadeInStart = 0;
                fadeInLength = 0;
            }
        }

        if (fadeOutWaitFrames > 0) {
            fadeOutWaitFrames--;
            fadeOutStart = Time.now;
        } else if (fadeOutStart > 0 && fadeOutLength > 0) {

            F32 thru = (F32)(Time.now - fadeOutStart) / fadeOutLength;

            if (thru >= 0 && thru <= 1) {
                R_RectangleFilled(Render.target, 0, 0, Screen.windowWidth, Screen.windowHeight, {0, 0, 0, (U8)(thru * 255)});
            } else {
                R_RectangleFilled(Render.target, 0, 0, Screen.windowWidth, Screen.windowHeight, {0, 0, 0, 255});
                fadeOutStart = 0;
                fadeOutLength = 0;
                fadeOutCallback();
            }
        }

        R_Flip(Render.target);

        GameIsolate_.profiler.End(Profiler::Stage::Rendering);

#pragma endregion Render

        EngineUpdateEnd();

        frameCount++;
        if (Time.now - lastFPS >= 1000) {
            lastFPS = Time.now;
            Time.fps = frameCount;
            frameCount = 0;

            // calculate "feels like" fps
            F32 sum = 0;
            F32 num = 0.01;

            for (int i = 0; i < FrameTimeNum; i++) {
                F32 weight = frameTimes[i];
                sum += weight * frameTimes[i];
                num += weight;
            }

            Time.feelsLikeFps = 1000 / (sum / num);
        }

        for (int i = 1; i < FrameTimeNum; i++) {
            frameTimes[i - 1] = frameTimes[i];
        }
        frameTimes[FrameTimeNum - 1] = (U16)(Time::millis() - Time.now);

        Time.lastTime = Time.now;
    }

    return exit();
}

int Game::exit() {
    METADOT_INFO("Shutting down...");

    GameIsolate_.world->saveWorld();

    running = false;

    // TODO CppScript

    // release resources & shutdown

    global.scripts->End();
    METADOT_DELETE(C, global.scripts, Scripts);

    ReleaseGameData();

    UIRendererFree();

    METADOT_DELETE(C, objectDelete, U8);
    GameIsolate_.backgrounds->Unload();

    GameSystem_.console.End();

    METADOT_DELETE(C, debugDraw, DebugDraw);
    METADOT_DELETE(C, movingTiles, U16);

    FontCache_FreeFont(font);

    METADOT_DELETE(C, GameIsolate_.updateDirtyPool, ThreadPool);
    metadot_thpool_destroy(GameIsolate_.updateDirtyPool2);

    if (GameIsolate_.world) {
        METADOT_DELETE(C, GameIsolate_.world, World);
        GameIsolate_.world = nullptr;
    }

    EndShaders(&global.shaderworker);

    EndWindow();
    global.audioEngine.Shutdown();
    SDL_DestroyWindow(Core.window);

    EndEngine(0);

    METADOT_INFO("Clean done...");

    return METADOT_OK;
}

void Game::updateFrameEarly() {

    // handle controls
    if (Controls::DEBUG_UI->get()) {
        GameUI::DebugDrawUI::visible ^= true;
        GameIsolate_.globaldef.ui_tweak ^= true;
    }

    if (GameIsolate_.globaldef.draw_frame_graph) {
        if (Controls::STATS_DISPLAY->get()) {
            GameIsolate_.globaldef.draw_frame_graph = false;
            GameIsolate_.globaldef.draw_debug_stats = false;
            GameIsolate_.globaldef.draw_chunk_state = false;
            GameIsolate_.globaldef.draw_detailed_material_info = false;
        }
    } else {
        if (Controls::STATS_DISPLAY->get()) {
            GameIsolate_.globaldef.draw_frame_graph = true;
            GameIsolate_.globaldef.draw_debug_stats = true;

            if (Controls::STATS_DISPLAY_DETAILED->get()) {
                GameIsolate_.globaldef.draw_chunk_state = true;
                GameIsolate_.globaldef.draw_detailed_material_info = true;
            }
        }
    }

    if (Controls::DEBUG_REFRESH->get()) {
        for (int x = 0; x < GameIsolate_.world->width; x++) {
            for (int y = 0; y < GameIsolate_.world->height; y++) {
                GameIsolate_.world->dirty[x + y * GameIsolate_.world->width] = true;
                GameIsolate_.world->layer2Dirty[x + y * GameIsolate_.world->width] = true;
                GameIsolate_.world->backgroundDirty[x + y * GameIsolate_.world->width] = true;
            }
        }
    }

    if (Controls::DEBUG_RIGID->get()) {
        for (auto &cur : GameIsolate_.world->WorldIsolate_.rigidBodies) {
            if (cur->body->IsEnabled()) {
                F32 s = sin(cur->body->GetAngle());
                F32 c = cos(cur->body->GetAngle());
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
        GameIsolate_.world->WorldIsolate_.rigidBodies.clear();
    }

    if (Controls::DEBUG_UPDATE_WORLD_MESH->get()) {
        GameIsolate_.world->updateWorldMesh();
    }

    if (Controls::DEBUG_EXPLODE->get()) {
        int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / scale);
        int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / scale);
        GameIsolate_.world->explosion(x, y, 30);
    }

    if (Controls::DEBUG_CARVE->get()) {
        // carve square

        int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / scale - 16);
        int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / scale - 16);

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
            RigidBody *rb = GameIsolate_.world->makeRigidBody(b2_dynamicBody, (F32)x, (F32)y, 0, s, 1, (F32)0.3, tex);
            for (int tx = 0; tx < tex->w; tx++) {
                b2Filter bf = {};
                bf.categoryBits = 0x0002;
                bf.maskBits = 0x0001;
                rb->body->GetFixtureList()[0].SetFilterData(bf);
            }
            GameIsolate_.world->WorldIsolate_.rigidBodies.push_back(rb);
            GameIsolate_.world->updateRigidBodyHitbox(rb);

            GameIsolate_.world->updateWorldMesh();
        }
    }

    if (Controls::DEBUG_BRUSHSIZE_INC->get()) {
        GameUI::DebugDrawUI::brushSize = GameUI::DebugDrawUI::brushSize < 50 ? GameUI::DebugDrawUI::brushSize + 1 : GameUI::DebugDrawUI::brushSize;
    }

    if (Controls::DEBUG_BRUSHSIZE_DEC->get()) {
        GameUI::DebugDrawUI::brushSize = GameUI::DebugDrawUI::brushSize > 1 ? GameUI::DebugDrawUI::brushSize - 1 : GameUI::DebugDrawUI::brushSize;
    }

    if (Controls::DEBUG_TOGGLE_PLAYER->get()) {
        if (GameIsolate_.world->WorldIsolate_.player) {
            global.GameData_.freeCamX = GameIsolate_.world->WorldIsolate_.player->x + GameIsolate_.world->WorldIsolate_.player->hw / 2.0f;
            global.GameData_.freeCamY = GameIsolate_.world->WorldIsolate_.player->y - GameIsolate_.world->WorldIsolate_.player->hh / 2.0f;
            GameIsolate_.world->WorldIsolate_.entities.erase(
                    std::remove(GameIsolate_.world->WorldIsolate_.entities.begin(), GameIsolate_.world->WorldIsolate_.entities.end(), GameIsolate_.world->WorldIsolate_.player),
                    GameIsolate_.world->WorldIsolate_.entities.end());
            GameIsolate_.world->b2world->DestroyBody(GameIsolate_.world->WorldIsolate_.player->rb->body);
            delete GameIsolate_.world->WorldIsolate_.player;
            GameIsolate_.world->WorldIsolate_.player = nullptr;
        } else {
            Player *e = new Player();
            e->x = -GameIsolate_.world->loadZone.x + GameIsolate_.world->tickZone.x + GameIsolate_.world->tickZone.w / 2.0f;
            e->y = -GameIsolate_.world->loadZone.y + GameIsolate_.world->tickZone.y + GameIsolate_.world->tickZone.h / 2.0f;
            e->vx = 0;
            e->vy = 0;
            e->hw = 10;
            e->hh = 20;
            b2PolygonShape sh;
            sh.SetAsBox(e->hw / 2.0f + 1, e->hh / 2.0f);
            e->rb = GameIsolate_.world->makeRigidBody(b2BodyType::b2_kinematicBody, e->x + e->hw / 2.0f - 0.5, e->y + e->hh / 2.0f - 0.5, 0, sh, 1, 1, NULL);
            e->rb->body->SetGravityScale(0);
            e->rb->body->SetLinearDamping(0);
            e->rb->body->SetAngularDamping(0);

            Item *i3 = new Item();
            i3->setFlag(ItemFlags::VACUUM);
            i3->vacuumParticles = {};
            i3->surface = LoadTexture("data/assets/objects/testVacuum.png")->surface;
            i3->texture = R_CopyImageFromSurface(i3->surface);
            R_SetImageFilter(i3->texture, R_FILTER_NEAREST);
            i3->pivotX = 6;
            e->setItemInHand(i3, GameIsolate_.world);

            b2Filter bf = {};
            bf.categoryBits = 0x0001;
            // bf.maskBits = 0x0000;
            e->rb->body->GetFixtureList()[0].SetFilterData(bf);

            GameIsolate_.world->WorldIsolate_.entities.push_back(e);
            GameIsolate_.world->WorldIsolate_.player = e;

            /*accLoadX = 0;
accLoadY = 0;*/
        }
    }

    if (Controls::PAUSE->get()) {
        if (this->state == GameState::INGAME) {
            GameUI::MainMenuUI__visible = !GameUI::MainMenuUI__visible;
        }
    }

    global.audioEngine.Update();

    if (state == LOADING) {

    } else {
        global.audioEngine.SetEventParameter("event:/World/Sand", "Sand", 0);
        if (GameIsolate_.world->WorldIsolate_.player && GameIsolate_.world->WorldIsolate_.player->heldItem != NULL &&
            GameIsolate_.world->WorldIsolate_.player->heldItem->getFlag(ItemFlags::FLUID_CONTAINER)) {
            if (Controls::lmouse && GameIsolate_.world->WorldIsolate_.player->heldItem->carry.size() > 0) {
                // shoot fluid from container

                int x = (int)(GameIsolate_.world->WorldIsolate_.player->x + GameIsolate_.world->WorldIsolate_.player->hw / 2.0f + GameIsolate_.world->loadZone.x +
                              10 * (F32)cos((GameIsolate_.world->WorldIsolate_.player->holdAngle + 180) * 3.1415f / 180.0f));
                int y = (int)(GameIsolate_.world->WorldIsolate_.player->y + GameIsolate_.world->WorldIsolate_.player->hh / 2.0f + GameIsolate_.world->loadZone.y +
                              10 * (F32)sin((GameIsolate_.world->WorldIsolate_.player->holdAngle + 180) * 3.1415f / 180.0f));

                MaterialInstance mat = GameIsolate_.world->WorldIsolate_.player->heldItem->carry[GameIsolate_.world->WorldIsolate_.player->heldItem->carry.size() - 1];
                GameIsolate_.world->WorldIsolate_.player->heldItem->carry.pop_back();
                GameIsolate_.world->addParticle(new Particle(mat, (F32)x, (F32)y,
                                                             (F32)(GameIsolate_.world->WorldIsolate_.player->vx / 2 + (rand() % 10 - 5) / 10.0f +
                                                                   1.5f * (F32)cos((GameIsolate_.world->WorldIsolate_.player->holdAngle + 180) * 3.1415f / 180.0f)),
                                                             (F32)(GameIsolate_.world->WorldIsolate_.player->vy / 2 + -(rand() % 5 + 5) / 10.0f +
                                                                   1.5f * (F32)sin((GameIsolate_.world->WorldIsolate_.player->holdAngle + 180) * 3.1415f / 180.0f)),
                                                             0, (F32)0.1));

                int i = (int)GameIsolate_.world->WorldIsolate_.player->heldItem->carry.size();
                i = (int)((i / (F32)GameIsolate_.world->WorldIsolate_.player->heldItem->capacity) * GameIsolate_.world->WorldIsolate_.player->heldItem->fill.size());
                U16Point pt = GameIsolate_.world->WorldIsolate_.player->heldItem->fill[i];
                R_GET_PIXEL(GameIsolate_.world->WorldIsolate_.player->heldItem->surface, pt.x, pt.y) = 0x00;

                GameIsolate_.world->WorldIsolate_.player->heldItem->texture = R_CopyImageFromSurface(GameIsolate_.world->WorldIsolate_.player->heldItem->surface);
                R_SetImageFilter(GameIsolate_.world->WorldIsolate_.player->heldItem->texture, R_FILTER_NEAREST);

                global.audioEngine.SetEventParameter("event:/World/Sand", "Sand", 1);

            } else {
                // pick up fluid into container

                F32 breakSize = GameIsolate_.world->WorldIsolate_.player->heldItem->breakSize;

                int x = (int)(GameIsolate_.world->WorldIsolate_.player->x + GameIsolate_.world->WorldIsolate_.player->hw / 2.0f + GameIsolate_.world->loadZone.x +
                              10 * (F32)cos((GameIsolate_.world->WorldIsolate_.player->holdAngle + 180) * 3.1415f / 180.0f) - breakSize / 2);
                int y = (int)(GameIsolate_.world->WorldIsolate_.player->y + GameIsolate_.world->WorldIsolate_.player->hh / 2.0f + GameIsolate_.world->loadZone.y +
                              10 * (F32)sin((GameIsolate_.world->WorldIsolate_.player->holdAngle + 180) * 3.1415f / 180.0f) - breakSize / 2);

                int n = 0;
                for (int xx = 0; xx < breakSize; xx++) {
                    for (int yy = 0; yy < breakSize; yy++) {
                        if (GameIsolate_.world->WorldIsolate_.player->heldItem->capacity == 0 ||
                            (GameIsolate_.world->WorldIsolate_.player->heldItem->carry.size() < GameIsolate_.world->WorldIsolate_.player->heldItem->capacity)) {
                            F32 cx = (F32)((xx / breakSize) - 0.5);
                            F32 cy = (F32)((yy / breakSize) - 0.5);

                            if (cx * cx + cy * cy > 0.25f) continue;

                            if (GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width].mat->physicsType == PhysicsType::SAND ||
                                GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width].mat->physicsType == PhysicsType::SOUP) {
                                GameIsolate_.world->WorldIsolate_.player->heldItem->carry.push_back(GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width]);

                                int i = (int)GameIsolate_.world->WorldIsolate_.player->heldItem->carry.size() - 1;
                                i = (int)((i / (F32)GameIsolate_.world->WorldIsolate_.player->heldItem->capacity) * GameIsolate_.world->WorldIsolate_.player->heldItem->fill.size());
                                U16Point pt = GameIsolate_.world->WorldIsolate_.player->heldItem->fill[i];
                                U32 c = GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width].color;
                                R_GET_PIXEL(GameIsolate_.world->WorldIsolate_.player->heldItem->surface, pt.x, pt.y) =
                                        (GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width].mat->alpha << 24) + c;

                                GameIsolate_.world->WorldIsolate_.player->heldItem->texture = R_CopyImageFromSurface(GameIsolate_.world->WorldIsolate_.player->heldItem->surface);
                                R_SetImageFilter(GameIsolate_.world->WorldIsolate_.player->heldItem->texture, R_FILTER_NEAREST);

                                GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width] = Tiles_NOTHING;
                                GameIsolate_.world->dirty[(x + xx) + (y + yy) * GameIsolate_.world->width] = true;
                                n++;
                            }
                        }
                    }
                }

                if (n > 0) {
                    global.audioEngine.PlayEvent("event:/Player/Impact");
                }
            }
        }

        // rigidbody hover

        int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / scale);
        int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / scale);

        bool swapped = false;
        F32 hoverDelta = 10.0 * Time.deltaTime / 1000.0;

        // this copies the vector
        std::vector<RigidBody *> rbs = GameIsolate_.world->WorldIsolate_.rigidBodies;
        for (size_t i = 0; i < rbs.size(); i++) {
            RigidBody *cur = rbs[i];

            if (swapped) {
                cur->hover = (F32)std::fmax(0, cur->hover - hoverDelta);
                continue;
            }

            bool connect = false;
            if (cur->body->IsEnabled()) {
                F32 s = sin(-cur->body->GetAngle());
                F32 c = cos(-cur->body->GetAngle());
                bool upd = false;
                for (F32 xx = -3; xx <= 3; xx += 0.5) {
                    for (F32 yy = -3; yy <= 3; yy += 0.5) {
                        if (abs(xx) + abs(yy) == 6) continue;
                        // rotate point

                        F32 tx = x + xx - cur->body->GetPosition().x;
                        F32 ty = y + yy - cur->body->GetPosition().y;

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
                cur->hover = (F32)std::fmin(1, cur->hover + hoverDelta);
            } else {
                cur->hover = (F32)std::fmax(0, cur->hover - hoverDelta);
            }
        }

        // update GameIsolate_.world->tickZone

        GameIsolate_.world->tickZone = {CHUNK_W, CHUNK_H, GameIsolate_.world->width - CHUNK_W * 2, GameIsolate_.world->height - CHUNK_H * 2};
        if (GameIsolate_.world->tickZone.x + GameIsolate_.world->tickZone.w > GameIsolate_.world->width) {
            GameIsolate_.world->tickZone.x = GameIsolate_.world->width - GameIsolate_.world->tickZone.w;
        }

        if (GameIsolate_.world->tickZone.y + GameIsolate_.world->tickZone.h > GameIsolate_.world->height) {
            GameIsolate_.world->tickZone.y = GameIsolate_.world->height - GameIsolate_.world->tickZone.h;
        }
    }
}

void update_ParticlePixels(void *arg) {
    // SDL_SetRenderTarget(renderer, textureParticles);
    void *particlePixels = global.game->TexturePack_.pixelsParticles_ar;

    memset(particlePixels, 0, (size_t)global.game->GameIsolate_.world->width * global.game->GameIsolate_.world->height * 4);

    global.game->GameIsolate_.world->renderParticles((U8 **)&particlePixels);
    global.game->GameIsolate_.world->tickParticles();

    // SDL_SetRenderTarget(renderer, NULL);
}

void update_ObjectBounds(void *arg) { global.game->GameIsolate_.world->tickObjectBounds(); }

void Game::tick() {

    // METADOT_BUG("{0:d} {0:d}", accLoadX, accLoadY);
    if (state == LOADING) {
        if (GameIsolate_.world) {
            // tick chunkloading
            GameIsolate_.world->frame();
            if (GameIsolate_.world->WorldIsolate_.readyToMerge.size() == 0 && fadeOutStart == 0) {
                fadeOutStart = Time.now;
                fadeOutLength = 250;
                fadeOutCallback = [&]() {
                    fadeInStart = Time.now;
                    fadeInLength = 500;
                    fadeInWaitFrames = 4;
                    state = stateAfterLoad;
                };

                SetWindowFlash(engine_windowflashaction::START_COUNT, 1, 333);
            }
        }
    } else {

        int lastReadyToMergeSize = (int)GameIsolate_.world->WorldIsolate_.readyToMerge.size();

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

        if (GameIsolate_.globaldef.tick_world && GameIsolate_.world->WorldIsolate_.readyToMerge.size() == 0) {
            GameIsolate_.world->tickChunks();
        }

        // render objects

        memset(objectDelete, false, (size_t)GameIsolate_.world->width * GameIsolate_.world->height * sizeof(bool));

        R_SetBlendMode(TexturePack_.textureObjects, R_BLEND_NORMAL);
        R_SetBlendMode(TexturePack_.textureObjectsLQ, R_BLEND_NORMAL);
        R_SetBlendMode(TexturePack_.textureObjectsBack, R_BLEND_NORMAL);

        for (size_t i = 0; i < GameIsolate_.world->WorldIsolate_.rigidBodies.size(); i++) {
            RigidBody *cur = GameIsolate_.world->WorldIsolate_.rigidBodies[i];
            if (cur == nullptr) continue;
            if (cur->surface == nullptr) continue;
            if (!cur->body->IsEnabled()) continue;

            F32 x = cur->body->GetPosition().x;
            F32 y = cur->body->GetPosition().y;

            // draw

            R_Target *tgt = cur->back ? TexturePack_.textureObjectsBack->target : TexturePack_.textureObjects->target;
            R_Target *tgtLQ = cur->back ? TexturePack_.textureObjectsBack->target : TexturePack_.textureObjectsLQ->target;
            int scaleObjTex = GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1;

            R_Rect r = {x * scaleObjTex, y * scaleObjTex, (F32)cur->surface->w * scaleObjTex, (F32)cur->surface->h * scaleObjTex};

            if (cur->texNeedsUpdate) {
                if (cur->texture != nullptr) {
                    R_FreeImage(cur->texture);
                }
                cur->texture = R_CopyImageFromSurface(cur->surface);
                R_SetImageFilter(cur->texture, R_FILTER_NEAREST);
                cur->texNeedsUpdate = false;
            }

            R_BlitRectX(cur->texture, NULL, tgt, &r, cur->body->GetAngle() * 180 / (F32)M_PI, 0, 0, R_FLIP_NONE);

            // draw outline

            U8 outlineAlpha = (U8)(cur->hover * 255);
            if (outlineAlpha > 0) {
                METAENGINE_Color col = {0xff, 0xff, 0x80, outlineAlpha};
                R_SetShapeBlendMode(R_BLEND_NORMAL_FACTOR_ALPHA);  // SDL_BLENDMODE_BLEND
                for (auto &l : cur->outline) {
                    b2Vec2 *vec = new b2Vec2[l.GetNumPoints()];
                    for (int j = 0; j < l.GetNumPoints(); j++) {
                        vec[j] = {(F32)l.GetPoint(j).x / scale, (F32)l.GetPoint(j).y / scale};
                    }
                    Drawing::drawPolygon(tgtLQ, col, vec, (int)x, (int)y, scale, l.GetNumPoints(), cur->body->GetAngle(), 0, 0);
                    delete[] vec;
                }
                R_SetShapeBlendMode(R_BLEND_NORMAL);  // SDL_BLENDMODE_NONE
            }

            // displace fluids

            F32 s = sin(cur->body->GetAngle());
            F32 c = cos(cur->body->GetAngle());

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
                            GameIsolate_.world->addParticle(new Particle(GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width], (F32)wxd, (F32)(wyd - 3), (F32)((rand() % 10 - 5) / 10.0f),
                                                                         (F32)(-(rand() % 5 + 5) / 10.0f), 0, (F32)0.1));
                            GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width] = rmat;
                            // objectDelete[wxd + wyd * GameIsolate_.world->width] = true;
                            GameIsolate_.world->dirty[wxd + wyd * GameIsolate_.world->width] = true;
                            cur->body->SetLinearVelocity({cur->body->GetLinearVelocity().x * (F32)0.99, cur->body->GetLinearVelocity().y * (F32)0.99});
                            cur->body->SetAngularVelocity(cur->body->GetAngularVelocity() * (F32)0.98);
                            break;
                        } else if (GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width].mat->physicsType == PhysicsType::SOUP) {
                            GameIsolate_.world->addParticle(new Particle(GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width], (F32)wxd, (F32)(wyd - 3), (F32)((rand() % 10 - 5) / 10.0f),
                                                                         (F32)(-(rand() % 5 + 5) / 10.0f), 0, (F32)0.1));
                            GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width] = rmat;
                            // objectDelete[wxd + wyd * GameIsolate_.world->width] = true;
                            GameIsolate_.world->dirty[wxd + wyd * GameIsolate_.world->width] = true;
                            cur->body->SetLinearVelocity({cur->body->GetLinearVelocity().x * (F32)0.998, cur->body->GetLinearVelocity().y * (F32)0.998});
                            cur->body->SetAngularVelocity(cur->body->GetAngularVelocity() * (F32)0.99);
                            break;
                        }
                    }
                }
            }
        }

        // render entities

        if (lastReadyToMergeSize == 0) {
            GameIsolate_.world->tickEntities(TexturePack_.textureEntities->target);

            if (GameIsolate_.world->WorldIsolate_.player) {
                if (GameIsolate_.world->WorldIsolate_.player->holdHammer) {
                    int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / scale);
                    int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / scale);
                    R_Line(TexturePack_.textureEntitiesLQ->target, x, y, GameIsolate_.world->WorldIsolate_.player->hammerX, GameIsolate_.world->WorldIsolate_.player->hammerY,
                           {0xff, 0xff, 0x00, 0xff});
                }
            }
        }
        R_SetShapeBlendMode(R_BLEND_NORMAL);  // SDL_BLENDMODE_NONE

        // entity fluid displacement & make solid

        for (size_t i = 0; i < GameIsolate_.world->WorldIsolate_.entities.size(); i++) {
            WorldEntity cur = *GameIsolate_.world->WorldIsolate_.entities[i];

            for (int tx = 0; tx < cur.hw; tx++) {
                for (int ty = 0; ty < cur.hh; ty++) {

                    int wx = (int)(tx + cur.x + GameIsolate_.world->loadZone.x);
                    int wy = (int)(ty + cur.y + GameIsolate_.world->loadZone.y);
                    if (wx < 0 || wy < 0 || wx >= GameIsolate_.world->width || wy >= GameIsolate_.world->height) continue;
                    if (GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width].mat->physicsType == PhysicsType::AIR) {
                        GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width] = Tiles_OBJECT;
                        objectDelete[wx + wy * GameIsolate_.world->width] = true;
                    } else if (GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width].mat->physicsType == PhysicsType::SAND ||
                               GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width].mat->physicsType == PhysicsType::SOUP) {
                        GameIsolate_.world->addParticle(new Particle(GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width], (F32)(wx + rand() % 3 - 1 - cur.vx), (F32)(wy - abs(cur.vy)),
                                                                     (F32)(-cur.vx / 4 + (rand() % 10 - 5) / 5.0f), (F32)(-cur.vy / 4 + -(rand() % 5 + 5) / 5.0f), 0, (F32)0.1));
                        GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width] = Tiles_OBJECT;
                        objectDelete[wx + wy * GameIsolate_.world->width] = true;
                        GameIsolate_.world->dirty[wx + wy * GameIsolate_.world->width] = true;
                    }
                }
            }
        }

        if (GameIsolate_.globaldef.tick_world && GameIsolate_.world->WorldIsolate_.readyToMerge.size() == 0) {
            GameIsolate_.world->tick();
        }

        if (Controls::DEBUG_TICK->get()) {
            GameIsolate_.world->tick();
        }

        // Tick Cam zoom

        if (state == INGAME && (Controls::ZOOM_IN->get() || Controls::ZOOM_OUT->get())) {
            F32 CamZoomIn = (F32)(Controls::ZOOM_IN->get());
            F32 CamZoomOut = (F32)(Controls::ZOOM_OUT->get());

            F32 deltaScale = CamZoomIn - CamZoomOut;
            int oldScale = scale;
            scale += deltaScale;
            if (scale < 1) scale = 1;

            global.GameData_.ofsX = (global.GameData_.ofsX - Screen.windowWidth / 2) / oldScale * scale + Screen.windowWidth / 2;
            global.GameData_.ofsY = (global.GameData_.ofsY - Screen.windowHeight / 2) / oldScale * scale + Screen.windowHeight / 2;
        } else {
        }

        // player movement
        tickPlayer();

        // update particles, tickObjects, update dirty
        // TODO: this is not entirely thread safe since tickParticles changes World::tiles and World::dirty

        bool hadDirty = false;
        bool hadLayer2Dirty = false;
        bool hadBackgroundDirty = false;
        bool hadFire = false;
        bool hadFlow = false;

        // int pitch;
        // void* vdpixels_ar = texture->data;
        // U8* dpixels_ar = (U8*)vdpixels_ar;
        U8 *dpixels_ar = TexturePack_.pixels_ar;
        U8 *dpixelsFire_ar = TexturePack_.pixelsFire_ar;
        U8 *dpixelsFlow_ar = TexturePack_.pixelsFlow_ar;
        U8 *dpixelsEmission_ar = TexturePack_.pixelsEmission_ar;

        std::vector<std::future<void>> results = {};

        int i = 1;
        metadot_thpool_addwork(GameIsolate_.updateDirtyPool2, update_ParticlePixels, (void *)(uintptr_t)i);

        if (GameIsolate_.world->WorldIsolate_.readyToMerge.size() == 0) {
            metadot_thpool_addwork(GameIsolate_.updateDirtyPool2, update_ObjectBounds, (void *)(uintptr_t)i);
        }

        for (int i = 0; i < results.size(); i++) {
            results[i].get();
        }

        metadot_thpool_wait(GameIsolate_.updateDirtyPool2);

        for (size_t i = 0; i < GameIsolate_.world->WorldIsolate_.rigidBodies.size(); i++) {
            RigidBody *cur = GameIsolate_.world->WorldIsolate_.rigidBodies[i];
            if (cur == nullptr) continue;
            if (cur->surface == nullptr) continue;
            if (!cur->body->IsEnabled()) continue;

            F32 x = cur->body->GetPosition().x;
            F32 y = cur->body->GetPosition().y;

            F32 s = sin(cur->body->GetAngle());
            F32 c = cos(cur->body->GetAngle());

            std::vector<std::pair<int, int>> checkDirs = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (int tx = 0; tx < cur->matWidth; tx++) {
                for (int ty = 0; ty < cur->matHeight; ty++) {
                    MaterialInstance rmat = cur->tiles[tx + ty * cur->matWidth];
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

        if (GameIsolate_.world->WorldIsolate_.readyToMerge.size() == 0) {
            if (GameIsolate_.globaldef.tick_box2d) GameIsolate_.world->tickObjects();
        }

        if (tickTime % 10 == 0) GameIsolate_.world->tickObjectsMesh();

        for (int i = 0; i < global.GameData_.materials_count; i++) movingTiles[i] = 0;

        results.clear();
        results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
            for (int i = 0; i < GameIsolate_.world->width * GameIsolate_.world->height; i++) {
                const unsigned int offset = i * 4;

                if (GameIsolate_.world->dirty[i]) {
                    hadDirty = true;
                    movingTiles[GameIsolate_.world->tiles[i].mat->id]++;
                    if (GameIsolate_.world->tiles[i].mat->physicsType == PhysicsType::AIR) {
                        dpixels_ar[offset + 0] = 0;                             // b
                        dpixels_ar[offset + 1] = 0;                             // g
                        dpixels_ar[offset + 2] = 0;                             // r
                        dpixels_ar[offset + 3] = METAENGINE_ALPHA_TRANSPARENT;  // a

                        dpixelsFire_ar[offset + 0] = 0;                             // b
                        dpixelsFire_ar[offset + 1] = 0;                             // g
                        dpixelsFire_ar[offset + 2] = 0;                             // r
                        dpixelsFire_ar[offset + 3] = METAENGINE_ALPHA_TRANSPARENT;  // a

                        dpixelsEmission_ar[offset + 0] = 0;                             // b
                        dpixelsEmission_ar[offset + 1] = 0;                             // g
                        dpixelsEmission_ar[offset + 2] = 0;                             // r
                        dpixelsEmission_ar[offset + 3] = METAENGINE_ALPHA_TRANSPARENT;  // a

                        GameIsolate_.world->flowY[i] = 0;
                        GameIsolate_.world->flowX[i] = 0;
                    } else {
                        U32 color = GameIsolate_.world->tiles[i].color;
                        U32 emit = GameIsolate_.world->tiles[i].mat->emitColor;
                        // F32 br = GameIsolate_.world->light[i];
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

                            F32 newFlowX = GameIsolate_.world->prevFlowX[i] + (GameIsolate_.world->flowX[i] - GameIsolate_.world->prevFlowX[i]) * 0.25;
                            F32 newFlowY = GameIsolate_.world->prevFlowY[i] + (GameIsolate_.world->flowY[i] - GameIsolate_.world->prevFlowY[i]) * 0.25;
                            if (newFlowY < 0) newFlowY *= 0.5;

                            F64 a;
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
        // U8* dpixelsLayer2_ar = (U8*)vdpixelsLayer2_ar;
        U8 *dpixelsLayer2_ar = TexturePack_.pixelsLayer2_ar;
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
                            U32 color = ((i) % 2) == 0 ? 0x888888 : 0x444444;
                            dpixelsLayer2_ar[offset + 2] = (color >> 0) & 0xff;   // b
                            dpixelsLayer2_ar[offset + 1] = (color >> 8) & 0xff;   // g
                            dpixelsLayer2_ar[offset + 0] = (color >> 16) & 0xff;  // r
                            dpixelsLayer2_ar[offset + 3] = SDL_ALPHA_OPAQUE;      // a
                            continue;
                        } else {
                            dpixelsLayer2_ar[offset + 0] = 0;                             // b
                            dpixelsLayer2_ar[offset + 1] = 0;                             // g
                            dpixelsLayer2_ar[offset + 2] = 0;                             // r
                            dpixelsLayer2_ar[offset + 3] = METAENGINE_ALPHA_TRANSPARENT;  // a
                            continue;
                        }
                    }
                    U32 color = GameIsolate_.world->layer2[i].color;
                    dpixelsLayer2_ar[offset + 2] = (color >> 0) & 0xff;                       // b
                    dpixelsLayer2_ar[offset + 1] = (color >> 8) & 0xff;                       // g
                    dpixelsLayer2_ar[offset + 0] = (color >> 16) & 0xff;                      // r
                    dpixelsLayer2_ar[offset + 3] = GameIsolate_.world->layer2[i].mat->alpha;  // a
                }
            }
        }));

        // void* vdpixelsBackground_ar = textureBackground->data;
        // U8* dpixelsBackground_ar = (U8*)vdpixelsBackground_ar;
        U8 *dpixelsBackground_ar = TexturePack_.pixelsBackground_ar;
        results.push_back(GameIsolate_.updateDirtyPool->push([&](int id) {
            for (int i = 0; i < GameIsolate_.world->width * GameIsolate_.world->height; i++) {
                /*for (int x = 0; x < GameIsolate_.world->width; x++) {
for (int y = 0; y < GameIsolate_.world->height; y++) {*/
                // const unsigned int i = x + y * GameIsolate_.world->width;
                const unsigned int offset = i * 4;

                if (GameIsolate_.world->backgroundDirty[i]) {
                    hadBackgroundDirty = true;
                    U32 color = GameIsolate_.world->background[i];
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

        R_UpdateImageBytes(TexturePack_.textureParticles, NULL, &TexturePack_.pixelsParticles_ar[0], GameIsolate_.world->width * 4);

        if (hadDirty) memset(GameIsolate_.world->dirty, false, (size_t)GameIsolate_.world->width * GameIsolate_.world->height);
        if (hadLayer2Dirty) memset(GameIsolate_.world->layer2Dirty, false, (size_t)GameIsolate_.world->width * GameIsolate_.world->height);
        if (hadBackgroundDirty) memset(GameIsolate_.world->backgroundDirty, false, (size_t)GameIsolate_.world->width * GameIsolate_.world->height);

        if (GameIsolate_.globaldef.tick_temperature && tickTime % GameTick == 2) {
            GameIsolate_.world->tickTemperature();
        }
        if (GameIsolate_.globaldef.draw_temperature_map && tickTime % GameTick == 0) {
            renderTemperatureMap(GameIsolate_.world);
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

            global.shaderworker.waterFlowPassShader->dirty = true;
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

        if (GameIsolate_.globaldef.tick_box2d && tickTime % GameTick == 0) GameIsolate_.world->updateWorldMesh();
    }
}

void Game::tickChunkLoading() {

    // if need to load chunks
    if ((abs(accLoadX) > CHUNK_W / 2 || abs(accLoadY) > CHUNK_H / 2)) {
        while (GameIsolate_.world->WorldIsolate_.toLoad.size() > 0) {
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
                    UCH_SET_PIXEL(TexturePack_.pixels_ar, offset, 0, 0, 0, METAENGINE_ALPHA_TRANSPARENT);
                } else {
                    U32 color = GameIsolate_.world->tiles[i].color;
                    U32 emit = GameIsolate_.world->tiles[i].mat->emitColor;
                    UCH_SET_PIXEL(TexturePack_.pixels_ar, offset, (color >> 0) & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, GameIsolate_.world->tiles[i].mat->alpha);
                    UCH_SET_PIXEL(TexturePack_.pixelsEmission_ar, offset, (emit >> 0) & 0xff, (emit >> 8) & 0xff, (emit >> 16) & 0xff, (emit >> 24) & 0xff);
                }
            }

            if (GameIsolate_.world->layer2Dirty[i]) {
                if (GameIsolate_.world->layer2[i].mat->physicsType == PhysicsType::AIR) {
                    if (GameIsolate_.globaldef.draw_background_grid) {
                        U32 color = ((i) % 2) == 0 ? 0x888888 : 0x444444;
                        UCH_SET_PIXEL(TexturePack_.pixelsLayer2_ar, offset, (color >> 0) & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, SDL_ALPHA_OPAQUE);
                    } else {
                        UCH_SET_PIXEL(TexturePack_.pixelsLayer2_ar, offset, 0, 0, 0, METAENGINE_ALPHA_TRANSPARENT);
                    }
                    continue;
                }
                U32 color = GameIsolate_.world->layer2[i].color;
                UCH_SET_PIXEL(TexturePack_.pixelsLayer2_ar, offset, (color >> 0) & 0xff, (color >> 8) & 0xff, (color >> 16) & 0xff, GameIsolate_.world->layer2[i].mat->alpha);
            }

            if (GameIsolate_.world->backgroundDirty[i]) {
                U32 color = GameIsolate_.world->background[i];
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
    pixels[ofs + 3] = METAENGINE_ALPHA_TRANSPARENT

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

            global.GameData_.ofsX -= subX * scale;
            global.GameData_.ofsY -= subY * scale;
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

    if (GameIsolate_.world->WorldIsolate_.player) {
        if (Controls::PLAYER_UP->get() && !Controls::DEBUG_DRAW->get()) {
            if (GameIsolate_.world->WorldIsolate_.player->ground) {
                GameIsolate_.world->WorldIsolate_.player->vy = -4;
                global.audioEngine.PlayEvent("event:/Player/Jump");
            }
        }

        GameIsolate_.world->WorldIsolate_.player->vy += (F32)(((Controls::PLAYER_UP->get() && !Controls::DEBUG_DRAW->get()) ? (GameIsolate_.world->WorldIsolate_.player->vy > -1 ? -0.8 : -0.35) : 0) +
                                                              (Controls::PLAYER_DOWN->get() ? 0.1 : 0));
        if (Controls::PLAYER_UP->get() && !Controls::DEBUG_DRAW->get()) {
            global.audioEngine.SetEventParameter("event:/Player/Fly", "Intensity", 1);
            for (int i = 0; i < 4; i++) {
                Particle *p = new Particle(TilesCreateLava(),
                                           (F32)(GameIsolate_.world->WorldIsolate_.player->x + GameIsolate_.world->loadZone.x + GameIsolate_.world->WorldIsolate_.player->hw / 2 + rand() % 5 - 2 +
                                                 GameIsolate_.world->WorldIsolate_.player->vx),
                                           (F32)(GameIsolate_.world->WorldIsolate_.player->y + GameIsolate_.world->loadZone.y + GameIsolate_.world->WorldIsolate_.player->hh +
                                                 GameIsolate_.world->WorldIsolate_.player->vy),
                                           (F32)((rand() % 10 - 5) / 10.0f + GameIsolate_.world->WorldIsolate_.player->vx / 2.0f),
                                           (F32)((rand() % 10) / 10.0f + 1 + GameIsolate_.world->WorldIsolate_.player->vy / 2.0f), 0, (F32)0.025);
                p->temporary = true;
                p->lifetime = 120;
                GameIsolate_.world->addParticle(p);
            }
        } else {
            global.audioEngine.SetEventParameter("event:/Player/Fly", "Intensity", 0);
        }

        if (GameIsolate_.world->WorldIsolate_.player->vy > 0) {
            global.audioEngine.SetEventParameter("event:/Player/Wind", "Wind", (F32)(GameIsolate_.world->WorldIsolate_.player->vy / 12.0));
        } else {
            global.audioEngine.SetEventParameter("event:/Player/Wind", "Wind", 0);
        }

        GameIsolate_.world->WorldIsolate_.player->vx += (F32)((Controls::PLAYER_LEFT->get() ? (GameIsolate_.world->WorldIsolate_.player->vx > 0 ? -0.4 : -0.2) : 0) +
                                                              (Controls::PLAYER_RIGHT->get() ? (GameIsolate_.world->WorldIsolate_.player->vx < 0 ? 0.4 : 0.2) : 0));
        if (!Controls::PLAYER_LEFT->get() && !Controls::PLAYER_RIGHT->get()) GameIsolate_.world->WorldIsolate_.player->vx *= (F32)(GameIsolate_.world->WorldIsolate_.player->ground ? 0.85 : 0.96);
        if (GameIsolate_.world->WorldIsolate_.player->vx > 4.5) GameIsolate_.world->WorldIsolate_.player->vx = 4.5;
        if (GameIsolate_.world->WorldIsolate_.player->vx < -4.5) GameIsolate_.world->WorldIsolate_.player->vx = -4.5;
    } else {
        if (state == INGAME) {
            global.GameData_.freeCamX += (F32)((Controls::PLAYER_LEFT->get() ? -5 : 0) + (Controls::PLAYER_RIGHT->get() ? 5 : 0));
            global.GameData_.freeCamY += (F32)((Controls::PLAYER_UP->get() ? -5 : 0) + (Controls::PLAYER_DOWN->get() ? 5 : 0));
        } else {
        }
    }

    if (GameIsolate_.world->WorldIsolate_.player) {
        global.GameData_.desCamX = (F32)(-(mx - (Screen.windowWidth / 2)) / 4);
        global.GameData_.desCamY = (F32)(-(my - (Screen.windowHeight / 2)) / 4);

        GameIsolate_.world->WorldIsolate_.player->holdAngle = (F32)(atan2(global.GameData_.desCamY, global.GameData_.desCamX) * 180 / (F32)M_PI);

        global.GameData_.desCamX = 0;
        global.GameData_.desCamY = 0;
    } else {
        global.GameData_.desCamX = 0;
        global.GameData_.desCamY = 0;
    }

    if (GameIsolate_.world->WorldIsolate_.player) {
        if (GameIsolate_.world->WorldIsolate_.player->heldItem) {
            if (GameIsolate_.world->WorldIsolate_.player->heldItem->getFlag(ItemFlags::VACUUM)) {
                if (GameIsolate_.world->WorldIsolate_.player->holdVacuum) {

                    int wcx = (int)((Screen.windowWidth / 2.0f - global.GameData_.ofsX - global.GameData_.camX) / scale);
                    int wcy = (int)((Screen.windowHeight / 2.0f - global.GameData_.ofsY - global.GameData_.camY) / scale);

                    int wmx = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / scale);
                    int wmy = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / scale);

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

                        std::function<void(MaterialInstance, int, int)> makeParticle = [&](MaterialInstance tile, int xPos, int yPos) {
                            Particle *par = new Particle(tile, xPos, yPos, 0, 0, 0, (F32)0.01f);
                            par->vx = (rand() % 10 - 5) / 5.0f * 1.0f;
                            par->vy = (rand() % 10 - 5) / 5.0f * 1.0f;
                            par->ax = -par->vx / 10.0f;
                            par->ay = -par->vy / 10.0f;
                            if (par->ay == 0 && par->ax == 0) par->ay = 0.01f;

                            // par->targetX = GameIsolate_.world->WorldIsolate_.player->x + GameIsolate_.world->WorldIsolate_.player->hw / 2 + GameIsolate_.world->loadZone.x;
                            // par->targetY = GameIsolate_.world->WorldIsolate_.player->y + GameIsolate_.world->WorldIsolate_.player->hh / 2 + GameIsolate_.world->loadZone.y;
                            // par->targetForce = 0.35f;

                            par->lifetime = 6;

                            par->phase = true;

                            GameIsolate_.world->WorldIsolate_.player->heldItem->vacuumParticles.push_back(par);

                            par->killCallback = [&]() {
                                auto &v = GameIsolate_.world->WorldIsolate_.player->heldItem->vacuumParticles;
                                v.erase(std::remove(v.begin(), v.end(), par), v.end());
                            };

                            GameIsolate_.world->addParticle(par);
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
                                    makeParticle(tile, x + xx, y + yy);
                                    GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width] = Tiles_NOTHING;
                                    // GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width] = TilesCreateFire();
                                    GameIsolate_.world->dirty[(x + xx) + (y + yy) * GameIsolate_.world->width] = true;
                                }
                            }
                        }

                        auto func = [&](Particle *cur) {
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

                                            // par->targetX = GameIsolate_.world->WorldIsolate_.player->x + GameIsolate_.world->WorldIsolate_.player->hw / 2 + GameIsolate_.world->loadZone.x;
                                            // par->targetY = GameIsolate_.world->WorldIsolate_.player->y + GameIsolate_.world->WorldIsolate_.player->hh / 2 + GameIsolate_.world->loadZone.y;
                                            // par->targetForce = 0.35f;

                                            cur->lifetime = 6;

                                            cur->phase = true;

                                            GameIsolate_.world->WorldIsolate_.player->heldItem->vacuumParticles.push_back(cur);

                                            cur->killCallback = [&]() {
                                                auto &v = GameIsolate_.world->WorldIsolate_.player->heldItem->vacuumParticles;
                                                v.erase(std::remove(v.begin(), v.end(), cur), v.end());
                                            };

                                            return false;
                                        }
                                    }
                                }
                            }

                            return false;
                        };

                        GameIsolate_.world->WorldIsolate_.particles.erase(std::remove_if(GameIsolate_.world->WorldIsolate_.particles.begin(), GameIsolate_.world->WorldIsolate_.particles.end(), func),
                                                                          GameIsolate_.world->WorldIsolate_.particles.end());

                        std::vector<RigidBody *> *rbs = &GameIsolate_.world->WorldIsolate_.rigidBodies;

                        for (size_t i = 0; i < rbs->size(); i++) {
                            RigidBody *cur = (*rbs)[i];
                            if (!static_cast<bool>(cur->surface)) continue;
                            if (cur->body->IsEnabled()) {
                                F32 s = sin(-cur->body->GetAngle());
                                F32 c = cos(-cur->body->GetAngle());
                                bool upd = false;
                                for (int xx = -rad; xx <= rad; xx++) {
                                    for (int yy = -rad; yy <= rad; yy++) {
                                        if ((yy == -rad || yy == rad) && (xx == -rad || x == rad)) continue;
                                        // rotate point

                                        F32 tx = x + xx - cur->body->GetPosition().x;
                                        F32 ty = y + yy - cur->body->GetPosition().y;

                                        int ntx = (int)(tx * c - ty * s);
                                        int nty = (int)(tx * s + ty * c);

                                        if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w && nty < cur->surface->h) {
                                            U32 pixel = R_GET_PIXEL(cur->surface, ntx, nty);
                                            if (((pixel >> 24) & 0xff) != 0x00) {
                                                R_GET_PIXEL(cur->surface, ntx, nty) = 0x00000000;
                                                upd = true;

                                                makeParticle(MaterialInstance(&MaterialsList::GENERIC_SOLID, pixel), (x + xx), (y + yy));
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

                if (GameIsolate_.world->WorldIsolate_.player->heldItem->vacuumParticles.size() > 0) {
                    GameIsolate_.world->WorldIsolate_.player->heldItem->vacuumParticles.erase(
                            std::remove_if(GameIsolate_.world->WorldIsolate_.player->heldItem->vacuumParticles.begin(), GameIsolate_.world->WorldIsolate_.player->heldItem->vacuumParticles.end(),
                                           [&](Particle *cur) {
                                               if (cur->lifetime <= 0) {
                                                   cur->targetForce = 0.45f;
                                                   cur->targetX = GameIsolate_.world->WorldIsolate_.player->x + GameIsolate_.world->WorldIsolate_.player->hw / 2.0f + GameIsolate_.world->loadZone.x;
                                                   cur->targetY = GameIsolate_.world->WorldIsolate_.player->y + GameIsolate_.world->WorldIsolate_.player->hh / 2.0f + GameIsolate_.world->loadZone.y;
                                                   cur->ax = 0;
                                                   cur->ay = 0.01f;
                                               }

                                               F32 tdx = cur->targetX - cur->x;
                                               F32 tdy = cur->targetY - cur->y;

                                               if (tdx * tdx + tdy * tdy < 10 * 10) {
                                                   cur->temporary = true;
                                                   cur->lifetime = 0;
                                                   // METADOT_BUG("vacuum {}", cur->tile.mat->name.c_str());
                                                   return true;
                                               }

                                               return false;
                                           }),
                            GameIsolate_.world->WorldIsolate_.player->heldItem->vacuumParticles.end());
                }
            }
        }
    }
}

void Game::updateFrameLate() {

    if (state == LOADING) {

    } else {

        // update camera

        int nofsX;
        int nofsY;

        if (GameIsolate_.world->WorldIsolate_.player) {

            if (Time.now - Time.lastTick <= Time.mspt) {
                F32 thruTick = (F32)((Time.now - Time.lastTick) / (F64)Time.mspt);

                global.GameData_.plPosX = GameIsolate_.world->WorldIsolate_.player->x + (int)(GameIsolate_.world->WorldIsolate_.player->vx * thruTick);
                global.GameData_.plPosY = GameIsolate_.world->WorldIsolate_.player->y + (int)(GameIsolate_.world->WorldIsolate_.player->vy * thruTick);
            } else {
                // plPosX = GameIsolate_.world->WorldIsolate_.player->x;
                // plPosY = GameIsolate_.world->WorldIsolate_.player->y;
            }

            // plPosX = (F32)(plPosX + (GameIsolate_.world->WorldIsolate_.player->x - plPosX) / 25.0);
            // plPosY = (F32)(plPosY + (GameIsolate_.world->WorldIsolate_.player->y - plPosY) / 25.0);

            nofsX = (int)(-((int)global.GameData_.plPosX + GameIsolate_.world->WorldIsolate_.player->hw / 2 + GameIsolate_.world->loadZone.x) * scale + Screen.windowWidth / 2);
            nofsY = (int)(-((int)global.GameData_.plPosY + GameIsolate_.world->WorldIsolate_.player->hh / 2 + GameIsolate_.world->loadZone.y) * scale + Screen.windowHeight / 2);
        } else {
            global.GameData_.plPosX = (F32)(global.GameData_.plPosX + (global.GameData_.freeCamX - global.GameData_.plPosX) / 50.0f);
            global.GameData_.plPosY = (F32)(global.GameData_.plPosY + (global.GameData_.freeCamY - global.GameData_.plPosY) / 50.0f);

            nofsX = (int)(-(global.GameData_.plPosX + 0 + GameIsolate_.world->loadZone.x) * scale + Screen.windowWidth / 2.0f);
            nofsY = (int)(-(global.GameData_.plPosY + 0 + GameIsolate_.world->loadZone.y) * scale + Screen.windowHeight / 2.0f);
        }

        accLoadX += (nofsX - global.GameData_.ofsX) / (F32)scale;
        accLoadY += (nofsY - global.GameData_.ofsY) / (F32)scale;
        // METADOT_BUG("{0:f} {0:f}", plPosX, plPosY);
        // METADOT_BUG("a {0:d} {0:d}", nofsX, nofsY);
        // METADOT_BUG("{0:d} {0:d}", nofsX - ofsX, nofsY - ofsY);
        global.GameData_.ofsX += (nofsX - global.GameData_.ofsX);
        global.GameData_.ofsY += (nofsY - global.GameData_.ofsY);

        global.GameData_.camX = (F32)(global.GameData_.camX + (global.GameData_.desCamX - global.GameData_.camX) * (Time.now - Time.lastTime) / 250.0f);
        global.GameData_.camY = (F32)(global.GameData_.camY + (global.GameData_.desCamY - global.GameData_.camY) * (Time.now - Time.lastTime) / 250.0f);
    }
}

void Game::renderEarly() {

    UIRendererPostUpdate();

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
                        /*if (x >= drop - 1 && x <= drop + 1) {
newState = true;
}else if (x >= drop2 - 1 && x <= drop2 + 1) {
newState = true;
}*/
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
                U32 tmp = loadingOnColor;
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

        FontCache_DrawAlign(font, Render.target, Screen.windowWidth / 2, Screen.windowHeight / 2 - 32, FontCache_ALIGN_CENTER, "Loading...");

    } else {
        // render entities with LERP

        if (Time.now - Time.lastTick <= Time.mspt) {
            R_Clear(TexturePack_.textureEntities->target);
            R_Clear(TexturePack_.textureEntitiesLQ->target);
            if (GameIsolate_.world->WorldIsolate_.player) {
                F32 thruTick = (F32)((Time.now - Time.lastTick) / (F64)Time.mspt);

                R_SetBlendMode(TexturePack_.textureEntities, R_BLEND_ADD);
                R_SetBlendMode(TexturePack_.textureEntitiesLQ, R_BLEND_ADD);
                int scaleEnt = GameIsolate_.globaldef.hd_objects ? GameIsolate_.globaldef.hd_objects_size : 1;

                for (auto &v : GameIsolate_.world->WorldIsolate_.entities) {
                    ((Player *)v)->renderLQ(TexturePack_.textureEntitiesLQ->target, GameIsolate_.world->loadZone.x + (int)(v->vx * thruTick), GameIsolate_.world->loadZone.y + (int)(v->vy * thruTick));
                    ((Player *)v)->render(TexturePack_.textureEntities->target, GameIsolate_.world->loadZone.x + (int)(v->vx * thruTick), GameIsolate_.world->loadZone.y + (int)(v->vy * thruTick));
                }

                if (GameIsolate_.world->WorldIsolate_.player && GameIsolate_.world->WorldIsolate_.player->heldItem != NULL) {
                    if (GameIsolate_.world->WorldIsolate_.player->heldItem->getFlag(ItemFlags::HAMMER)) {
                        if (GameIsolate_.world->WorldIsolate_.player->holdHammer) {
                            int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / scale);
                            int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / scale);

                            int dx = x - GameIsolate_.world->WorldIsolate_.player->hammerX;
                            int dy = y - GameIsolate_.world->WorldIsolate_.player->hammerY;
                            F32 len = sqrt(dx * dx + dy * dy);
                            if (len > 40) {
                                dx = dx / len * 40;
                                dy = dy / len * 40;
                            }

                            R_Line(TexturePack_.textureEntitiesLQ->target, GameIsolate_.world->WorldIsolate_.player->hammerX + dx, GameIsolate_.world->WorldIsolate_.player->hammerY + dy,
                                   GameIsolate_.world->WorldIsolate_.player->hammerX, GameIsolate_.world->WorldIsolate_.player->hammerY, {0xff, 0xff, 0x00, 0xff});
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
                R_SetBlendMode(TexturePack_.textureEntities, R_BLEND_NORMAL);
                R_SetBlendMode(TexturePack_.textureEntitiesLQ, R_BLEND_NORMAL);
            }
        }

        if (Controls::mmouse) {
            int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / scale);
            int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / scale);
            R_RectangleFilled(TexturePack_.textureEntitiesLQ->target, x - GameUI::DebugDrawUI::brushSize / 2.0f, y - GameUI::DebugDrawUI::brushSize / 2.0f,
                              x + (int)(ceil(GameUI::DebugDrawUI::brushSize / 2.0)), y + (int)(ceil(GameUI::DebugDrawUI::brushSize / 2.0)), {0xff, 0x40, 0x40, 0x90});
            R_Rectangle(TexturePack_.textureEntitiesLQ->target, x - GameUI::DebugDrawUI::brushSize / 2.0f, y - GameUI::DebugDrawUI::brushSize / 2.0f,
                        x + (int)(ceil(GameUI::DebugDrawUI::brushSize / 2.0)) + 1, y + (int)(ceil(GameUI::DebugDrawUI::brushSize / 2.0)) + 1, {0xff, 0x40, 0x40, 0xE0});
        } else if (Controls::DEBUG_DRAW->get()) {
            int x = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / scale);
            int y = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / scale);
            R_RectangleFilled(TexturePack_.textureEntitiesLQ->target, x - GameUI::DebugDrawUI::brushSize / 2.0f, y - GameUI::DebugDrawUI::brushSize / 2.0f,
                              x + (int)(ceil(GameUI::DebugDrawUI::brushSize / 2.0)), y + (int)(ceil(GameUI::DebugDrawUI::brushSize / 2.0)), {0x00, 0xff, 0xB0, 0x80});
            R_Rectangle(TexturePack_.textureEntitiesLQ->target, x - GameUI::DebugDrawUI::brushSize / 2.0f, y - GameUI::DebugDrawUI::brushSize / 2.0f,
                        x + (int)(ceil(GameUI::DebugDrawUI::brushSize / 2.0)) + 1, y + (int)(ceil(GameUI::DebugDrawUI::brushSize / 2.0)) + 1, {0x00, 0xff, 0xB0, 0xE0});
        }
    }
}

void Game::renderLate() {

    Render.target = TexturePack_.backgroundImage->target;
    R_Clear(Render.target);

    if (state == LOADING) {

    } else {
        // draw backgrounds

        Background *bg = GameIsolate_.backgrounds->Get("TEST_OVERWORLD");
        if (GameIsolate_.globaldef.draw_background && scale <= bg->layers[0].surface.size() && GameIsolate_.world->loadZone.y > -5 * CHUNK_H) {
            R_SetShapeBlendMode(R_BLEND_SET);
            METAENGINE_Color col = {static_cast<U8>((bg->solid >> 16) & 0xff), static_cast<U8>((bg->solid >> 8) & 0xff), static_cast<U8>((bg->solid >> 0) & 0xff), 0xff};
            R_ClearColor(Render.target, col);

            R_Rect *dst = nullptr;
            R_Rect *src = nullptr;
            METADOT_NEW(C, dst, R_Rect);
            METADOT_NEW(C, src, R_Rect);

            F32 arX = (F32)Screen.windowWidth / (bg->layers[0].surface[0]->w);
            F32 arY = (F32)Screen.windowHeight / (bg->layers[0].surface[0]->h);

            F64 time = Time::millis() / 1000.0;

            R_SetShapeBlendMode(R_BLEND_NORMAL);

            for (size_t i = 0; i < bg->layers.size(); i++) {
                BackgroundLayer cur = bg->layers[i];

                C_Surface *texture = cur.surface[(size_t)scale - 1];

                R_Image *tex = cur.texture[(size_t)scale - 1];
                R_SetBlendMode(tex, R_BLEND_NORMAL);

                int tw = texture->w;
                int th = texture->h;

                int iter = (int)ceil((F32)Screen.windowWidth / (tw)) + 1;
                for (int n = 0; n < iter; n++) {

                    src->x = 0;
                    src->y = 0;
                    src->w = tw;
                    src->h = th;

                    dst->x = (((global.GameData_.ofsX + global.GameData_.camX) + GameIsolate_.world->loadZone.x * scale) + n * tw / cur.parralaxX) * cur.parralaxX +
                             GameIsolate_.world->width / 2.0f * scale - tw / 2.0f;
                    dst->y = ((global.GameData_.ofsY + global.GameData_.camY) + GameIsolate_.world->loadZone.y * scale) * cur.parralaxY + GameIsolate_.world->height / 2.0f * scale - th / 2.0f -
                             Screen.windowHeight / 3.0f * (scale - 1);
                    dst->w = (F32)tw;
                    dst->h = (F32)th;

                    dst->x += (F32)(scale * fmod(cur.moveX * time, tw));

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

            METADOT_DELETE(C, dst, R_Rect);
            METADOT_DELETE(C, src, R_Rect);
        }

        R_Rect r1 = R_Rect{(F32)(global.GameData_.ofsX + global.GameData_.camX), (F32)(global.GameData_.ofsY + global.GameData_.camY), (F32)(GameIsolate_.world->width * scale),
                           (F32)(GameIsolate_.world->height * scale)};
        R_SetBlendMode(TexturePack_.textureBackground, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureBackground, NULL, Render.target, &r1);

        R_SetBlendMode(TexturePack_.textureLayer2, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureLayer2, NULL, Render.target, &r1);

        R_SetBlendMode(TexturePack_.textureObjectsBack, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureObjectsBack, NULL, Render.target, &r1);

        // shader

        if (GameIsolate_.globaldef.draw_shaders) {

            if (global.shaderworker.waterFlowPassShader->dirty && GameIsolate_.globaldef.water_showFlow) {

                ShaderActivate(&global.shaderworker.waterFlowPassShader->sb);
                WaterFlowPassShader__update(global.shaderworker.waterFlowPassShader, GameIsolate_.world->width, GameIsolate_.world->height);
                R_SetBlendMode(TexturePack_.textureFlow, R_BLEND_SET);
                R_BlitRect(TexturePack_.textureFlow, NULL, TexturePack_.textureFlowSpead->target, NULL);

                global.shaderworker.waterFlowPassShader->dirty = false;
            }

            ShaderActivate(&global.shaderworker.waterShader->sb);
            F32 t = (Time.now - Time.startTime) / 1000.0;
            WaterShader__update(global.shaderworker.waterShader, t, Render.target->w * scale, Render.target->h * scale, TexturePack_.texture, r1.x, r1.y, r1.w, r1.h, scale,
                                TexturePack_.textureFlowSpead, GameIsolate_.globaldef.water_overlay, GameIsolate_.globaldef.water_showFlow, GameIsolate_.globaldef.water_pixelated);
        }

        Render.target = Render.realTarget;

        R_BlitRect(TexturePack_.backgroundImage, NULL, Render.target, NULL);

        R_SetBlendMode(TexturePack_.texture, R_BLEND_NORMAL);
        R_ActivateShaderProgram(0, NULL);

        // done shader

        int lmsx = (int)((mx - global.GameData_.ofsX - global.GameData_.camX) / scale);
        int lmsy = (int)((my - global.GameData_.ofsY - global.GameData_.camY) / scale);

        R_Clear(TexturePack_.worldTexture->target);

        R_BlitRect(TexturePack_.texture, NULL, TexturePack_.worldTexture->target, NULL);

        R_SetBlendMode(TexturePack_.textureObjects, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureObjects, NULL, TexturePack_.worldTexture->target, NULL);
        R_SetBlendMode(TexturePack_.textureObjectsLQ, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureObjectsLQ, NULL, TexturePack_.worldTexture->target, NULL);

        R_SetBlendMode(TexturePack_.textureParticles, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureParticles, NULL, TexturePack_.worldTexture->target, NULL);

        R_SetBlendMode(TexturePack_.textureEntitiesLQ, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureEntitiesLQ, NULL, TexturePack_.worldTexture->target, NULL);
        R_SetBlendMode(TexturePack_.textureEntities, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.textureEntities, NULL, TexturePack_.worldTexture->target, NULL);

        if (GameIsolate_.globaldef.draw_shaders) ShaderActivate(&global.shaderworker.newLightingShader->sb);

        // I use this to only rerender the lighting when a parameter changes or N times per second anyway
        // Doing this massively reduces the GPU load of the shader
        bool needToRerenderLighting = false;

        static long long lastLightingForceRefresh = 0;
        long long now = Time::millis();
        if (now - lastLightingForceRefresh > 100) {
            lastLightingForceRefresh = now;
            needToRerenderLighting = true;
        }

        if (GameIsolate_.globaldef.draw_shaders && GameIsolate_.world) {
            F32 lightTx;
            F32 lightTy;

            if (GameIsolate_.world->WorldIsolate_.player) {
                lightTx = (GameIsolate_.world->loadZone.x + GameIsolate_.world->WorldIsolate_.player->x + GameIsolate_.world->WorldIsolate_.player->hw / 2.0f) / (F32)GameIsolate_.world->width;
                lightTy = (GameIsolate_.world->loadZone.y + GameIsolate_.world->WorldIsolate_.player->y + GameIsolate_.world->WorldIsolate_.player->hh / 2.0f) / (F32)GameIsolate_.world->height;
            } else {
                lightTx = lmsx / (F32)GameIsolate_.world->width;
                lightTy = lmsy / (F32)GameIsolate_.world->height;
            }

            if (global.shaderworker.newLightingShader->lastLx != lightTx || global.shaderworker.newLightingShader->lastLy != lightTy) needToRerenderLighting = true;
            NewLightingShader__update(global.shaderworker.newLightingShader, TexturePack_.worldTexture, TexturePack_.emissionTexture, lightTx, lightTy);
            if (global.shaderworker.newLightingShader->lastQuality != GameIsolate_.globaldef.lightingQuality) {
                needToRerenderLighting = true;
            }
            NewLightingShader__setQuality(global.shaderworker.newLightingShader, GameIsolate_.globaldef.lightingQuality);

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

            global.shaderworker.newLightingShader_insideDes = std::min(std::max(0.0f, (F32)nBg / ((range * 2) * (range * 2))), 1.0f);
            global.shaderworker.newLightingShader_insideCur += (global.shaderworker.newLightingShader_insideDes - global.shaderworker.newLightingShader_insideCur) / 2.0f * (Time.deltaTime / 1000.0f);

            F32 ins = global.shaderworker.newLightingShader_insideCur < 0.05 ? 0.0 : global.shaderworker.newLightingShader_insideCur;
            if (global.shaderworker.newLightingShader->lastInside != ins) needToRerenderLighting = true;
            NewLightingShader__setInside(global.shaderworker.newLightingShader, ins);
            NewLightingShader__setBounds(global.shaderworker.newLightingShader, GameIsolate_.world->tickZone.x * GameIsolate_.globaldef.hd_objects_size,
                                         GameIsolate_.world->tickZone.y * GameIsolate_.globaldef.hd_objects_size,
                                         (GameIsolate_.world->tickZone.x + GameIsolate_.world->tickZone.w) * GameIsolate_.globaldef.hd_objects_size,
                                         (GameIsolate_.world->tickZone.y + GameIsolate_.world->tickZone.h) * GameIsolate_.globaldef.hd_objects_size);

            if (global.shaderworker.newLightingShader->lastSimpleMode != GameIsolate_.globaldef.simpleLighting) needToRerenderLighting = true;
            NewLightingShader__setSimpleMode(global.shaderworker.newLightingShader, GameIsolate_.globaldef.simpleLighting);

            if (global.shaderworker.newLightingShader->lastEmissionEnabled != GameIsolate_.globaldef.lightingEmission) needToRerenderLighting = true;
            NewLightingShader__setEmissionEnabled(global.shaderworker.newLightingShader, GameIsolate_.globaldef.lightingEmission);

            if (global.shaderworker.newLightingShader->lastDitheringEnabled != GameIsolate_.globaldef.lightingDithering) needToRerenderLighting = true;
            NewLightingShader__setDitheringEnabled(global.shaderworker.newLightingShader, GameIsolate_.globaldef.lightingDithering);
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

            ShaderActivate(&global.shaderworker.fireShader->sb);
            FireShader__update(global.shaderworker.fireShader, TexturePack_.textureFire);
            R_BlitRect(TexturePack_.textureFire, NULL, TexturePack_.texture2Fire->target, NULL);
            R_ActivateShaderProgram(0, NULL);

            ShaderActivate(&global.shaderworker.fire2Shader->sb);
            Fire2Shader__update(global.shaderworker.fire2Shader, TexturePack_.texture2Fire);
            R_BlitRect(TexturePack_.texture2Fire, NULL, Render.target, &r1);
            R_ActivateShaderProgram(0, NULL);
        }

        // done light

        renderOverlays();
    }
}

void Game::renderOverlays() {

    FontCache_DrawAlign(font, Render.target, Screen.windowWidth, 0, FontCache_ALIGN_RIGHT, "%.1f ms/frame (%.1f(%d) FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate,
                        Time.feelsLikeFps);

    R_Rect r1 = R_Rect{(F32)(global.GameData_.ofsX + global.GameData_.camX), (F32)(global.GameData_.ofsY + global.GameData_.camY), (F32)(GameIsolate_.world->width * scale),
                       (F32)(GameIsolate_.world->height * scale)};
    R_Rect r2 = R_Rect{(F32)(global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->tickZone.x * scale),
                       (F32)(global.GameData_.ofsY + global.GameData_.camY + GameIsolate_.world->tickZone.y * scale), (F32)(GameIsolate_.world->tickZone.w * scale),
                       (F32)(GameIsolate_.world->tickZone.h * scale)};

    if (GameIsolate_.globaldef.draw_temperature_map) {
        R_SetBlendMode(TexturePack_.temperatureMap, R_BLEND_NORMAL);
        R_BlitRect(TexturePack_.temperatureMap, NULL, Render.target, &r1);
    }

    if (GameIsolate_.globaldef.draw_load_zones) {
        R_Rect r2m = R_Rect{(F32)(global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->meshZone.x * scale),
                            (F32)(global.GameData_.ofsY + global.GameData_.camY + GameIsolate_.world->meshZone.y * scale), (F32)(GameIsolate_.world->meshZone.w * scale),
                            (F32)(GameIsolate_.world->meshZone.h * scale)};

        R_Rectangle2(Render.target, r2m, {0x00, 0xff, 0xff, 0xff});
        R_Rectangle2(Render.target, r2, {0xff, 0x00, 0x00, 0xff});
    }

    if (GameIsolate_.globaldef.draw_load_zones) {

        METAENGINE_Color col = {0xff, 0x00, 0x00, 0x20};
        R_SetShapeBlendMode(R_BLEND_NORMAL);

        R_Rect r3 = R_Rect{(F32)(0), (F32)(0), (F32)((global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->tickZone.x * scale)), (F32)(Screen.windowHeight)};
        R_Rectangle2(Render.target, r3, col);

        R_Rect r4 = R_Rect{(F32)(global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->tickZone.x * scale + GameIsolate_.world->tickZone.w * scale), (F32)(0),
                           (F32)((Screen.windowWidth) - (global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->tickZone.x * scale + GameIsolate_.world->tickZone.w * scale)),
                           (F32)(Screen.windowHeight)};
        R_Rectangle2(Render.target, r3, col);

        R_Rect r5 = R_Rect{(F32)(global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->tickZone.x * scale), (F32)(0), (F32)(GameIsolate_.world->tickZone.w * scale),
                           (F32)(global.GameData_.ofsY + global.GameData_.camY + GameIsolate_.world->tickZone.y * scale)};
        R_Rectangle2(Render.target, r3, col);

        R_Rect r6 = R_Rect{(F32)(global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->tickZone.x * scale),
                           (F32)(global.GameData_.ofsY + global.GameData_.camY + GameIsolate_.world->tickZone.y * scale + GameIsolate_.world->tickZone.h * scale),
                           (F32)(GameIsolate_.world->tickZone.w * scale),
                           (F32)(Screen.windowHeight - (global.GameData_.ofsY + global.GameData_.camY + GameIsolate_.world->tickZone.y * scale + GameIsolate_.world->tickZone.h * scale))};
        R_Rectangle2(Render.target, r6, col);

        col = {0x00, 0xff, 0x00, 0xff};
        R_Rect r7 = R_Rect{(F32)(global.GameData_.ofsX + global.GameData_.camX + GameIsolate_.world->width / 2 * scale - (Screen.windowWidth / 3 * scale / 2)),
                           (F32)(global.GameData_.ofsY + global.GameData_.camY + GameIsolate_.world->height / 2 * scale - (Screen.windowHeight / 3 * scale / 2)), (F32)(Screen.windowWidth / 3 * scale),
                           (F32)(Screen.windowHeight / 3 * scale)};
        R_Rectangle2(Render.target, r7, col);
    }

    if (GameIsolate_.globaldef.draw_physics_debug) {
        //
        // for(size_t i = 0; i < GameIsolate_.world->WorldIsolate_.rigidBodies.size(); i++) {
        //    RigidBody cur = *GameIsolate_.world->WorldIsolate_.rigidBodies[i];

        //    F32 x = cur.body->GetPosition().x;
        //    F32 y = cur.body->GetPosition().y;
        //    x = ((x)*scale + ofsX + camX);
        //    y = ((y)*scale + ofsY + camY);

        //    /*SDL_Rect* r = new SDL_Rect{ (int)x, (int)y, cur.surface->w * scale, cur.surface->h * scale };
        //    SDL_RenderCopyEx(renderer, cur.texture, NULL, r, cur.body->GetAngle() * 180 / M_PI, new SDL_Point{ 0, 0 }, SDL_RendererFlip::SDL_FLIP_NONE);
        //    delete r;*/

        //    U32 color = 0x0000ff;

        //    SDL_Color col = {(color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff};

        //    b2Fixture* fix = cur.body->GetFixtureList();
        //    while(fix) {
        //        b2Shape* shape = fix->GetShape();

        //        switch(shape->GetType()) {
        //        case b2Shape::Type::e_polygon:
        //            b2PolygonShape* poly = (b2PolygonShape*)shape;
        //            b2Vec2* verts = poly->m_vertices;

        //            Drawing::drawPolygon(target, col, verts, (int)x, (int)y, scale, poly->m_count, cur.body->GetAngle()/* + fmod((Time::millis() / 1000.0), 360)*/, 0, 0);

        //            break;
        //        }

        //        fix = fix->GetNext();
        //    }
        //}

        // if(GameIsolate_.world->WorldIsolate_.player) {
        //     RigidBody cur = *GameIsolate_.world->WorldIsolate_.player->rb;

        //    F32 x = cur.body->GetPosition().x;
        //    F32 y = cur.body->GetPosition().y;
        //    x = ((x)*scale + ofsX + camX);
        //    y = ((y)*scale + ofsY + camY);

        //    U32 color = 0x0000ff;

        //    SDL_Color col = {(color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff};

        //    b2Fixture* fix = cur.body->GetFixtureList();
        //    while(fix) {
        //        b2Shape* shape = fix->GetShape();
        //        switch(shape->GetType()) {
        //        case b2Shape::Type::e_polygon:
        //            b2PolygonShape* poly = (b2PolygonShape*)shape;
        //            b2Vec2* verts = poly->m_vertices;

        //            Drawing::drawPolygon(target, col, verts, (int)x, (int)y, scale, poly->m_count, cur.body->GetAngle()/* + fmod((Time::millis() / 1000.0), 360)*/, 0, 0);

        //            break;
        //        }

        //        fix = fix->GetNext();
        //    }
        //}

        // for(size_t i = 0; i < GameIsolate_.world->WorldIsolate_.worldRigidBodies.size(); i++) {
        //     RigidBody cur = *GameIsolate_.world->WorldIsolate_.worldRigidBodies[i];

        //    F32 x = cur.body->GetPosition().x;
        //    F32 y = cur.body->GetPosition().y;
        //    x = ((x)*scale + ofsX + camX);
        //    y = ((y)*scale + ofsY + camY);

        //    U32 color = 0x00ff00;

        //    SDL_Color col = {(color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff};

        //    b2Fixture* fix = cur.body->GetFixtureList();
        //    while(fix) {
        //        b2Shape* shape = fix->GetShape();
        //        switch(shape->GetType()) {
        //        case b2Shape::Type::e_polygon:
        //            b2PolygonShape* poly = (b2PolygonShape*)shape;
        //            b2Vec2* verts = poly->m_vertices;

        //            Drawing::drawPolygon(target, col, verts, (int)x, (int)y, scale, poly->m_count, cur.body->GetAngle()/* + fmod((Time::millis() / 1000.0), 360)*/, 0, 0);

        //            break;
        //        }

        //        fix = fix->GetNext();
        //    }

        //    /*color = 0xff0000;
        //    SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff);
        //    SDL_RenderDrawPoint(renderer, x, y);
        //    color = 0x00ff00;
        //    SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xff, (color >> 8) & 0xff, (color >> 0) & 0xff, 0xff);
        //    SDL_RenderDrawPoint(renderer, cur.body->GetLocalCenter().x * scale + ofsX, cur.body->GetLocalCenter().y * scale + ofsY);*/
        //}

        int minChX = (int)floor((GameIsolate_.world->meshZone.x - GameIsolate_.world->loadZone.x) / CHUNK_W);
        int minChY = (int)floor((GameIsolate_.world->meshZone.y - GameIsolate_.world->loadZone.y) / CHUNK_H);
        int maxChX = (int)ceil((GameIsolate_.world->meshZone.x + GameIsolate_.world->meshZone.w - GameIsolate_.world->loadZone.x) / CHUNK_W);
        int maxChY = (int)ceil((GameIsolate_.world->meshZone.y + GameIsolate_.world->meshZone.h - GameIsolate_.world->loadZone.y) / CHUNK_H);

        for (int cx = minChX; cx <= maxChX; cx++) {
            for (int cy = minChY; cy <= maxChY; cy++) {
                Chunk *ch = GameIsolate_.world->getChunk(cx, cy);
                SDL_Color col = {255, 0, 0, 255};

                F32 x = ((ch->x * CHUNK_W + GameIsolate_.world->loadZone.x) * scale + global.GameData_.ofsX + global.GameData_.camX);
                F32 y = ((ch->y * CHUNK_H + GameIsolate_.world->loadZone.y) * scale + global.GameData_.ofsY + global.GameData_.camY);

                R_Rectangle(Render.target, x, y, x + CHUNK_W * scale, y + CHUNK_H * scale, {50, 50, 0, 255});

                // for(int i = 0; i < ch->polys.size(); i++) {
                //     Drawing::drawPolygon(target, col, ch->polys[i].m_vertices, (int)x, (int)y, scale, ch->polys[i].m_count, 0/* + fmod((Time::millis() / 1000.0), 360)*/, 0, 0);
                // }
            }
        }

        //

        GameIsolate_.world->b2world->SetDebugDraw(debugDraw);
        debugDraw->scale = scale;
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
    //                   MetaEngine::Format("{} FPS\n Feels Like: {} FPS", Time.fps,
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
        int pchxf = (int)(((F32)pposX / CHUNK_W) * chSize);
        int pchyf = (int)(((F32)pposY / CHUNK_H) * chSize);

        R_Rectangle(Render.target, centerX - chSize * CHUNK_UNLOAD_DIST + chSize, centerY - chSize * CHUNK_UNLOAD_DIST + chSize, centerX + chSize * CHUNK_UNLOAD_DIST + chSize,
                    centerY + chSize * CHUNK_UNLOAD_DIST + chSize, {0xcc, 0xcc, 0xcc, 0xff});

        R_Rect r = {0, 0, (F32)chSize, (F32)chSize};
        for (auto &p : GameIsolate_.world->WorldIsolate_.chunkCache) {
            if (p.first == INT_MIN) continue;
            int cx = p.first;
            for (auto &p2 : p.second) {
                if (p2.first == INT_MIN) continue;
                int cy = p2.first;
                Chunk *m = p2.second;
                r.x = centerX + m->x * chSize - pchx;
                r.y = centerY + m->y * chSize - pchy;
                METAENGINE_Color col;
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

        int loadx = (int)(((F32)-GameIsolate_.world->loadZone.x / CHUNK_W) * chSize);
        int loady = (int)(((F32)-GameIsolate_.world->loadZone.y / CHUNK_H) * chSize);

        int loadx2 = (int)(((F32)(-GameIsolate_.world->loadZone.x + GameIsolate_.world->loadZone.w) / CHUNK_W) * chSize);
        int loady2 = (int)(((F32)(-GameIsolate_.world->loadZone.y + GameIsolate_.world->loadZone.h) / CHUNK_H) * chSize);
        R_Rectangle(Render.target, centerX - pchx + loadx, centerY - pchy + loady, centerX - pchx + loadx2, centerY - pchy + loady2, {0x00, 0xff, 0xff, 0xff});

        R_Rectangle(Render.target, centerX - pchx + pchxf, centerY - pchy + pchyf, centerX + 1 - pchx + pchxf, centerY + 1 - pchy + pchyf, {0x00, 0xff, 0x00, 0xff});
    }

    if (GameIsolate_.globaldef.draw_debug_stats) {

        int rbCt = 0;
        for (auto &r : GameIsolate_.world->WorldIsolate_.rigidBodies) {
            if (r->body->IsEnabled()) rbCt++;
        }

        int rbTriACt = 0;
        int rbTriCt = 0;
        for (size_t i = 0; i < GameIsolate_.world->WorldIsolate_.rigidBodies.size(); i++) {
            RigidBody cur = *GameIsolate_.world->WorldIsolate_.rigidBodies[i];

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
        for (auto &p : GameIsolate_.world->WorldIsolate_.chunkCache) {
            if (p.first == INT_MIN) continue;
            int cx = p.first;
            for (auto &p2 : p.second) {
                if (p2.first == INT_MIN) continue;
                chCt++;
            }
        }

        const char *buffAsStdStr1 = R"(
{0} {1}
XY: {2:.2f} / {3:.2f}
V: {4:.2f} / {5:.2f}
Particles: {6}
Entities: {7}
RigidBodies: {8}/{9} O, {10} W
Tris: {11}/{12} O, {13} W
Cached Chunks: {14}
ReadyToReadyToMerge ({15})
ReadyToMerge ({16})
)";

        // Drawing::drawText(
        //         "info",
        //         MetaEngine::Format(buffAsStdStr1, win_title_client, METADOT_VERSION_TEXT, GameData_.plPosX,
        //                     GameData_.plPosY,
        //                     GameIsolate_.world->WorldIsolate_.player
        //                             ? GameIsolate_.world->WorldIsolate_.player->vx
        //                             : 0.0f,
        //                     GameIsolate_.world->WorldIsolate_.player
        //                             ? GameIsolate_.world->WorldIsolate_.player->vy
        //                             : 0.0f,
        //                     (int) GameIsolate_.world->WorldIsolate_.particles.size(),
        //                     (int) GameIsolate_.world->WorldIsolate_.entities.size(), rbCt,
        //                     (int) GameIsolate_.world->WorldIsolate_.rigidBodies.size(),
        //                     (int) GameIsolate_.world->WorldIsolate_.worldRigidBodies.size(),
        //                     rbTriACt, rbTriCt, rbTriWCt, chCt,
        //                     (int) GameIsolate_.world->WorldIsolate_.readyToReadyToMerge.size(),
        //                     (int) GameIsolate_.world->WorldIsolate_.readyToMerge.size()),
        //         4, 12);

        auto a = MetaEngine::Format(buffAsStdStr1, win_title_client, METADOT_VERSION_TEXT, global.GameData_.plPosX, global.GameData_.plPosY,
                                    GameIsolate_.world->WorldIsolate_.player ? GameIsolate_.world->WorldIsolate_.player->vx : 0.0f,
                                    GameIsolate_.world->WorldIsolate_.player ? GameIsolate_.world->WorldIsolate_.player->vy : 0.0f, (int)GameIsolate_.world->WorldIsolate_.particles.size(),
                                    (int)GameIsolate_.world->WorldIsolate_.entities.size(), rbCt, (int)GameIsolate_.world->WorldIsolate_.rigidBodies.size(),
                                    (int)GameIsolate_.world->WorldIsolate_.worldRigidBodies.size(), rbTriACt, rbTriCt, rbTriWCt, chCt,
                                    (int)GameIsolate_.world->WorldIsolate_.readyToReadyToMerge.size(), (int)GameIsolate_.world->WorldIsolate_.readyToMerge.size());

        FontCache_DrawAlign(font, Render.target, 10, 0, FontCache_ALIGN_LEFT, "%s", a.c_str());

        // for (size_t i = 0; i < GameIsolate_.world->readyToReadyToMerge.size(); i++) {
        //     char buff[10];
        //     snprintf(buff, sizeof(buff), "    #%d", (int) i);
        //     std::string buffAsStdStr = buff;
        //     Drawing::drawTextBG(Render.target, buffAsStdStr.c_str(), font16, 4,
        //                         2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff,
        //                         {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);
        // }

        // for (size_t i = 0; i < GameIsolate_.world->WorldIsolate_.readyToMerge.size(); i++) {
        //     char buff[20];
        //     snprintf(buff, sizeof(buff), "    #%d (%d, %d)", (int) i,
        //              GameIsolate_.world->WorldIsolate_.readyToMerge[i]->x,
        //              GameIsolate_.world->WorldIsolate_.readyToMerge[i]->y);
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
            R_Line(Render.target, Screen.windowWidth - 30 - FrameTimeNum - 5, Screen.windowHeight - 10 - (i * 25), Screen.windowWidth - 25, Screen.windowHeight - 10 - (i * 25),
                   {0xff, 0xff, 0xff, 0xff});
        }
        /*for (int i = 0; i <= 100; i += 25) {
char buff[20];
snprintf(buff, sizeof(buff), "%d", i);
std::string buffAsStdStr = buff;
Drawing::drawText(renderer, buffAsStdStr.c_str(), font14, WIDTH - 20, HEIGHT - 15 - i - 2, 0xff, 0xff, 0xff, ALIGN_LEFT);
SDL_RenderDrawLine(renderer, WIDTH - 30 - FrameTimeNum - 5, HEIGHT - 10 - i, WIDTH - 25, HEIGHT - 10 - i);
}*/

        for (int i = 0; i < FrameTimeNum; i++) {
            int h = frameTimes[i];

            METAENGINE_Color col;
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

            R_Line(Render.target, Screen.windowWidth - FrameTimeNum - 30 + i, Screen.windowHeight - 10 - h, Screen.windowWidth - FrameTimeNum - 30 + i, Screen.windowHeight - 10, col);
            // SDL_RenderDrawLine(renderer, WIDTH - FrameTimeNum - 30 + i, HEIGHT - 10 - h, WIDTH - FrameTimeNum - 30 + i, HEIGHT - 10);
        }

        R_Line(Render.target, Screen.windowWidth - 30 - FrameTimeNum - 5, Screen.windowHeight - 10 - (int)(1000.0 / Time.fps), Screen.windowWidth - 25,
               Screen.windowHeight - 10 - (int)(1000.0 / Time.fps), {0x00, 0xff, 0xff, 0xff});
        R_Line(Render.target, Screen.windowWidth - 30 - FrameTimeNum - 5, Screen.windowHeight - 10 - (int)(1000.0 / Time.feelsLikeFps), Screen.windowWidth - 25,
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
            I32 temp = t.temperature;
            U32 color = (U8)((temp + 1024) / 2048.0f * 255);

            const unsigned int offset = (GameIsolate_.world->width * 4 * y) + x * 4;
            TexturePack_.pixelsTemp_ar[offset + 0] = color;  // b
            TexturePack_.pixelsTemp_ar[offset + 1] = color;  // g
            TexturePack_.pixelsTemp_ar[offset + 2] = color;  // r
            TexturePack_.pixelsTemp_ar[offset + 3] = 0xf0;   // a
        }
    }
}

void Game::resolu(int newWidth, int newHeight) {

    int prevWidth = Screen.windowWidth;
    int prevHeight = Screen.windowHeight;

    Screen.windowWidth = newWidth;
    Screen.windowHeight = newHeight;

    global.game->createTexture();

    global.game->accLoadX -= (newWidth - prevWidth) / 2.0f / global.game->scale;
    global.game->accLoadY -= (newHeight - prevHeight) / 2.0f / global.game->scale;

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

    F32 len = sqrtf(dcx * dcx + dcy * dcy);
    F32 udx = dcx / len;
    F32 udy = dcy / len;

    int mmx = Screen.windowWidth / 2.0f + udx * dist;
    int mmy = Screen.windowHeight / 2.0f + udy * dist;

    int wcx = (int)((Screen.windowWidth / 2.0f - global.GameData_.ofsX - global.GameData_.camX) / scale);
    int wcy = (int)((Screen.windowHeight / 2.0f - global.GameData_.ofsY - global.GameData_.camY) / scale);

    int wmx = (int)((mmx - global.GameData_.ofsX - global.GameData_.camX) / scale);
    int wmy = (int)((mmy - global.GameData_.ofsY - global.GameData_.camY) / scale);

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
    char *wn = (char *)worldName.c_str();

    METADOT_INFO("Loading main menu @ %s", METADOT_RESLOC(MetaEngine::Format("saves/{0}", wn).c_str()));
    GameUI::MainMenuUI__visible = false;
    state = LOADING;
    stateAfterLoad = MAIN_MENU;

    METADOT_DELETE(C, GameIsolate_.world, World);
    GameIsolate_.world = nullptr;

    WorldGenerator *generator = new MaterialTestGenerator();

    std::string wpStr = METADOT_RESLOC(MetaEngine::Format("saves/{0}", wn).c_str());

    METADOT_NEW(C, GameIsolate_.world, World);
    GameIsolate_.world->noSaveLoad = true;
    GameIsolate_.world->init(wpStr, (int)ceil(WINDOWS_MAX_WIDTH / RENDER_C_TEST / (F64)CHUNK_W) * CHUNK_W + CHUNK_W * RENDER_C_TEST,
                             (int)ceil(WINDOWS_MAX_HEIGHT / RENDER_C_TEST / (F64)CHUNK_H) * CHUNK_H + CHUNK_H * RENDER_C_TEST, Render.target, &global.audioEngine, generator);

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
    std::fill(TexturePack_.pixelsParticles.begin(), TexturePack_.pixelsParticles.end(), 0);

    R_UpdateImageBytes(TexturePack_.texture, NULL, &TexturePack_.pixels[0], GameIsolate_.world->width * 4);

    R_UpdateImageBytes(TexturePack_.textureBackground, NULL, &TexturePack_.pixelsBackground[0], GameIsolate_.world->width * 4);

    R_UpdateImageBytes(TexturePack_.textureLayer2, NULL, &TexturePack_.pixelsLayer2[0], GameIsolate_.world->width * 4);

    R_UpdateImageBytes(TexturePack_.textureFire, NULL, &TexturePack_.pixelsFire[0], GameIsolate_.world->width * 4);

    R_UpdateImageBytes(TexturePack_.textureFlow, NULL, &TexturePack_.pixelsFlow[0], GameIsolate_.world->width * 4);

    R_UpdateImageBytes(TexturePack_.emissionTexture, NULL, &TexturePack_.pixelsEmission[0], GameIsolate_.world->width * 4);

    R_UpdateImageBytes(TexturePack_.textureParticles, NULL, &TexturePack_.pixelsParticles[0], GameIsolate_.world->width * 4);

    GameUI::MainMenuUI__visible = true;
}

int Game::getAimSolidSurface(int dist) {
    int dcx = this->mx - Screen.windowWidth / 2;
    int dcy = this->my - Screen.windowHeight / 2;

    F32 len = sqrtf(dcx * dcx + dcy * dcy);
    F32 udx = dcx / len;
    F32 udy = dcy / len;

    int mmx = Screen.windowWidth / 2.0f + udx * dist;
    int mmy = Screen.windowHeight / 2.0f + udy * dist;

    int wcx = (int)((Screen.windowWidth / 2.0f - global.GameData_.ofsX - global.GameData_.camX) / scale);
    int wcy = (int)((Screen.windowHeight / 2.0f - global.GameData_.ofsY - global.GameData_.camY) / scale);

    int wmx = (int)((mmx - global.GameData_.ofsX - global.GameData_.camX) / scale);
    int wmy = (int)((mmy - global.GameData_.ofsY - global.GameData_.camY) / scale);

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
    U16 waterCt = std::min(movingTiles[MaterialsList::WATER.id], (U16)5000);
    F32 water = (F32)waterCt / 3000;
    // METADOT_BUG("{} / {} = {}", waterCt, 3000, water);
    global.audioEngine.SetEventParameter("event:/World/WaterFlow", "FlowIntensity", water);
}
