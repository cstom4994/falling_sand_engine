// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game.hpp"
#include <iterator>
#include <regex>

#include "Core/Const.hpp"
#include "Core/Core.hpp"
#include "Core/DebugImpl.hpp"
#include "Core/Global.hpp"
#include "Core/Macros.hpp"
#include "Core/ThreadPool.hpp"
#include "DefaultGenerator.cpp"
#include "Engine/ImGuiImplement.hpp"
#include "Engine/LuaCore.hpp"
#include "Engine/Memory.hpp"
#include "Engine/ReflectionFlat.hpp"
#include "Engine/RendererGPU.h"
#include "Engine/SDLWrapper.hpp"
#include "Engine/Scripting.hpp"
#include "Engine/gc.h"
#include "Game/Console.hpp"
#include "Game/FileSystem.hpp"
#include "Game/GameResources.hpp"
#include "Game/GameUI.hpp"
#include "Game/ImGuiCore.hpp"
#include "Game/Settings.hpp"
#include "Game/Utils.hpp"
#include "Game/console.hpp"
#include "MRender.hpp"
#include "MaterialTestGenerator.cpp"

#include "Libs/glad/glad.h"

#include <imgui/IconsFontAwesome5.h>
#include <string>
#include <string_view>

#include "Game/InEngine.h"
#include "fmt/core.h"

const char *logo = R"(
      __  __      _        _____        _   
     |  \/  |    | |      |  __ \      | |  
     | \  / | ___| |_ __ _| |  | | ___ | |_ 
     | |\/| |/ _ \ __/ _` | |  | |/ _ \| __|
     | |  | |  __/ || (_| | |__| | (_) | |_ 
     |_|  |_|\___|\__\__,_|_____/ \___/ \__|
                                             
)";

extern void fuckme();

Game::Game(int argc, char *argv[]) {
    METAENGINE_Memory_Init(argc, argv);

    global.game = this;

    // init console & print title
    std::cout << logo << std::endl;
    Logging::init(argc, argv);

    METADOT_INFO("{} {}", METADOT_NAME, METADOT_VERSION_TEXT);

    //data = new HostData;
}

Game::~Game() {

    global.game = nullptr;

    METAENGINE_Memory_End();

    //delete data;
}

int Game::init(int argc, char *argv[]) {

    global.platform.ParseRunArgs(argc, argv);

    Resource::init();

    METADOT_INFO("Starting game...");

    bool openDebugUIs = false;
    GameUI::DebugCheatsUI::visible = openDebugUIs;
    GameUI::DebugDrawUI::visible = openDebugUIs;
    Settings::draw_frame_graph = openDebugUIs;
    if (!openDebugUIs) {
        Settings::draw_background = true;
        Settings::draw_background_grid = false;
        Settings::draw_load_zones = false;
        Settings::draw_physics_debug = false;
        Settings::draw_chunk_state = false;
        Settings::draw_debug_stats = false;
        Settings::draw_detailed_material_info = false;
        Settings::draw_temperature_map = false;
    }

    global.GameDir = GameDir(METADOT_RESLOC("saves/"));

    Networking::init();
    if (Settings::networkMode == NetworkMode::SERVER) {
        int port = Settings::server_port;
        if (argc >= 3) { port = atoi(argv[2]); }
        global.server = Server::start(port);
        SDL_SetWindowTitle(global.platform.window, win_title_server.c_str());

        /*while (true) {
            METADOT_BUG("[SERVER] tick {0:d}", server->server->connectedPeers);
            server->tick();
            Sleep(500);
        }
        return 0;*/

    } else {
        global.client = Client::start();
        //client->connect(Settings::server_ip.c_str(), Settings::server_port);

        //while (true) {
        //	METADOT_BUG("[CLIENT] tick");

        //	/* Create a reliable packet of size 7 containing "packet\0" */
        //	ENetPacket * packet = enet_packet_create("keepalive",
        //		strlen("keepalive") + 1, 0);
        //	/* Send the packet to the peer over channel id 0. */
        //	/* One could also broadcast the packet by         */
        //	/* enet_host_broadcast (host, 0, packet);         */
        //	enet_peer_send(client->peer, 0, packet);

        //	for (int i = 0; i < 1000; i++) {
        //		client->tick();
        //		Sleep(1);
        //	}
        //}
        //return 0;

        SDL_SetWindowTitle(global.platform.window, win_title_client.c_str());
    }

    global.platform.InitWindow();

    // if (Settings::networkMode != NetworkMode::SERVER) {

    //     // init fmod
    //     initThread = initThreadPool->push([&](int id) {
    //         METADOT_INFO("Initializing audio engine...");

    //         audioEngine.Init();

    //         // audioEngine.LoadBank(METADOT_RESLOC("data/assets/audio/fmod/Build/Desktop/Master.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL);
    //         // audioEngine.LoadBank(METADOT_RESLOC("data/assets/audio/fmod/Build/Desktop/Master.strings.bank"), FMOD_STUDIO_LOAD_BANK_NORMAL);

    //         audioEngine.LoadEvent("event:/Music/Title");

    //         audioEngine.LoadEvent("event:/Player/Jump");
    //         audioEngine.LoadEvent("event:/Player/Fly");
    //         audioEngine.LoadEvent("event:/Player/Wind");
    //         audioEngine.LoadEvent("event:/Player/Impact");

    //         audioEngine.LoadEvent("event:/World/Sand");
    //         audioEngine.LoadEvent("event:/World/WaterFlow");

    //         audioEngine.LoadEvent("event:/GUI/GUI_Hover");
    //         audioEngine.LoadEvent("event:/GUI/GUI_Slider");

    //         audioEngine.PlayEvent("event:/Player/Fly");
    //         audioEngine.PlayEvent("event:/Player/Wind");
    //         audioEngine.PlayEvent("event:/World/Sand");
    //         audioEngine.PlayEvent("event:/World/WaterFlow");
    //     });
    // }

    // scripting system
    auto loadscript = [&]() {
        METADOT_LOG_SCOPE_FUNCTION(INFO);
        METADOT_INFO("Loading Script...");
        METADOT_NEW(C, global.scripts, Scripts);
        global.scripts->Init();
    };
    loadscript();

    global.I18N.Init();

    GameSystem_.console.Init();

    if (Settings::networkMode != NetworkMode::SERVER) { GameIsolate_.backgrounds->Load(); }

    // init the rng
    METADOT_INFO("Seeding RNG...");
    unsigned int seed = (unsigned int) UTime::millis();
    srand(seed);

    // register & set up materials
    METADOT_INFO("Setting up materials...");

    Textures::InitTexture();
    Materials::Init();

    METADOT_NEW_ARRAY(C, movingTiles, UInt16, Materials::nMaterials);
    METADOT_NEW(C, b2DebugDraw, b2DebugDraw_impl, RenderTarget_.target);

    //worldInitThread.get();

    METADOT_INFO("Initializing world...");
    METADOT_NEW(C, GameIsolate_.world, World);
    GameIsolate_.world->noSaveLoad = true;
    GameIsolate_.world->init(
            global.GameDir.getWorldPath("mainMenu"),
            (int) ceil(WINDOWS_MAX_WIDTH / RENDER_C_TEST / (double) CHUNK_W) * CHUNK_W +
                    CHUNK_W * RENDER_C_TEST,
            (int) ceil(WINDOWS_MAX_HEIGHT / RENDER_C_TEST / (double) CHUNK_H) * CHUNK_H +
                    CHUNK_H * RENDER_C_TEST,
            RenderTarget_.target, &global.audioEngine, Settings::networkMode);

    if (Settings::networkMode != NetworkMode::SERVER) {
        // set up main menu ui

        METADOT_INFO("Setting up main menu...");
        std::string displayMode = "windowed";

        if (displayMode == "windowed") {
            global.platform.SetDisplayMode(DisplayMode::WINDOWED);
        } else if (displayMode == "borderless") {
            global.platform.SetDisplayMode(DisplayMode::BORDERLESS);
        } else if (displayMode == "fullscreen") {
            global.platform.SetDisplayMode(DisplayMode::FULLSCREEN);
        }

        global.platform.SetVSync(true);
        global.platform.SetMinimizeOnLostFocus(false);
    }

    // init threadpools
    METADOT_NEW(C, updateDirtyPool, ThreadPool, 6);
    METADOT_NEW(C, rotateVectorsPool, ThreadPool, 3);

    if (Settings::networkMode != NetworkMode::SERVER) { global.shaderworker.LoadShaders(); }

    return this->run(argc, argv);
}

void Game::createTexture() {

    METADOT_LOG_SCOPE_FUNCTION(INFO);
    METADOT_INFO("Creating world textures...");

    if (TexturePack_.loadingTexture) {
        METAENGINE_Render_FreeImage(TexturePack_.loadingTexture);
        METAENGINE_Render_FreeImage(TexturePack_.texture);
        METAENGINE_Render_FreeImage(TexturePack_.worldTexture);
        METAENGINE_Render_FreeImage(TexturePack_.lightingTexture);
        METAENGINE_Render_FreeImage(TexturePack_.emissionTexture);
        METAENGINE_Render_FreeImage(TexturePack_.textureFire);
        METAENGINE_Render_FreeImage(TexturePack_.textureFlow);
        METAENGINE_Render_FreeImage(TexturePack_.texture2Fire);
        METAENGINE_Render_FreeImage(TexturePack_.textureLayer2);
        METAENGINE_Render_FreeImage(TexturePack_.textureBackground);
        METAENGINE_Render_FreeImage(TexturePack_.textureObjects);
        METAENGINE_Render_FreeImage(TexturePack_.textureObjectsLQ);
        METAENGINE_Render_FreeImage(TexturePack_.textureObjectsBack);
        METAENGINE_Render_FreeImage(TexturePack_.textureParticles);
        METAENGINE_Render_FreeImage(TexturePack_.textureEntities);
        METAENGINE_Render_FreeImage(TexturePack_.textureEntitiesLQ);
        METAENGINE_Render_FreeImage(TexturePack_.temperatureMap);
        METAENGINE_Render_FreeImage(TexturePack_.backgroundImage);
    }

    // create textures
    loadingOnColor = 0xFFFFFFFF;
    loadingOffColor = 0x000000FF;

    std::vector<std::function<void(void)>> Funcs = {
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "loadingTexture");
                TexturePack_.loadingTexture = METAENGINE_Render_CreateImage(
                        TexturePack_.loadingScreenW = (global.platform.WIDTH / 20),
                        TexturePack_.loadingScreenH = (global.platform.HEIGHT / 20),
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);

                METAENGINE_Render_SetImageFilter(TexturePack_.loadingTexture,
                                                 METAENGINE_Render_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "texture");
                TexturePack_.texture = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);

                METAENGINE_Render_SetImageFilter(TexturePack_.texture,
                                                 METAENGINE_Render_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "worldTexture");
                TexturePack_.worldTexture = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width * Settings::hd_objects_size,
                        GameIsolate_.world->height * Settings::hd_objects_size,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);

                METAENGINE_Render_SetImageFilter(TexturePack_.worldTexture,
                                                 METAENGINE_Render_FILTER_NEAREST);

                METAENGINE_Render_LoadTarget(TexturePack_.worldTexture);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "lightingTexture");
                TexturePack_.lightingTexture = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(TexturePack_.lightingTexture,
                                                 METAENGINE_Render_FILTER_NEAREST);
                METAENGINE_Render_LoadTarget(TexturePack_.lightingTexture);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "emissionTexture");
                TexturePack_.emissionTexture = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(TexturePack_.emissionTexture,
                                                 METAENGINE_Render_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureFlow");
                TexturePack_.textureFlow = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(TexturePack_.textureFlow,
                                                 METAENGINE_Render_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureFlowSpead");
                TexturePack_.textureFlowSpead = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(TexturePack_.textureFlowSpead,
                                                 METAENGINE_Render_FILTER_NEAREST);
                METAENGINE_Render_LoadTarget(TexturePack_.textureFlowSpead);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureFire");
                TexturePack_.textureFire = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(TexturePack_.textureFire,
                                                 METAENGINE_Render_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "texture2Fire");
                TexturePack_.texture2Fire = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(TexturePack_.texture2Fire,
                                                 METAENGINE_Render_FILTER_NEAREST);
                METAENGINE_Render_LoadTarget(TexturePack_.texture2Fire);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureLayer2");
                TexturePack_.textureLayer2 = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(TexturePack_.textureLayer2,
                                                 METAENGINE_Render_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureBackground");
                TexturePack_.textureBackground = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(TexturePack_.textureBackground,
                                                 METAENGINE_Render_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureObjects");
                TexturePack_.textureObjects = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width *
                                (Settings::hd_objects ? Settings::hd_objects_size : 1),
                        GameIsolate_.world->height *
                                (Settings::hd_objects ? Settings::hd_objects_size : 1),
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(TexturePack_.textureObjects,
                                                 METAENGINE_Render_FILTER_NEAREST);
                METAENGINE_Render_LoadTarget(TexturePack_.textureObjects);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureObjectsLQ");
                TexturePack_.textureObjectsLQ = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(TexturePack_.textureObjectsLQ,
                                                 METAENGINE_Render_FILTER_NEAREST);
                METAENGINE_Render_LoadTarget(TexturePack_.textureObjectsLQ);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureObjectsBack");
                TexturePack_.textureObjectsBack = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width *
                                (Settings::hd_objects ? Settings::hd_objects_size : 1),
                        GameIsolate_.world->height *
                                (Settings::hd_objects ? Settings::hd_objects_size : 1),
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);
                METAENGINE_Render_SetImageFilter(TexturePack_.textureObjectsBack,
                                                 METAENGINE_Render_FILTER_NEAREST);
                METAENGINE_Render_LoadTarget(TexturePack_.textureObjectsBack);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureParticles");
                TexturePack_.textureParticles = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);

                METAENGINE_Render_SetImageFilter(TexturePack_.textureParticles,
                                                 METAENGINE_Render_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureEntities");
                TexturePack_.textureEntities = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width *
                                (Settings::hd_objects ? Settings::hd_objects_size : 1),
                        GameIsolate_.world->height *
                                (Settings::hd_objects ? Settings::hd_objects_size : 1),
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);

                METAENGINE_Render_LoadTarget(TexturePack_.textureEntities);

                METAENGINE_Render_SetImageFilter(TexturePack_.textureEntities,
                                                 METAENGINE_Render_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "textureEntitiesLQ");
                TexturePack_.textureEntitiesLQ = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);

                METAENGINE_Render_LoadTarget(TexturePack_.textureEntitiesLQ);

                METAENGINE_Render_SetImageFilter(TexturePack_.textureEntitiesLQ,
                                                 METAENGINE_Render_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "temperatureMap");
                TexturePack_.temperatureMap = METAENGINE_Render_CreateImage(
                        GameIsolate_.world->width, GameIsolate_.world->height,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);

                METAENGINE_Render_SetImageFilter(TexturePack_.temperatureMap,
                                                 METAENGINE_Render_FILTER_NEAREST);
            },
            [&]() {
                METADOT_LOG_SCOPE_F(INFO, "backgroundImage");
                TexturePack_.backgroundImage = METAENGINE_Render_CreateImage(
                        global.platform.WIDTH, global.platform.HEIGHT,
                        METAENGINE_Render_FormatEnum::METAENGINE_Render_FORMAT_RGBA);

                METAENGINE_Render_SetImageFilter(TexturePack_.backgroundImage,
                                                 METAENGINE_Render_FILTER_NEAREST);

                METAENGINE_Render_LoadTarget(TexturePack_.backgroundImage);
            }};

    for (auto f: Funcs) f();

    // create texture pixel buffers

    TexturePack_.pixels =
            std::vector<UInt8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
    TexturePack_.pixels_ar = &TexturePack_.pixels[0];

    TexturePack_.pixelsLayer2 =
            std::vector<UInt8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
    TexturePack_.pixelsLayer2_ar = &TexturePack_.pixelsLayer2[0];

    TexturePack_.pixelsBackground =
            std::vector<UInt8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
    TexturePack_.pixelsBackground_ar = &TexturePack_.pixelsBackground[0];

    TexturePack_.pixelsObjects = std::vector<UInt8>(
            GameIsolate_.world->width * GameIsolate_.world->height * 4, SDL_ALPHA_TRANSPARENT);
    TexturePack_.pixelsObjects_ar = &TexturePack_.pixelsObjects[0];

    TexturePack_.pixelsTemp = std::vector<UInt8>(
            GameIsolate_.world->width * GameIsolate_.world->height * 4, SDL_ALPHA_TRANSPARENT);
    TexturePack_.pixelsTemp_ar = &TexturePack_.pixelsTemp[0];

    TexturePack_.pixelsParticles = std::vector<UInt8>(
            GameIsolate_.world->width * GameIsolate_.world->height * 4, SDL_ALPHA_TRANSPARENT);
    TexturePack_.pixelsParticles_ar = &TexturePack_.pixelsParticles[0];

    TexturePack_.pixelsLoading =
            std::vector<UInt8>(TexturePack_.loadingTexture->w * TexturePack_.loadingTexture->h * 4,
                               SDL_ALPHA_TRANSPARENT);
    TexturePack_.pixelsLoading_ar = &TexturePack_.pixelsLoading[0];

    TexturePack_.pixelsFire =
            std::vector<UInt8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
    TexturePack_.pixelsFire_ar = &TexturePack_.pixelsFire[0];

    TexturePack_.pixelsFlow =
            std::vector<UInt8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
    TexturePack_.pixelsFlow_ar = &TexturePack_.pixelsFlow[0];

    TexturePack_.pixelsEmission =
            std::vector<UInt8>(GameIsolate_.world->width * GameIsolate_.world->height * 4, 0);
    TexturePack_.pixelsEmission_ar = &TexturePack_.pixelsEmission[0];

    METADOT_INFO("Creating world textures done");
}

int Game::run(int argc, char *argv[]) {
    GameIsolate_.game_timestate.startTime = UTime::millis();

    // start loading chunks
    METADOT_INFO("Queueing chunk loading...");
    for (int x = -CHUNK_W * 4; x < GameIsolate_.world->width + CHUNK_W * 4; x += CHUNK_W) {
        for (int y = -CHUNK_H * 3; y < GameIsolate_.world->height + CHUNK_H * 8; y += CHUNK_H) {
            GameIsolate_.world->queueLoadChunk(x / CHUNK_W, y / CHUNK_H, true, true);
        }
    }

    // start game loop
    METADOT_INFO("Starting game loop...");
    GameData_.freeCamX = GameIsolate_.world->width / 2.0f - CHUNK_W / 2;
    GameData_.freeCamY = GameIsolate_.world->height / 2.0f - (int) (CHUNK_H * 0.75);
    if (GameIsolate_.world->player) {
        GameData_.plPosX = GameIsolate_.world->player->x;
        GameData_.plPosY = GameIsolate_.world->player->y;
    } else {
        GameData_.plPosX = GameData_.freeCamX;
        GameData_.plPosY = GameData_.freeCamY;
    }

    SDL_Event windowEvent;

    long long lastFPS = UTime::millis();
    int frames = 0;
    GameIsolate_.game_timestate.fps = 0;

    GameIsolate_.game_timestate.lastTime = UTime::millis();
    GameIsolate_.game_timestate.lastTick = GameIsolate_.game_timestate.lastTime;
    long long lastTickPhysics = GameIsolate_.game_timestate.lastTime;

    GameIsolate_.game_timestate.mspt = 33;
    long msptPhysics = 16;

    scale = 3;
    GameData_.ofsX = (int) (-CHUNK_W * 4);
    GameData_.ofsY = (int) (-CHUNK_H * 2.5);

    GameData_.ofsX =
            (GameData_.ofsX - global.platform.WIDTH / 2) / 2 * 3 + global.platform.WIDTH / 2;
    GameData_.ofsY =
            (GameData_.ofsY - global.platform.HEIGHT / 2) / 2 * 3 + global.platform.HEIGHT / 2;

    for (int i = 0; i < FrameTimeNum; i++) { frameTime[i] = 0; }
    METADOT_NEW_ARRAY(C, objectDelete, UInt8,
                      GameIsolate_.world->width * GameIsolate_.world->height);

    fadeInStart = UTime::millis();
    fadeInLength = 250;
    fadeInWaitFrames = 5;

    // game loop
    while (this->running) {

        GameIsolate_.game_timestate.now = UTime::millis();
        GameIsolate_.game_timestate.deltaTime =
                GameIsolate_.game_timestate.now - GameIsolate_.game_timestate.lastTime;

        if (Settings::networkMode != NetworkMode::SERVER) {

            // handle window events
            while (SDL_PollEvent(&windowEvent)) {

                if (windowEvent.type == SDL_WINDOWEVENT) {
                    if (windowEvent.window.event == SDL_WINDOWEVENT_RESIZED) {
                        //METADOT_INFO("Resizing window...");
                        METAENGINE_Render_SetWindowResolution(windowEvent.window.data1,
                                                              windowEvent.window.data2);
                        METAENGINE_Render_ResetProjection(RenderTarget_.realTarget);
                        global.platform.HandleWindowSizeChange(windowEvent.window.data1,
                                                               windowEvent.window.data2);
                    }
                }

                ImGui_ImplSDL2_ProcessEvent(&windowEvent);

                if (windowEvent.type == SDL_QUIT) { goto exit; }

                if (ImGui::GetIO().WantCaptureMouse) {
                    if (windowEvent.type == SDL_MOUSEBUTTONDOWN ||
                        windowEvent.type == SDL_MOUSEBUTTONUP ||
                        windowEvent.type == SDL_MOUSEMOTION || windowEvent.type == SDL_MOUSEWHEEL) {
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

                        int x = (int) ((windowEvent.motion.x - GameData_.ofsX - GameData_.camX) /
                                       scale);
                        int y = (int) ((windowEvent.motion.y - GameData_.ofsY - GameData_.camY) /
                                       scale);

                        if (lastDrawMX == 0 && lastDrawMY == 0) {
                            lastDrawMX = x;
                            lastDrawMY = y;
                        }

                        GameIsolate_.world->forLine(lastDrawMX, lastDrawMY, x, y, [&](int index) {
                            int lineX = index % GameIsolate_.world->width;
                            int lineY = index / GameIsolate_.world->width;

                            for (int xx = -GameUI::DebugDrawUI::brushSize / 2;
                                 xx < (int) (ceil(GameUI::DebugDrawUI::brushSize / 2.0)); xx++) {
                                for (int yy = -GameUI::DebugDrawUI::brushSize / 2;
                                     yy < (int) (ceil(GameUI::DebugDrawUI::brushSize / 2.0));
                                     yy++) {
                                    if (lineX + xx < 0 || lineY + yy < 0 ||
                                        lineX + xx >= GameIsolate_.world->width ||
                                        lineY + yy >= GameIsolate_.world->height)
                                        continue;
                                    MaterialInstance tp =
                                            Tiles::create(GameUI::DebugDrawUI::selectedMaterial,
                                                          lineX + xx, lineY + yy);
                                    GameIsolate_.world
                                            ->tiles[(lineX + xx) +
                                                    (lineY + yy) * GameIsolate_.world->width] = tp;
                                    GameIsolate_.world
                                            ->dirty[(lineX + xx) +
                                                    (lineY + yy) * GameIsolate_.world->width] =
                                            true;
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
                        int x = (int) ((windowEvent.motion.x - GameData_.ofsX - GameData_.camX) /
                                       scale);
                        int y = (int) ((windowEvent.motion.y - GameData_.ofsY - GameData_.camY) /
                                       scale);

                        if (lastEraseMX == 0 && lastEraseMY == 0) {
                            lastEraseMX = x;
                            lastEraseMY = y;
                        }

                        GameIsolate_.world->forLine(lastEraseMX, lastEraseMY, x, y, [&](int index) {
                            int lineX = index % GameIsolate_.world->width;
                            int lineY = index / GameIsolate_.world->width;

                            for (int xx = -GameUI::DebugDrawUI::brushSize / 2;
                                 xx < (int) (ceil(GameUI::DebugDrawUI::brushSize / 2.0)); xx++) {
                                for (int yy = -GameUI::DebugDrawUI::brushSize / 2;
                                     yy < (int) (ceil(GameUI::DebugDrawUI::brushSize / 2.0));
                                     yy++) {

                                    if (abs(xx) + abs(yy) == GameUI::DebugDrawUI::brushSize)
                                        continue;
                                    if (GameIsolate_.world->getTile(lineX + xx, lineY + yy)
                                                .mat->physicsType != PhysicsType::AIR) {
                                        GameIsolate_.world->setTile(lineX + xx, lineY + yy,
                                                                    Tiles::NOTHING);
                                        GameIsolate_.world->lastMeshZone.x--;
                                    }
                                    if (GameIsolate_.world->getTileLayer2(lineX + xx, lineY + yy)
                                                .mat->physicsType != PhysicsType::AIR) {
                                        GameIsolate_.world->setTileLayer2(lineX + xx, lineY + yy,
                                                                          Tiles::NOTHING);
                                    }
                                }
                            }
                            return false;
                        });

                        lastEraseMX = x;
                        lastEraseMY = y;

                        // erase from rigidbodies
                        // this copies the vector
                        std::vector<RigidBody *> rbs = GameIsolate_.world->rigidBodies;

                        for (size_t i = 0; i < rbs.size(); i++) {
                            RigidBody *cur = rbs[i];
                            if (!static_cast<bool>(cur->surface)) continue;
                            if (cur->body->IsEnabled()) {
                                float s = sin(-cur->body->GetAngle());
                                float c = cos(-cur->body->GetAngle());
                                bool upd = false;
                                for (float xx = -3; xx <= 3; xx += 0.5) {
                                    for (float yy = -3; yy <= 3; yy += 0.5) {
                                        if (abs(xx) + abs(yy) == 6) continue;
                                        // rotate point

                                        float tx = x + xx - cur->body->GetPosition().x;
                                        float ty = y + yy - cur->body->GetPosition().y;

                                        int ntx = (int) (tx * c - ty * s);
                                        int nty = (int) (tx * s + ty * c);

                                        if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w &&
                                            nty < cur->surface->h) {
                                            UInt32 pixel =
                                                    METADOT_GET_PIXEL(cur->surface, ntx, nty);
                                            if (((pixel >> 24) & 0xff) != 0x00) {
                                                METADOT_GET_PIXEL(cur->surface, ntx, nty) =
                                                        0x00000000;
                                                upd = true;
                                            }
                                        }
                                    }
                                }

                                if (upd) {
                                    METAENGINE_Render_FreeImage(cur->texture);
                                    cur->texture =
                                            METAENGINE_Render_CopyImageFromSurface(cur->surface);
                                    METAENGINE_Render_SetImageFilter(
                                            cur->texture, METAENGINE_Render_FILTER_NEAREST);
                                    //GameIsolate_.world->updateRigidBodyHitbox(cur);
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

                        if (GameIsolate_.world->player &&
                            GameIsolate_.world->player->heldItem != NULL) {
                            if (GameIsolate_.world->player->heldItem->getFlag(ItemFlags::VACUUM)) {
                                GameIsolate_.world->player->holdVacuum = true;
                            } else if (GameIsolate_.world->player->heldItem->getFlag(
                                               ItemFlags::HAMMER)) {
                                //#define HAMMER_DEBUG_PHYSICS
#ifdef HAMMER_DEBUG_PHYSICS
                                int x = (int) ((windowEvent.button.x - ofsX - camX) / scale);
                                int y = (int) ((windowEvent.button.y - ofsY - camY) / scale);

                                GameIsolate_.world->physicsCheck(x, y);
#else
                                mx = windowEvent.button.x;
                                my = windowEvent.button.y;
                                int startInd = getAimSolidSurface(64);

                                if (startInd != -1) {
                                    //GameIsolate_.world->player->hammerX = x;
                                    //GameIsolate_.world->player->hammerY = y;
                                    GameIsolate_.world->player->hammerX =
                                            startInd % GameIsolate_.world->width;
                                    GameIsolate_.world->player->hammerY =
                                            startInd / GameIsolate_.world->width;
                                    GameIsolate_.world->player->holdHammer = true;
                                    //METADOT_BUG("hammer down: {0:d} {0:d} {0:d} {0:d} {0:d}", x, y, startInd, startInd % GameIsolate_.world->width, startInd / GameIsolate_.world->width);
                                    //GameIsolate_.world->setTile(GameIsolate_.world->player->hammerX, GameIsolate_.world->player->hammerY, MaterialInstance(&Materials::GENERIC_SOLID, 0x00ff00ff));
                                }
#endif
#undef HAMMER_DEBUG_PHYSICS
                            } else if (GameIsolate_.world->player->heldItem->getFlag(
                                               ItemFlags::CHISEL)) {
                                // if hovering rigidbody, open in chisel

                                int x = (int) ((mx - GameData_.ofsX - GameData_.camX) / scale);
                                int y = (int) ((my - GameData_.ofsY - GameData_.camY) / scale);

                                std::vector<RigidBody *> rbs =
                                        GameIsolate_.world->rigidBodies;// copy
                                for (size_t i = 0; i < rbs.size(); i++) {
                                    RigidBody *cur = rbs[i];

                                    bool connect = false;
                                    if (cur->body->IsEnabled()) {
                                        float s = sin(-cur->body->GetAngle());
                                        float c = cos(-cur->body->GetAngle());
                                        bool upd = false;
                                        for (float xx = -3; xx <= 3; xx += 0.5) {
                                            for (float yy = -3; yy <= 3; yy += 0.5) {
                                                if (abs(xx) + abs(yy) == 6) continue;
                                                // rotate point

                                                float tx = x + xx - cur->body->GetPosition().x;
                                                float ty = y + yy - cur->body->GetPosition().y;

                                                int ntx = (int) (tx * c - ty * s);
                                                int nty = (int) (tx * s + ty * c);

                                                if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w &&
                                                    nty < cur->surface->h) {
                                                    UInt32 pixel = METADOT_GET_PIXEL(cur->surface,
                                                                                     ntx, nty);
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

                            } else if (GameIsolate_.world->player->heldItem->getFlag(
                                               ItemFlags::TOOL)) {
                                // break with pickaxe

                                float breakSize = GameIsolate_.world->player->heldItem->breakSize;

                                int x = (int) (GameIsolate_.world->player->x +
                                               GameIsolate_.world->player->hw / 2.0f +
                                               GameIsolate_.world->loadZone.x +
                                               10 * (float) cos(
                                                            (GameIsolate_.world->player->holdAngle +
                                                             180) *
                                                            3.1415f / 180.0f) -
                                               breakSize / 2);
                                int y = (int) (GameIsolate_.world->player->y +
                                               GameIsolate_.world->player->hh / 2.0f +
                                               GameIsolate_.world->loadZone.y +
                                               10 * (float) sin(
                                                            (GameIsolate_.world->player->holdAngle +
                                                             180) *
                                                            3.1415f / 180.0f) -
                                               breakSize / 2);

                                C_Surface *tex = SDL_CreateRGBSurfaceWithFormat(
                                        0, (int) breakSize, (int) breakSize, 32,
                                        SDL_PIXELFORMAT_ARGB8888);

                                int n = 0;
                                for (int xx = 0; xx < breakSize; xx++) {
                                    for (int yy = 0; yy < breakSize; yy++) {
                                        float cx = (float) ((xx / breakSize) - 0.5);
                                        float cy = (float) ((yy / breakSize) - 0.5);

                                        if (cx * cx + cy * cy > 0.25f) continue;

                                        if (GameIsolate_.world
                                                    ->tiles[(x + xx) +
                                                            (y + yy) * GameIsolate_.world->width]
                                                    .mat->physicsType == PhysicsType::SOLID) {
                                            METADOT_GET_PIXEL(tex, xx, yy) =
                                                    GameIsolate_.world
                                                            ->tiles[(x + xx) +
                                                                    (y + yy) * GameIsolate_.world
                                                                                       ->width]
                                                            .color;
                                            GameIsolate_.world
                                                    ->tiles[(x + xx) +
                                                            (y + yy) * GameIsolate_.world->width] =
                                                    Tiles::NOTHING;
                                            GameIsolate_.world
                                                    ->dirty[(x + xx) +
                                                            (y + yy) * GameIsolate_.world->width] =
                                                    true;

                                            n++;
                                        }
                                    }
                                }

                                if (n > 0) {
                                    global.audioEngine.PlayEvent("event:/Player/Impact");
                                    b2PolygonShape s;
                                    s.SetAsBox(1, 1);
                                    RigidBody *rb = GameIsolate_.world->makeRigidBody(
                                            b2_dynamicBody, (float) x, (float) y, 0, s, 1,
                                            (float) 0.3, tex);

                                    b2Filter bf = {};
                                    bf.categoryBits = 0x0001;
                                    bf.maskBits = 0xffff;
                                    rb->body->GetFixtureList()[0].SetFilterData(bf);

                                    rb->body->SetLinearVelocity(
                                            {(float) ((rand() % 100) / 100.0 - 0.5),
                                             (float) ((rand() % 100) / 100.0 - 0.5)});

                                    GameIsolate_.world->rigidBodies.push_back(rb);
                                    GameIsolate_.world->updateRigidBodyHitbox(rb);

                                    GameIsolate_.world->lastMeshLoadZone.x--;
                                    GameIsolate_.world->updateWorldMesh();
                                }
                            }
                        }

                    } else if (windowEvent.button.button == SDL_BUTTON_RIGHT) {
                        Controls::rmouse = true;
                        if (GameIsolate_.world->player)
                            GameIsolate_.world->player->startThrow = UTime::millis();
                    } else if (windowEvent.button.button == SDL_BUTTON_MIDDLE) {
                        Controls::mmouse = true;
                    }
                } else if (windowEvent.type == SDL_MOUSEBUTTONUP) {
                    if (windowEvent.button.button == SDL_BUTTON_LEFT) {
                        Controls::lmouse = false;

                        if (GameIsolate_.world->player) {
                            if (GameIsolate_.world->player->heldItem) {
                                if (GameIsolate_.world->player->heldItem->getFlag(
                                            ItemFlags::VACUUM)) {
                                    if (GameIsolate_.world->player->holdVacuum) {
                                        GameIsolate_.world->player->holdVacuum = false;
                                    }
                                } else if (GameIsolate_.world->player->heldItem->getFlag(
                                                   ItemFlags::HAMMER)) {
                                    if (GameIsolate_.world->player->holdHammer) {
                                        int x = (int) ((windowEvent.button.x - GameData_.ofsX -
                                                        GameData_.camX) /
                                                       scale);
                                        int y = (int) ((windowEvent.button.y - GameData_.ofsY -
                                                        GameData_.camY) /
                                                       scale);

                                        int dx = GameIsolate_.world->player->hammerX - x;
                                        int dy = GameIsolate_.world->player->hammerY - y;
                                        float len = sqrtf(dx * dx + dy * dy);
                                        float udx = dx / len;
                                        float udy = dy / len;

                                        int ex = GameIsolate_.world->player->hammerX + dx;
                                        int ey = GameIsolate_.world->player->hammerY + dy;
                                        METADOT_BUG("hammer up: {0:d} {0:d} {0:d} {0:d}", ex, ey,
                                                    dx, dy);
                                        int endInd = -1;

                                        int nSegments = 1 + len / 10;
                                        std::vector<std::tuple<int, int>> points = {};
                                        for (int i = 0; i < nSegments; i++) {
                                            int sx = GameIsolate_.world->player->hammerX +
                                                     (int) ((float) (dx / nSegments) * (i + 1));
                                            int sy = GameIsolate_.world->player->hammerY +
                                                     (int) ((float) (dy / nSegments) * (i + 1));
                                            sx += rand() % 3 - 1;
                                            sy += rand() % 3 - 1;
                                            points.push_back(std::tuple<int, int>(sx, sy));
                                        }

                                        int nTilesChanged = 0;
                                        for (size_t i = 0; i < points.size(); i++) {
                                            int segSx = i == 0 ? GameIsolate_.world->player->hammerX
                                                               : std::get<0>(points[i - 1]);
                                            int segSy = i == 0 ? GameIsolate_.world->player->hammerY
                                                               : std::get<1>(points[i - 1]);
                                            int segEx = std::get<0>(points[i]);
                                            int segEy = std::get<1>(points[i]);

                                            bool hitSolidYet = false;
                                            bool broke = false;
                                            GameIsolate_.world->forLineCornered(
                                                    segSx, segSy, segEx, segEy, [&](int index) {
                                                        if (GameIsolate_.world->tiles[index]
                                                                    .mat->physicsType !=
                                                            PhysicsType::SOLID) {
                                                            if (hitSolidYet &&
                                                                (abs((index %
                                                                      GameIsolate_.world->width) -
                                                                     segSx) +
                                                                         (abs((index /
                                                                               GameIsolate_.world
                                                                                       ->width) -
                                                                              segSy)) >
                                                                 1)) {
                                                                broke = true;
                                                                return true;
                                                            }
                                                            return false;
                                                        }
                                                        hitSolidYet = true;
                                                        GameIsolate_.world
                                                                ->tiles[index] = MaterialInstance(
                                                                &Materials::GENERIC_SAND,
                                                                Drawing::darkenColor(
                                                                        GameIsolate_.world
                                                                                ->tiles[index]
                                                                                .color,
                                                                        0.5f));
                                                        GameIsolate_.world->dirty[index] = true;
                                                        endInd = index;
                                                        nTilesChanged++;
                                                        return false;
                                                    });

                                            //GameIsolate_.world->setTile(segSx, segSy, MaterialInstance(&Materials::GENERIC_SOLID, 0x00ff00ff));
                                            if (broke) break;
                                        }

                                        //GameIsolate_.world->setTile(ex, ey, MaterialInstance(&Materials::GENERIC_SOLID, 0xff0000ff));

                                        int hx = (GameIsolate_.world->player->hammerX +
                                                  (endInd % GameIsolate_.world->width)) /
                                                 2;
                                        int hy = (GameIsolate_.world->player->hammerY +
                                                  (endInd / GameIsolate_.world->width)) /
                                                 2;

                                        if (GameIsolate_.world
                                                    ->getTile((int) (hx + udy * 2),
                                                              (int) (hy - udx * 2))
                                                    .mat->physicsType == PhysicsType::SOLID) {
                                            GameIsolate_.world->physicsCheck((int) (hx + udy * 2),
                                                                             (int) (hy - udx * 2));
                                        }

                                        if (GameIsolate_.world
                                                    ->getTile((int) (hx - udy * 2),
                                                              (int) (hy + udx * 2))
                                                    .mat->physicsType == PhysicsType::SOLID) {
                                            GameIsolate_.world->physicsCheck((int) (hx - udy * 2),
                                                                             (int) (hy + udx * 2));
                                        }

                                        if (nTilesChanged > 0) {
                                            global.audioEngine.PlayEvent("event:/Player/Impact");
                                        }

                                        //GameIsolate_.world->setTile((int)(hx), (int)(hy), MaterialInstance(&Materials::GENERIC_SOLID, 0xffffffff));
                                        //GameIsolate_.world->setTile((int)(hx + udy * 6), (int)(hy - udx * 6), MaterialInstance(&Materials::GENERIC_SOLID, 0xffff00ff));
                                        //GameIsolate_.world->setTile((int)(hx - udy * 6), (int)(hy + udx * 6), MaterialInstance(&Materials::GENERIC_SOLID, 0x00ffffff));
                                    }
                                    GameIsolate_.world->player->holdHammer = false;
                                }
                            }
                        }
                    } else if (windowEvent.button.button == SDL_BUTTON_RIGHT) {
                        Controls::rmouse = false;
                        // pick up / throw item

                        int x = (int) ((mx - GameData_.ofsX - GameData_.camX) / scale);
                        int y = (int) ((my - GameData_.ofsY - GameData_.camY) / scale);

                        bool swapped = false;
                        std::vector<RigidBody *> rbs = GameIsolate_.world->rigidBodies;// copy;
                        for (size_t i = 0; i < rbs.size(); i++) {
                            RigidBody *cur = rbs[i];

                            bool connect = false;
                            if (cur->body->IsEnabled()) {
                                float s = sin(-cur->body->GetAngle());
                                float c = cos(-cur->body->GetAngle());
                                bool upd = false;
                                for (float xx = -3; xx <= 3; xx += 0.5) {
                                    for (float yy = -3; yy <= 3; yy += 0.5) {
                                        if (abs(xx) + abs(yy) == 6) continue;
                                        // rotate point

                                        float tx = x + xx - cur->body->GetPosition().x;
                                        float ty = y + yy - cur->body->GetPosition().y;

                                        int ntx = (int) (tx * c - ty * s);
                                        int nty = (int) (tx * s + ty * c);

                                        if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w &&
                                            nty < cur->surface->h) {
                                            if (((METADOT_GET_PIXEL(cur->surface, ntx, nty) >> 24) &
                                                 0xff) != 0x00) {
                                                connect = true;
                                            }
                                        }
                                    }
                                }
                            }

                            if (connect) {
                                if (GameIsolate_.world->player) {
                                    GameIsolate_.world->player->setItemInHand(
                                            Item::makeItem(ItemFlags::RIGIDBODY, cur),
                                            GameIsolate_.world);

                                    GameIsolate_.world->b2world->DestroyBody(cur->body);
                                    GameIsolate_.world->rigidBodies.erase(
                                            std::remove(GameIsolate_.world->rigidBodies.begin(),
                                                        GameIsolate_.world->rigidBodies.end(), cur),
                                            GameIsolate_.world->rigidBodies.end());

                                    swapped = true;
                                }
                                break;
                            }
                        }

                        if (!swapped) {
                            if (GameIsolate_.world->player)
                                GameIsolate_.world->player->setItemInHand(NULL, GameIsolate_.world);
                        }

                    } else if (windowEvent.button.button == SDL_BUTTON_MIDDLE) {
                        Controls::mmouse = false;
                    }
                }

                if (windowEvent.type == SDL_MOUSEMOTION) {
                    mx = windowEvent.motion.x;
                    my = windowEvent.motion.y;
                }
            }
        }

        if (Settings::networkMode == NetworkMode::SERVER) {
            global.server->tick();
        } else if (Settings::networkMode == NetworkMode::CLIENT) {
            global.client->tick();
        }

        if (Settings::networkMode != NetworkMode::SERVER) {
            //if(Settings::tick_world)
            updateFrameEarly();
            global.scripts->Update();
        }

        while (GameIsolate_.game_timestate.now - GameIsolate_.game_timestate.lastTick >
               GameIsolate_.game_timestate.mspt) {
            if (Settings::tick_world && Settings::networkMode != NetworkMode::CLIENT) tick();
            RenderTarget_.target = RenderTarget_.realTarget;
            GameIsolate_.game_timestate.lastTick = GameIsolate_.game_timestate.now;
            tickTime++;
        }

        if (Settings::networkMode != NetworkMode::SERVER) {
            if (Settings::tick_world) updateFrameLate();
        }

        if (Settings::networkMode != NetworkMode::SERVER) {
            // render

            RenderTarget_.target = RenderTarget_.realTarget;
            METAENGINE_Render_Clear(RenderTarget_.target);

            renderEarly();
            RenderTarget_.target = RenderTarget_.realTarget;

            renderLate();
            RenderTarget_.target = RenderTarget_.realTarget;

            auto image2 = METAENGINE_Render_CopyImageFromSurface(Textures::testAse);
            METAENGINE_Render_BlitScale(image2, NULL, global.game->RenderTarget_.target, 200, 200,
                                        1.0f, 1.0f);

            // render ImGui
            METAENGINE_Render_ActivateShaderProgram(0, NULL);
            METAENGINE_Render_FlushBlitBuffer();

            global.ImGuiCore->Render();

            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu(ICON_FA_ARCHIVE " 工具")) {
                    if (ImGui::MenuItem("八个雅鹿", "CTRL+A")) {}
                    ImGui::Separator();
                    ImGui::Checkbox("鼠标", &Settings::draw_cursor);
                    ImGui::Checkbox("调整", &Settings::ui_tweak);
                    ImGui::Checkbox("脚本编辑器", &Settings::ui_code_editor);
                    ImGui::EndMenu();
                }

                ImGui::SameLine(ImGui::GetWindowWidth() - 390);

                ImGui::Separator();
                ImGui::Text("%.3f ms/frame (%.1f(%d) FPS)(GC %d)",
                            1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate,
                            GameIsolate_.game_timestate.feelsLikeFps, (int) GC::C_Count);

                ImGui::EndMainMenuBar();
            }

            if (Settings::draw_material_info && !ImGui::GetIO().WantCaptureMouse) {

                int msx = (int) ((mx - GameData_.ofsX - GameData_.camX) / scale);
                int msy = (int) ((my - GameData_.ofsY - GameData_.camY) / scale);

                MaterialInstance tile;

                if (msx >= 0 && msy >= 0 && msx < GameIsolate_.world->width &&
                    msy < GameIsolate_.world->height) {
                    tile = GameIsolate_.world->tiles[msx + msy * GameIsolate_.world->width];
                    //Drawing::drawText(target, tile.mat->name.c_str(), font16, mx + 14, my, 0xff, 0xff, 0xff, ALIGN_LEFT);

                    if (tile.mat->id == Materials::GENERIC_AIR.id) {
                        std::vector<RigidBody *> rbs = GameIsolate_.world->rigidBodies;

                        for (size_t i = 0; i < rbs.size(); i++) {
                            RigidBody *cur = rbs[i];
                            if (cur->body->IsEnabled() && static_cast<bool>(cur->surface)) {
                                float s = sin(-cur->body->GetAngle());
                                float c = cos(-cur->body->GetAngle());
                                bool upd = false;

                                float tx = msx - cur->body->GetPosition().x;
                                float ty = msy - cur->body->GetPosition().y;

                                int ntx = (int) (tx * c - ty * s);
                                int nty = (int) (tx * s + ty * c);

                                if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w &&
                                    nty < cur->surface->h) {
                                    tile = cur->tiles[ntx + nty * cur->matWidth];
                                }
                            }
                        }
                    }

                    if (tile.mat->id != Materials::GENERIC_AIR.id) {

                        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.11f, 0.11f, 0.11f, 0.4f));
                        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.00f, 1.00f, 1.00f, 0.2f));
                        ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", tile.mat->name.c_str());

                        if (Settings::draw_detailed_material_info) {

                            if (tile.mat->physicsType == PhysicsType::SOUP) {
                                ImGui::Text("fluidAmount = %f", tile.fluidAmount);
                            }

                            int ln = 0;
                            if (tile.mat->interact) {
                                for (size_t i = 0; i < Materials::MATERIALS.size(); i++) {
                                    if (tile.mat->nInteractions[i] > 0) {
                                        char buff2[40];
                                        snprintf(buff2, sizeof(buff2), "    %s",
                                                 Materials::MATERIALS[i]->name.c_str());
                                        //Drawing::drawText(target, buff2, font16, mx + 14, my + 14 * ++ln, 0xff, 0xff, 0xff, ALIGN_LEFT);
                                        ImGui::Text("%s", buff2);

                                        for (int j = 0; j < tile.mat->nInteractions[i]; j++) {
                                            MaterialInteraction inter =
                                                    tile.mat->interactions[i][j];
                                            char buff1[40];
                                            if (inter.type == INTERACT_TRANSFORM_MATERIAL) {
                                                snprintf(buff1, sizeof(buff1),
                                                         "        %s %s r=%d x=%d y=%d",
                                                         "TRANSFORM",
                                                         Materials::MATERIALS[inter.data1]
                                                                 ->name.c_str(),
                                                         inter.data2, inter.ofsX, inter.ofsY);
                                            } else if (inter.type == INTERACT_SPAWN_MATERIAL) {
                                                snprintf(buff1, sizeof(buff1),
                                                         "        %s %s r=%d x=%d y=%d", "SPAWN",
                                                         Materials::MATERIALS[inter.data1]
                                                                 ->name.c_str(),
                                                         inter.data2, inter.ofsX, inter.ofsY);
                                            }
                                            //Drawing::drawText(target, buff1, font16, mx + 14, my + 14 * ++ln, 0xff, 0xff, 0xff, ALIGN_LEFT);
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

            global.ImGuiCore->end();

            // render fade in/out
            if (fadeInWaitFrames > 0) {
                fadeInWaitFrames--;
                fadeInStart = GameIsolate_.game_timestate.now;
                METAENGINE_Render_RectangleFilled(RenderTarget_.target, 0, 0, global.platform.WIDTH,
                                                  global.platform.HEIGHT, {0, 0, 0, 255});
            } else if (fadeInStart > 0 && fadeInLength > 0) {

                float thru =
                        1 - (float) (GameIsolate_.game_timestate.now - fadeInStart) / fadeInLength;

                if (thru >= 0 && thru <= 1) {
                    METAENGINE_Render_RectangleFilled(RenderTarget_.target, 0, 0,
                                                      global.platform.WIDTH, global.platform.HEIGHT,
                                                      {0, 0, 0, (uint8) (thru * 255)});
                } else {
                    fadeInStart = 0;
                    fadeInLength = 0;
                }
            }

            if (fadeOutWaitFrames > 0) {
                fadeOutWaitFrames--;
                fadeOutStart = GameIsolate_.game_timestate.now;
            } else if (fadeOutStart > 0 && fadeOutLength > 0) {

                float thru =
                        (float) (GameIsolate_.game_timestate.now - fadeOutStart) / fadeOutLength;

                if (thru >= 0 && thru <= 1) {
                    METAENGINE_Render_RectangleFilled(RenderTarget_.target, 0, 0,
                                                      global.platform.WIDTH, global.platform.HEIGHT,
                                                      {0, 0, 0, (uint8) (thru * 255)});
                } else {
                    METAENGINE_Render_RectangleFilled(RenderTarget_.target, 0, 0,
                                                      global.platform.WIDTH, global.platform.HEIGHT,
                                                      {0, 0, 0, 255});
                    fadeOutStart = 0;
                    fadeOutLength = 0;
                    fadeOutCallback();
                }
            }

            METAENGINE_Render_Flip(RenderTarget_.target);
        }

        //        if (Time::millis() - now < 2) {
        //#ifdef _WIN32
        //            Sleep(2 - (Time::millis() - now));
        //#else
        //            sleep((2 - (Time::millis() - now)) / 1000.0f);
        //#endif
        //        }

        frames++;
        if (GameIsolate_.game_timestate.now - lastFPS >= 1000) {
            lastFPS = GameIsolate_.game_timestate.now;
            //METADOT_INFO("{0:d} FPS", frames);
            if (Settings::networkMode == NetworkMode::SERVER) {
                //METADOT_BUG("{0:d} peers connected.", server->server->connectedPeers);
            }
            GameIsolate_.game_timestate.fps = frames;
            frames = 0;

            // calculate "feels like" fps
            float sum = 0;
            float num = 0.01;

            for (int i = 0; i < FrameTimeNum; i++) {
                float weight = frameTime[i];
                sum += weight * frameTime[i];
                num += weight;
            }

            GameIsolate_.game_timestate.feelsLikeFps = 1000 / (sum / num);
        }

        for (int i = 1; i < FrameTimeNum; i++) { frameTime[i - 1] = frameTime[i]; }
        frameTime[FrameTimeNum - 1] =
                (uint16_t) (UTime::millis() - GameIsolate_.game_timestate.now);

        GameIsolate_.game_timestate.lastTime = GameIsolate_.game_timestate.now;
    }

exit:

    METADOT_INFO("Shutting down...");

    std::vector<std::future<void>> results = {};

    for (auto &p: GameIsolate_.world->chunkCache) {
        if (p.first == INT_MIN) continue;
        for (auto &p2: p.second) {
            if (p2.first == INT_MIN) continue;

            results.push_back(updateDirtyPool->push([&](int id) {
                Chunk *m = p2.second;
                GameIsolate_.world->unloadChunk(m);
            }));
        }
    }

    for (int i = 0; i < results.size(); i++) { results[i].get(); }

    // TODO CppScript

    // release resources & shutdown

    global.scripts->End();
    METADOT_DELETE(C, global.scripts, Scripts);

    ReleaseGameData();

    global.ImGuiCore->onDetach();
    METADOT_DELETE(C, global.ImGuiCore, ImGuiCore);

    METADOT_DELETE(C, objectDelete, UInt8);
    GameIsolate_.backgrounds->Unload();

    GameSystem_.console.End();

    running = false;

    METADOT_DELETE(C, b2DebugDraw, b2DebugDraw_impl);
    METADOT_DELETE(C, movingTiles, UInt16);

    METADOT_DELETE(C, updateDirtyPool, ThreadPool);
    METADOT_DELETE(C, rotateVectorsPool, ThreadPool);

    if (GameIsolate_.world) {
        METADOT_DELETE(C, GameIsolate_.world, World);
        GameIsolate_.world = nullptr;
    }

    if (Settings::networkMode != NetworkMode::SERVER) {
        SDL_DestroyWindow(global.platform.window);
        SDL_Quit();
        global.audioEngine.Shutdown();
    }

    METADOT_INFO("Clean done...");

    return EXIT_SUCCESS;
}

void Game::updateFrameEarly() {

    // handle controls

    if (Controls::DEBUG_UI->get()) {
        GameUI::DebugDrawUI::visible ^= true;
        GameUI::DebugCheatsUI::visible ^= true;
    }

    if (Settings::draw_frame_graph) {
        if (Controls::STATS_DISPLAY->get()) {
            Settings::draw_frame_graph = false;
            Settings::draw_debug_stats = false;
            Settings::draw_chunk_state = false;
            Settings::draw_detailed_material_info = false;
        }
    } else {
        if (Controls::STATS_DISPLAY->get()) {
            Settings::draw_frame_graph = true;
            Settings::draw_debug_stats = true;

            if (Controls::STATS_DISPLAY_DETAILED->get()) {
                Settings::draw_chunk_state = true;
                Settings::draw_detailed_material_info = true;
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
        for (auto &cur: GameIsolate_.world->rigidBodies) {
            if (cur->body->IsEnabled()) {
                float s = sin(cur->body->GetAngle());
                float c = cos(cur->body->GetAngle());
                bool upd = false;

                for (int xx = 0; xx < cur->matWidth; xx++) {
                    for (int yy = 0; yy < cur->matHeight; yy++) {
                        int tx = xx * c - yy * s + cur->body->GetPosition().x;
                        int ty = xx * s + yy * c + cur->body->GetPosition().y;

                        MaterialInstance tt = cur->tiles[xx + yy * cur->matWidth];
                        if (tt.mat->id != Materials::GENERIC_AIR.id) {
                            if (GameIsolate_.world->tiles[tx + ty * GameIsolate_.world->width]
                                        .mat->id == Materials::GENERIC_AIR.id) {
                                GameIsolate_.world->tiles[tx + ty * GameIsolate_.world->width] = tt;
                                GameIsolate_.world->dirty[tx + ty * GameIsolate_.world->width] =
                                        true;
                            } else if (GameIsolate_.world
                                               ->tiles[(tx + 1) + ty * GameIsolate_.world->width]
                                               .mat->id == Materials::GENERIC_AIR.id) {
                                GameIsolate_.world
                                        ->tiles[(tx + 1) + ty * GameIsolate_.world->width] = tt;
                                GameIsolate_.world
                                        ->dirty[(tx + 1) + ty * GameIsolate_.world->width] = true;
                            } else if (GameIsolate_.world
                                               ->tiles[(tx - 1) + ty * GameIsolate_.world->width]
                                               .mat->id == Materials::GENERIC_AIR.id) {
                                GameIsolate_.world
                                        ->tiles[(tx - 1) + ty * GameIsolate_.world->width] = tt;
                                GameIsolate_.world
                                        ->dirty[(tx - 1) + ty * GameIsolate_.world->width] = true;
                            } else if (GameIsolate_.world
                                               ->tiles[tx + (ty + 1) * GameIsolate_.world->width]
                                               .mat->id == Materials::GENERIC_AIR.id) {
                                GameIsolate_.world
                                        ->tiles[tx + (ty + 1) * GameIsolate_.world->width] = tt;
                                GameIsolate_.world
                                        ->dirty[tx + (ty + 1) * GameIsolate_.world->width] = true;
                            } else if (GameIsolate_.world
                                               ->tiles[tx + (ty - 1) * GameIsolate_.world->width]
                                               .mat->id == Materials::GENERIC_AIR.id) {
                                GameIsolate_.world
                                        ->tiles[tx + (ty - 1) * GameIsolate_.world->width] = tt;
                                GameIsolate_.world
                                        ->dirty[tx + (ty - 1) * GameIsolate_.world->width] = true;
                            } else {
                                GameIsolate_.world->tiles[tx + ty * GameIsolate_.world->width] =
                                        Tiles::createObsidian(tx, ty);
                                GameIsolate_.world->dirty[tx + ty * GameIsolate_.world->width] =
                                        true;
                            }
                        }
                    }
                }

                if (upd) {
                    METAENGINE_Render_FreeImage(cur->texture);
                    cur->texture = METAENGINE_Render_CopyImageFromSurface(cur->surface);
                    METAENGINE_Render_SetImageFilter(cur->texture,
                                                     METAENGINE_Render_FILTER_NEAREST);
                    //GameIsolate_.world->updateRigidBodyHitbox(cur);
                    cur->needsUpdate = true;
                }
            }

            GameIsolate_.world->b2world->DestroyBody(cur->body);
        }
        GameIsolate_.world->rigidBodies.clear();
    }

    if (Controls::DEBUG_UPDATE_WORLD_MESH->get()) { GameIsolate_.world->updateWorldMesh(); }

    if (Controls::DEBUG_EXPLODE->get()) {
        int x = (int) ((mx - GameData_.ofsX - GameData_.camX) / scale);
        int y = (int) ((my - GameData_.ofsY - GameData_.camY) / scale);
        GameIsolate_.world->explosion(x, y, 30);
    }

    if (Controls::DEBUG_CARVE->get()) {
        // carve square

        int x = (int) ((mx - GameData_.ofsX - GameData_.camX) / scale - 16);
        int y = (int) ((my - GameData_.ofsY - GameData_.camY) / scale - 16);

        C_Surface *tex = SDL_CreateRGBSurfaceWithFormat(0, 32, 32, 32, SDL_PIXELFORMAT_ARGB8888);

        int n = 0;
        for (int xx = 0; xx < 32; xx++) {
            for (int yy = 0; yy < 32; yy++) {

                if (GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width]
                            .mat->physicsType == PhysicsType::SOLID) {
                    METADOT_GET_PIXEL(tex, xx, yy) =
                            GameIsolate_.world
                                    ->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width]
                                    .color;
                    GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width] =
                            Tiles::NOTHING;
                    GameIsolate_.world->dirty[(x + xx) + (y + yy) * GameIsolate_.world->width] =
                            true;
                    n++;
                }
            }
        }
        if (n > 0) {
            b2PolygonShape s;
            s.SetAsBox(1, 1);
            RigidBody *rb = GameIsolate_.world->makeRigidBody(b2_dynamicBody, (float) x, (float) y,
                                                              0, s, 1, (float) 0.3, tex);
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

    if (Controls::DEBUG_BRUSHSIZE_INC->get()) {
        GameUI::DebugDrawUI::brushSize = GameUI::DebugDrawUI::brushSize < 50
                                                 ? GameUI::DebugDrawUI::brushSize + 1
                                                 : GameUI::DebugDrawUI::brushSize;
    }

    if (Controls::DEBUG_BRUSHSIZE_DEC->get()) {
        GameUI::DebugDrawUI::brushSize = GameUI::DebugDrawUI::brushSize > 1
                                                 ? GameUI::DebugDrawUI::brushSize - 1
                                                 : GameUI::DebugDrawUI::brushSize;
    }

    if (Controls::DEBUG_TOGGLE_PLAYER->get()) {
        if (GameIsolate_.world->player) {
            GameData_.freeCamX =
                    GameIsolate_.world->player->x + GameIsolate_.world->player->hw / 2.0f;
            GameData_.freeCamY =
                    GameIsolate_.world->player->y - GameIsolate_.world->player->hh / 2.0f;
            GameIsolate_.world->entities.erase(std::remove(GameIsolate_.world->entities.begin(),
                                                           GameIsolate_.world->entities.end(),
                                                           GameIsolate_.world->player),
                                               GameIsolate_.world->entities.end());
            GameIsolate_.world->b2world->DestroyBody(GameIsolate_.world->player->rb->body);
            delete GameIsolate_.world->player;
            GameIsolate_.world->player = nullptr;
        } else {
            Player *e = new Player();
            e->x = -GameIsolate_.world->loadZone.x + GameIsolate_.world->tickZone.x +
                   GameIsolate_.world->tickZone.w / 2.0f;
            e->y = -GameIsolate_.world->loadZone.y + GameIsolate_.world->tickZone.y +
                   GameIsolate_.world->tickZone.h / 2.0f;
            e->vx = 0;
            e->vy = 0;
            e->hw = 10;
            e->hh = 20;
            b2PolygonShape sh;
            sh.SetAsBox(e->hw / 2.0f + 1, e->hh / 2.0f);
            e->rb = GameIsolate_.world->makeRigidBody(b2BodyType::b2_kinematicBody,
                                                      e->x + e->hw / 2.0f - 0.5,
                                                      e->y + e->hh / 2.0f - 0.5, 0, sh, 1, 1, NULL);
            e->rb->body->SetGravityScale(0);
            e->rb->body->SetLinearDamping(0);
            e->rb->body->SetAngularDamping(0);

            Item *i3 = new Item();
            i3->setFlag(ItemFlags::VACUUM);
            i3->vacuumParticles = {};
            i3->surface = Textures::LoadTexture("data/assets/objects/testVacuum.png");
            i3->texture = METAENGINE_Render_CopyImageFromSurface(i3->surface);
            METAENGINE_Render_SetImageFilter(i3->texture, METAENGINE_Render_FILTER_NEAREST);
            i3->pivotX = 6;
            e->setItemInHand(i3, GameIsolate_.world);

            b2Filter bf = {};
            bf.categoryBits = 0x0001;
            //bf.maskBits = 0x0000;
            e->rb->body->GetFixtureList()[0].SetFilterData(bf);

            GameIsolate_.world->entities.push_back(e);
            GameIsolate_.world->player = e;

            /*accLoadX = 0;
            accLoadY = 0;*/
        }
    }

    if (Controls::PAUSE->get()) {
        if (this->state == GameState::INGAME) {
            GameUI::IngameUI::visible = !GameUI::IngameUI::visible;
        }
    }

    global.audioEngine.Update();

    if (state == LOADING) {

    } else {
        global.audioEngine.SetEventParameter("event:/World/Sand", "Sand", 0);
        if (GameIsolate_.world->player && GameIsolate_.world->player->heldItem != NULL &&
            GameIsolate_.world->player->heldItem->getFlag(ItemFlags::FLUID_CONTAINER)) {
            if (Controls::lmouse && GameIsolate_.world->player->heldItem->carry.size() > 0) {
                // shoot fluid from container

                int x = (int) (GameIsolate_.world->player->x +
                               GameIsolate_.world->player->hw / 2.0f +
                               GameIsolate_.world->loadZone.x +
                               10 * (float) cos((GameIsolate_.world->player->holdAngle + 180) *
                                                3.1415f / 180.0f));
                int y = (int) (GameIsolate_.world->player->y +
                               GameIsolate_.world->player->hh / 2.0f +
                               GameIsolate_.world->loadZone.y +
                               10 * (float) sin((GameIsolate_.world->player->holdAngle + 180) *
                                                3.1415f / 180.0f));

                MaterialInstance mat =
                        GameIsolate_.world->player->heldItem
                                ->carry[GameIsolate_.world->player->heldItem->carry.size() - 1];
                GameIsolate_.world->player->heldItem->carry.pop_back();
                GameIsolate_.world->addParticle(new Particle(
                        mat, (float) x, (float) y,
                        (float) (GameIsolate_.world->player->vx / 2 + (rand() % 10 - 5) / 10.0f +
                                 1.5f * (float) cos((GameIsolate_.world->player->holdAngle + 180) *
                                                    3.1415f / 180.0f)),
                        (float) (GameIsolate_.world->player->vy / 2 + -(rand() % 5 + 5) / 10.0f +
                                 1.5f * (float) sin((GameIsolate_.world->player->holdAngle + 180) *
                                                    3.1415f / 180.0f)),
                        0, (float) 0.1));

                int i = (int) GameIsolate_.world->player->heldItem->carry.size();
                i = (int) ((i / (float) GameIsolate_.world->player->heldItem->capacity) *
                           GameIsolate_.world->player->heldItem->fill.size());
                UInt16Point pt = GameIsolate_.world->player->heldItem->fill[i];
                METADOT_GET_PIXEL(GameIsolate_.world->player->heldItem->surface, pt.x, pt.y) = 0x00;

                GameIsolate_.world->player->heldItem->texture =
                        METAENGINE_Render_CopyImageFromSurface(
                                GameIsolate_.world->player->heldItem->surface);
                METAENGINE_Render_SetImageFilter(GameIsolate_.world->player->heldItem->texture,
                                                 METAENGINE_Render_FILTER_NEAREST);

                global.audioEngine.SetEventParameter("event:/World/Sand", "Sand", 1);

            } else {
                // pick up fluid into container

                float breakSize = GameIsolate_.world->player->heldItem->breakSize;

                int x = (int) (GameIsolate_.world->player->x +
                               GameIsolate_.world->player->hw / 2.0f +
                               GameIsolate_.world->loadZone.x +
                               10 * (float) cos((GameIsolate_.world->player->holdAngle + 180) *
                                                3.1415f / 180.0f) -
                               breakSize / 2);
                int y = (int) (GameIsolate_.world->player->y +
                               GameIsolate_.world->player->hh / 2.0f +
                               GameIsolate_.world->loadZone.y +
                               10 * (float) sin((GameIsolate_.world->player->holdAngle + 180) *
                                                3.1415f / 180.0f) -
                               breakSize / 2);

                int n = 0;
                for (int xx = 0; xx < breakSize; xx++) {
                    for (int yy = 0; yy < breakSize; yy++) {
                        if (GameIsolate_.world->player->heldItem->capacity == 0 ||
                            (GameIsolate_.world->player->heldItem->carry.size() <
                             GameIsolate_.world->player->heldItem->capacity)) {
                            float cx = (float) ((xx / breakSize) - 0.5);
                            float cy = (float) ((yy / breakSize) - 0.5);

                            if (cx * cx + cy * cy > 0.25f) continue;

                            if (GameIsolate_.world
                                                ->tiles[(x + xx) +
                                                        (y + yy) * GameIsolate_.world->width]
                                                .mat->physicsType == PhysicsType::SAND ||
                                GameIsolate_.world
                                                ->tiles[(x + xx) +
                                                        (y + yy) * GameIsolate_.world->width]
                                                .mat->physicsType == PhysicsType::SOUP) {
                                GameIsolate_.world->player->heldItem->carry.push_back(
                                        GameIsolate_.world
                                                ->tiles[(x + xx) +
                                                        (y + yy) * GameIsolate_.world->width]);

                                int i = (int) GameIsolate_.world->player->heldItem->carry.size() -
                                        1;
                                i = (int) ((i / (float) GameIsolate_.world->player->heldItem
                                                        ->capacity) *
                                           GameIsolate_.world->player->heldItem->fill.size());
                                UInt16Point pt = GameIsolate_.world->player->heldItem->fill[i];
                                UInt32 c = GameIsolate_.world
                                                   ->tiles[(x + xx) +
                                                           (y + yy) * GameIsolate_.world->width]
                                                   .color;
                                METADOT_GET_PIXEL(GameIsolate_.world->player->heldItem->surface,
                                                  pt.x, pt.y) =
                                        (GameIsolate_.world
                                                 ->tiles[(x + xx) +
                                                         (y + yy) * GameIsolate_.world->width]
                                                 .mat->alpha
                                         << 24) +
                                        c;

                                GameIsolate_.world->player->heldItem->texture =
                                        METAENGINE_Render_CopyImageFromSurface(
                                                GameIsolate_.world->player->heldItem->surface);
                                METAENGINE_Render_SetImageFilter(
                                        GameIsolate_.world->player->heldItem->texture,
                                        METAENGINE_Render_FILTER_NEAREST);

                                GameIsolate_.world
                                        ->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width] =
                                        Tiles::NOTHING;
                                GameIsolate_.world
                                        ->dirty[(x + xx) + (y + yy) * GameIsolate_.world->width] =
                                        true;
                                n++;
                            }
                        }
                    }
                }

                if (n > 0) { global.audioEngine.PlayEvent("event:/Player/Impact"); }
            }
        }

        // rigidbody hover

        int x = (int) ((mx - GameData_.ofsX - GameData_.camX) / scale);
        int y = (int) ((my - GameData_.ofsY - GameData_.camY) / scale);

        bool swapped = false;
        float hoverDelta = 10.0 * GameIsolate_.game_timestate.deltaTime / 1000.0;

        // this copies the vector
        std::vector<RigidBody *> rbs = GameIsolate_.world->rigidBodies;
        for (size_t i = 0; i < rbs.size(); i++) {
            RigidBody *cur = rbs[i];

            if (swapped) {
                cur->hover = (float) std::fmax(0, cur->hover - hoverDelta);
                continue;
            }

            bool connect = false;
            if (cur->body->IsEnabled()) {
                float s = sin(-cur->body->GetAngle());
                float c = cos(-cur->body->GetAngle());
                bool upd = false;
                for (float xx = -3; xx <= 3; xx += 0.5) {
                    for (float yy = -3; yy <= 3; yy += 0.5) {
                        if (abs(xx) + abs(yy) == 6) continue;
                        // rotate point

                        float tx = x + xx - cur->body->GetPosition().x;
                        float ty = y + yy - cur->body->GetPosition().y;

                        int ntx = (int) (tx * c - ty * s);
                        int nty = (int) (tx * s + ty * c);

                        if (cur->surface != nullptr) {
                            if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w &&
                                nty < cur->surface->h) {
                                if (((METADOT_GET_PIXEL(cur->surface, ntx, nty) >> 24) & 0xff) !=
                                    0x00) {
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
                cur->hover = (float) std::fmin(1, cur->hover + hoverDelta);
            } else {
                cur->hover = (float) std::fmax(0, cur->hover - hoverDelta);
            }
        }

        // update GameIsolate_.world->tickZone

        GameIsolate_.world->tickZone = {CHUNK_W, CHUNK_H, GameIsolate_.world->width - CHUNK_W * 2,
                                        GameIsolate_.world->height - CHUNK_H * 2};
        if (GameIsolate_.world->tickZone.x + GameIsolate_.world->tickZone.w >
            GameIsolate_.world->width) {
            GameIsolate_.world->tickZone.x =
                    GameIsolate_.world->width - GameIsolate_.world->tickZone.w;
        }

        if (GameIsolate_.world->tickZone.y + GameIsolate_.world->tickZone.h >
            GameIsolate_.world->height) {
            GameIsolate_.world->tickZone.y =
                    GameIsolate_.world->height - GameIsolate_.world->tickZone.h;
        }
    }
}

void Game::tick() {

    //METADOT_BUG("{0:d} {0:d}", accLoadX, accLoadY);
    if (state == LOADING) {
        if (GameIsolate_.world) {
            // tick chunkloading
            GameIsolate_.world->frame();
            if (GameIsolate_.world->readyToMerge.size() == 0 && fadeOutStart == 0) {
                fadeOutStart = GameIsolate_.game_timestate.now;
                fadeOutLength = 250;
                fadeOutCallback = [&]() {
                    fadeInStart = GameIsolate_.game_timestate.now;
                    fadeInLength = 500;
                    fadeInWaitFrames = 4;
                    state = stateAfterLoad;
                };

                global.platform.SetWindowFlash(WindowFlashAction::START_COUNT, 1, 333);
            }
        }
    } else {

        int lastReadyToMergeSize = (int) GameIsolate_.world->readyToMerge.size();

        // check chunk loading
        tickChunkLoading();

        if (GameIsolate_.world->needToTickGeneration) GameIsolate_.world->tickChunkGeneration();

        // clear objects
        METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_Clear(TexturePack_.textureObjects->target);

        METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_Clear(TexturePack_.textureObjectsLQ->target);

        METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_Clear(TexturePack_.textureObjectsBack->target);

        if (Settings::tick_world && GameIsolate_.world->readyToMerge.size() == 0) {
            GameIsolate_.world->tickChunks();
        }

        // render objects

        memset(objectDelete, false,
               (size_t) GameIsolate_.world->width * GameIsolate_.world->height * sizeof(bool));

        METAENGINE_Render_SetBlendMode(TexturePack_.textureObjects, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_SetBlendMode(TexturePack_.textureObjectsLQ,
                                       METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_SetBlendMode(TexturePack_.textureObjectsBack,
                                       METAENGINE_Render_BLEND_NORMAL);

        for (size_t i = 0; i < GameIsolate_.world->rigidBodies.size(); i++) {
            RigidBody *cur = GameIsolate_.world->rigidBodies[i];
            if (cur == nullptr) continue;
            if (cur->surface == nullptr) continue;
            if (!cur->body->IsEnabled()) continue;

            float x = cur->body->GetPosition().x;
            float y = cur->body->GetPosition().y;

            // draw

            METAENGINE_Render_Target *tgt = cur->back ? TexturePack_.textureObjectsBack->target
                                                      : TexturePack_.textureObjects->target;
            METAENGINE_Render_Target *tgtLQ = cur->back ? TexturePack_.textureObjectsBack->target
                                                        : TexturePack_.textureObjectsLQ->target;
            int scaleObjTex = Settings::hd_objects ? Settings::hd_objects_size : 1;

            METAENGINE_Render_Rect r = {x * scaleObjTex, y * scaleObjTex,
                                        (float) cur->surface->w * scaleObjTex,
                                        (float) cur->surface->h * scaleObjTex};

            if (cur->texNeedsUpdate) {
                if (cur->texture != nullptr) { METAENGINE_Render_FreeImage(cur->texture); }
                cur->texture = METAENGINE_Render_CopyImageFromSurface(cur->surface);
                METAENGINE_Render_SetImageFilter(cur->texture, METAENGINE_Render_FILTER_NEAREST);
                cur->texNeedsUpdate = false;
            }

            METAENGINE_Render_BlitRectX(cur->texture, NULL, tgt, &r,
                                        cur->body->GetAngle() * 180 / (float) M_PI, 0, 0,
                                        METAENGINE_Render_FLIP_NONE);

            // draw outline

            UInt8 outlineAlpha = (UInt8) (cur->hover * 255);
            if (outlineAlpha > 0) {
                SDL_Color col = {0xff, 0xff, 0x80, outlineAlpha};
                METAENGINE_Render_SetShapeBlendMode(
                        METAENGINE_Render_BLEND_NORMAL_FACTOR_ALPHA);// SDL_BLENDMODE_BLEND
                for (auto &l: cur->outline) {
                    b2Vec2 *vec = new b2Vec2[l.GetNumPoints()];
                    for (int j = 0; j < l.GetNumPoints(); j++) {
                        vec[j] = {(float) l.GetPoint(j).x / scale, (float) l.GetPoint(j).y / scale};
                    }
                    Drawing::drawPolygon(tgtLQ, col, vec, (int) x, (int) y, scale, l.GetNumPoints(),
                                         cur->body->GetAngle(), 0, 0);
                    delete[] vec;
                }
                METAENGINE_Render_SetShapeBlendMode(
                        METAENGINE_Render_BLEND_NORMAL);// SDL_BLENDMODE_NONE
            }

            // displace fluids

            float s = sin(cur->body->GetAngle());
            float c = cos(cur->body->GetAngle());

            std::vector<std::pair<int, int>> checkDirs = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};

            for (int tx = 0; tx < cur->matWidth; tx++) {
                for (int ty = 0; ty < cur->matHeight; ty++) {
                    MaterialInstance rmat = cur->tiles[tx + ty * cur->matWidth];
                    if (rmat.mat->id == Materials::GENERIC_AIR.id) continue;

                    // rotate point
                    int wx = (int) (tx * c - (ty + 1) * s + x);
                    int wy = (int) (tx * s + (ty + 1) * c + y);

                    for (auto &dir: checkDirs) {
                        int wxd = wx + dir.first;
                        int wyd = wy + dir.second;

                        if (wxd < 0 || wyd < 0 || wxd >= GameIsolate_.world->width ||
                            wyd >= GameIsolate_.world->height)
                            continue;
                        if (GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width]
                                    .mat->physicsType == PhysicsType::AIR) {
                            GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width] = rmat;
                            GameIsolate_.world->dirty[wxd + wyd * GameIsolate_.world->width] = true;
                            //objectDelete[wxd + wyd * GameIsolate_.world->width] = true;
                            break;
                        } else if (GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width]
                                           .mat->physicsType == PhysicsType::SAND) {
                            GameIsolate_.world->addParticle(new Particle(
                                    GameIsolate_.world
                                            ->tiles[wxd + wyd * GameIsolate_.world->width],
                                    (float) wxd, (float) (wyd - 3),
                                    (float) ((rand() % 10 - 5) / 10.0f),
                                    (float) (-(rand() % 5 + 5) / 10.0f), 0, (float) 0.1));
                            GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width] = rmat;
                            //objectDelete[wxd + wyd * GameIsolate_.world->width] = true;
                            GameIsolate_.world->dirty[wxd + wyd * GameIsolate_.world->width] = true;
                            cur->body->SetLinearVelocity(
                                    {cur->body->GetLinearVelocity().x * (float) 0.99,
                                     cur->body->GetLinearVelocity().y * (float) 0.99});
                            cur->body->SetAngularVelocity(cur->body->GetAngularVelocity() *
                                                          (float) 0.98);
                            break;
                        } else if (GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width]
                                           .mat->physicsType == PhysicsType::SOUP) {
                            GameIsolate_.world->addParticle(new Particle(
                                    GameIsolate_.world
                                            ->tiles[wxd + wyd * GameIsolate_.world->width],
                                    (float) wxd, (float) (wyd - 3),
                                    (float) ((rand() % 10 - 5) / 10.0f),
                                    (float) (-(rand() % 5 + 5) / 10.0f), 0, (float) 0.1));
                            GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width] = rmat;
                            //objectDelete[wxd + wyd * GameIsolate_.world->width] = true;
                            GameIsolate_.world->dirty[wxd + wyd * GameIsolate_.world->width] = true;
                            cur->body->SetLinearVelocity(
                                    {cur->body->GetLinearVelocity().x * (float) 0.998,
                                     cur->body->GetLinearVelocity().y * (float) 0.998});
                            cur->body->SetAngularVelocity(cur->body->GetAngularVelocity() *
                                                          (float) 0.99);
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
                if (GameIsolate_.world->player->holdHammer) {
                    int x = (int) ((mx - GameData_.ofsX - GameData_.camX) / scale);
                    int y = (int) ((my - GameData_.ofsY - GameData_.camY) / scale);
                    METAENGINE_Render_Line(TexturePack_.textureEntitiesLQ->target, x, y,
                                           GameIsolate_.world->player->hammerX,
                                           GameIsolate_.world->player->hammerY,
                                           {0xff, 0xff, 0x00, 0xff});
                }
            }
        }
        METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);// SDL_BLENDMODE_NONE

        // entity fluid displacement & make solid

        for (size_t i = 0; i < GameIsolate_.world->entities.size(); i++) {
            Entity cur = *GameIsolate_.world->entities[i];

            for (int tx = 0; tx < cur.hw; tx++) {
                for (int ty = 0; ty < cur.hh; ty++) {

                    int wx = (int) (tx + cur.x + GameIsolate_.world->loadZone.x);
                    int wy = (int) (ty + cur.y + GameIsolate_.world->loadZone.y);
                    if (wx < 0 || wy < 0 || wx >= GameIsolate_.world->width ||
                        wy >= GameIsolate_.world->height)
                        continue;
                    if (GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width]
                                .mat->physicsType == PhysicsType::AIR) {
                        GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width] =
                                Tiles::OBJECT;
                        objectDelete[wx + wy * GameIsolate_.world->width] = true;
                    } else if (GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width]
                                               .mat->physicsType == PhysicsType::SAND ||
                               GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width]
                                               .mat->physicsType == PhysicsType::SOUP) {
                        GameIsolate_.world->addParticle(new Particle(
                                GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width],
                                (float) (wx + rand() % 3 - 1 - cur.vx), (float) (wy - abs(cur.vy)),
                                (float) (-cur.vx / 4 + (rand() % 10 - 5) / 5.0f),
                                (float) (-cur.vy / 4 + -(rand() % 5 + 5) / 5.0f), 0, (float) 0.1));
                        GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width] =
                                Tiles::OBJECT;
                        objectDelete[wx + wy * GameIsolate_.world->width] = true;
                        GameIsolate_.world->dirty[wx + wy * GameIsolate_.world->width] = true;
                    }
                }
            }
        }

        if (Settings::tick_world && GameIsolate_.world->readyToMerge.size() == 0) {
            GameIsolate_.world->tick();
        }

        if (Controls::DEBUG_TICK->get()) { GameIsolate_.world->tick(); }

        // Tick Cam zoom

        if (state == INGAME && (Controls::ZOOM_IN->get() || Controls::ZOOM_OUT->get())) {
            float CamZoomIn = (float) (Controls::ZOOM_IN->get());
            float CamZoomOut = (float) (Controls::ZOOM_OUT->get());

            float deltaScale = CamZoomIn - CamZoomOut;
            int oldScale = scale;
            scale += deltaScale;
            if (scale < 1) scale = 1;

            GameData_.ofsX = (GameData_.ofsX - global.platform.WIDTH / 2) / oldScale * scale +
                             global.platform.WIDTH / 2;
            GameData_.ofsY = (GameData_.ofsY - global.platform.HEIGHT / 2) / oldScale * scale +
                             global.platform.HEIGHT / 2;
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

        //int pitch;
        //void* vdpixels_ar = texture->data;
        //UInt8* dpixels_ar = (UInt8*)vdpixels_ar;
        UInt8 *dpixels_ar = TexturePack_.pixels_ar;
        UInt8 *dpixelsFire_ar = TexturePack_.pixelsFire_ar;
        UInt8 *dpixelsFlow_ar = TexturePack_.pixelsFlow_ar;
        UInt8 *dpixelsEmission_ar = TexturePack_.pixelsEmission_ar;

        std::vector<std::future<void>> results = {};

        results.push_back(updateDirtyPool->push([&](int id) {
            //SDL_SetRenderTarget(renderer, textureParticles);
            void *particlePixels = TexturePack_.pixelsParticles_ar;

            memset(particlePixels, 0,
                   (size_t) GameIsolate_.world->width * GameIsolate_.world->height * 4);

            GameIsolate_.world->renderParticles((UInt8 **) &particlePixels);
            GameIsolate_.world->tickParticles();

            //SDL_SetRenderTarget(renderer, NULL);
        }));

        if (GameIsolate_.world->readyToMerge.size() == 0) {
            results.push_back(
                    updateDirtyPool->push([&](int id) { GameIsolate_.world->tickObjectBounds(); }));
        }

        for (int i = 0; i < results.size(); i++) { results[i].get(); }

        for (size_t i = 0; i < GameIsolate_.world->rigidBodies.size(); i++) {
            RigidBody *cur = GameIsolate_.world->rigidBodies[i];
            if (cur == nullptr) continue;
            if (cur->surface == nullptr) continue;
            if (!cur->body->IsEnabled()) continue;

            float x = cur->body->GetPosition().x;
            float y = cur->body->GetPosition().y;

            float s = sin(cur->body->GetAngle());
            float c = cos(cur->body->GetAngle());

            std::vector<std::pair<int, int>> checkDirs = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (int tx = 0; tx < cur->matWidth; tx++) {
                for (int ty = 0; ty < cur->matHeight; ty++) {
                    MaterialInstance rmat = cur->tiles[tx + ty * cur->matWidth];
                    if (rmat.mat->id == Materials::GENERIC_AIR.id) continue;

                    // rotate point
                    int wx = (int) (tx * c - (ty + 1) * s + x);
                    int wy = (int) (tx * s + (ty + 1) * c + y);

                    bool found = false;
                    for (auto &dir: checkDirs) {
                        int wxd = wx + dir.first;
                        int wyd = wy + dir.second;

                        if (wxd < 0 || wyd < 0 || wxd >= GameIsolate_.world->width ||
                            wyd >= GameIsolate_.world->height)
                            continue;
                        if (GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width] ==
                            rmat) {
                            cur->tiles[tx + ty * cur->matWidth] =
                                    GameIsolate_.world
                                            ->tiles[wxd + wyd * GameIsolate_.world->width];
                            GameIsolate_.world->tiles[wxd + wyd * GameIsolate_.world->width] =
                                    Tiles::NOTHING;
                            GameIsolate_.world->dirty[wxd + wyd * GameIsolate_.world->width] = true;
                            found = true;

                            //for(int dxx = -1; dxx <= 1; dxx++) {
                            //    for(int dyy = -1; dyy <= 1; dyy++) {
                            //        if(GameIsolate_.world->tiles[(wxd + dxx) + (wyd + dyy) * GameIsolate_.world->width].mat->physicsType == PhysicsType::SAND || GameIsolate_.world->tiles[(wxd + dxx) + (wyd + dyy) * GameIsolate_.world->width].mat->physicsType == PhysicsType::SOUP) {
                            //            uint32_t color = GameIsolate_.world->tiles[(wxd + dxx) + (wyd + dyy) * GameIsolate_.world->width].color;

                            //            unsigned int offset = ((wxd + dxx) + (wyd + dyy) * GameIsolate_.world->width) * 4;

                            //            dpixels_ar[offset + 2] = ((color >> 0) & 0xff);        // b
                            //            dpixels_ar[offset + 1] = ((color >> 8) & 0xff);        // g
                            //            dpixels_ar[offset + 0] = ((color >> 16) & 0xff);        // r
                            //            dpixels_ar[offset + 3] = GameIsolate_.world->tiles[(wxd + dxx) + (wyd + dyy) * GameIsolate_.world->width].mat->alpha;    // a
                            //        }
                            //    }
                            //}

                            //if(!Settings::draw_load_zones) {
                            //    unsigned int offset = (wxd + wyd * GameIsolate_.world->width) * 4;
                            //    dpixels_ar[offset + 2] = 0;        // b
                            //    dpixels_ar[offset + 1] = 0;        // g
                            //    dpixels_ar[offset + 0] = 0xff;        // r
                            //    dpixels_ar[offset + 3] = 0xff;    // a
                            //}

                            break;
                        }
                    }

                    if (!found) {
                        if (GameIsolate_.world->tiles[wx + wy * GameIsolate_.world->width]
                                    .mat->id == Materials::GENERIC_AIR.id) {
                            cur->tiles[tx + ty * cur->matWidth] = Tiles::NOTHING;
                        }
                    }
                }
            }

            for (int tx = 0; tx < cur->surface->w; tx++) {
                for (int ty = 0; ty < cur->surface->h; ty++) {
                    MaterialInstance mat = cur->tiles[tx + ty * cur->surface->w];
                    if (mat.mat->id == Materials::GENERIC_AIR.id) {
                        METADOT_GET_PIXEL(cur->surface, tx, ty) = 0x00000000;
                    } else {
                        METADOT_GET_PIXEL(cur->surface, tx, ty) =
                                (mat.mat->alpha << 24) + (mat.color & 0x00ffffff);
                    }
                }
            }

            METAENGINE_Render_FreeImage(cur->texture);
            cur->texture = METAENGINE_Render_CopyImageFromSurface(cur->surface);
            METAENGINE_Render_SetImageFilter(cur->texture, METAENGINE_Render_FILTER_NEAREST);

            cur->needsUpdate = true;
        }

        if (GameIsolate_.world->readyToMerge.size() == 0) {
            if (Settings::tick_box2d) GameIsolate_.world->tickObjects();
        }

        if (tickTime % 10 == 0) GameIsolate_.world->tickObjectsMesh();

        for (int i = 0; i < Materials::nMaterials; i++) movingTiles[i] = 0;

        results.clear();
        results.push_back(updateDirtyPool->push([&](int id) {
            for (int i = 0; i < GameIsolate_.world->width * GameIsolate_.world->height; i++) {
                const unsigned int offset = i * 4;

                if (GameIsolate_.world->dirty[i]) {
                    hadDirty = true;
                    movingTiles[GameIsolate_.world->tiles[i].mat->id]++;
                    if (GameIsolate_.world->tiles[i].mat->physicsType == PhysicsType::AIR) {
                        dpixels_ar[offset + 0] = 0;                    // b
                        dpixels_ar[offset + 1] = 0;                    // g
                        dpixels_ar[offset + 2] = 0;                    // r
                        dpixels_ar[offset + 3] = SDL_ALPHA_TRANSPARENT;// a

                        dpixelsFire_ar[offset + 0] = 0;                    // b
                        dpixelsFire_ar[offset + 1] = 0;                    // g
                        dpixelsFire_ar[offset + 2] = 0;                    // r
                        dpixelsFire_ar[offset + 3] = SDL_ALPHA_TRANSPARENT;// a

                        dpixelsEmission_ar[offset + 0] = 0;                    // b
                        dpixelsEmission_ar[offset + 1] = 0;                    // g
                        dpixelsEmission_ar[offset + 2] = 0;                    // r
                        dpixelsEmission_ar[offset + 3] = SDL_ALPHA_TRANSPARENT;// a

                        GameIsolate_.world->flowY[i] = 0;
                        GameIsolate_.world->flowX[i] = 0;
                    } else {
                        UInt32 color = GameIsolate_.world->tiles[i].color;
                        UInt32 emit = GameIsolate_.world->tiles[i].mat->emitColor;
                        //float br = GameIsolate_.world->light[i];
                        dpixels_ar[offset + 2] = ((color >> 0) & 0xff);                  // b
                        dpixels_ar[offset + 1] = ((color >> 8) & 0xff);                  // g
                        dpixels_ar[offset + 0] = ((color >> 16) & 0xff);                 // r
                        dpixels_ar[offset + 3] = GameIsolate_.world->tiles[i].mat->alpha;// a

                        dpixelsEmission_ar[offset + 2] = ((emit >> 0) & 0xff); // b
                        dpixelsEmission_ar[offset + 1] = ((emit >> 8) & 0xff); // g
                        dpixelsEmission_ar[offset + 0] = ((emit >> 16) & 0xff);// r
                        dpixelsEmission_ar[offset + 3] = ((emit >> 24) & 0xff);// a

                        if (GameIsolate_.world->tiles[i].mat->id == Materials::FIRE.id) {
                            dpixelsFire_ar[offset + 2] = ((color >> 0) & 0xff); // b
                            dpixelsFire_ar[offset + 1] = ((color >> 8) & 0xff); // g
                            dpixelsFire_ar[offset + 0] = ((color >> 16) & 0xff);// r
                            dpixelsFire_ar[offset + 3] =
                                    GameIsolate_.world->tiles[i].mat->alpha;// a
                            hadFire = true;
                        }
                        if (GameIsolate_.world->tiles[i].mat->physicsType == PhysicsType::SOUP) {

                            float newFlowX = GameIsolate_.world->prevFlowX[i] +
                                             (GameIsolate_.world->flowX[i] -
                                              GameIsolate_.world->prevFlowX[i]) *
                                                     0.25;
                            float newFlowY = GameIsolate_.world->prevFlowY[i] +
                                             (GameIsolate_.world->flowY[i] -
                                              GameIsolate_.world->prevFlowY[i]) *
                                                     0.25;
                            if (newFlowY < 0) newFlowY *= 0.5;

                            dpixelsFlow_ar[offset + 2] = 0;// b
                            dpixelsFlow_ar[offset + 1] =
                                    std::min(
                                            std::max(newFlowY *
                                                                     (3.0 / GameIsolate_.world
                                                                                      ->tiles[i]
                                                                                      .mat
                                                                                      ->iterations +
                                                                      0.5) /
                                                                     4.0 +
                                                             0.5,
                                                     0.0),
                                            1.0) *
                                    255;// g
                            dpixelsFlow_ar[offset + 0] =
                                    std::min(
                                            std::max(newFlowX *
                                                                     (3.0 / GameIsolate_.world
                                                                                      ->tiles[i]
                                                                                      .mat
                                                                                      ->iterations +
                                                                      0.5) /
                                                                     4.0 +
                                                             0.5,
                                                     0.0),
                                            1.0) *
                                    255;                      // r
                            dpixelsFlow_ar[offset + 3] = 0xff;// a
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

        //void* vdpixelsLayer2_ar = textureLayer2->data;
        //UInt8* dpixelsLayer2_ar = (UInt8*)vdpixelsLayer2_ar;
        UInt8 *dpixelsLayer2_ar = TexturePack_.pixelsLayer2_ar;
        results.push_back(updateDirtyPool->push([&](int id) {
            for (int i = 0; i < GameIsolate_.world->width * GameIsolate_.world->height; i++) {
                /*for (int x = 0; x < GameIsolate_.world->width; x++) {
                    for (int y = 0; y < GameIsolate_.world->height; y++) {*/
                //const unsigned int i = x + y * GameIsolate_.world->width;
                const unsigned int offset = i * 4;
                if (GameIsolate_.world->layer2Dirty[i]) {
                    hadLayer2Dirty = true;
                    if (GameIsolate_.world->layer2[i].mat->physicsType == PhysicsType::AIR) {
                        if (Settings::draw_background_grid) {
                            UInt32 color = ((i) % 2) == 0 ? 0x888888 : 0x444444;
                            dpixelsLayer2_ar[offset + 2] = (color >> 0) & 0xff; // b
                            dpixelsLayer2_ar[offset + 1] = (color >> 8) & 0xff; // g
                            dpixelsLayer2_ar[offset + 0] = (color >> 16) & 0xff;// r
                            dpixelsLayer2_ar[offset + 3] = SDL_ALPHA_OPAQUE;    // a
                            continue;
                        } else {
                            dpixelsLayer2_ar[offset + 0] = 0;                    // b
                            dpixelsLayer2_ar[offset + 1] = 0;                    // g
                            dpixelsLayer2_ar[offset + 2] = 0;                    // r
                            dpixelsLayer2_ar[offset + 3] = SDL_ALPHA_TRANSPARENT;// a
                            continue;
                        }
                    }
                    UInt32 color = GameIsolate_.world->layer2[i].color;
                    dpixelsLayer2_ar[offset + 2] = (color >> 0) & 0xff;                     // b
                    dpixelsLayer2_ar[offset + 1] = (color >> 8) & 0xff;                     // g
                    dpixelsLayer2_ar[offset + 0] = (color >> 16) & 0xff;                    // r
                    dpixelsLayer2_ar[offset + 3] = GameIsolate_.world->layer2[i].mat->alpha;// a
                }
            }
        }));

        //void* vdpixelsBackground_ar = textureBackground->data;
        //UInt8* dpixelsBackground_ar = (UInt8*)vdpixelsBackground_ar;
        UInt8 *dpixelsBackground_ar = TexturePack_.pixelsBackground_ar;
        results.push_back(updateDirtyPool->push([&](int id) {
            for (int i = 0; i < GameIsolate_.world->width * GameIsolate_.world->height; i++) {
                /*for (int x = 0; x < GameIsolate_.world->width; x++) {
                    for (int y = 0; y < GameIsolate_.world->height; y++) {*/
                //const unsigned int i = x + y * GameIsolate_.world->width;
                const unsigned int offset = i * 4;

                if (GameIsolate_.world->backgroundDirty[i]) {
                    hadBackgroundDirty = true;
                    UInt32 color = GameIsolate_.world->background[i];
                    dpixelsBackground_ar[offset + 2] = (color >> 0) & 0xff; // b
                    dpixelsBackground_ar[offset + 1] = (color >> 8) & 0xff; // g
                    dpixelsBackground_ar[offset + 0] = (color >> 16) & 0xff;// r
                    dpixelsBackground_ar[offset + 3] = (color >> 24) & 0xff;// a
                }

                //}
            }
        }));

        for (int i = 0; i < GameIsolate_.world->width * GameIsolate_.world->height; i++) {
            /*for (int x = 0; x < GameIsolate_.world->width; x++) {
                for (int y = 0; y < GameIsolate_.world->height; y++) {*/
            //const unsigned int i = x + y * GameIsolate_.world->width;
            const unsigned int offset = i * 4;

            if (objectDelete[i]) { GameIsolate_.world->tiles[i] = Tiles::NOTHING; }
        }

        //results.push_back(updateDirtyPool->push([&](int id) {

        //}));

        for (int i = 0; i < results.size(); i++) { results[i].get(); }

        updateMaterialSounds();

        METAENGINE_Render_UpdateImageBytes(TexturePack_.textureParticles, NULL,
                                           &TexturePack_.pixelsParticles_ar[0],
                                           GameIsolate_.world->width * 4);

        if (hadDirty)
            memset(GameIsolate_.world->dirty, false,
                   (size_t) GameIsolate_.world->width * GameIsolate_.world->height);
        if (hadLayer2Dirty)
            memset(GameIsolate_.world->layer2Dirty, false,
                   (size_t) GameIsolate_.world->width * GameIsolate_.world->height);
        if (hadBackgroundDirty)
            memset(GameIsolate_.world->backgroundDirty, false,
                   (size_t) GameIsolate_.world->width * GameIsolate_.world->height);

        if (Settings::tick_temperature && tickTime % GameTick == 2) {
            GameIsolate_.world->tickTemperature();
        }
        if (Settings::draw_temperature_map && tickTime % GameTick == 0) {
            renderTemperatureMap(GameIsolate_.world);
        }

        if (hadDirty) {
            METAENGINE_Render_UpdateImageBytes(TexturePack_.texture, NULL, &TexturePack_.pixels[0],
                                               GameIsolate_.world->width * 4);

            METAENGINE_Render_UpdateImageBytes(TexturePack_.emissionTexture, NULL,
                                               &TexturePack_.pixelsEmission[0],
                                               GameIsolate_.world->width * 4);
        }

        if (hadLayer2Dirty) {
            METAENGINE_Render_UpdateImageBytes(TexturePack_.textureLayer2, NULL,
                                               &TexturePack_.pixelsLayer2[0],
                                               GameIsolate_.world->width * 4);
        }

        if (hadBackgroundDirty) {
            METAENGINE_Render_UpdateImageBytes(TexturePack_.textureBackground, NULL,
                                               &TexturePack_.pixelsBackground[0],
                                               GameIsolate_.world->width * 4);
        }

        if (hadFlow) {
            METAENGINE_Render_UpdateImageBytes(TexturePack_.textureFlow, NULL,
                                               &TexturePack_.pixelsFlow[0],
                                               GameIsolate_.world->width * 4);

            global.shaderworker.waterFlowPassShader->dirty = true;
        }

        if (hadFire) {
            METAENGINE_Render_UpdateImageBytes(TexturePack_.textureFire, NULL,
                                               &TexturePack_.pixelsFire[0],
                                               GameIsolate_.world->width * 4);
        }

        if (Settings::draw_temperature_map) {
            METAENGINE_Render_UpdateImageBytes(TexturePack_.temperatureMap, NULL,
                                               &TexturePack_.pixelsTemp[0],
                                               GameIsolate_.world->width * 4);
        }

        /*METAENGINE_Render_UpdateImageBytes(
            textureObjects,
            NULL,
            &pixelsObjects[0],
            GameIsolate_.world->width * 4
        );*/

        if (Settings::tick_box2d && tickTime % GameTick == 0) GameIsolate_.world->updateWorldMesh();
    }
}

void Game::tickChunkLoading() {

    // if need to load chunks
    if ((abs(accLoadX) > CHUNK_W / 2 || abs(accLoadY) > CHUNK_H / 2)) {
        while (GameIsolate_.world->toLoad.size() > 0) {
            // tick chunkloading
            GameIsolate_.world->frame();
        }

        //iterate

        for (int i = 0; i < GameIsolate_.world->width * GameIsolate_.world->height; i++) {
            const unsigned int offset = i * 4;

#define UCH_SET_PIXEL(pix_ar, ofs, c_r, c_g, c_b, c_a)                                             \
    pix_ar[ofs + 0] = c_b;                                                                         \
    pix_ar[ofs + 1] = c_g;                                                                         \
    pix_ar[ofs + 2] = c_r;                                                                         \
    pix_ar[ofs + 3] = c_a;

            if (GameIsolate_.world->dirty[i]) {
                if (GameIsolate_.world->tiles[i].mat->physicsType == PhysicsType::AIR) {
                    UCH_SET_PIXEL(TexturePack_.pixels_ar, offset, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
                } else {
                    UInt32 color = GameIsolate_.world->tiles[i].color;
                    UInt32 emit = GameIsolate_.world->tiles[i].mat->emitColor;
                    UCH_SET_PIXEL(TexturePack_.pixels_ar, offset, (color >> 0) & 0xff,
                                  (color >> 8) & 0xff, (color >> 16) & 0xff,
                                  GameIsolate_.world->tiles[i].mat->alpha);
                    UCH_SET_PIXEL(TexturePack_.pixelsEmission_ar, offset, (emit >> 0) & 0xff,
                                  (emit >> 8) & 0xff, (emit >> 16) & 0xff, (emit >> 24) & 0xff);
                }
            }

            if (GameIsolate_.world->layer2Dirty[i]) {
                if (GameIsolate_.world->layer2[i].mat->physicsType == PhysicsType::AIR) {
                    if (Settings::draw_background_grid) {
                        UInt32 color = ((i) % 2) == 0 ? 0x888888 : 0x444444;
                        UCH_SET_PIXEL(TexturePack_.pixelsLayer2_ar, offset, (color >> 0) & 0xff,
                                      (color >> 8) & 0xff, (color >> 16) & 0xff, SDL_ALPHA_OPAQUE);
                    } else {
                        UCH_SET_PIXEL(TexturePack_.pixelsLayer2_ar, offset, 0, 0, 0,
                                      SDL_ALPHA_TRANSPARENT);
                    }
                    continue;
                }
                UInt32 color = GameIsolate_.world->layer2[i].color;
                UCH_SET_PIXEL(TexturePack_.pixelsLayer2_ar, offset, (color >> 0) & 0xff,
                              (color >> 8) & 0xff, (color >> 16) & 0xff,
                              GameIsolate_.world->layer2[i].mat->alpha);
            }

            if (GameIsolate_.world->backgroundDirty[i]) {
                UInt32 color = GameIsolate_.world->background[i];
                UCH_SET_PIXEL(TexturePack_.pixelsBackground_ar, offset, (color >> 0) & 0xff,
                              (color >> 8) & 0xff, (color >> 16) & 0xff, (color >> 24) & 0xff);
            }
#undef UCH_SET_PIXEL
        }

        memset(GameIsolate_.world->dirty, false,
               (size_t) GameIsolate_.world->width * GameIsolate_.world->height);
        memset(GameIsolate_.world->layer2Dirty, false,
               (size_t) GameIsolate_.world->width * GameIsolate_.world->height);
        memset(GameIsolate_.world->backgroundDirty, false,
               (size_t) GameIsolate_.world->width * GameIsolate_.world->height);

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
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixels_ar[0]),
                                &(TexturePack_.pixels_ar[GameIsolate_.world->width *
                                                         GameIsolate_.world->height * 4]) -
                                        delta,
                                &(TexturePack_.pixels_ar[GameIsolate_.world->width *
                                                         GameIsolate_.world->height * 4]));
                    //rotate(pixels.begin(), pixels.end() - delta, pixels.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsLayer2_ar[0]),
                                &(TexturePack_.pixelsLayer2_ar[GameIsolate_.world->width *
                                                               GameIsolate_.world->height * 4]) -
                                        delta,
                                &(TexturePack_.pixelsLayer2_ar[GameIsolate_.world->width *
                                                               GameIsolate_.world->height * 4]));
                    //rotate(pixelsLayer2.begin(), pixelsLayer2.end() - delta, pixelsLayer2.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(
                            &(TexturePack_.pixelsBackground_ar[0]),
                            &(TexturePack_.pixelsBackground_ar[GameIsolate_.world->width *
                                                               GameIsolate_.world->height * 4]) -
                                    delta,
                            &(TexturePack_.pixelsBackground_ar[GameIsolate_.world->width *
                                                               GameIsolate_.world->height * 4]));
                    //rotate(pixelsBackground.begin(), pixelsBackground.end() - delta, pixelsBackground.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsFire_ar[0]),
                                &(TexturePack_.pixelsFire_ar[GameIsolate_.world->width *
                                                             GameIsolate_.world->height * 4]) -
                                        delta,
                                &(TexturePack_.pixelsFire_ar[GameIsolate_.world->width *
                                                             GameIsolate_.world->height * 4]));
                    //rotate(pixelsFire_ar.begin(), pixelsFire_ar.end() - delta, pixelsFire_ar.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsFlow_ar[0]),
                                &(TexturePack_.pixelsFlow_ar[GameIsolate_.world->width *
                                                             GameIsolate_.world->height * 4]) -
                                        delta,
                                &(TexturePack_.pixelsFlow_ar[GameIsolate_.world->width *
                                                             GameIsolate_.world->height * 4]));
                    //rotate(pixelsFlow_ar.begin(), pixelsFlow_ar.end() - delta, pixelsFlow_ar.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsEmission_ar[0]),
                                &(TexturePack_.pixelsEmission_ar[GameIsolate_.world->width *
                                                                 GameIsolate_.world->height * 4]) -
                                        delta,
                                &(TexturePack_.pixelsEmission_ar[GameIsolate_.world->width *
                                                                 GameIsolate_.world->height * 4]));
                    //rotate(pixelsEmission_ar.begin(), pixelsEmission_ar.end() - delta, pixelsEmission_ar.end());
                }));
            } else if (delta < 0) {
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixels_ar[0]), &(TexturePack_.pixels_ar[0]) - delta,
                                &(TexturePack_.pixels_ar[GameIsolate_.world->width *
                                                         GameIsolate_.world->height * 4]));
                    //rotate(pixels.begin(), pixels.begin() - delta, pixels.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsLayer2_ar[0]),
                                &(TexturePack_.pixelsLayer2_ar[0]) - delta,
                                &(TexturePack_.pixelsLayer2_ar[GameIsolate_.world->width *
                                                               GameIsolate_.world->height * 4]));
                    //rotate(pixelsLayer2.begin(), pixelsLayer2.begin() - delta, pixelsLayer2.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(
                            &(TexturePack_.pixelsBackground_ar[0]),
                            &(TexturePack_.pixelsBackground_ar[0]) - delta,
                            &(TexturePack_.pixelsBackground_ar[GameIsolate_.world->width *
                                                               GameIsolate_.world->height * 4]));
                    //rotate(pixelsBackground.begin(), pixelsBackground.begin() - delta, pixelsBackground.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsFire_ar[0]),
                                &(TexturePack_.pixelsFire_ar[0]) - delta,
                                &(TexturePack_.pixelsFire_ar[GameIsolate_.world->width *
                                                             GameIsolate_.world->height * 4]));
                    //rotate(pixelsFire_ar.begin(), pixelsFire_ar.begin() - delta, pixelsFire_ar.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsFlow_ar[0]),
                                &(TexturePack_.pixelsFlow_ar[0]) - delta,
                                &(TexturePack_.pixelsFlow_ar[GameIsolate_.world->width *
                                                             GameIsolate_.world->height * 4]));
                    //rotate(pixelsFlow_ar.begin(), pixelsFlow_ar.begin() - delta, pixelsFlow_ar.end());
                }));
                results.push_back(updateDirtyPool->push([&](int id) {
                    std::rotate(&(TexturePack_.pixelsEmission_ar[0]),
                                &(TexturePack_.pixelsEmission_ar[0]) - delta,
                                &(TexturePack_.pixelsEmission_ar[GameIsolate_.world->width *
                                                                 GameIsolate_.world->height * 4]));
                    //rotate(pixelsEmission_ar.begin(), pixelsEmission_ar.begin() - delta, pixelsEmission_ar.end());
                }));
            }

            for (auto &v: results) { v.get(); }

#define CLEARPIXEL(pixels, ofs)                                                                    \
    pixels[ofs + 0] = pixels[ofs + 1] = pixels[ofs + 2] = 0xff;                                    \
    pixels[ofs + 3] = SDL_ALPHA_TRANSPARENT

#define CLEARPIXEL_C(offset)                                                                       \
    CLEARPIXEL(TexturePack_.pixels_ar, offset);                                                    \
    CLEARPIXEL(TexturePack_.pixelsLayer2_ar, offset);                                              \
    CLEARPIXEL(TexturePack_.pixelsObjects_ar, offset);                                             \
    CLEARPIXEL(TexturePack_.pixelsBackground_ar, offset);                                          \
    CLEARPIXEL(TexturePack_.pixelsFire_ar, offset);                                                \
    CLEARPIXEL(TexturePack_.pixelsFlow_ar, offset);                                                \
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

            GameData_.ofsX -= subX * scale;
            GameData_.ofsY -= subY * scale;
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

    if (GameIsolate_.world->player) {
        if (Controls::PLAYER_UP->get() && !Controls::DEBUG_DRAW->get()) {
            if (GameIsolate_.world->player->ground) {
                GameIsolate_.world->player->vy = -4;
                global.audioEngine.PlayEvent("event:/Player/Jump");
            }
        }

        GameIsolate_.world->player->vy +=
                (float) (((Controls::PLAYER_UP->get() && !Controls::DEBUG_DRAW->get())
                                  ? (GameIsolate_.world->player->vy > -1 ? -0.8 : -0.35)
                                  : 0) +
                         (Controls::PLAYER_DOWN->get() ? 0.1 : 0));
        if (Controls::PLAYER_UP->get() && !Controls::DEBUG_DRAW->get()) {
            global.audioEngine.SetEventParameter("event:/Player/Fly", "Intensity", 1);
            for (int i = 0; i < 4; i++) {
                Particle *p = new Particle(
                        Tiles::createLava(),
                        (float) (GameIsolate_.world->player->x + GameIsolate_.world->loadZone.x +
                                 GameIsolate_.world->player->hw / 2 + rand() % 5 - 2 +
                                 GameIsolate_.world->player->vx),
                        (float) (GameIsolate_.world->player->y + GameIsolate_.world->loadZone.y +
                                 GameIsolate_.world->player->hh + GameIsolate_.world->player->vy),
                        (float) ((rand() % 10 - 5) / 10.0f + GameIsolate_.world->player->vx / 2.0f),
                        (float) ((rand() % 10) / 10.0f + 1 + GameIsolate_.world->player->vy / 2.0f),
                        0, (float) 0.025);
                p->temporary = true;
                p->lifetime = 120;
                GameIsolate_.world->addParticle(p);
            }
        } else {
            global.audioEngine.SetEventParameter("event:/Player/Fly", "Intensity", 0);
        }

        if (GameIsolate_.world->player->vy > 0) {
            global.audioEngine.SetEventParameter("event:/Player/Wind", "Wind",
                                                 (float) (GameIsolate_.world->player->vy / 12.0));
        } else {
            global.audioEngine.SetEventParameter("event:/Player/Wind", "Wind", 0);
        }

        GameIsolate_.world->player->vx +=
                (float) ((Controls::PLAYER_LEFT->get()
                                  ? (GameIsolate_.world->player->vx > 0 ? -0.4 : -0.2)
                                  : 0) +
                         (Controls::PLAYER_RIGHT->get()
                                  ? (GameIsolate_.world->player->vx < 0 ? 0.4 : 0.2)
                                  : 0));
        if (!Controls::PLAYER_LEFT->get() && !Controls::PLAYER_RIGHT->get())
            GameIsolate_.world->player->vx *=
                    (float) (GameIsolate_.world->player->ground ? 0.85 : 0.96);
        if (GameIsolate_.world->player->vx > 4.5) GameIsolate_.world->player->vx = 4.5;
        if (GameIsolate_.world->player->vx < -4.5) GameIsolate_.world->player->vx = -4.5;
    } else {
        if (state == INGAME) {
            GameData_.freeCamX += (float) ((Controls::PLAYER_LEFT->get() ? -5 : 0) +
                                           (Controls::PLAYER_RIGHT->get() ? 5 : 0));
            GameData_.freeCamY += (float) ((Controls::PLAYER_UP->get() ? -5 : 0) +
                                           (Controls::PLAYER_DOWN->get() ? 5 : 0));
        } else {
        }
    }

    if (GameIsolate_.world->player) {
        GameData_.desCamX = (float) (-(mx - (global.platform.WIDTH / 2)) / 4);
        GameData_.desCamY = (float) (-(my - (global.platform.HEIGHT / 2)) / 4);

        GameIsolate_.world->player->holdAngle =
                (float) (atan2(GameData_.desCamY, GameData_.desCamX) * 180 / (float) M_PI);

        GameData_.desCamX = 0;
        GameData_.desCamY = 0;
    } else {
        GameData_.desCamX = 0;
        GameData_.desCamY = 0;
    }

    if (GameIsolate_.world->player) {
        if (GameIsolate_.world->player->heldItem) {
            if (GameIsolate_.world->player->heldItem->getFlag(ItemFlags::VACUUM)) {
                if (GameIsolate_.world->player->holdVacuum) {

                    int wcx = (int) ((global.platform.WIDTH / 2.0f - GameData_.ofsX -
                                      GameData_.camX) /
                                     scale);
                    int wcy = (int) ((global.platform.HEIGHT / 2.0f - GameData_.ofsY -
                                      GameData_.camY) /
                                     scale);

                    int wmx = (int) ((mx - GameData_.ofsX - GameData_.camX) / scale);
                    int wmy = (int) ((my - GameData_.ofsY - GameData_.camY) / scale);

                    int mdx = wmx - wcx;
                    int mdy = wmy - wcy;

                    int distSq = mdx * mdx + mdy * mdy;
                    if (distSq <= 256 * 256) {

                        int sind = -1;
                        bool inObject = true;
                        GameIsolate_.world->forLine(wcx, wcy, wmx, wmy, [&](int ind) {
                            if (GameIsolate_.world->tiles[ind].mat->physicsType ==
                                PhysicsType::OBJECT) {
                                if (!inObject) {
                                    sind = ind;
                                    return true;
                                }
                            } else {
                                inObject = false;
                            }

                            if (GameIsolate_.world->tiles[ind].mat->physicsType ==
                                        PhysicsType::SOLID ||
                                GameIsolate_.world->tiles[ind].mat->physicsType ==
                                        PhysicsType::SAND ||
                                GameIsolate_.world->tiles[ind].mat->physicsType ==
                                        PhysicsType::SOUP) {
                                sind = ind;
                                return true;
                            }
                            return false;
                        });

                        int x = sind == -1 ? wmx : sind % GameIsolate_.world->width;
                        int y = sind == -1 ? wmy : sind / GameIsolate_.world->width;

                        std::function<void(MaterialInstance, int, int)> makeParticle =
                                [&](MaterialInstance tile, int xPos, int yPos) {
                                    Particle *par =
                                            new Particle(tile, xPos, yPos, 0, 0, 0, (float) 0.01f);
                                    par->vx = (rand() % 10 - 5) / 5.0f * 1.0f;
                                    par->vy = (rand() % 10 - 5) / 5.0f * 1.0f;
                                    par->ax = -par->vx / 10.0f;
                                    par->ay = -par->vy / 10.0f;
                                    if (par->ay == 0 && par->ax == 0) par->ay = 0.01f;

                                    //par->targetX = GameIsolate_.world->player->x + GameIsolate_.world->player->hw / 2 + GameIsolate_.world->loadZone.x;
                                    //par->targetY = GameIsolate_.world->player->y + GameIsolate_.world->player->hh / 2 + GameIsolate_.world->loadZone.y;
                                    //par->targetForce = 0.35f;

                                    par->lifetime = 6;

                                    par->phase = true;

                                    GameIsolate_.world->player->heldItem->vacuumParticles.push_back(
                                            par);

                                    par->killCallback = [&]() {
                                        auto &v = GameIsolate_.world->player->heldItem
                                                          ->vacuumParticles;
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

                                MaterialInstance tile =
                                        GameIsolate_.world
                                                ->tiles[(x + xx) +
                                                        (y + yy) * GameIsolate_.world->width];
                                if (tile.mat->physicsType == PhysicsType::SOLID ||
                                    tile.mat->physicsType == PhysicsType::SAND ||
                                    tile.mat->physicsType == PhysicsType::SOUP) {
                                    makeParticle(tile, x + xx, y + yy);
                                    GameIsolate_.world
                                            ->tiles[(x + xx) +
                                                    (y + yy) * GameIsolate_.world->width] =
                                            Tiles::NOTHING;
                                    //GameIsolate_.world->tiles[(x + xx) + (y + yy) * GameIsolate_.world->width] = Tiles::createFire();
                                    GameIsolate_.world
                                            ->dirty[(x + xx) +
                                                    (y + yy) * GameIsolate_.world->width] = true;
                                }
                            }
                        }

                        GameIsolate_.world->particles.erase(
                                std::remove_if(
                                        GameIsolate_.world->particles.begin(),
                                        GameIsolate_.world->particles.end(),
                                        [&](Particle *cur) {
                                            if (cur->targetForce == 0 && !cur->phase) {
                                                int rad = 5;
                                                for (int xx = -rad; xx <= rad; xx++) {
                                                    for (int yy = -rad; yy <= rad; yy++) {
                                                        if ((yy == -rad || yy == rad) &&
                                                            (xx == -rad || x == rad))
                                                            continue;

                                                        if (((int) (cur->x) == (x + xx)) &&
                                                            ((int) (cur->y) == (y + yy))) {

                                                            cur->vx =
                                                                    (rand() % 10 - 5) / 5.0f * 1.0f;
                                                            cur->vy =
                                                                    (rand() % 10 - 5) / 5.0f * 1.0f;
                                                            cur->ax = -cur->vx / 10.0f;
                                                            cur->ay = -cur->vy / 10.0f;
                                                            if (cur->ay == 0 && cur->ax == 0)
                                                                cur->ay = 0.01f;

                                                            //par->targetX = GameIsolate_.world->player->x + GameIsolate_.world->player->hw / 2 + GameIsolate_.world->loadZone.x;
                                                            //par->targetY = GameIsolate_.world->player->y + GameIsolate_.world->player->hh / 2 + GameIsolate_.world->loadZone.y;
                                                            //par->targetForce = 0.35f;

                                                            cur->lifetime = 6;

                                                            cur->phase = true;

                                                            GameIsolate_.world->player->heldItem
                                                                    ->vacuumParticles.push_back(
                                                                            cur);

                                                            cur->killCallback = [&]() {
                                                                auto &v = GameIsolate_.world->player
                                                                                  ->heldItem
                                                                                  ->vacuumParticles;
                                                                v.erase(std::remove(v.begin(),
                                                                                    v.end(), cur),
                                                                        v.end());
                                                            };

                                                            return false;
                                                        }
                                                    }
                                                }
                                            }

                                            return false;
                                        }),
                                GameIsolate_.world->particles.end());

                        std::vector<RigidBody *> rbs = GameIsolate_.world->rigidBodies;

                        for (size_t i = 0; i < rbs.size(); i++) {
                            RigidBody *cur = rbs[i];
                            if (!static_cast<bool>(cur->surface)) continue;
                            if (cur->body->IsEnabled()) {
                                float s = sin(-cur->body->GetAngle());
                                float c = cos(-cur->body->GetAngle());
                                bool upd = false;
                                for (int xx = -rad; xx <= rad; xx++) {
                                    for (int yy = -rad; yy <= rad; yy++) {
                                        if ((yy == -rad || yy == rad) && (xx == -rad || x == rad))
                                            continue;
                                        // rotate point

                                        float tx = x + xx - cur->body->GetPosition().x;
                                        float ty = y + yy - cur->body->GetPosition().y;

                                        int ntx = (int) (tx * c - ty * s);
                                        int nty = (int) (tx * s + ty * c);

                                        if (ntx >= 0 && nty >= 0 && ntx < cur->surface->w &&
                                            nty < cur->surface->h) {
                                            UInt32 pixel =
                                                    METADOT_GET_PIXEL(cur->surface, ntx, nty);
                                            if (((pixel >> 24) & 0xff) != 0x00) {
                                                METADOT_GET_PIXEL(cur->surface, ntx, nty) =
                                                        0x00000000;
                                                upd = true;

                                                makeParticle(
                                                        MaterialInstance(&Materials::GENERIC_SOLID,
                                                                         pixel),
                                                        (x + xx), (y + yy));
                                            }
                                        }
                                    }
                                }

                                if (upd) {
                                    METAENGINE_Render_FreeImage(cur->texture);
                                    cur->texture =
                                            METAENGINE_Render_CopyImageFromSurface(cur->surface);
                                    METAENGINE_Render_SetImageFilter(
                                            cur->texture, METAENGINE_Render_FILTER_NEAREST);
                                    //GameIsolate_.world->updateRigidBodyHitbox(cur);
                                    cur->needsUpdate = true;
                                }
                            }
                        }
                    }
                }

                if (GameIsolate_.world->player->heldItem->vacuumParticles.size() > 0) {
                    GameIsolate_.world->player->heldItem->vacuumParticles.erase(
                            std::remove_if(
                                    GameIsolate_.world->player->heldItem->vacuumParticles.begin(),
                                    GameIsolate_.world->player->heldItem->vacuumParticles.end(),
                                    [&](Particle *cur) {
                                        if (cur->lifetime <= 0) {
                                            cur->targetForce = 0.45f;
                                            cur->targetX = GameIsolate_.world->player->x +
                                                           GameIsolate_.world->player->hw / 2.0f +
                                                           GameIsolate_.world->loadZone.x;
                                            cur->targetY = GameIsolate_.world->player->y +
                                                           GameIsolate_.world->player->hh / 2.0f +
                                                           GameIsolate_.world->loadZone.y;
                                            cur->ax = 0;
                                            cur->ay = 0.01f;
                                        }

                                        float tdx = cur->targetX - cur->x;
                                        float tdy = cur->targetY - cur->y;

                                        if (tdx * tdx + tdy * tdy < 10 * 10) {
                                            cur->temporary = true;
                                            cur->lifetime = 0;
                                            //METADOT_BUG("vacuum {}", cur->tile.mat->name.c_str());
                                            return true;
                                        }

                                        return false;
                                    }),
                            GameIsolate_.world->player->heldItem->vacuumParticles.end());
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

        if (GameIsolate_.world->player) {

            if (GameIsolate_.game_timestate.now - GameIsolate_.game_timestate.lastTick <=
                GameIsolate_.game_timestate.mspt) {
                float thruTick = (float) ((GameIsolate_.game_timestate.now -
                                           GameIsolate_.game_timestate.lastTick) /
                                          (double) GameIsolate_.game_timestate.mspt);

                GameData_.plPosX = GameIsolate_.world->player->x +
                                   (int) (GameIsolate_.world->player->vx * thruTick);
                GameData_.plPosY = GameIsolate_.world->player->y +
                                   (int) (GameIsolate_.world->player->vy * thruTick);
            } else {
                //plPosX = GameIsolate_.world->player->x;
                //plPosY = GameIsolate_.world->player->y;
            }

            //plPosX = (float)(plPosX + (GameIsolate_.world->player->x - plPosX) / 25.0);
            //plPosY = (float)(plPosY + (GameIsolate_.world->player->y - plPosY) / 25.0);

            nofsX = (int) (-((int) GameData_.plPosX + GameIsolate_.world->player->hw / 2 +
                             GameIsolate_.world->loadZone.x) *
                                   scale +
                           global.platform.WIDTH / 2);
            nofsY = (int) (-((int) GameData_.plPosY + GameIsolate_.world->player->hh / 2 +
                             GameIsolate_.world->loadZone.y) *
                                   scale +
                           global.platform.HEIGHT / 2);
        } else {
            GameData_.plPosX =
                    (float) (GameData_.plPosX + (GameData_.freeCamX - GameData_.plPosX) / 50.0f);
            GameData_.plPosY =
                    (float) (GameData_.plPosY + (GameData_.freeCamY - GameData_.plPosY) / 50.0f);

            nofsX = (int) (-(GameData_.plPosX + 0 + GameIsolate_.world->loadZone.x) * scale +
                           global.platform.WIDTH / 2.0f);
            nofsY = (int) (-(GameData_.plPosY + 0 + GameIsolate_.world->loadZone.y) * scale +
                           global.platform.HEIGHT / 2.0f);
        }

        accLoadX += (nofsX - GameData_.ofsX) / (float) scale;
        accLoadY += (nofsY - GameData_.ofsY) / (float) scale;
        //METADOT_BUG("{0:f} {0:f}", plPosX, plPosY);
        //METADOT_BUG("a {0:d} {0:d}", nofsX, nofsY);
        //METADOT_BUG("{0:d} {0:d}", nofsX - ofsX, nofsY - ofsY);
        GameData_.ofsX += (nofsX - GameData_.ofsX);
        GameData_.ofsY += (nofsY - GameData_.ofsY);

        GameData_.camX = (float) (GameData_.camX + (GameData_.desCamX - GameData_.camX) *
                                                           (GameIsolate_.game_timestate.now -
                                                            GameIsolate_.game_timestate.lastTime) /
                                                           250.0f);
        GameData_.camY = (float) (GameData_.camY + (GameData_.desCamY - GameData_.camY) *
                                                           (GameIsolate_.game_timestate.now -
                                                            GameIsolate_.game_timestate.lastTime) /
                                                           250.0f);
    }
}

void Game::renderEarly() {

    global.ImGuiCore->begin();

    if (state == LOADING) {
        if (GameIsolate_.game_timestate.now - GameIsolate_.game_timestate.lastLoadingTick > 20) {
            // render loading screen

            unsigned int *ldPixels = (unsigned int *) TexturePack_.pixelsLoading_ar;
            bool anyFalse = false;
            //int drop  = (sin(now / 250.0) + 1) / 2 * loadingScreenW;
            //int drop2 = (-sin(now / 250.0) + 1) / 2 * loadingScreenW;
            for (int x = 0; x < TexturePack_.loadingScreenW; x++) {
                for (int y = TexturePack_.loadingScreenH - 1; y >= 0; y--) {
                    int i = (x + y * TexturePack_.loadingScreenW);
                    bool state = ldPixels[i] == loadingOnColor;

                    if (!state) anyFalse = true;
                    bool newState = state;
                    //newState = rand() % 2;

                    if (!state && y == 0) {
                        if (rand() % 6 == 0) { newState = true; }
                        /*if (x >= drop - 1 && x <= drop + 1) {
                            newState = true;
                        }else if (x >= drop2 - 1 && x <= drop2 + 1) {
                            newState = true;
                        }*/
                    }

                    if (state && y < TexturePack_.loadingScreenH - 1) {
                        if (ldPixels[(x + (y + 1) * TexturePack_.loadingScreenW)] ==
                            loadingOffColor) {
                            ldPixels[(x + (y + 1) * TexturePack_.loadingScreenW)] = loadingOnColor;
                            newState = false;
                        } else {
                            bool canLeft =
                                    x > 0 &&
                                    ldPixels[((x - 1) + (y + 1) * TexturePack_.loadingScreenW)] ==
                                            loadingOffColor;
                            bool canRight =
                                    x < TexturePack_.loadingScreenW - 1 &&
                                    ldPixels[((x + 1) + (y + 1) * TexturePack_.loadingScreenW)] ==
                                            loadingOffColor;
                            if (canLeft && !(canRight && (rand() % 2 == 0))) {
                                ldPixels[((x - 1) + (y + 1) * TexturePack_.loadingScreenW)] =
                                        loadingOnColor;
                                newState = false;
                            } else if (canRight) {
                                ldPixels[((x + 1) + (y + 1) * TexturePack_.loadingScreenW)] =
                                        loadingOnColor;
                                newState = false;
                            }
                        }
                    }

                    ldPixels[(x + y * TexturePack_.loadingScreenW)] =
                            (newState ? loadingOnColor : loadingOffColor);
                    int sx = global.platform.WIDTH / TexturePack_.loadingScreenW;
                    int sy = global.platform.HEIGHT / TexturePack_.loadingScreenH;
                    //METAENGINE_Render_RectangleFilled(target, x * sx, y * sy, x * sx + sx, y * sy + sy, state ? SDL_Color{ 0xff, 0, 0, 0xff } : SDL_Color{ 0, 0xff, 0, 0xff });
                }
            }
            if (!anyFalse) {
                uint32 tmp = loadingOnColor;
                loadingOnColor = loadingOffColor;
                loadingOffColor = tmp;
            }

            METAENGINE_Render_UpdateImageBytes(TexturePack_.loadingTexture, NULL,
                                               &TexturePack_.pixelsLoading_ar[0],
                                               TexturePack_.loadingScreenW * 4);

            GameIsolate_.game_timestate.lastLoadingTick = GameIsolate_.game_timestate.now;
        } else {
            //#ifdef _WIN32
            //            Sleep(5);
            //#else
            //            sleep(5 / 1000.0f);
            //#endif
        }
        METAENGINE_Render_ActivateShaderProgram(0, NULL);
        METAENGINE_Render_BlitRect(TexturePack_.loadingTexture, NULL, RenderTarget_.target, NULL);
        Drawing::drawText("loading", "Loading...", global.platform.WIDTH / 2, global.platform.HEIGHT / 2 - 32);
    } else {
        // render entities with LERP

        if (GameIsolate_.game_timestate.now - GameIsolate_.game_timestate.lastTick <=
            GameIsolate_.game_timestate.mspt) {
            METAENGINE_Render_Clear(TexturePack_.textureEntities->target);
            METAENGINE_Render_Clear(TexturePack_.textureEntitiesLQ->target);
            if (GameIsolate_.world->player) {
                float thruTick = (float) ((GameIsolate_.game_timestate.now -
                                           GameIsolate_.game_timestate.lastTick) /
                                          (double) GameIsolate_.game_timestate.mspt);

                METAENGINE_Render_SetBlendMode(TexturePack_.textureEntities,
                                               METAENGINE_Render_BLEND_ADD);
                METAENGINE_Render_SetBlendMode(TexturePack_.textureEntitiesLQ,
                                               METAENGINE_Render_BLEND_ADD);
                int scaleEnt = Settings::hd_objects ? Settings::hd_objects_size : 1;

                for (auto &v: GameIsolate_.world->entities) {
                    v->renderLQ(TexturePack_.textureEntitiesLQ->target,
                                GameIsolate_.world->loadZone.x + (int) (v->vx * thruTick),
                                GameIsolate_.world->loadZone.y + (int) (v->vy * thruTick));
                    v->render(TexturePack_.textureEntities->target,
                              GameIsolate_.world->loadZone.x + (int) (v->vx * thruTick),
                              GameIsolate_.world->loadZone.y + (int) (v->vy * thruTick));
                }

                if (GameIsolate_.world->player && GameIsolate_.world->player->heldItem != NULL) {
                    if (GameIsolate_.world->player->heldItem->getFlag(ItemFlags::HAMMER)) {
                        if (GameIsolate_.world->player->holdHammer) {
                            int x = (int) ((mx - GameData_.ofsX - GameData_.camX) / scale);
                            int y = (int) ((my - GameData_.ofsY - GameData_.camY) / scale);

                            int dx = x - GameIsolate_.world->player->hammerX;
                            int dy = y - GameIsolate_.world->player->hammerY;
                            float len = sqrt(dx * dx + dy * dy);
                            if (len > 40) {
                                dx = dx / len * 40;
                                dy = dy / len * 40;
                            }

                            METAENGINE_Render_Line(TexturePack_.textureEntitiesLQ->target,
                                                   GameIsolate_.world->player->hammerX + dx,
                                                   GameIsolate_.world->player->hammerY + dy,
                                                   GameIsolate_.world->player->hammerX,
                                                   GameIsolate_.world->player->hammerY,
                                                   {0xff, 0xff, 0x00, 0xff});
                        } else {
                            int startInd = getAimSolidSurface(64);

                            if (startInd != -1) {
                                int x = startInd % GameIsolate_.world->width;
                                int y = startInd / GameIsolate_.world->width;
                                METAENGINE_Render_Rectangle(TexturePack_.textureEntitiesLQ->target,
                                                            x - 1, y - 1, x + 1, y + 1,
                                                            {0xff, 0xff, 0x00, 0xE0});
                            }
                        }
                    }
                }
                METAENGINE_Render_SetBlendMode(TexturePack_.textureEntities,
                                               METAENGINE_Render_BLEND_NORMAL);
                METAENGINE_Render_SetBlendMode(TexturePack_.textureEntitiesLQ,
                                               METAENGINE_Render_BLEND_NORMAL);
            }
        }

        if (Controls::mmouse) {
            int x = (int) ((mx - GameData_.ofsX - GameData_.camX) / scale);
            int y = (int) ((my - GameData_.ofsY - GameData_.camY) / scale);
            METAENGINE_Render_RectangleFilled(
                    TexturePack_.textureEntitiesLQ->target,
                    x - GameUI::DebugDrawUI::brushSize / 2.0f,
                    y - GameUI::DebugDrawUI::brushSize / 2.0f,
                    x + (int) (ceil(GameUI::DebugDrawUI::brushSize / 2.0)),
                    y + (int) (ceil(GameUI::DebugDrawUI::brushSize / 2.0)),
                    {0xff, 0x40, 0x40, 0x90});
            METAENGINE_Render_Rectangle(TexturePack_.textureEntitiesLQ->target,
                                        x - GameUI::DebugDrawUI::brushSize / 2.0f,
                                        y - GameUI::DebugDrawUI::brushSize / 2.0f,
                                        x + (int) (ceil(GameUI::DebugDrawUI::brushSize / 2.0)) + 1,
                                        y + (int) (ceil(GameUI::DebugDrawUI::brushSize / 2.0)) + 1,
                                        {0xff, 0x40, 0x40, 0xE0});
        } else if (Controls::DEBUG_DRAW->get()) {
            int x = (int) ((mx - GameData_.ofsX - GameData_.camX) / scale);
            int y = (int) ((my - GameData_.ofsY - GameData_.camY) / scale);
            METAENGINE_Render_RectangleFilled(
                    TexturePack_.textureEntitiesLQ->target,
                    x - GameUI::DebugDrawUI::brushSize / 2.0f,
                    y - GameUI::DebugDrawUI::brushSize / 2.0f,
                    x + (int) (ceil(GameUI::DebugDrawUI::brushSize / 2.0)),
                    y + (int) (ceil(GameUI::DebugDrawUI::brushSize / 2.0)),
                    {0x00, 0xff, 0xB0, 0x80});
            METAENGINE_Render_Rectangle(TexturePack_.textureEntitiesLQ->target,
                                        x - GameUI::DebugDrawUI::brushSize / 2.0f,
                                        y - GameUI::DebugDrawUI::brushSize / 2.0f,
                                        x + (int) (ceil(GameUI::DebugDrawUI::brushSize / 2.0)) + 1,
                                        y + (int) (ceil(GameUI::DebugDrawUI::brushSize / 2.0)) + 1,
                                        {0x00, 0xff, 0xB0, 0xE0});
        }
    }
}

void Game::renderLate() {

    RenderTarget_.target = TexturePack_.backgroundImage->target;
    METAENGINE_Render_Clear(RenderTarget_.target);

    if (state == LOADING) {

    } else {
        // draw backgrounds

        Background *bg = GameIsolate_.backgrounds->Get("TEST_OVERWORLD");
        if (Settings::draw_background && scale <= bg->layers[0].surface.size() &&
            GameIsolate_.world->loadZone.y > -5 * CHUNK_H) {
            METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_SET);
            SDL_Color col = {static_cast<UInt8>((bg->solid >> 16) & 0xff),
                             static_cast<UInt8>((bg->solid >> 8) & 0xff),
                             static_cast<UInt8>((bg->solid >> 0) & 0xff), 0xff};
            METAENGINE_Render_ClearColor(RenderTarget_.target, col);

            METAENGINE_Render_Rect *dst = nullptr;
            METAENGINE_Render_Rect *src = nullptr;
            METADOT_NEW(C, dst, METAENGINE_Render_Rect);
            METADOT_NEW(C, src, METAENGINE_Render_Rect);

            float arX = (float) global.platform.WIDTH / (bg->layers[0].surface[0]->w);
            float arY = (float) global.platform.HEIGHT / (bg->layers[0].surface[0]->h);

            double time = UTime::millis() / 1000.0;

            METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);

            for (size_t i = 0; i < bg->layers.size(); i++) {
                BackgroundLayer cur = bg->layers[i];

                C_Surface *texture = cur.surface[(size_t) scale - 1];

                METAENGINE_Render_Image *tex = cur.texture[(size_t) scale - 1];
                METAENGINE_Render_SetBlendMode(tex, METAENGINE_Render_BLEND_NORMAL);

                int tw = texture->w;
                int th = texture->h;

                int iter = (int) ceil((float) global.platform.WIDTH / (tw)) + 1;
                for (int n = 0; n < iter; n++) {

                    src->x = 0;
                    src->y = 0;
                    src->w = tw;
                    src->h = th;

                    dst->x = (((GameData_.ofsX + GameData_.camX) +
                               GameIsolate_.world->loadZone.x * scale) +
                              n * tw / cur.parralaxX) *
                                     cur.parralaxX +
                             GameIsolate_.world->width / 2.0f * scale - tw / 2.0f;
                    dst->y = ((GameData_.ofsY + GameData_.camY) +
                              GameIsolate_.world->loadZone.y * scale) *
                                     cur.parralaxY +
                             GameIsolate_.world->height / 2.0f * scale - th / 2.0f -
                             global.platform.HEIGHT / 3.0f * (scale - 1);
                    dst->w = (float) tw;
                    dst->h = (float) th;

                    dst->x += (float) (scale * fmod(cur.moveX * time, tw));

                    //TODO: optimize
                    while (dst->x >= global.platform.WIDTH - 10) dst->x -= (iter * tw);
                    while (dst->x + dst->w < 0) dst->x += (iter * tw - 1);

                    //TODO: optimize
                    if (dst->x < 0) {
                        dst->w += dst->x;
                        src->x -= (int) dst->x;
                        src->w += (int) dst->x;
                        dst->x = 0;
                    }

                    if (dst->y < 0) {
                        dst->h += dst->y;
                        src->y -= (int) dst->y;
                        src->h += (int) dst->y;
                        dst->y = 0;
                    }

                    if (dst->x + dst->w >= global.platform.WIDTH) {
                        src->w -= (int) ((dst->x + dst->w) - global.platform.WIDTH);
                        dst->w += global.platform.WIDTH - (dst->x + dst->w);
                    }

                    if (dst->y + dst->h >= global.platform.HEIGHT) {
                        src->h -= (int) ((dst->y + dst->h) - global.platform.HEIGHT);
                        dst->h += global.platform.HEIGHT - (dst->y + dst->h);
                    }

                    METAENGINE_Render_BlitRect(tex, src, RenderTarget_.target, dst);
                }
            }

            METADOT_DELETE(C, dst, METAENGINE_Render_Rect);
            METADOT_DELETE(C, src, METAENGINE_Render_Rect);
        }

        METAENGINE_Render_Rect r1 =
                METAENGINE_Render_Rect{(float) (GameData_.ofsX + GameData_.camX),
                                       (float) (GameData_.ofsY + GameData_.camY),
                                       (float) (GameIsolate_.world->width * scale),
                                       (float) (GameIsolate_.world->height * scale)};
        METAENGINE_Render_SetBlendMode(TexturePack_.textureBackground,
                                       METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(TexturePack_.textureBackground, NULL, RenderTarget_.target, &r1);

        METAENGINE_Render_SetBlendMode(TexturePack_.textureLayer2, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(TexturePack_.textureLayer2, NULL, RenderTarget_.target, &r1);

        METAENGINE_Render_SetBlendMode(TexturePack_.textureObjectsBack,
                                       METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(TexturePack_.textureObjectsBack, NULL, RenderTarget_.target,
                                   &r1);

        // shader

        if (Settings::draw_shaders) {

            if (global.shaderworker.waterFlowPassShader->dirty && Settings::water_showFlow) {

                global.shaderworker.waterFlowPassShader->activate();
                global.shaderworker.waterFlowPassShader->update(GameIsolate_.world->width,
                                                                GameIsolate_.world->height);
                METAENGINE_Render_SetBlendMode(TexturePack_.textureFlow,
                                               METAENGINE_Render_BLEND_SET);
                METAENGINE_Render_BlitRect(TexturePack_.textureFlow, NULL,
                                           TexturePack_.textureFlowSpead->target, NULL);

                global.shaderworker.waterFlowPassShader->dirty = false;
            }

            global.shaderworker.waterShader->activate();
            float t = (GameIsolate_.game_timestate.now - GameIsolate_.game_timestate.startTime) /
                      1000.0;
            global.shaderworker.waterShader->update(
                    t, RenderTarget_.target->w * scale, RenderTarget_.target->h * scale,
                    TexturePack_.texture, r1.x, r1.y, r1.w, r1.h, scale,
                    TexturePack_.textureFlowSpead, Settings::water_overlay,
                    Settings::water_showFlow, Settings::water_pixelated);
        }

        RenderTarget_.target = RenderTarget_.realTarget;

        METAENGINE_Render_BlitRect(TexturePack_.backgroundImage, NULL, RenderTarget_.target, NULL);

        METAENGINE_Render_SetBlendMode(TexturePack_.texture, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_ActivateShaderProgram(0, NULL);

        // done shader

        int lmsx = (int) ((mx - GameData_.ofsX - GameData_.camX) / scale);
        int lmsy = (int) ((my - GameData_.ofsY - GameData_.camY) / scale);

        METAENGINE_Render_Clear(TexturePack_.worldTexture->target);

        METAENGINE_Render_BlitRect(TexturePack_.texture, NULL, TexturePack_.worldTexture->target,
                                   NULL);

        METAENGINE_Render_SetBlendMode(TexturePack_.textureObjects, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(TexturePack_.textureObjects, NULL,
                                   TexturePack_.worldTexture->target, NULL);
        METAENGINE_Render_SetBlendMode(TexturePack_.textureObjectsLQ,
                                       METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(TexturePack_.textureObjectsLQ, NULL,
                                   TexturePack_.worldTexture->target, NULL);

        METAENGINE_Render_SetBlendMode(TexturePack_.textureParticles,
                                       METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(TexturePack_.textureParticles, NULL,
                                   TexturePack_.worldTexture->target, NULL);

        METAENGINE_Render_SetBlendMode(TexturePack_.textureEntitiesLQ,
                                       METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(TexturePack_.textureEntitiesLQ, NULL,
                                   TexturePack_.worldTexture->target, NULL);
        METAENGINE_Render_SetBlendMode(TexturePack_.textureEntities,
                                       METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(TexturePack_.textureEntities, NULL,
                                   TexturePack_.worldTexture->target, NULL);

        if (Settings::draw_shaders) global.shaderworker.newLightingShader->activate();

        // I use this to only rerender the lighting when a parameter changes or N times per second anyway
        // Doing this massively reduces the GPU load of the shader
        bool needToRerenderLighting = false;

        static long long lastLightingForceRefresh = 0;
        long long now = UTime::millis();
        if (now - lastLightingForceRefresh > 100) {
            lastLightingForceRefresh = now;
            needToRerenderLighting = true;
        }

        if (Settings::draw_shaders && GameIsolate_.world) {
            float lightTx;
            float lightTy;

            if (GameIsolate_.world->player) {
                lightTx = (GameIsolate_.world->loadZone.x + GameIsolate_.world->player->x +
                           GameIsolate_.world->player->hw / 2.0f) /
                          (float) GameIsolate_.world->width;
                lightTy = (GameIsolate_.world->loadZone.y + GameIsolate_.world->player->y +
                           GameIsolate_.world->player->hh / 2.0f) /
                          (float) GameIsolate_.world->height;
            } else {
                lightTx = lmsx / (float) GameIsolate_.world->width;
                lightTy = lmsy / (float) GameIsolate_.world->height;
            }

            if (global.shaderworker.newLightingShader->lastLx != lightTx ||
                global.shaderworker.newLightingShader->lastLy != lightTy)
                needToRerenderLighting = true;
            global.shaderworker.newLightingShader->update(
                    TexturePack_.worldTexture, TexturePack_.emissionTexture, lightTx, lightTy);
            if (global.shaderworker.newLightingShader->lastQuality != Settings::lightingQuality) {
                needToRerenderLighting = true;
            }
            global.shaderworker.newLightingShader->setQuality(Settings::lightingQuality);

            int nBg = 0;
            int range = 64;
            for (int xx = std::max(0, (int) (lightTx * GameIsolate_.world->width) - range);
                 xx <= std::min((int) (lightTx * GameIsolate_.world->width) + range,
                                GameIsolate_.world->width - 1);
                 xx++) {
                for (int yy = std::max(0, (int) (lightTy * GameIsolate_.world->height) - range);
                     yy <= std::min((int) (lightTy * GameIsolate_.world->height) + range,
                                    GameIsolate_.world->height - 1);
                     yy++) {
                    if (GameIsolate_.world->background[xx + yy * GameIsolate_.world->width] !=
                        0x00) {
                        nBg++;
                    }
                }
            }

            global.shaderworker.newLightingShader_insideDes =
                    std::min(std::max(0.0f, (float) nBg / ((range * 2) * (range * 2))), 1.0f);
            global.shaderworker.newLightingShader_insideCur +=
                    (global.shaderworker.newLightingShader_insideDes -
                     global.shaderworker.newLightingShader_insideCur) /
                    2.0f * (GameIsolate_.game_timestate.deltaTime / 1000.0f);

            float ins = global.shaderworker.newLightingShader_insideCur < 0.05
                                ? 0.0
                                : global.shaderworker.newLightingShader_insideCur;
            if (global.shaderworker.newLightingShader->lastInside != ins)
                needToRerenderLighting = true;
            global.shaderworker.newLightingShader->setInside(ins);
            global.shaderworker.newLightingShader->setBounds(
                    GameIsolate_.world->tickZone.x * Settings::hd_objects_size,
                    GameIsolate_.world->tickZone.y * Settings::hd_objects_size,
                    (GameIsolate_.world->tickZone.x + GameIsolate_.world->tickZone.w) *
                            Settings::hd_objects_size,
                    (GameIsolate_.world->tickZone.y + GameIsolate_.world->tickZone.h) *
                            Settings::hd_objects_size);

            if (global.shaderworker.newLightingShader->lastSimpleMode != Settings::simpleLighting)
                needToRerenderLighting = true;
            global.shaderworker.newLightingShader->setSimpleMode(Settings::simpleLighting);

            if (global.shaderworker.newLightingShader->lastEmissionEnabled !=
                Settings::lightingEmission)
                needToRerenderLighting = true;
            global.shaderworker.newLightingShader->setEmissionEnabled(Settings::lightingEmission);

            if (global.shaderworker.newLightingShader->lastDitheringEnabled !=
                Settings::lightingDithering)
                needToRerenderLighting = true;
            global.shaderworker.newLightingShader->setDitheringEnabled(Settings::lightingDithering);
        }

        if (Settings::draw_shaders && needToRerenderLighting) {
            METAENGINE_Render_Clear(TexturePack_.lightingTexture->target);
            METAENGINE_Render_BlitRect(TexturePack_.worldTexture, NULL,
                                       TexturePack_.lightingTexture->target, NULL);
        }
        if (Settings::draw_shaders) METAENGINE_Render_ActivateShaderProgram(0, NULL);

        METAENGINE_Render_BlitRect(TexturePack_.worldTexture, NULL, RenderTarget_.target, &r1);

        if (Settings::draw_shaders) {
            METAENGINE_Render_SetBlendMode(TexturePack_.lightingTexture,
                                           Settings::draw_light_overlay
                                                   ? METAENGINE_Render_BLEND_NORMAL
                                                   : METAENGINE_Render_BLEND_MULTIPLY);
            METAENGINE_Render_BlitRect(TexturePack_.lightingTexture, NULL, RenderTarget_.target,
                                       &r1);
        }

        if (Settings::draw_shaders) {
            METAENGINE_Render_Clear(TexturePack_.texture2Fire->target);

            global.shaderworker.fireShader->activate();
            global.shaderworker.fireShader->update(TexturePack_.textureFire);
            METAENGINE_Render_BlitRect(TexturePack_.textureFire, NULL,
                                       TexturePack_.texture2Fire->target, NULL);
            METAENGINE_Render_ActivateShaderProgram(0, NULL);

            global.shaderworker.fire2Shader->activate();
            global.shaderworker.fire2Shader->update(TexturePack_.texture2Fire);
            METAENGINE_Render_BlitRect(TexturePack_.texture2Fire, NULL, RenderTarget_.target, &r1);
            METAENGINE_Render_ActivateShaderProgram(0, NULL);
        }

        // done light

        renderOverlays();
    }
}

void Game::renderOverlays() {

    METAENGINE_Render_Rect r1 = METAENGINE_Render_Rect{
            (float) (GameData_.ofsX + GameData_.camX), (float) (GameData_.ofsY + GameData_.camY),
            (float) (GameIsolate_.world->width * scale),
            (float) (GameIsolate_.world->height * scale)};
    METAENGINE_Render_Rect r2 = METAENGINE_Render_Rect{
            (float) (GameData_.ofsX + GameData_.camX + GameIsolate_.world->tickZone.x * scale),
            (float) (GameData_.ofsY + GameData_.camY + GameIsolate_.world->tickZone.y * scale),
            (float) (GameIsolate_.world->tickZone.w * scale),
            (float) (GameIsolate_.world->tickZone.h * scale)};

    if (Settings::draw_temperature_map) {
        METAENGINE_Render_SetBlendMode(TexturePack_.temperatureMap, METAENGINE_Render_BLEND_NORMAL);
        METAENGINE_Render_BlitRect(TexturePack_.temperatureMap, NULL, RenderTarget_.target, &r1);
    }

    if (Settings::draw_load_zones) {
        METAENGINE_Render_Rect r2m = METAENGINE_Render_Rect{
                (float) (GameData_.ofsX + GameData_.camX + GameIsolate_.world->meshZone.x * scale),
                (float) (GameData_.ofsY + GameData_.camY + GameIsolate_.world->meshZone.y * scale),
                (float) (GameIsolate_.world->meshZone.w * scale),
                (float) (GameIsolate_.world->meshZone.h * scale)};

        METAENGINE_Render_Rectangle2(RenderTarget_.target, r2m, {0x00, 0xff, 0xff, 0xff});
        METAENGINE_Render_Rectangle2(RenderTarget_.target, r2, {0xff, 0x00, 0x00, 0xff});
    }

    if (Settings::draw_load_zones) {

        SDL_Color col = {0xff, 0x00, 0x00, 0x20};
        METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);

        METAENGINE_Render_Rect r3 =
                METAENGINE_Render_Rect{(float) (0), (float) (0),
                                       (float) ((GameData_.ofsX + GameData_.camX +
                                                 GameIsolate_.world->tickZone.x * scale)),
                                       (float) (global.platform.HEIGHT)};
        METAENGINE_Render_Rectangle2(RenderTarget_.target, r3, col);

        METAENGINE_Render_Rect r4 = METAENGINE_Render_Rect{
                (float) (GameData_.ofsX + GameData_.camX + GameIsolate_.world->tickZone.x * scale +
                         GameIsolate_.world->tickZone.w * scale),
                (float) (0),
                (float) ((global.platform.WIDTH) -
                         (GameData_.ofsX + GameData_.camX + GameIsolate_.world->tickZone.x * scale +
                          GameIsolate_.world->tickZone.w * scale)),
                (float) (global.platform.HEIGHT)};
        METAENGINE_Render_Rectangle2(RenderTarget_.target, r3, col);

        METAENGINE_Render_Rect r5 = METAENGINE_Render_Rect{
                (float) (GameData_.ofsX + GameData_.camX + GameIsolate_.world->tickZone.x * scale),
                (float) (0), (float) (GameIsolate_.world->tickZone.w * scale),
                (float) (GameData_.ofsY + GameData_.camY + GameIsolate_.world->tickZone.y * scale)};
        METAENGINE_Render_Rectangle2(RenderTarget_.target, r3, col);

        METAENGINE_Render_Rect r6 = METAENGINE_Render_Rect{
                (float) (GameData_.ofsX + GameData_.camX + GameIsolate_.world->tickZone.x * scale),
                (float) (GameData_.ofsY + GameData_.camY + GameIsolate_.world->tickZone.y * scale +
                         GameIsolate_.world->tickZone.h * scale),
                (float) (GameIsolate_.world->tickZone.w * scale),
                (float) (global.platform.HEIGHT -
                         (GameData_.ofsY + GameData_.camY + GameIsolate_.world->tickZone.y * scale +
                          GameIsolate_.world->tickZone.h * scale))};
        METAENGINE_Render_Rectangle2(RenderTarget_.target, r6, col);

        col = {0x00, 0xff, 0x00, 0xff};
        METAENGINE_Render_Rect r7 = METAENGINE_Render_Rect{
                (float) (GameData_.ofsX + GameData_.camX + GameIsolate_.world->width / 2 * scale -
                         (global.platform.WIDTH / 3 * scale / 2)),
                (float) (GameData_.ofsY + GameData_.camY + GameIsolate_.world->height / 2 * scale -
                         (global.platform.HEIGHT / 3 * scale / 2)),
                (float) (global.platform.WIDTH / 3 * scale),
                (float) (global.platform.HEIGHT / 3 * scale)};
        METAENGINE_Render_Rectangle2(RenderTarget_.target, r7, col);
    }

    if (Settings::draw_physics_debug) {
        //
        //for(size_t i = 0; i < GameIsolate_.world->rigidBodies.size(); i++) {
        //    RigidBody cur = *GameIsolate_.world->rigidBodies[i];

        //    float x = cur.body->GetPosition().x;
        //    float y = cur.body->GetPosition().y;
        //    x = ((x)*scale + ofsX + camX);
        //    y = ((y)*scale + ofsY + camY);

        //    /*SDL_Rect* r = new SDL_Rect{ (int)x, (int)y, cur.surface->w * scale, cur.surface->h * scale };
        //    SDL_RenderCopyEx(renderer, cur.texture, NULL, r, cur.body->GetAngle() * 180 / M_PI, new SDL_Point{ 0, 0 }, SDL_RendererFlip::SDL_FLIP_NONE);
        //    delete r;*/

        //    UInt32 color = 0x0000ff;

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

        //if(GameIsolate_.world->player) {
        //    RigidBody cur = *GameIsolate_.world->player->rb;

        //    float x = cur.body->GetPosition().x;
        //    float y = cur.body->GetPosition().y;
        //    x = ((x)*scale + ofsX + camX);
        //    y = ((y)*scale + ofsY + camY);

        //    UInt32 color = 0x0000ff;

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

        //for(size_t i = 0; i < GameIsolate_.world->worldRigidBodies.size(); i++) {
        //    RigidBody cur = *GameIsolate_.world->worldRigidBodies[i];

        //    float x = cur.body->GetPosition().x;
        //    float y = cur.body->GetPosition().y;
        //    x = ((x)*scale + ofsX + camX);
        //    y = ((y)*scale + ofsY + camY);

        //    UInt32 color = 0x00ff00;

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

        int minChX = (int) floor((GameIsolate_.world->meshZone.x - GameIsolate_.world->loadZone.x) /
                                 CHUNK_W);
        int minChY = (int) floor((GameIsolate_.world->meshZone.y - GameIsolate_.world->loadZone.y) /
                                 CHUNK_H);
        int maxChX = (int) ceil((GameIsolate_.world->meshZone.x + GameIsolate_.world->meshZone.w -
                                 GameIsolate_.world->loadZone.x) /
                                CHUNK_W);
        int maxChY = (int) ceil((GameIsolate_.world->meshZone.y + GameIsolate_.world->meshZone.h -
                                 GameIsolate_.world->loadZone.y) /
                                CHUNK_H);

        for (int cx = minChX; cx <= maxChX; cx++) {
            for (int cy = minChY; cy <= maxChY; cy++) {
                Chunk *ch = GameIsolate_.world->getChunk(cx, cy);
                SDL_Color col = {255, 0, 0, 255};

                float x = ((ch->x * CHUNK_W + GameIsolate_.world->loadZone.x) * scale +
                           GameData_.ofsX + GameData_.camX);
                float y = ((ch->y * CHUNK_H + GameIsolate_.world->loadZone.y) * scale +
                           GameData_.ofsY + GameData_.camY);

                METAENGINE_Render_Rectangle(RenderTarget_.target, x, y, x + CHUNK_W * scale,
                                            y + CHUNK_H * scale, {50, 50, 0, 255});

                //for(int i = 0; i < ch->polys.size(); i++) {
                //    Drawing::drawPolygon(target, col, ch->polys[i].m_vertices, (int)x, (int)y, scale, ch->polys[i].m_count, 0/* + fmod((Time::millis() / 1000.0), 360)*/, 0, 0);
                //}
            }
        }

        //

        GameIsolate_.world->b2world->SetDebugDraw(b2DebugDraw);
        b2DebugDraw->scale = scale;
        b2DebugDraw->xOfs = GameData_.ofsX + GameData_.camX;
        b2DebugDraw->yOfs = GameData_.ofsY + GameData_.camY;
        b2DebugDraw->SetFlags(0);
        if (Settings::draw_b2d_shape) b2DebugDraw->AppendFlags(b2Draw::e_shapeBit);
        if (Settings::draw_b2d_joint) b2DebugDraw->AppendFlags(b2Draw::e_jointBit);
        if (Settings::draw_b2d_aabb) b2DebugDraw->AppendFlags(b2Draw::e_aabbBit);
        if (Settings::draw_b2d_pair) b2DebugDraw->AppendFlags(b2Draw::e_pairBit);
        if (Settings::draw_b2d_centerMass) b2DebugDraw->AppendFlags(b2Draw::e_centerOfMassBit);
        GameIsolate_.world->b2world->DebugDraw();
    }

    // Drawing::drawText("fps",
    //                   fmt::format("{} FPS\n Feels Like: {} FPS", GameIsolate_.game_timestate.fps,
    //                               GameIsolate_.game_timestate.feelsLikeFps),
    //                   global.platform.WIDTH, 20);

    if (Settings::draw_chunk_state) {

        int chSize = 10;

        int centerX = global.platform.WIDTH / 2;
        int centerY = CHUNK_UNLOAD_DIST * chSize + 10;

        int pposX = GameData_.plPosX;
        int pposY = GameData_.plPosY;
        int pchx = (int) ((pposX / CHUNK_W) * chSize);
        int pchy = (int) ((pposY / CHUNK_H) * chSize);
        int pchxf = (int) (((float) pposX / CHUNK_W) * chSize);
        int pchyf = (int) (((float) pposY / CHUNK_H) * chSize);

        METAENGINE_Render_Rectangle(
                RenderTarget_.target, centerX - chSize * CHUNK_UNLOAD_DIST + chSize,
                centerY - chSize * CHUNK_UNLOAD_DIST + chSize,
                centerX + chSize * CHUNK_UNLOAD_DIST + chSize,
                centerY + chSize * CHUNK_UNLOAD_DIST + chSize, {0xcc, 0xcc, 0xcc, 0xff});

        METAENGINE_Render_Rect r = {0, 0, (float) chSize, (float) chSize};
        for (auto &p: GameIsolate_.world->chunkCache) {
            if (p.first == INT_MIN) continue;
            int cx = p.first;
            for (auto &p2: p.second) {
                if (p2.first == INT_MIN) continue;
                int cy = p2.first;
                Chunk *m = p2.second;
                r.x = centerX + m->x * chSize - pchx;
                r.y = centerY + m->y * chSize - pchy;
                SDL_Color col;
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
                METAENGINE_Render_Rectangle2(RenderTarget_.target, r, col);
            }
        }

        int loadx = (int) (((float) -GameIsolate_.world->loadZone.x / CHUNK_W) * chSize);
        int loady = (int) (((float) -GameIsolate_.world->loadZone.y / CHUNK_H) * chSize);

        int loadx2 =
                (int) (((float) (-GameIsolate_.world->loadZone.x + GameIsolate_.world->loadZone.w) /
                        CHUNK_W) *
                       chSize);
        int loady2 =
                (int) (((float) (-GameIsolate_.world->loadZone.y + GameIsolate_.world->loadZone.h) /
                        CHUNK_H) *
                       chSize);
        METAENGINE_Render_Rectangle(RenderTarget_.target, centerX - pchx + loadx,
                                    centerY - pchy + loady, centerX - pchx + loadx2,
                                    centerY - pchy + loady2, {0x00, 0xff, 0xff, 0xff});

        METAENGINE_Render_Rectangle(RenderTarget_.target, centerX - pchx + pchxf,
                                    centerY - pchy + pchyf, centerX + 1 - pchx + pchxf,
                                    centerY + 1 - pchy + pchyf, {0x00, 0xff, 0x00, 0xff});
    }

    if (Settings::draw_debug_stats) {

        int rbCt = 0;
        for (auto &r: GameIsolate_.world->rigidBodies) {
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

        int minChX = (int) floor((GameIsolate_.world->meshZone.x - GameIsolate_.world->loadZone.x) /
                                 CHUNK_W);
        int minChY = (int) floor((GameIsolate_.world->meshZone.y - GameIsolate_.world->loadZone.y) /
                                 CHUNK_H);
        int maxChX = (int) ceil((GameIsolate_.world->meshZone.x + GameIsolate_.world->meshZone.w -
                                 GameIsolate_.world->loadZone.x) /
                                CHUNK_W);
        int maxChY = (int) ceil((GameIsolate_.world->meshZone.y + GameIsolate_.world->meshZone.h -
                                 GameIsolate_.world->loadZone.y) /
                                CHUNK_H);

        for (int cx = minChX; cx <= maxChX; cx++) {
            for (int cy = minChY; cy <= maxChY; cy++) {
                Chunk *ch = GameIsolate_.world->getChunk(cx, cy);
                for (int i = 0; i < ch->polys.size(); i++) { rbTriWCt++; }
            }
        }

        int chCt = 0;
        for (auto &p: GameIsolate_.world->chunkCache) {
            if (p.first == INT_MIN) continue;
            int cx = p.first;
            for (auto &p2: p.second) {
                if (p2.first == INT_MIN) continue;
                chCt++;
            }
        }

        const char* buffAsStdStr1 = R"(
XY: {:.2f} / {:.2f}
V: {:.2f} / {:.2f}
Particles: {}
Entities: {}
RigidBodies: {}/{} O, {} W
Tris: {}/{} O, {} W
Cached Chunks: {}
GameIsolate_.world->readyToReadyToMerge ({})
GameIsolate_.world->readyToMerge ({})
)";

        Drawing::drawText(
                "info",
                fmt::format(buffAsStdStr1, GameData_.plPosX, GameData_.plPosY,
                            GameIsolate_.world->player ? GameIsolate_.world->player->vx : 0.0f,
                            GameIsolate_.world->player ? GameIsolate_.world->player->vy : 0.0f,
                            (int)GameIsolate_.world->particles.size(),
                            (int)GameIsolate_.world->entities.size(), rbCt,
                            (int) GameIsolate_.world->rigidBodies.size(),
                            (int) GameIsolate_.world->worldRigidBodies.size(), rbTriACt, rbTriCt,
                            rbTriWCt, chCt, (int) GameIsolate_.world->readyToReadyToMerge.size(),
                            (int) GameIsolate_.world->readyToMerge.size()),
                4, 12);

        // for (size_t i = 0; i < GameIsolate_.world->readyToReadyToMerge.size(); i++) {
        //     char buff[10];
        //     snprintf(buff, sizeof(buff), "    #%d", (int) i);
        //     std::string buffAsStdStr = buff;
        //     Drawing::drawTextBG(RenderTarget_.target, buffAsStdStr.c_str(), font16, 4,
        //                         2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff,
        //                         {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);
        // }

        // for (size_t i = 0; i < GameIsolate_.world->readyToMerge.size(); i++) {
        //     char buff[20];
        //     snprintf(buff, sizeof(buff), "    #%d (%d, %d)", (int) i,
        //              GameIsolate_.world->readyToMerge[i]->x,
        //              GameIsolate_.world->readyToMerge[i]->y);
        //     std::string buffAsStdStr = buff;
        //     Drawing::drawTextBG(RenderTarget_.target, buffAsStdStr.c_str(), font16, 4,
        //                         2 + (lineHeight * dbgIndex++), 0xff, 0xff, 0xff,
        //                         {0x00, 0x00, 0x00, 0x40}, ALIGN_LEFT);
        // }
    }

    if (Settings::draw_frame_graph) {

        for (int i = 0; i <= 4; i++) {
            // Drawing::drawText(RenderTarget_.target, dt_frameGraph[i], global.platform.WIDTH - 20,
            //                   global.platform.HEIGHT - 15 - (i * 25) - 2);
            METAENGINE_Render_Line(
                    RenderTarget_.target, global.platform.WIDTH - 30 - FrameTimeNum - 5,
                    global.platform.HEIGHT - 10 - (i * 25), global.platform.WIDTH - 25,
                    global.platform.HEIGHT - 10 - (i * 25), {0xff, 0xff, 0xff, 0xff});
        }
        /*for (int i = 0; i <= 100; i += 25) {
            char buff[20];
            snprintf(buff, sizeof(buff), "%d", i);
            std::string buffAsStdStr = buff;
            Drawing::drawText(renderer, buffAsStdStr.c_str(), font14, WIDTH - 20, HEIGHT - 15 - i - 2, 0xff, 0xff, 0xff, ALIGN_LEFT);
            SDL_RenderDrawLine(renderer, WIDTH - 30 - FrameTimeNum - 5, HEIGHT - 10 - i, WIDTH - 25, HEIGHT - 10 - i);
        }*/

        for (int i = 0; i < FrameTimeNum; i++) {
            int h = frameTime[i];

            SDL_Color col;
            if (h <= (int) (1000 / 144.0)) {
                col = {0x00, 0xff, 0x00, 0xff};
            } else if (h <= (int) (1000 / 60.0)) {
                col = {0xa0, 0xe0, 0x00, 0xff};
            } else if (h <= (int) (1000 / 30.0)) {
                col = {0xff, 0xff, 0x00, 0xff};
            } else if (h <= (int) (1000 / 15.0)) {
                col = {0xff, 0x80, 0x00, 0xff};
            } else {
                col = {0xff, 0x00, 0x00, 0xff};
            }

            METAENGINE_Render_Line(
                    RenderTarget_.target, global.platform.WIDTH - FrameTimeNum - 30 + i,
                    global.platform.HEIGHT - 10 - h, global.platform.WIDTH - FrameTimeNum - 30 + i,
                    global.platform.HEIGHT - 10, col);
            //SDL_RenderDrawLine(renderer, WIDTH - FrameTimeNum - 30 + i, HEIGHT - 10 - h, WIDTH - FrameTimeNum - 30 + i, HEIGHT - 10);
        }

        METAENGINE_Render_Line(
                RenderTarget_.target, global.platform.WIDTH - 30 - FrameTimeNum - 5,
                global.platform.HEIGHT - 10 - (int) (1000.0 / GameIsolate_.game_timestate.fps),
                global.platform.WIDTH - 25,
                global.platform.HEIGHT - 10 - (int) (1000.0 / GameIsolate_.game_timestate.fps),
                {0x00, 0xff, 0xff, 0xff});
        METAENGINE_Render_Line(RenderTarget_.target, global.platform.WIDTH - 30 - FrameTimeNum - 5,
                               global.platform.HEIGHT - 10 -
                                       (int) (1000.0 / GameIsolate_.game_timestate.feelsLikeFps),
                               global.platform.WIDTH - 25,
                               global.platform.HEIGHT - 10 -
                                       (int) (1000.0 / GameIsolate_.game_timestate.feelsLikeFps),
                               {0xff, 0x00, 0xff, 0xff});
    }

    METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BLEND_NORMAL);

    /*

#ifdef DEVELOPMENT_BUILD
    if (dt_versionInfo1.w == -1) {
        char buffDevBuild[40];
        snprintf(buffDevBuild, sizeof(buffDevBuild), "Development Build");
        //if (dt_versionInfo1.t1 != nullptr) METAENGINE_Render_FreeImage(dt_versionInfo1.t1);
        //if (dt_versionInfo1.t2 != nullptr) METAENGINE_Render_FreeImage(dt_versionInfo1.t2);
        dt_versionInfo1 = Drawing::drawTextParams(RenderTarget_.target, buffDevBuild, font16, 4, global.platform.HEIGHT - 32 - 13, 0xff, 0xff, 0xff, ALIGN_LEFT);

        char buffVersion[40];
        snprintf(buffVersion, sizeof(buffVersion), "Version %s - dev", VERSION);
        //if (dt_versionInfo2.t1 != nullptr) METAENGINE_Render_FreeImage(dt_versionInfo2.t1);
        //if (dt_versionInfo2.t2 != nullptr) METAENGINE_Render_FreeImage(dt_versionInfo2.t2);
        dt_versionInfo2 = Drawing::drawTextParams(RenderTarget_.target, buffVersion, font16, 4, global.platform.HEIGHT - 32, 0xff, 0xff, 0xff, ALIGN_LEFT);

        char buffBuildDate[40];
        snprintf(buffBuildDate, sizeof(buffBuildDate), "%s : %s", __DATE__, __TIME__);
        //if (dt_versionInfo3.t1 != nullptr) METAENGINE_Render_FreeImage(dt_versionInfo3.t1);
        //if (dt_versionInfo3.t2 != nullptr) METAENGINE_Render_FreeImage(dt_versionInfo3.t2);
        dt_versionInfo3 = Drawing::drawTextParams(RenderTarget_.target, buffBuildDate, font16, 4, global.platform.HEIGHT - 32 + 13, 0xff, 0xff, 0xff, ALIGN_LEFT);
    }

    Drawing::drawText(RenderTarget_.target, dt_versionInfo1, 4, global.platform.HEIGHT - 32 - 13, ALIGN_LEFT);
    Drawing::drawText(RenderTarget_.target, dt_versionInfo2, 4, global.platform.HEIGHT - 32, ALIGN_LEFT);
    Drawing::drawText(RenderTarget_.target, dt_versionInfo3, 4, global.platform.HEIGHT - 32 + 13, ALIGN_LEFT);
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
            int32_t temp = t.temperature;
            UInt32 color = (UInt8) ((temp + 1024) / 2048.0f * 255);

            const unsigned int offset = (GameIsolate_.world->width * 4 * y) + x * 4;
            TexturePack_.pixelsTemp_ar[offset + 0] = color;// b
            TexturePack_.pixelsTemp_ar[offset + 1] = color;// g
            TexturePack_.pixelsTemp_ar[offset + 2] = color;// r
            TexturePack_.pixelsTemp_ar[offset + 3] = 0xf0; // a
        }
    }
}

int Game::getAimSurface(int dist) {
    int dcx = this->mx - global.platform.WIDTH / 2;
    int dcy = this->my - global.platform.HEIGHT / 2;

    float len = sqrtf(dcx * dcx + dcy * dcy);
    float udx = dcx / len;
    float udy = dcy / len;

    int mmx = global.platform.WIDTH / 2.0f + udx * dist;
    int mmy = global.platform.HEIGHT / 2.0f + udy * dist;

    int wcx = (int) ((global.platform.WIDTH / 2.0f - GameData_.ofsX - GameData_.camX) / scale);
    int wcy = (int) ((global.platform.HEIGHT / 2.0f - GameData_.ofsY - GameData_.camY) / scale);

    int wmx = (int) ((mmx - GameData_.ofsX - GameData_.camX) / scale);
    int wmy = (int) ((mmy - GameData_.ofsY - GameData_.camY) / scale);

    int startInd = -1;
    GameIsolate_.world->forLine(wcx, wcy, wmx, wmy, [&](int ind) {
        if (GameIsolate_.world->tiles[ind].mat->physicsType == PhysicsType::SOLID ||
            GameIsolate_.world->tiles[ind].mat->physicsType == PhysicsType::SAND ||
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

    std::string pref = "Saved in: ";

    std::string worldName = "mainMenu";
    char *wn = (char *) worldName.c_str();

    METADOT_INFO("Loading main menu @ {0}", global.GameDir.getWorldPath(wn));
    GameUI::MainMenuUI::visible = false;
    state = LOADING;
    stateAfterLoad = MAIN_MENU;

    METADOT_DELETE(C, GameIsolate_.world, World);
    GameIsolate_.world = nullptr;

    WorldGenerator *generator = new MaterialTestGenerator();

    std::string wpStr = global.GameDir.getWorldPath(wn);

    METADOT_NEW(C, GameIsolate_.world, World);
    GameIsolate_.world->noSaveLoad = true;
    GameIsolate_.world->init(
            wpStr,
            (int) ceil(WINDOWS_MAX_WIDTH / RENDER_C_TEST / (double) CHUNK_W) * CHUNK_W +
                    CHUNK_W * RENDER_C_TEST,
            (int) ceil(WINDOWS_MAX_HEIGHT / RENDER_C_TEST / (double) CHUNK_H) * CHUNK_H +
                    CHUNK_H * RENDER_C_TEST,
            RenderTarget_.target, &global.audioEngine, Settings::networkMode, generator);

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

    METAENGINE_Render_UpdateImageBytes(TexturePack_.texture, NULL, &TexturePack_.pixels[0],
                                       GameIsolate_.world->width * 4);

    METAENGINE_Render_UpdateImageBytes(TexturePack_.textureBackground, NULL,
                                       &TexturePack_.pixelsBackground[0],
                                       GameIsolate_.world->width * 4);

    METAENGINE_Render_UpdateImageBytes(TexturePack_.textureLayer2, NULL,
                                       &TexturePack_.pixelsLayer2[0],
                                       GameIsolate_.world->width * 4);

    METAENGINE_Render_UpdateImageBytes(TexturePack_.textureFire, NULL, &TexturePack_.pixelsFire[0],
                                       GameIsolate_.world->width * 4);

    METAENGINE_Render_UpdateImageBytes(TexturePack_.textureFlow, NULL, &TexturePack_.pixelsFlow[0],
                                       GameIsolate_.world->width * 4);

    METAENGINE_Render_UpdateImageBytes(TexturePack_.emissionTexture, NULL,
                                       &TexturePack_.pixelsEmission[0],
                                       GameIsolate_.world->width * 4);

    METAENGINE_Render_UpdateImageBytes(TexturePack_.textureParticles, NULL,
                                       &TexturePack_.pixelsParticles[0],
                                       GameIsolate_.world->width * 4);

    GameUI::MainMenuUI::visible = true;
}

int Game::getAimSolidSurface(int dist) {
    int dcx = this->mx - global.platform.WIDTH / 2;
    int dcy = this->my - global.platform.HEIGHT / 2;

    float len = sqrtf(dcx * dcx + dcy * dcy);
    float udx = dcx / len;
    float udy = dcy / len;

    int mmx = global.platform.WIDTH / 2.0f + udx * dist;
    int mmy = global.platform.HEIGHT / 2.0f + udy * dist;

    int wcx = (int) ((global.platform.WIDTH / 2.0f - GameData_.ofsX - GameData_.camX) / scale);
    int wcy = (int) ((global.platform.HEIGHT / 2.0f - GameData_.ofsY - GameData_.camY) / scale);

    int wmx = (int) ((mmx - GameData_.ofsX - GameData_.camX) / scale);
    int wmy = (int) ((mmy - GameData_.ofsY - GameData_.camY) / scale);

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
    UInt16 waterCt = std::min(movingTiles[Materials::WATER.id], (UInt16) 5000);
    float water = (float) waterCt / 3000;
    //METADOT_BUG("{} / {} = {}", waterCt, 3000, water);
    global.audioEngine.SetEventParameter("event:/World/WaterFlow", "FlowIntensity", water);
}