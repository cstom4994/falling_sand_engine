// Copyright(c) 2022, KaoruXun All rights reserved.

#include "GameScriptingWrap.hpp"
#include "Core/Core.hpp"
#include "Core/Global.hpp"
#include "Engine/ReflectionFlat.hpp"
#include "Game/FileSystem.hpp"
#include "Game/GameResources.hpp"
#include "Game/Materials.hpp"
#include "JsWrapper.hpp"
#include <string>

#pragma region GameScriptingBind_1

static void println(JsWrapper::rest<std::string> args) {
    for (auto const &arg: args) METADOT_INFO("{}", arg);
}

static void create_biome(std::string name, int id) {
    Biome *b = new Biome(name, id);
    GameData_.biome_container.push_back(b);
}

static void audio_init() { global.audioEngine.Init(); }

static void audio_load_bank(std::string name, unsigned int type) {
#if defined(METADOT_BUILD_AUDIO)
    global.audioEngine.LoadBank(METADOT_RESLOC(name), type);
#endif
}

static void audio_load_event(std::string event) { global.audioEngine.LoadEvent(event); }
static void audio_play_event(std::string event) { global.audioEngine.PlayEvent(event); }

static void textures_init() { Textures::Init(); }
static void textures_load(std::string name, std::string path) {}
static void materials_init() { Materials::Init(); }

static void load_lua(std::string luafile) {}
static void load_script(std::string scriptfile) {
    auto context = global.scripts->JsContext;
    context->evalFile(METADOT_RESLOC_STR(scriptfile));
}

#pragma endregion GameScriptingBind_1

void GameScriptingWrap::Init() {}

Biome *GameScriptingWrap::BiomeGet(std::string name) {
    for (auto t: GameData_.biome_container) {
        if (t->name == name) { return t; }
    }
    return nullptr;
}

void GameScriptingWrap::Bind() {
    auto context = global.scripts->JsContext;
    try {
        auto &module = context->addModule("CoreModule");
        module.function<&println>("println");
        module.function<&create_biome>("create_biome");
        module.function<&load_lua>("load_lua");
        module.function<&load_script>("load_script");
        module.function<&audio_init>("audio_init");
        module.function<&audio_load_bank>("audio_load_bank");
        module.function<&audio_load_event>("audio_load_event");
        module.function<&audio_play_event>("audio_play_event");
        module.function<&materials_init>("materials_init");
        module.function<&textures_init>("textures_init");
        module.function<&textures_load>("textures_load");

        // module.class_<Biome>("Biome")
        //         .constructor<std::string, int>("Biome")
        //         .fun<&Biome::id>("id")
        //         .fun<&Biome::name>("name");

        context->eval(R"Js(
            import * as test from 'CoreModule';
            globalThis.test = test;
        )Js",
                      "<import>", JS_EVAL_TYPE_MODULE);

        context->evalFile(METADOT_RESLOC_STR("data/scripts/init.js"));

        auto OnGameDataLoad = (std::function<void(void)>) context->eval("OnGameDataLoad");
        auto OnGameEngineLoad = (std::function<void(void)>) context->eval("OnGameEngineLoad");
        OnGameEngineLoad();
        OnGameDataLoad();

    } catch (JsWrapper::exception) {
        auto exc = context->getException();
        std::cerr << (std::string) exc << std::endl;
        if ((bool) exc["stack"]) std::cerr << (std::string) exc["stack"] << std::endl;
        return;
    }
}
