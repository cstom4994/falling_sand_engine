// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "game_basic.hpp"

#include <string>
#include <string_view>

#include "engine/core/base_memory.h"
#include "engine/core/core.hpp"
#include "engine/core/global.hpp"
#include "engine/core/io/filesystem.h"
#include "engine/scripting/lua_wrapper.hpp"
#include "engine/scripting/scripting.hpp"
#include "game.hpp"
#include "game/player.hpp"
#include "game_datastruct.hpp"
#include "game_ui.hpp"
#include "reflectionflat.hpp"
#include "textures.hpp"

namespace ME {

#pragma region GameScriptingBind_1

static void audio_init() { global.audio.Init(); }

static void audio_load_bank(std::string name, unsigned int type) { global.audio.LoadBank(METADOT_RESLOC(name.c_str()), type); }

static void audio_load_event(std::string event) { global.audio.LoadEvent(event); }
static void audio_play_event(std::string event) { global.audio.PlayEvent(event); }

static void textures_init() {
    // 贴图初始化
    InitTexture(global.game->Iso.texturepack);
}
static void textures_end() { EndTexture(global.game->Iso.texturepack); }
static void textures_load(std::string name, std::string path) {}
static void controls_init() { ControlSystem::InitKey(); }

static void init_ecs() { auto &luawrap = scripting::get_singleton_ptr()->s_lua; }

static void load_lua(std::string luafile) {}

#pragma endregion GameScriptingBind_1

Biome *Biome::biomeGet(std::string name) {
    for (auto t : GAME()->biome_container)
        if (t->name == name) return t;
    return GAME()->biome_container[0];  // 没有找到指定生物群系则返回默认生物群系
}

void Biome::createBiome(std::string name, int id) {
    METADOT_BUG("[LUA] create_biome ", name, " = ", id);
    Biome *b = alloc<Biome>::safe_malloc(name, id);
    GAME()->biome_container.push_back(b);
}

void GameplayScriptSystem::create() {
    METADOT_BUG("GameplayScriptSystem created");

    scripting::get_singleton_ptr()->fast_load_lua(METADOT_RESLOC("data/scripts/game.lua"));
    scripting::get_singleton_ptr()->fast_call_func("OnGameEngineLoad")();
    scripting::get_singleton_ptr()->fast_call_func("OnGameLoad")(global.game);

    // GlobalDEF table initialization
    InitGlobalDEF(&global.game->Iso.globaldef, false);

    // I18N must be initialized after scripting system
    // It uses i18n.lua to function
    global.I18N.Init();
}

void GameplayScriptSystem::destory() { scripting::get_singleton_ptr()->fast_call_func("OnGameEngineUnLoad")(); }

void GameplayScriptSystem::reload() {}

void GameplayScriptSystem::registerLua(lua_wrapper::State &s_lua) {
    s_lua["controls_init"] = lua_wrapper::function(controls_init);
    s_lua["materials_init"] = lua_wrapper::function(InitMaterials);
    s_lua["materials_register"] = lua_wrapper::function(RegisterMaterial);
    s_lua["materials_push"] = lua_wrapper::function(PushMaterials);
    s_lua["textures_load"] = lua_wrapper::function(textures_load);
    s_lua["textures_init"] = lua_wrapper::function(textures_init);
    s_lua["textures_end"] = lua_wrapper::function(textures_end);
    s_lua["audio_load_event"] = lua_wrapper::function(audio_load_event);
    s_lua["audio_play_event"] = lua_wrapper::function(audio_play_event);
    s_lua["audio_load_bank"] = lua_wrapper::function(audio_load_bank);
    s_lua["audio_init"] = lua_wrapper::function(audio_init);
    s_lua["create_biome"] = lua_wrapper::function(Biome::createBiome);
    s_lua["init_ecs"] = lua_wrapper::function(init_ecs);

    s_lua["DrawMainMenuUI"] = lua_wrapper::function(GameUI::MainMenuUI__Draw);
    s_lua["DrawDebugUI"] = lua_wrapper::function(GameUI::DebugDrawUI__Draw);

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

}  // namespace ME
