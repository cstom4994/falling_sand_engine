// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_GLOBAL_HPP_
#define _METADOT_GLOBAL_HPP_

#include <map>
#include <unordered_map>

#include "audio/audio.h"
#include "meta/meta.hpp"
#include "core/core.hpp"
#include "engine/engine.h"
#include "engine/engine_platform.h"
#include "filesystem.h"
#include "game_shaders.hpp"
#include "game_ui.hpp"
#include "imgui/imgui_impl.hpp"

class Game;
class Scripts;
class UIData;

struct Global {
    Game *game = nullptr;
    Scripts *scripts = nullptr;
    UIData *uidata = nullptr;
    GameData GameData_;
    Audio audioEngine;
    I18N I18N;
};

extern Global global;

#endif