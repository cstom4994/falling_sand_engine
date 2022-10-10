// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.


#pragma once

//#define b2_maxTranslation 10.0f
//#define b2_maxTranslationSquared (b2_maxTranslation * b2_maxTranslation)

#include "Macros.hpp"

#include <SDL.h>

#include "Engine/render/renderer_gpu.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>

#include "Drawing.hpp"
#include "Networking.hpp"

#include "lib/sparsehash/sparse_hash_map.h"
#ifndef INC_World
#include "world.hpp"
#endif
#include "Background.hpp"
#include "Controls.hpp"
#include "Settings.hpp"
#include "Textures.hpp"
#include "Utils.hpp"
#include <box2d/b2_distance_joint.h>
#include <chrono>
#include <thread>
#ifdef _WIN32
#include <io.h>
#else
#include <sys/io.h>
#endif
#include "Drawing.hpp"
#include "Shaders.hpp"
#include <codecvt>

#include "DebugImpl.hpp"

#include "Engine/FileSystem.hpp"
#include "Engine/ImGuiLayer.hpp"

#include "Shared/Interface.hpp"


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


class Game {
public:
    static const int MAX_WIDTH = 1920;
    static const int MAX_HEIGHT = 1080;

    GameState state = LOADING;
    GameState stateAfterLoad = MAIN_MENU;
    int networkMode = -1;
    Client *client = nullptr;
    Server *server = nullptr;

    static HostData data;

    bool steamAPI = false;

    CAudioEngine audioEngine;

    int WIDTH = 1360;
    int HEIGHT = 870;

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

    b2DebugDraw_impl *b2DebugDraw;

    int ent_prevLoadZoneX = 0;
    int ent_prevLoadZoneY = 0;
    ctpl::thread_pool *updateDirtyPool = nullptr;
    ctpl::thread_pool *rotateVectorsPool = nullptr;

    uint16_t *movingTiles;

    int tickTime = 0;

    bool running = true;

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

    WaterShader *waterShader = nullptr;
    WaterFlowPassShader *waterFlowPassShader = nullptr;
    NewLightingShader *newLightingShader = nullptr;
    float newLightingShader_insideDes = 0.0f;
    float newLightingShader_insideCur = 0.0f;
    FireShader *fireShader = nullptr;
    Fire2Shader *fire2Shader = nullptr;

    Backgrounds *backgrounds = nullptr;

    int fps = 0;
    int feelsLikeFps = 0;
    long long lastTime = 0;
    long long lastTick = 0;
    long long lastLoadingTick = 0;
    long long now = 0;
    long long startTime = 0;
    long long deltaTime;
    long mspt = 0;
    uint32 loadingOnColor = 0;
    uint32 loadingOffColor = 0;

    DrawTextParams_t dt_versionInfo1;
    DrawTextParams_t dt_versionInfo2;
    DrawTextParams_t dt_versionInfo3;

    DrawTextParams_t dt_fps;
    DrawTextParams_t dt_feelsLikeFps;

    DrawTextParams_t dt_frameGraph[5];
    DrawTextParams_t dt_loading;

    long long fadeInStart = 0;
    long long fadeInLength = 0;
    int fadeInWaitFrames = 0;

    long long fadeOutStart = 0;`
    long long fadeOutLength = 0;
    int fadeOutWaitFrames = 0;
    std::function<void()> fadeOutCallback = []() {};

    MetaEngine::GameDir gameDir;
    MetaEngine::ImGuiLayer *m_ImGuiLayer = nullptr;
    MetaEngine::ModuleStack *m_ModuleStack = nullptr;

    void loadShaders();
    void updateMaterialSounds();
    void setDisplayMode(DisplayMode mode);
    void setVSync(bool vsync);
    void setMinimizeOnLostFocus(bool minimize);
    void setWindowFlash(WindowFlashAction action, int count, int period);
    void handleWindowSizeChange(int newWidth, int newHeight);

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
