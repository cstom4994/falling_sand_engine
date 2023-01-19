// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "game_basic.hpp"

#include <string>
#include <string_view>

#include "core/core.hpp"
#include "core/global.hpp"
#include "engine/filesystem.h"
#include "engine/reflectionflat.hpp"
#include "engine_scripting.hpp"
#include "game/controls.hpp"
#include "game/game.hpp"
#include "game/game_datastruct.hpp"
#include "game/game_resources.hpp"
#include "game/game_ui.hpp"
#include "scripting/lua_wrapper.hpp"

#pragma region GameScriptingBind_1

static void create_biome(std::string name, int id) {
    Biome *b = new Biome(name, id);
    global.GameData_.biome_container.push_back(b);
}

static void audio_init() { global.audioEngine.Init(); }

static void audio_load_bank(std::string name, unsigned int type) {
#if defined(METADOT_BUILD_AUDIO)
    global.audioEngine.LoadBank(METADOT_RESLOC(name.c_str()), type);
#endif
}

static void audio_load_event(std::string event) { global.audioEngine.LoadEvent(event); }
static void audio_play_event(std::string event) { global.audioEngine.PlayEvent(event); }

static void textures_init() { InitTexture(global.game->GameIsolate_.texturepack); }
static void textures_end() { EndTexture(global.game->GameIsolate_.texturepack); }
static void textures_load(std::string name, std::string path) {}
static void materials_init() { InitMaterials(); }
static void controls_init() { Controls::InitKey(); }

static void init_ecs() {
    auto luacore = global.scripts->LuaCoreCpp;
    auto &luawrap = luacore->s_lua;
}

static void load_lua(std::string luafile) {}

#pragma endregion GameScriptingBind_1

Biome *BiomeGet(std::string name) {
    for (auto t : global.GameData_.biome_container)
        if (t->name == name) return t;
    return nullptr;
}

void GameplayScriptSystem::Create() {
    auto luacore = global.scripts->LuaCoreCpp;
    auto &luawrap = luacore->s_lua;
    luawrap.dofile(METADOT_RESLOC("data/scripts/game.lua"));
    luawrap["OnGameEngineLoad"]();
    luawrap["OnGameLoad"](global.game);
}

void GameplayScriptSystem::Destory() {
    auto luacore = global.scripts->LuaCoreCpp;
    auto &luawrap = luacore->s_lua;
    auto EndFunc = luawrap["OnGameEngineUnLoad"];
    EndFunc();
}

void GameplayScriptSystem::RegisterLua(LuaWrapper::State &s_lua) {
    s_lua["controls_init"] = LuaWrapper::function(controls_init);
    s_lua["materials_init"] = LuaWrapper::function(materials_init);
    s_lua["textures_load"] = LuaWrapper::function(textures_load);
    s_lua["textures_init"] = LuaWrapper::function(textures_init);
    s_lua["textures_end"] = LuaWrapper::function(textures_end);
    s_lua["audio_load_event"] = LuaWrapper::function(audio_load_event);
    s_lua["audio_play_event"] = LuaWrapper::function(audio_play_event);
    s_lua["audio_load_bank"] = LuaWrapper::function(audio_load_bank);
    s_lua["audio_init"] = LuaWrapper::function(audio_init);
    s_lua["create_biome"] = LuaWrapper::function(create_biome);
    s_lua["init_ecs"] = LuaWrapper::function(init_ecs);

    s_lua["DrawMainMenuUI"] = LuaWrapper::function(GameUI::MainMenuUI__Draw);
    s_lua["DrawDebugUI"] = LuaWrapper::function(GameUI::DebugDrawUI::Draw);
}