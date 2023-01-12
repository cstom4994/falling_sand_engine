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

#include "background.hpp"
#include "controls.hpp"
#include "core/const.h"
#include "core/debug_impl.hpp"
#include "core/macros.h"
#include "core/threadpool.h"
#include "engine/audio.hpp"
#include "engine/console.hpp"
#include "engine/engine_scripting.hpp"
#include "engine/filesystem.h"
#include "engine/fonts.h"
#include "engine/imgui_core.hpp"
#include "engine/internal/builtin_box2d.h"
#include "engine/renderer/renderer_gpu.h"
#include "engine/utils.hpp"
#include "game/game_datastruct.hpp"
#include "game/game_resources.hpp"
#include "game/game_scriptingwrap.hpp"
#include "game/game_shaders.hpp"
#include "libs/sparsehash/sparse_hash_map.h"
#include "world.hpp"

enum GameState { MAIN_MENU, LOADING, INGAME };



class Game {
public:
    I32 argc;

    GameState state = LOADING;
    GameState stateAfterLoad = MAIN_MENU;

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

    FontCache_Font *font = nullptr;

public:
    bool running = true;

    I32 tickTime = 0;

    I32 scale = 4;
    F32 accLoadX = 0;
    F32 accLoadY = 0;

    I64 fadeInStart = 0;
    I64 fadeInLength = 0;
    I32 fadeInWaitFrames = 0;
    I32 fadeOutWaitFrames = 0;
    I64 fadeOutStart = 0;
    I64 fadeOutLength = 0;
    std::function<void()> fadeOutCallback = []() {};

    struct {
        Backgrounds *backgrounds = nullptr;
        Profiler profiler;
        GlobalDEF globaldef;
        World *world = nullptr;
        TexturePack *texturepack = nullptr;

        ThreadPool *updateDirtyPool = nullptr;
        ThreadPoolC updateDirtyPool2;
    } GameIsolate_;

    struct {
        Console console;
    } GameSystem_;

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
        R_Image *textureParticles = nullptr;
        std::vector<U8> pixelsParticles;
        U8 *pixelsParticles_ar = nullptr;
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
    GameState getGameState() const { return state; }
    void setGameState(GameState state, std::optional<GameState> stateal) {
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
    void updateFrameLate();
    void renderOverlays();
    void updateMaterialSounds();
    void createTexture();
    void renderEarly();
    void renderLate();
    void renderTemperatureMap(World *world);
    void resolu(int newWidth, int newHeight);
    int getAimSolidSurface(int dist);
    int getAimSurface(int dist);
    void quitToMainMenu();
};

#endif
