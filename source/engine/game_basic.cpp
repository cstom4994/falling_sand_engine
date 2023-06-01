// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "game_basic.hpp"

#include <string>
#include <string_view>

#include "engine/core/core.hpp"
#include "engine/core/global.hpp"
#include "engine/core/io/filesystem.h"
#include "game.hpp"
#include "game_datastruct.hpp"
#include "game_resources.hpp"
#include "game_ui.hpp"
#include "reflectionflat.hpp"
#include "engine/scripting/lua_wrapper.hpp"
#include "engine/scripting/scripting.hpp"

#pragma region GameScriptingBind_1

static void create_biome(std::string name, int id) {
    Biome *b = new Biome(name, id);
    global.GameData_.biome_container.push_back(b);
}

// static void audio_load_bank(std::string name, unsigned int type) {}
// static void audio_load_event(std::string event) { global.audioEngine.LoadEvent(event); }
// static void audio_play_event(std::string event) { global.audioEngine.PlayEvent(event); }

static void textures_init() { InitTexture(global.game->GameIsolate_.texturepack); }
static void textures_end() { EndTexture(global.game->GameIsolate_.texturepack); }
static void textures_load(std::string name, std::string path) {}
static void controls_init() { ControlSystem::InitKey(); }

static void init_ecs() {
    auto luacore = Scripting::GetSingletonPtr()->Lua;
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
    METADOT_BUG("GameplayScriptSystem created");

    auto luacore = Scripting::GetSingletonPtr()->Lua;
    auto &luawrap = luacore->s_lua;
    luawrap.dofile(METADOT_RESLOC("data/scripts/game.lua"));
    luawrap["OnGameEngineLoad"]();
    luawrap["OnGameLoad"](global.game);

    // GlobalDEF table initialization
    InitGlobalDEF(&global.game->GameIsolate_.globaldef, false);

    // I18N must be initialized after scripting system
    // It uses i18n.lua to function
    global.I18N.Init();
}

void GameplayScriptSystem::Destory() {
    auto luacore = Scripting::GetSingletonPtr()->Lua;
    auto &luawrap = luacore->s_lua;
    auto EndFunc = luawrap["OnGameEngineUnLoad"];
    EndFunc();
}

void GameplayScriptSystem::Reload() {}

void GameplayScriptSystem::RegisterLua(LuaWrapper::State &s_lua) {
    s_lua["controls_init"] = LuaWrapper::function(controls_init);
    s_lua["materials_init"] = LuaWrapper::function(InitMaterials);
    s_lua["materials_register"] = LuaWrapper::function(RegisterMaterial);
    s_lua["materials_push"] = LuaWrapper::function(PushMaterials);
    s_lua["textures_load"] = LuaWrapper::function(textures_load);
    s_lua["textures_init"] = LuaWrapper::function(textures_init);
    s_lua["textures_end"] = LuaWrapper::function(textures_end);
    // s_lua["audio_load_event"] = LuaWrapper::function(audio_load_event);
    // s_lua["audio_play_event"] = LuaWrapper::function(audio_play_event);
    // s_lua["audio_load_bank"] = LuaWrapper::function(audio_load_bank);
    s_lua["create_biome"] = LuaWrapper::function(create_biome);
    s_lua["init_ecs"] = LuaWrapper::function(init_ecs);

    s_lua["DrawMainMenuUI"] = LuaWrapper::function(GameUI::MainMenuUI__Draw);
    s_lua["DrawDebugUI"] = LuaWrapper::function(GameUI::DebugDrawUI__Draw);

    // ItemBinding::register_class(s_lua.state());
    RigidBodyBinding::register_class(s_lua.state());

    // Test
    s_lua(R"(
a = RigidBody(1, "hello")
    )");

    {
        lua_getglobal(s_lua.state(), "a");
        auto b = RigidBodyBinding::fromStackThrow(s_lua.state(), -1);
        lua_pop(s_lua.state(), 1);

        METADOT_INFO("Use count is now: ", b.use_count());
    }

    // {
    //     MyActorPtr actor = std::make_shared<MyActor>("Nigel", 39);
    //     METADOT_INFO("Actor use count is: %ld", actor.use_count());
    //     MyActorBinding::push(s_lua.state(), actor);
    //     lua_setglobal(s_lua.state(), "actor");
    //     METADOT_INFO("Pushed to Lua");
    //     METADOT_INFO("Actor use count is: %ld", actor.use_count());
    //     s_lua("actor:walk()");
    //     s_lua("actor.age = actor.age + 1 print( 'Happy Birthday')");
    //     s_lua("print( actor.age )");
    //     METADOT_INFO("%d", actor->_age);
    //     // Should print Coffee, nil as 'added' members/properties are per instance.
    //     s_lua("print( a.extra, actor.extra )");
    // }
}
