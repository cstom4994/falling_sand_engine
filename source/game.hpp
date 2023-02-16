// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_GAME_HPP_
#define _METADOT_GAME_HPP_

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
#include "console.hpp"
#include "core/const.h"
#include "core/cpp/utils.hpp"
#include "core/debug_impl.hpp"
#include "core/macros.h"
#include "core/threadpool.h"
#include "cvar.hpp"
#include "engine/engine_input.hpp"
#include "engine/engine_scripting.hpp"
#include "filesystem.h"
#include "game_basic.hpp"
#include "game_datastruct.hpp"
#include "game_resources.hpp"
#include "game_shaders.hpp"
#include "game_utils/rng.h"
#include "internal/builtin_box2d.h"
#include "libs/parallel_hashmap/phmap.h"
#include "meta/meta.hpp"
#include "renderer/renderer_gpu.h"
#include "ui/imgui/imgui_core.hpp"
#include "ui/ttf.h"
#include "world.hpp"

enum EnumGameState { MAIN_MENU, LOADING, INGAME };

class Game {
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
    Meta::AnyFunction fadeOutCallback = []() {};

    struct {
        std::shared_ptr<BackgroundSystem> backgrounds;
        std::shared_ptr<GameplayScriptSystem> gameplayscript;
        std::shared_ptr<ShaderWorkerSystem> shaderworker;
        std::shared_ptr<ConsoleSystem> console;

        MetaEngine::vector<std::shared_ptr<IGameSystem>> systemList = {};

        GlobalDEF globaldef;
        World *world = nullptr;
        TexturePack *texturepack = nullptr;

        ThreadPool *updateDirtyPool = nullptr;
        ThreadPoolC updateDirtyPool2;
    } GameIsolate_;

    struct {
        R_Image *backgroundImage = nullptr;

        R_Image *loadingTexture = nullptr;
        MetaEngine::vector<U8> pixelsLoading;
        U8 *pixelsLoading_ar = nullptr;
        int loadingScreenW = 0;
        int loadingScreenH = 0;

        R_Image *worldTexture = nullptr;
        R_Image *lightingTexture = nullptr;

        R_Image *emissionTexture = nullptr;
        MetaEngine::vector<U8> pixelsEmission;
        U8 *pixelsEmission_ar = nullptr;

        R_Image *texture = nullptr;
        MetaEngine::vector<U8> pixels;
        U8 *pixels_ar = nullptr;
        R_Image *textureLayer2 = nullptr;
        MetaEngine::vector<U8> pixelsLayer2;
        U8 *pixelsLayer2_ar = nullptr;
        R_Image *textureBackground = nullptr;
        MetaEngine::vector<U8> pixelsBackground;
        U8 *pixelsBackground_ar = nullptr;
        R_Image *textureObjects = nullptr;
        R_Image *textureObjectsLQ = nullptr;
        MetaEngine::vector<U8> pixelsObjects;
        U8 *pixelsObjects_ar = nullptr;
        R_Image *textureObjectsBack = nullptr;
        R_Image *textureParticles = nullptr;
        MetaEngine::vector<U8> pixelsParticles;
        U8 *pixelsParticles_ar = nullptr;
        R_Image *textureEntities = nullptr;
        R_Image *textureEntitiesLQ = nullptr;

        R_Image *textureFire = nullptr;
        R_Image *texture2Fire = nullptr;
        MetaEngine::vector<U8> pixelsFire;
        U8 *pixelsFire_ar = nullptr;

        R_Image *textureFlowSpead = nullptr;
        R_Image *textureFlow = nullptr;
        MetaEngine::vector<U8> pixelsFlow;
        U8 *pixelsFlow_ar = nullptr;

        R_Image *temperatureMap = nullptr;
        MetaEngine::vector<U8> pixelsTemp;
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
