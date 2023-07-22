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

#include "background.hpp"
#include "cvar.hpp"
#include "engine/audio/audio.h"
#include "engine/core/base_debug.hpp"
#include "engine/core/const.h"
#include "engine/core/io/filesystem.h"
#include "engine/core/io/packer.hpp"
#include "engine/core/macros.hpp"
#include "engine/event/applicationevent.hpp"
#include "engine/event/event.hpp"
#include "engine/event/inputevent.hpp"
#include "engine/game_utils/rng.h"
#include "engine/meta/reflection.hpp"
#include "engine/physics/inc/physics2d.h"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/scripting.hpp"
#include "engine/ui/dbgui.hpp"
#include "engine/ui/font.hpp"
#include "engine/ui/ui.hpp"
#include "game_basic.hpp"
#include "game_datastruct.hpp"
#include "game_shaders.hpp"
#include "textures.hpp"
#include "world.hpp"

namespace ME {

using EventCallbackFn = std::function<void(Event &)>;
using SystemList = std::vector<ref<IGameSystem>>;

struct MEsurface_context;

enum EnumGameState { MAIN_MENU, LOADING, INGAME };

struct ME_assets_handle_t {
    const u8 *data;
    u32 size;
};

struct Iso_t {
    ref<BackgroundSystem> backgrounds;
    ref<gameplay> gameplayscript;
    ref<shader_worker> shaderworker;
    ref<gui> ui;

    SystemList systemList = {};

    ME_pack_reader pack_reader;

    GlobalDEF globaldef;
    scope<world> world;
    TexturePack texturepack;

    thread_pool *updateDirtyPool = nullptr;
    thread_pool *updateDirtyPool2 = nullptr;
};

struct TexturePack_t {
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
};

class game final : public engine::application {
public:
    bool initialize(int argc, char *argv[]) override;
    void shutdown() noexcept override;
    bool frame_tick() override;
    void frame_render() override;
    void frame_finalize() override;

public:
    EnumGameState state = LOADING;
    EnumGameState stateAfterLoad = MAIN_MENU;

    // ME_debugdraw *debugDraw;
    // fontcache fontcache;

    MEsurface_context *surface;
    int fontNormal;

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

    // profiler
    profiler_graph fps, cpuGraph;

    i64 fadeInStart = 0;
    i64 fadeInLength = 0;
    i32 fadeInWaitFrames = 0;
    i32 fadeOutWaitFrames = 0;
    i64 fadeOutStart = 0;
    i64 fadeOutLength = 0;
    meta::any_function fadeOutCallback = []() {};

    Iso_t Iso;
    TexturePack_t TexturePack_;

    EventCallbackFn EventCallback;

    BackgroundObject *bg = nullptr;

public:
    EnumGameState getGameState() const { return state; }
    void setGameState(EnumGameState state, std::optional<EnumGameState> stateal) {
        this->state = state;
        if (stateal.has_value()) {
            this->stateAfterLoad = stateal.value();
        }
    }

    game() = default;
    ~game() = default;

    int init(int argc, char *argv[]);
    int run(int argc, char *argv[]);
    int exit();
    void updateFrameEarly();
    void onEvent(Event &e);
    bool onWindowClose(WindowCloseEvent &e);
    bool onWindowResize(WindowResizeEvent &e);
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
    void renderTemperatureMap(world *world);
    void ResolutionChanged(int newWidth, int newHeight);
    int getAimSolidSurface(int dist);
    int getAimSurface(int dist);
    void quitToMainMenu();
    ME_assets_handle_t get_assets(std::string path);
};

}  // namespace ME

#endif
