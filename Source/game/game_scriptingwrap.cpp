// Copyright(c) 2022, KaoruXun All rights reserved.

#include "game_scriptingwrap.hpp"
#include "core/core.hpp"
#include "core/global.hpp"
#include "engine/imgui_binder.hpp"
#include "engine/reflectionflat.hpp"
#include "game/controls.hpp"
#include "engine/filesystem.h"
#include "game/game_resources.hpp"
#include "game/materials.hpp"
#include "js_wrapper.hpp"
#include "quickjs/quickjs-libc.h"
#include <string>

#pragma region GameScriptingBind_1

static void println(JsWrapper::rest<std::string> args) {
    for (auto const &arg: args) METADOT_INFO("%s", arg.c_str());
}

static void create_biome(std::string name, int id) {
    Biome *b = new Biome(name, id);
    GameData_.biome_container.push_back(b);
}

static void audio_init() { global.audioEngine.Init(); }

static void audio_load_bank(std::string name, unsigned int type) {
#if defined(METADOT_BUILD_AUDIO)
    global.audioEngine.LoadBank(METADOT_RESLOC(name.c_str()), type);
#endif
}

static void audio_load_event(std::string event) { global.audioEngine.LoadEvent(event); }
static void audio_play_event(std::string event) { global.audioEngine.PlayEvent(event); }

static void textures_init() { Textures::Init(); }
static void textures_load(std::string name, std::string path) {}
static void materials_init() { Materials::Init(); }
static void controls_init() { Controls::initKey(); }

static void load_lua(std::string luafile) {}
static void load_script(std::string scriptfile) {
    auto context = global.scripts->JsContext;
    context->evalFile(METADOT_RESLOC(scriptfile.c_str()));
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
        module.function<&controls_init>("controls_init");

        js_std_init_handlers(global.scripts->JsRuntime->rt);
        /* loader for ES6 modules */
        JS_SetModuleLoaderFunc(global.scripts->JsRuntime->rt, nullptr, js_module_loader, nullptr);
        js_std_add_helpers(context->ctx, 0, nullptr);

        js_init_module_std(context->ctx, "std");
        js_init_module_os(context->ctx, "os");

        init_imgui_module(context->ctx, "ImGuiModule");

        // module.class_<Biome>("Biome")
        //         .constructor<std::string, int>("Biome")
        //         .fun<&Biome::id>("id")
        //         .fun<&Biome::name>("name");

        context->eval(R"Js(
            import * as test from 'CoreModule';
            globalThis.test = test;
            import * as ImGui from 'ImGuiModule';
            globalThis.ImGui = ImGui;
            import * as std from 'std';
            globalThis.std = std;
            import * as os from 'os';
            globalThis.os = os;
        )Js",
                      "<import>", JS_EVAL_TYPE_MODULE);

        context->evalFile(METADOT_RESLOC("data/scripts/init.js"));

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
