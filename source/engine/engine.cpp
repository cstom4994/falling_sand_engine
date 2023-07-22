// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine/engine.hpp"

#include <cstdlib>

#include "engine/core/base_memory.h"
#include "engine/core/core.hpp"
#include "engine/core/global.hpp"
#include "engine/core/io/filesystem.h"
#include "engine/core/platform.h"
#include "game.hpp"
#include "textures.hpp"

////////////////////////////////////////////////////////////////////////////////

namespace ME {

int engine::init_eng() {

    if (the<engine>().is_running()) {
        METADOT_WARN("InitEngine: Engine already initialized");
    }

    METADOT_INFO("Initializing Engine...");

    bool init = init_time() || ME_fs_init() || init_screen(960, 540, 1, 60) || init_core() || ME_initwindow();

    if (init) {
        end_eng(1);
        return METADOT_FAILED;
    }

    init_reflection();

    the<engine>().running() = true;

    METADOT_INFO("Engine sucessfully initialized!");

    return METADOT_OK;
}

void engine::update_post() { update_time(); }

void engine::update_end() { process_tick_time(); }

void engine::end_eng(int errorOcurred) {

    global.audio.Shutdown();
    ME_endwindow();

    if (SDL_WasInit(SDL_INIT_EVERYTHING) != 0) {
        METADOT_INFO("SDL quit");
        SDL_Quit();
    }

    if (errorOcurred) {
        METADOT_WARN("Engine finished with errors!");
        abort();
    } else {
        METADOT_INFO("Engine finished sucessfully");
    }
}

void engine::draw_splash() {
    R_Clear(m_eng.target);
    R_Flip(m_eng.target);
    TextureRef splashSurf = LoadTexture("data/assets/ui/splash.png");
    R_Image *splashImg = R_CopyImageFromSurface(splashSurf->surface());
    R_SetImageFilter(splashImg, R_FILTER_NEAREST);
    R_BlitRect(splashImg, NULL, m_eng.target, NULL);
    R_FreeImage(splashImg);
    splashSurf.reset();
    R_Flip(m_eng.target);
}

int engine::init_core() { return METADOT_OK; }

bool engine::init_time() {

    m_eng.time.startTime = ME_gettime();
    m_eng.time.lastTime = m_eng.time.startTime;
    m_eng.time.lastTickTime = m_eng.time.lastTime;
    m_eng.time.lastCheckTime = m_eng.time.lastTime;

    m_eng.time.mspt = 0;
    m_eng.time.maxTps = 30;
    m_eng.time.framesPerSecond = 0;
    m_eng.time.tickCount = 0;
    m_eng.time.frameCount = 0;

    memset(m_eng.time.tpsTrace, 0, sizeof(m_eng.time.tpsTrace));
    memset(m_eng.time.frameTimesTrace, 0, sizeof(m_eng.time.frameTimesTrace));

    return METADOT_OK;
}

bool engine::init_screen(int windowWidth, int windowHeight, int scale, int maxFPS) {
    m_eng.windowWidth = windowWidth;
    m_eng.windowHeight = windowHeight;
    m_eng.maxFPS = maxFPS;
    m_eng.render_scale = scale;
    return METADOT_OK;
}

void engine::update_time() {
    m_eng.time.now = ME_gettime();
    m_eng.time.deltaTime = m_eng.time.now - m_eng.time.lastTime;
}

void engine::process_tick_time() {

    m_eng.time.frameCount++;
    if (m_eng.time.now - m_eng.time.lastCheckTime >= 1000.0f) {
        m_eng.time.lastCheckTime = m_eng.time.now;
        m_eng.time.framesPerSecond = m_eng.time.frameCount;
        m_eng.time.frameCount = 0;

        // calculate "feels like" fps
        f32 sum = 0;
        f32 num = 0.01;

        for (int i = 0; i < TraceTimeNum; i++) {
            f32 weight = m_eng.time.frameTimesTrace[i];
            sum += weight * m_eng.time.frameTimesTrace[i];
            num += weight;
        }

        m_eng.time.feelsLikeFps = 1000 / (sum / num);

        // Update tps trace
        for (int i = 1; i < TraceTimeNum; i++) {
            m_eng.time.tpsTrace[i - 1] = m_eng.time.tpsTrace[i];
        }
        m_eng.time.tpsTrace[TraceTimeNum - 1] = m_eng.time.tickCount;

        // Calculate tps
        sum = 0;

        int n = 0;
        for (int i = 0; i < TraceTimeNum; i++)
            if (m_eng.time.tpsTrace[i]) {
                sum += m_eng.time.tpsTrace[i];
                n++;
            }

        // m_eng.time.tps = 1000.0f / (sum / num);
        m_eng.time.tps = sum / n;

        m_eng.time.tickCount = 0;
    }

    m_eng.time.mspt = 1000.0f / m_eng.time.tps;

    for (int i = 1; i < TraceTimeNum; i++) {
        m_eng.time.frameTimesTrace[i - 1] = m_eng.time.frameTimesTrace[i];
    }
    m_eng.time.frameTimesTrace[TraceTimeNum - 1] = (u16)(ME_gettime() - m_eng.time.now);

    m_eng.time.lastTime = m_eng.time.now;
}

engine::parameters::parameters(std::string game_name, std::string debug_args) noexcept : m_game_name(std::move(game_name)), m_debug_args(std::move(debug_args)) {}

engine::engine(int argc, char *argv[], const parameters &params) : m_argc(argc) { m_argv = argv; }

engine::~engine() noexcept {}

bool engine::start(application_uptr app) {
    ME_ASSERT(is_in_main_thread());

    app->initialize(m_argc, m_argv);
    app->shutdown();

    return true;
}

}  // namespace ME