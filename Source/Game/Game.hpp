// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GAME_HPP_
#define _METADOT_GAME_HPP_

//#define b2_maxTranslation 10.0f
//#define b2_maxTranslationSquared (b2_maxTranslation * b2_maxTranslation)

#include "Background.hpp"
#include "Controls.hpp"
#include "Core/Const.hpp"
#include "Core/DebugImpl.hpp"
#include "Core/Macros.hpp"
#include "Engine/Audio.h"
#include "Engine/Internal/BuiltinBox2d.h"
#include "Engine/Networking.hpp"
#include "Engine/RendererGPU.h"
#include "Engine/Scripting.hpp"
#include "Game/Console.hpp"
#include "Game/FileSystem.hpp"
#include "Game/GameDataStruct.hpp"
#include "Game/GameResources.hpp"
#include "Game/GameScriptingWrap.hpp"
#include "Game/ImGuiCore.hpp"
#include "Game/Utils.hpp"
#include "Libs/sparsehash/sparse_hash_map.h"
#include "world.hpp"

#include <algorithm>
#include <chrono>
#include <codecvt>
#include <functional>
#include <iostream>
#include <thread>
#include <unordered_map>

enum GameState {
    MAIN_MENU,
    LOADING,
    INGAME
};

struct GameTimeState
{
    int fps = 0;
    int feelsLikeFps = 0;
    long long lastTime = 0;
    long long lastTick = 0;
    long long lastLoadingTick = 0;
    long long now = 0;
    long long startTime = 0;
    long long deltaTime;
    long mspt = 0;
};

class Game {
private:
    int argc;

    GameState state = LOADING;
    GameState stateAfterLoad = MAIN_MENU;

    uint16_t *frameTime = new uint16_t[FrameTimeNum];

    DebugDraw *debugDraw;

    int ent_prevLoadZoneX = 0;
    int ent_prevLoadZoneY = 0;
    ThreadPool *updateDirtyPool = nullptr;
    ThreadPool *rotateVectorsPool = nullptr;

    UInt16 *movingTiles;

    int mx = 0;
    int my = 0;
    int lastDrawMX = 0;
    int lastDrawMY = 0;
    int lastEraseMX = 0;
    int lastEraseMY = 0;

    UInt8 *objectDelete = nullptr;

    uint32 loadingOnColor = 0;
    uint32 loadingOffColor = 0;

    METAENGINE_Render_GLTtext *text1 = nullptr;
    METAENGINE_Render_GLTtext *text2 = nullptr;
    METAENGINE_Render_GLTtext *text3 = nullptr;

public:
    bool running = true;

    int tickTime = 0;

    int scale = 4;
    float accLoadX = 0;
    float accLoadY = 0;

    long long fadeInStart = 0;
    long long fadeInLength = 0;
    int fadeInWaitFrames = 0;
    int fadeOutWaitFrames = 0;
    long long fadeOutStart = 0;
    long long fadeOutLength = 0;
    std::function<void()> fadeOutCallback = []() {};

    struct
    {
        Backgrounds *backgrounds = nullptr;
        GameTimeState game_timestate;
        Profiler profiler;
        Settings settings;
        World *world = nullptr;
    } GameIsolate_;

    struct
    {
        Console console;
        GameScriptingWrap gameScriptwrap;
    } GameSystem_;

    struct
    {
        METAENGINE_Render_Target *realTarget = nullptr;
        METAENGINE_Render_Target *target = nullptr;
    } RenderTarget_;

    struct
    {
        METAENGINE_Render_Image *backgroundImage = nullptr;

        METAENGINE_Render_Image *loadingTexture = nullptr;
        std::vector<UInt8> pixelsLoading;
        UInt8 *pixelsLoading_ar = nullptr;
        int loadingScreenW = 0;
        int loadingScreenH = 0;

        METAENGINE_Render_Image *worldTexture = nullptr;
        METAENGINE_Render_Image *lightingTexture = nullptr;

        METAENGINE_Render_Image *emissionTexture = nullptr;
        std::vector<UInt8> pixelsEmission;
        UInt8 *pixelsEmission_ar = nullptr;

        METAENGINE_Render_Image *texture = nullptr;
        std::vector<UInt8> pixels;
        UInt8 *pixels_ar = nullptr;
        METAENGINE_Render_Image *textureLayer2 = nullptr;
        std::vector<UInt8> pixelsLayer2;
        UInt8 *pixelsLayer2_ar = nullptr;
        METAENGINE_Render_Image *textureBackground = nullptr;
        std::vector<UInt8> pixelsBackground;
        UInt8 *pixelsBackground_ar = nullptr;
        METAENGINE_Render_Image *textureObjects = nullptr;
        METAENGINE_Render_Image *textureObjectsLQ = nullptr;
        std::vector<UInt8> pixelsObjects;
        UInt8 *pixelsObjects_ar = nullptr;
        METAENGINE_Render_Image *textureObjectsBack = nullptr;
        METAENGINE_Render_Image *textureParticles = nullptr;
        std::vector<UInt8> pixelsParticles;
        UInt8 *pixelsParticles_ar = nullptr;
        METAENGINE_Render_Image *textureEntities = nullptr;
        METAENGINE_Render_Image *textureEntitiesLQ = nullptr;

        METAENGINE_Render_Image *textureFire = nullptr;
        METAENGINE_Render_Image *texture2Fire = nullptr;
        std::vector<UInt8> pixelsFire;
        UInt8 *pixelsFire_ar = nullptr;

        METAENGINE_Render_Image *textureFlowSpead = nullptr;
        METAENGINE_Render_Image *textureFlow = nullptr;
        std::vector<UInt8> pixelsFlow;
        UInt8 *pixelsFlow_ar = nullptr;

        METAENGINE_Render_Image *temperatureMap = nullptr;
        std::vector<UInt8> pixelsTemp;
        UInt8 *pixelsTemp_ar = nullptr;
    } TexturePack_;

public:
    GameState getGameState() const { return state; }
    void setGameState(GameState state, std::optional<GameState> stateal) {
        this->state = state;
        if (stateal.has_value()) { this->stateAfterLoad = stateal.value(); }
    }

    Game(int argc, char *argv[]);
    ~Game();

    int init(int argc, char *argv[]);
    int run(int argc, char *argv[]);
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
    int getAimSolidSurface(int dist);
    int getAimSurface(int dist);
    void quitToMainMenu();
};

#endif