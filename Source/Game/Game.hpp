// Copyright(c) 2022, KaoruXun All rights reserved.


#pragma once

//#define b2_maxTranslation 10.0f
//#define b2_maxTranslationSquared (b2_maxTranslation * b2_maxTranslation)

#include "DebugImpl.hpp"
#include "Drawing.hpp"
#include "Engine/AudioEngine/AudioEngine.h"
#include "Engine/Render/renderer_gpu.h"
#include "Game/Console.hpp"
#include "Game/FileSystem.hpp"
#include "Game/ImGuiLayer.hpp"
#include "Game/ImGuiTerminal.hpp"
#include "Game/ModuleStack.h"
#include "Libs/sparsehash/sparse_hash_map.h"
#include "Macros.hpp"
#include "Networking.hpp"
#include "Shared/Interface.hpp"
#ifndef INC_World
#include "world.hpp"
#endif
#include "Background.hpp"
#include "Controls.hpp"
#include "Settings.hpp"
#include "Textures.hpp"
#include "Utils.hpp"

#include <SDL.h>

#include <algorithm>
#include <box2d/b2_distance_joint.h>
#include <chrono>
#include <codecvt>
#include <iostream>
#include <thread>
#include <unordered_map>

#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#endif

#define frameTimeNum 100

enum GameState {
    MAIN_MENU,
    LOADING,
    INGAME
};

enum DisplayMode {
    WINDOWED,
    BORDERLESS,
    FULLSCREEN
};

enum WindowFlashAction {
    START,
    START_COUNT,
    START_UNTIL_FG,
    STOP
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
    GameState state = LOADING;
    GameState stateAfterLoad = MAIN_MENU;

    int networkMode = -1;
    Client *client = nullptr;
    Server *server = nullptr;

    custom_command_struct cmd_struct;
    ImTerm::terminal<terminal_commands> *terminal_log;

    CAudioEngine audioEngine;

    int scale = 4;

    int ofsX = 0;
    int ofsY = 0;

    float plPosX = 0;
    float plPosY = 0;

    float camX = 0;
    float camY = 0;

    float desCamX = 0;
    float desCamY = 0;

    float freeCamX = 0;
    float freeCamY = 0;

    uint16_t *frameTime = new uint16_t[frameTimeNum];

    STBTTF_Font *font64;
    STBTTF_Font *font16;
    STBTTF_Font *font14;

    SDL_Window *window = nullptr;

    b2DebugDraw_impl *b2DebugDraw;

    int ent_prevLoadZoneX = 0;
    int ent_prevLoadZoneY = 0;
    ctpl::thread_pool *updateDirtyPool = nullptr;
    ctpl::thread_pool *rotateVectorsPool = nullptr;

    uint16_t *movingTiles;

    int tickTime = 0;

    World *world = nullptr;

    float accLoadX = 0;
    float accLoadY = 0;

    int mx = 0;
    int my = 0;
    int lastDrawMX = 0;
    int lastDrawMY = 0;
    int lastEraseMX = 0;
    int lastEraseMY = 0;

    bool *objectDelete = nullptr;

    Backgrounds *backgrounds = nullptr;

    GameTimeState game_timestate;

    uint32 loadingOnColor = 0;
    uint32 loadingOffColor = 0;

    DrawTextParams_t dt_versionInfo1;
    DrawTextParams_t dt_versionInfo2;
    DrawTextParams_t dt_versionInfo3;

    DrawTextParams_t dt_fps;
    DrawTextParams_t dt_feelsLikeFps;

    DrawTextParams_t dt_frameGraph[5];
    DrawTextParams_t dt_loading;

    MetaEngine::GameDir m_GameDir;
    MetaEngine::ImGuiLayer *m_ImGuiLayer = nullptr;
    MetaEngine::ModuleStack *m_ModuleStack = nullptr;

public:

    int WIDTH = 1360;
    int HEIGHT = 870;

    bool running = true;

    HostData *data;

    long long fadeInStart = 0;
    long long fadeInLength = 0;
    int fadeInWaitFrames = 0;
    int fadeOutWaitFrames = 0;
    long long fadeOutStart = 0;
    long long fadeOutLength = 0;
    std::function<void()> fadeOutCallback = []() {};


    METAENGINE_Render_Target *realTarget = nullptr;
    METAENGINE_Render_Target *target = nullptr;

    METAENGINE_Render_Image *backgroundImage = nullptr;

    METAENGINE_Render_Image *loadingTexture = nullptr;
    std::vector<unsigned char> pixelsLoading;
    unsigned char *pixelsLoading_ar = nullptr;
    int loadingScreenW = 0;
    int loadingScreenH = 0;

    METAENGINE_Render_Image *worldTexture = nullptr;
    METAENGINE_Render_Image *lightingTexture = nullptr;

    METAENGINE_Render_Image *emissionTexture = nullptr;
    std::vector<unsigned char> pixelsEmission;
    unsigned char *pixelsEmission_ar = nullptr;

    METAENGINE_Render_Image *texture = nullptr;
    std::vector<unsigned char> pixels;
    unsigned char *pixels_ar = nullptr;
    METAENGINE_Render_Image *textureLayer2 = nullptr;
    std::vector<unsigned char> pixelsLayer2;
    unsigned char *pixelsLayer2_ar = nullptr;
    METAENGINE_Render_Image *textureBackground = nullptr;
    std::vector<unsigned char> pixelsBackground;
    unsigned char *pixelsBackground_ar = nullptr;
    METAENGINE_Render_Image *textureObjects = nullptr;
    METAENGINE_Render_Image *textureObjectsLQ = nullptr;
    std::vector<unsigned char> pixelsObjects;
    unsigned char *pixelsObjects_ar = nullptr;
    METAENGINE_Render_Image *textureObjectsBack = nullptr;
    METAENGINE_Render_Image *textureParticles = nullptr;
    std::vector<unsigned char> pixelsParticles;
    unsigned char *pixelsParticles_ar = nullptr;
    METAENGINE_Render_Image *textureEntities = nullptr;
    METAENGINE_Render_Image *textureEntitiesLQ = nullptr;

    METAENGINE_Render_Image *textureFire = nullptr;
    METAENGINE_Render_Image *texture2Fire = nullptr;
    std::vector<unsigned char> pixelsFire;
    unsigned char *pixelsFire_ar = nullptr;

    METAENGINE_Render_Image *textureFlowSpead = nullptr;
    METAENGINE_Render_Image *textureFlow = nullptr;
    std::vector<unsigned char> pixelsFlow;
    unsigned char *pixelsFlow_ar = nullptr;

    METAENGINE_Render_Image *temperatureMap = nullptr;
    std::vector<unsigned char> pixelsTemp;
    unsigned char *pixelsTemp_ar = nullptr;

public:
    MetaEngine::ModuleStack *getModuleStack() const { return m_ModuleStack; }
    MetaEngine::ImGuiLayer *getImGuiLayer() const { return m_ImGuiLayer; }
    CAudioEngine *getCAudioEngine() { return &audioEngine; }
    World *getWorld() { return world; }
    void setWorld(World *ptr) { world = ptr; }
    MetaEngine::GameDir *getGameDir() { return &m_GameDir; }
    GameTimeState &getGameTimeState() { return game_timestate; }
    GameState getGameState() const { return state; }
    void setGameState(GameState state, std::optional<GameState> stateal) {
        this->state = state;
        if (stateal.has_value()) { this->stateAfterLoad = stateal.value(); }
    }
    int getNetworkMode() const { return networkMode; }
    void setNetworkMode(NetworkMode networkMode) { this->networkMode = networkMode; }
    Client *getClient() { return client; }
    Server *getServer() { return server; }

    void loadShaders();
    void updateMaterialSounds();
    void setDisplayMode(DisplayMode mode);
    void setVSync(bool vsync);
    void setMinimizeOnLostFocus(bool minimize);
    void setWindowFlash(WindowFlashAction action, int count, int period);
    void handleWindowSizeChange(int newWidth, int newHeight);

    Game();
    ~Game();

    int init(int argc, char *argv[]);

    int run(int argc, char *argv[]);

    void updateFrameEarly();
    void tick();
    void tickChunkLoading();
    void tickPlayer();
    void updateFrameLate();
    void renderOverlays();

    void renderEarly();
    void renderLate();

    void renderTemperatureMap(World *world);

    int getAimSolidSurface(int dist);
    int getAimSurface(int dist);

    void quitToMainMenu();
};
