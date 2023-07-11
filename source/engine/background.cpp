// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "background.hpp"

#include <algorithm>
#include <memory>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/core/global.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/lua_wrapper.hpp"
#include "game.hpp"
#include "game_resources.hpp"

BackgroundObject::~BackgroundObject() {
    for (size_t i = 0; i < layers.size(); i++) {
        DestroyBackgroundLayer(layers[i]);
    }
}

void BackgroundObject::Init() {
    for (size_t i = 0; i < layers.size(); i++) {
        InitBackgroundLayer(layers[i]);
    }
}

void NewBackgroundObject(std::string name, u32 solid, ME::LuaWrapper::LuaRef table) {
    auto &L = Scripting::get_singleton_ptr()->Lua->s_lua;
    std::vector<ME::LuaWrapper::LuaRef> b = table;
    std::vector<BackgroundLayer *> Layers;

    for (auto &c : b) {
        METADOT_BUG(std::format("{0} {1}, [{2:.2} {3:.2} {4:.2} {5:.2}]", name, c["name"].get<std::string>().c_str(), c["p1"].get<f32>(), c["p2"].get<f32>(), c["x1"].get<f32>(), c["x2"].get<f32>())
                            .c_str());
        auto l = CreateBackgroundLayer(LoadTexture(c["name"].get<std::string>().c_str()), c["p1"].get<f32>(), c["p2"].get<f32>(), c["x1"].get<f32>(), c["x2"].get<f32>());
        Layers.push_back(l);
    }

    BackgroundObject *bg = new BackgroundObject(solid, Layers);

    global.game->GameIsolate_.backgrounds->Push(name, bg);
    global.game->GameIsolate_.backgrounds->Get(name)->Init();
}

void BackgroundSystem::Push(std::string name, BackgroundObject *bg) { m_backgrounds.insert(std::make_pair(name, bg)); }

BackgroundObject *BackgroundSystem::Get(std::string name) {
    ME_ASSERT(m_backgrounds[name], "background is not exist");
    return m_backgrounds[name];
}

// std::vector<BackgroundLayer> testOverworldLayers = {BackgroundLayer(LoadTexture("data/assets/backgrounds/TestOverworld/layer2.png"), 0.125, 0.125, 1, 0),
//                                                     BackgroundLayer(LoadTexture("data/assets/backgrounds/TestOverworld/layer3.png"), 0.25, 0.25, 0, 0),
//                                                     BackgroundLayer(LoadTexture("data/assets/backgrounds/TestOverworld/layer4.png"), 0.375, 0.375, 4, 0),
//                                                     BackgroundLayer(LoadTexture("data/assets/backgrounds/TestOverworld/layer5.png"), 0.5, 0.5, 0, 0)};
//
// BackgroundObject BackgroundSystem::TEST_OVERWORLD = BackgroundObject(0x7EAFCB, testOverworldLayers);

void BackgroundSystem::Create() {

    // NewBackgroundObject("TEST_OVERWORLD");
    auto &L = Scripting::get_singleton_ptr()->Lua->s_lua;

    this->RegisterLua(L);

    L["OnBackgroundLoad"]();
}

void BackgroundSystem::Destory() {
    for (auto &[name, bg] : m_backgrounds) {
        delete bg;
    }
}
void BackgroundSystem::Reload() {}
void BackgroundSystem::RegisterLua(ME::LuaWrapper::State &s_lua) { s_lua["NewBackgroundObject"] = ME::LuaWrapper::function(NewBackgroundObject); }

BackgroundLayer *CreateBackgroundLayer(Texture *texture, f32 parallaxX, f32 parallaxY, f32 moveX, f32 moveY) {
    BackgroundLayer *layer = (BackgroundLayer *)ME_MALLOC(sizeof(BackgroundLayer));
    layer->tex = texture;
    // layer->surface = {ScaleSurface(texture->surface, 1, 1), ScaleSurface(texture->surface, 2, 2), ScaleSurface(texture->surface, 3, 3)};
    for (int i = 1; i <= 3; i++) {
        layer->surface[i - 1] = ScaleSurface(texture->surface, i, i);
        // layer->surface[i - 1] = texture->surface;
    }
    layer->parralaxX = parallaxX;
    layer->parralaxY = parallaxY;
    layer->moveX = moveX;
    layer->moveY = moveY;
    return layer;
}

void DestroyBackgroundLayer(BackgroundLayer *layer) {
    for (auto &&image : layer->texture) R_FreeImage(image);
    if (layer->tex) DestroyTexture(layer->tex);
    ME_FREE(layer);
}

void InitBackgroundLayer(BackgroundLayer *layer) {
    // layer->texture = {R_CopyImageFromSurface(layer->surface[0]), R_CopyImageFromSurface(layer->surface[1]), R_CopyImageFromSurface(layer->surface[2])};

    for (int i = 1; i <= 3; i++) {
        layer->texture[i - 1] = R_CopyImageFromSurface(layer->surface[i - 1]);
    }

    R_SetImageFilter(layer->texture[0], R_FILTER_NEAREST);
    R_SetImageFilter(layer->texture[1], R_FILTER_NEAREST);
    R_SetImageFilter(layer->texture[2], R_FILTER_NEAREST);
}
