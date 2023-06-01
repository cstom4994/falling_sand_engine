// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_GAME_HPP
#define ME_GAME_HPP

// #define b2_maxTranslation 10.0f
// #define b2_maxTranslationSquared (b2_maxTranslation * b2_maxTranslation)

#include <algorithm>
#include <chrono>
#include <codecvt>
#include <cstddef>
#include <functional>
#include <iostream>
#include <thread>
#include <unordered_map>

#include "engine/audio/audio.h"
#include "background.hpp"
#include "engine/core/const.h"
#include "engine/core/cpp/utils.hpp"
#include "engine/core/debug.hpp"
#include "engine/core/io/filesystem.h"
#include "engine/core/macros.hpp"
#include "cvar.hpp"
#include "engine/event/applicationevent.hpp"
#include "engine/event/event.hpp"
#include "engine/event/inputevent.hpp"
#include "game_basic.hpp"
#include "game_datastruct.hpp"
#include "game_resources.hpp"
#include "game_shaders.hpp"
#include "engine/game_utils/rng.h"
#include "engine/physics/box2d.h"
// #include "libs/parallel_hashmap/phmap.h"
#include "engine/meta/reflection.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/scripting.hpp"
#include "engine/ui/imgui_layer.hpp"
#include "engine/ui/ui.hpp"
#include "world.hpp"

enum EnumGameState { MAIN_MENU, LOADING, INGAME };

class Game {
public:
    using EventCallbackFn = std::function<void(MetaEngine::Event &)>;
    using SystemList = std::vector<ME::ref<IGameSystem>>;

public:
    EnumGameState state = LOADING;
    EnumGameState stateAfterLoad = MAIN_MENU;

    DebugDraw *debugDraw;

    i32 ent_prevLoadZoneX = 0;
    i32 ent_prevLoadZoneY = 0;

    u16 *movingTiles;

    i32 mx = 0;
    i32 my = 0;
    i32 lastDrawMX = 0;
    i32 lastDrawMY = 0;
    i32 lastEraseMX = 0;
    i32 lastEraseMY = 0;

    u8 *objectDelete = nullptr;

    u32 loadingOnColor = 0;
    u32 loadingOffColor = 0;

    RNG *RNG = nullptr;

public:
    bool running = true;

    f32 accLoadX = 0;
    f32 accLoadY = 0;

    i64 fadeInStart = 0;
    i64 fadeInLength = 0;
    i32 fadeInWaitFrames = 0;
    i32 fadeOutWaitFrames = 0;
    i64 fadeOutStart = 0;
    i64 fadeOutLength = 0;
    ME::meta::any_function fadeOutCallback = []() {};

    EventCallbackFn EventCallback;

    struct {
        ME::ref<BackgroundSystem> backgrounds;
        ME::ref<GameplayScriptSystem> gameplayscript;
        ME::ref<ShaderWorkerSystem> shaderworker;
        ME::ref<UISystem> ui;

        SystemList systemList = {};

        GlobalDEF globaldef;
        ME::scope<World> world;
        TexturePack *texturepack = nullptr;

        MetaEngine::ThreadPool *updateDirtyPool = nullptr;
        MetaEngine::ThreadPool *updateDirtyPool2 = nullptr;
    } GameIsolate_;

    struct {
        R_Image *backgroundImage = nullptr;

        R_Image *loadingTexture = nullptr;
        std::vector<u8> pixelsLoading;
        u8 *pixelsLoading_ar = nullptr;
        int loadingScreenW = 0;
        int loadingScreenH = 0;

        R_Image *worldTexture = nullptr;
        R_Image *lightingTexture = nullptr;

        R_Image *emissionTexture = nullptr;
        std::vector<u8> pixelsEmission;
        u8 *pixelsEmission_ar = nullptr;

        R_Image *texture = nullptr;
        std::vector<u8> pixels;
        u8 *pixels_ar = nullptr;
        R_Image *textureLayer2 = nullptr;
        std::vector<u8> pixelsLayer2;
        u8 *pixelsLayer2_ar = nullptr;
        R_Image *textureBackground = nullptr;
        std::vector<u8> pixelsBackground;
        u8 *pixelsBackground_ar = nullptr;
        R_Image *textureObjects = nullptr;
        R_Image *textureObjectsLQ = nullptr;
        std::vector<u8> pixelsObjects;
        u8 *pixelsObjects_ar = nullptr;
        R_Image *textureObjectsBack = nullptr;
        R_Image *textureCells = nullptr;
        std::vector<u8> pixelsCells;
        u8 *pixelsCells_ar = nullptr;
        R_Image *textureEntities = nullptr;
        R_Image *textureEntitiesLQ = nullptr;

        R_Image *textureFire = nullptr;
        R_Image *texture2Fire = nullptr;
        std::vector<u8> pixelsFire;
        u8 *pixelsFire_ar = nullptr;

        R_Image *textureFlowSpead = nullptr;
        R_Image *textureFlow = nullptr;
        std::vector<u8> pixelsFlow;
        u8 *pixelsFlow_ar = nullptr;

        R_Image *temperatureMap = nullptr;
        std::vector<u8> pixelsTemp;
        u8 *pixelsTemp_ar = nullptr;
    } TexturePack_;

public:
    EnumGameState getGameState() const { return state; }
    void setGameState(EnumGameState state, std::optional<EnumGameState> stateal) {
        this->state = state;
        if (stateal.has_value()) {
            this->stateAfterLoad = stateal.value();
        }
    }

    Game(int argc, char *argv[]);
    ~Game();

    int init(int argc, char *argv[]);
    int run(int argc, char *argv[]);
    int exit();
    void updateFrameEarly();
    void onEvent(MetaEngine::Event &e);
    bool onWindowClose(MetaEngine::WindowCloseEvent &e);
    bool onWindowResize(MetaEngine::WindowResizeEvent &e);
    void setEventCallback(const EventCallbackFn &callback) { EventCallback = callback; }
    void tick();
    void tickChunkLoading();
    void tickPlayer();
    void tickProfiler();
    void updateFrameLate();
    void renderOverlays();
    void updateMaterialSounds();
    void createTexture();
    void renderEarly();
    void renderLate();
    void renderTemperatureMap(World *world);
    void ResolutionChanged(int newWidth, int newHeight);
    int getAimSolidSurface(int dist);
    int getAimSurface(int dist);
    void quitToMainMenu();
};

#endif
