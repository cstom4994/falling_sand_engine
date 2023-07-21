// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_ENGINE_H
#define ME_ENGINE_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <string>

#include "engine/core/core.hpp"
#include "engine/core/platform.h"
#include "engine/core/sdl_wrapper.h"
#include "engine/renderer/gpu.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/utils/module.hpp"
#include "engine/utils/utility.hpp"

namespace ME {

template <typename Module, typename... Args>
Module& safe_module_initialize(Args&&... args) {
    return modules::is_initialized<Module>() ? modules::instance<Module>() : modules::initialize<Module>(std::forward<Args>(args)...);
}

typedef struct engine_data {
    C_Window* window;
    C_GLContext* glContext;

    std::string gamepath;
    std::string exepath;

    // Maximum memory that can be used
    u64 max_mem = 4294967296;  // 4096mb

    int windowWidth;
    int windowHeight;

    i32 render_scale = 4;

    unsigned maxFPS;

    R_Target* realTarget;
    R_Target* target;

    struct {
        i32 feelsLikeFps;
        i64 lastTime;
        i64 lastCheckTime;
        i64 lastTickTime;
        i64 lastLoadingTick;
        i64 now;
        i64 startTime;
        i64 deltaTime;

        i32 tickCount;

        f32 mspt;
        i32 tpsTrace[TraceTimeNum];
        f32 tps;
        u32 maxTps;

        u16 frameTimesTrace[TraceTimeNum];
        u32 frameCount;
        f32 framesPerSecond;
    } time;

} engine_data;

class engine final : public module<engine> {
public:
    class application;
    using application_uptr = scope<application>;

public:
    class parameters;

public:
    engine(int argc, char* argv[], const parameters& params);
    ~engine() noexcept final;

    template <typename Application, typename... Args>
    bool start(Args&&... args);
    bool start(application_uptr app);

    bool is_running() const noexcept { return m_initialized_engine; }
    bool& running() noexcept { return m_initialized_engine; }
    engine_data* eng() noexcept { return &m_eng; }

    int init_eng();
    void update_post();
    void update_time();
    void update_end();
    void end_eng(int errorOcurred);
    void draw_splash();

    int init_core();
    bool init_time();
    bool init_screen(int windowWidth, int windowHeight, int scale, int maxFPS);

    f32 fps() const noexcept { return m_eng.time.framesPerSecond; };

private:
    void process_tick_time();

private:
    engine_data m_eng;

    bool m_initialized_engine = false;

    // ƽ̨
    int m_argc;
    char** m_argv;
};

class engine::parameters {
public:
    parameters() = delete;
    parameters(std::string game_name, std::string debug_args) noexcept;

    ME_INLINE parameters& get() noexcept { return *this; }

    parameters& game_name(std::string value) noexcept;
    std::string& game_name() noexcept;
    const std::string& game_name() const noexcept;

private:
    std::string m_game_name{"noname"};
    std::string m_debug_args{"debugargs"};
};

class engine::application : private ME::noncopyable {
public:
    virtual ~application() noexcept = default;
    virtual bool initialize(int argc, char* argv[]) = 0;
    virtual void shutdown() noexcept = 0;
    virtual bool frame_tick() = 0;
    virtual void frame_render() = 0;
    virtual void frame_finalize() = 0;
};

template <typename Application, typename... Args>
ME_INLINE bool engine::start(Args&&... args) {
    return start(std::make_unique<Application>(std::forward<Args>(args)...));
}

}  // namespace ME

#endif