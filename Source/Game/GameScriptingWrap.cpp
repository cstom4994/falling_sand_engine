// Copyright(c) 2022, KaoruXun All rights reserved.

#include "GameScriptingWrap.hpp"
#include "Core/Core.hpp"
#include "Core/Global.hpp"
#include "Engine/ReflectionFlat.hpp"
#include "Game/FileSystem.hpp"
#include "JsWrapper.hpp"
#include <string>

static void println(JsWrapper::rest<std::string> args) {
    for (auto const &arg: args) METADOT_INFO("{}", arg);
}

static void create_biome(std::string name, int id) {
    Biome *b = new Biome(name, id);
    GameData_.biome_container.push_back(b);
}

static void loadLua(std::string luafile) {}
static void loadScript(std::string scriptfile) {
    auto context = global.scripts->JsContext;
    context->evalFile(METADOT_RESLOC_STR(scriptfile));
}

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
        module.function<&loadLua>("load_lua");
        module.function<&loadScript>("load_script");

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
        OnGameDataLoad();

    } catch (JsWrapper::exception) {
        auto exc = context->getException();
        std::cerr << (std::string) exc << std::endl;
        if ((bool) exc["stack"]) std::cerr << (std::string) exc["stack"] << std::endl;
        return;
    }
}
