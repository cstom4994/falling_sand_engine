// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GLOBAL_HPP_
#define _METADOT_GLOBAL_HPP_

#include <map>

#include "core/core.hpp"
#include "engine/audio.hpp"
#include "engine/code_reflection.hpp"
#include "game/game_cpp.h"
#include "engine/engine_cpp.h"
#include "engine/engine_platform.h"
#include "engine/filesystem.h"
#include "engine/imgui_impl.hpp"
#include "game/game_ui.hpp"

class Game;
class Scripts;
class ImGuiCore;

#define RegisterFunctions(name, func)              \
    MetaEngine::any_function func_log_info{&func}; \
    global->HostData->Functions.insert(std::make_pair(#name, name))

struct Global {
    Game *game = nullptr;
    Scripts *scripts = nullptr;

    ImGuiCore *ImGuiCore = nullptr;

    ShaderWorker shaderworker;
    Audio audioEngine;
    I18N I18N;

    struct {
        ImGuiContext *imgui_context = nullptr;
        void *wndh = nullptr;

        std::map<std::string, Meta::any_function> Functions;

        // CppSource Functions register
        void (*draw)(void);
    } HostData;

    Global() {}
};

extern Global global;

#endif