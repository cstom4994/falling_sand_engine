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
#include "ui/imgui/imgui_impl.hpp"
#include "core/cpp/csingleton.h"

class Game;
class Scripting;
class UIData;

struct Global {
    Game *game = nullptr;
    GameData GameData_;
    UIData *uidata = nullptr;
    Audio audioEngine;
    I18N I18N;
};

extern Global global;

#endif