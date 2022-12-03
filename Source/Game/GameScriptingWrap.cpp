// Copyright(c) 2022, KaoruXun All rights reserved.

#include "GameScriptingWrap.hpp"
#include "Core/Core.hpp"
#include "Core/Global.hpp"
#include "Game/FileSystem.hpp"
#include "Engine/ReflectionFlat.hpp"
#include "JsWrapper.hpp"
#include <string>

void println(JsWrapper::rest<std::string> args) {
    for (auto const &arg: args) METADOT_INFO("{}", arg);
}

void createBiome(std::string name, int id) {
    Biome *b = new Biome(name, id);
    GameData_.biome_container.push_back(b);
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
        auto &module = context->addModule("BiomeModule");
        module.function<&println>("println");
        module.function<&createBiome>("createBiome");

        // module.class_<Biome>("Biome")
        //         .constructor<std::string, int>("Biome")
        //         .fun<&Biome::id>("id")
        //         .fun<&Biome::name>("name");

        context->eval(R"xxx(
            import * as test from 'BiomeModule';
            globalThis.test = test;
        )xxx",
                      "<import>", JS_EVAL_TYPE_MODULE);

        context->evalFile(METADOT_RESLOC_STR("data/scripts/biomes.js"));

    } catch (JsWrapper::exception) {
        auto exc = context->getException();
        std::cerr << (std::string) exc << std::endl;
        if ((bool) exc["stack"]) std::cerr << (std::string) exc["stack"] << std::endl;
        return;
    }
}
