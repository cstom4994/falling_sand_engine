// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_GLOBAL_HPP_
#define _METADOT_GLOBAL_HPP_

#include <map>
#include <unordered_map>

#include "core/core.hpp"
#include "engine/audio.hpp"
#include "engine/code_reflection.hpp"
#include "engine/engine.h"
#include "engine/engine_platform.h"
#include "engine/filesystem.h"
#include "engine/imgui_impl.hpp"
#include "game/game_shaders.hpp"
#include "game/game_ui.hpp"

class Game;
class Scripts;
class ImGuiCore;

struct Global {
    Game *game = nullptr;
    Scripts *scripts = nullptr;

    ImGuiCore *ImGuiCore = nullptr;

    GameData GameData_;
    ShaderWorker shaderworker;
    Audio audioEngine;
    I18N I18N;
};

extern Global global;

#endif