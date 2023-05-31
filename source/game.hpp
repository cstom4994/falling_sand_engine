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

#include "audio/audio.h"
#include "background.hpp"
#include "core/const.h"
#include "core/cpp/utils.hpp"
#include "core/debug.hpp"
#include "core/io/filesystem.h"
#include "core/macros.hpp"
#include "cvar.hpp"
#include "event/applicationevent.hpp"
#include "event/event.hpp"
#include "event/inputevent.hpp"
#include "game_basic.hpp"
#include "game_datastruct.hpp"
#include "game_resources.hpp"
#include "game_shaders.hpp"
#include "game_utils/rng.h"
#include "physics/box2d.h"
// #include "libs/parallel_hashmap/phmap.h"
#include "meta/reflection.hpp"
#include "renderer/renderer_gpu.h"
#include "scripting/scripting.hpp"
#include "ui/imgui/imgui_layer.hpp"
#include "ui/ttf.h"
#include "ui/ui.hpp"
#include "world.hpp"

enum EnumGameState { MAIN_MENU, LOADING, INGAME };

class Game {
public:
    using EventCallbackFn = std::function<void(MetaEngine::Event &)>;
    using SystemList = std::vector<MetaEngine::Ref<IGameSystem>>;

public:
    EnumGameState state = LOADING;
    EnumGameState stateAfterLoad = MAIN_MENU;

    DebugDraw *debugDraw;

    I32 ent_prevLoadZoneX = 0;
    I32 ent_prevLoadZoneY = 0;

    U16 *movingTiles;

    I32 mx = 0;
    I32 my = 0;
    I32 lastDrawMX = 0;
    I32 lastDrawMY = 0;
    I32 lastEraseMX = 0;
    I32 lastEraseMY = 0;

    U8 *objectDelete = nullptr;

    U32 loadingOnColor = 0;
    U32 loadingOffColor = 0;

    RNG *RNG = nullptr;

public:
    bool running = true;

    F32 accLoadX = 0;
    F32 accLoadY = 0;

    I64 fadeInStart = 0;
    I64 fadeInLength = 0;
    I32 fadeInWaitFrames = 0;
    I32 fadeOutWaitFrames = 0;
    I64 fadeOutStart = 0;
    I64 fadeOutLength = 0;
    ME::meta::any_function fadeOutCallback = []() {};

    EventCallbackFn EventCallback;

    struct {
        MetaEngine::Ref<BackgroundSystem> backgrounds;
        MetaEngine::Ref<GameplayScriptSystem> gameplayscript;
        MetaEngine::Ref<ShaderWorkerSystem> shaderworker;
        MetaEngine::Ref<UISystem> ui;

        SystemList systemList = {};

        GlobalDEF globaldef;
        MetaEngine::Scope<World> world;
        TexturePack *texturepack = nullptr;

        MetaEngine::ThreadPool *updateDirtyPool = nullptr;
        MetaEngine::ThreadPool *updateDirtyPool2 = nullptr;
    } GameIsolate_;

    struct {
        R_Image *backgroundImage = nullptr;

        R_Image *loadingTexture = nullptr;
        std::vector<U8> pixelsLoading;
        U8 *pixelsLoading_ar = nullptr;
        int loadingScreenW = 0;
        int loadingScreenH = 0;

        R_Image *worldTexture = nullptr;
        R_Image *lightingTexture = nullptr;

        R_Image *emissionTexture = nullptr;
        std::vector<U8> pixelsEmission;
        U8 *pixelsEmission_ar = nullptr;

        R_Image *texture = nullptr;
        std::vector<U8> pixels;
        U8 *pixels_ar = nullptr;
        R_Image *textureLayer2 = nullptr;
        std::vector<U8> pixelsLayer2;
        U8 *pixelsLayer2_ar = nullptr;
        R_Image *textureBackground = nullptr;
        std::vector<U8> pixelsBackground;
        U8 *pixelsBackground_ar = nullptr;
        R_Image *textureObjects = nullptr;
        R_Image *textureObjectsLQ = nullptr;
        std::vector<U8> pixelsObjects;
        U8 *pixelsObjects_ar = nullptr;
        R_Image *textureObjectsBack = nullptr;
        R_Image *textureCells = nullptr;
        std::vector<U8> pixelsCells;
        U8 *pixelsCells_ar = nullptr;
        R_Image *textureEntities = nullptr;
        R_Image *textureEntitiesLQ = nullptr;

        R_Image *textureFire = nullptr;
        R_Image *texture2Fire = nullptr;
        std::vector<U8> pixelsFire;
        U8 *pixelsFire_ar = nullptr;

        R_Image *textureFlowSpead = nullptr;
        R_Image *textureFlow = nullptr;
        std::vector<U8> pixelsFlow;
        U8 *pixelsFlow_ar = nullptr;

        R_Image *temperatureMap = nullptr;
        std::vector<U8> pixelsTemp;
        U8 *pixelsTemp_ar = nullptr;
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
