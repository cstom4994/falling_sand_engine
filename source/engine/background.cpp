// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "background.hpp"

#include <algorithm>
#include <memory>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/core/global.hpp"
#include "game.hpp"
#include "game_resources.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/lua_wrapper.hpp"

BackgroundLayer::BackgroundLayer(Texture *texture, f32 parallaxX, f32 parallaxY, f32 moveX, f32 moveY) {
    this->tex = texture;
    this->surface = {ScaleSurface(texture->surface, 1, 1), ScaleSurface(texture->surface, 2, 2), ScaleSurface(texture->surface, 3, 3)};
    this->parralaxX = parallaxX;
    this->parralaxY = parallaxY;
    this->moveX = moveX;
    this->moveY = moveY;
}

BackgroundLayer::~BackgroundLayer() {
    for (auto &&image : this->texture) R_FreeImage(image);
    if (this->tex) DestroyTexture(this->tex);
}

void BackgroundLayer::init() {
    this->texture = {R_CopyImageFromSurface(this->surface[0]), R_CopyImageFromSurface(this->surface[1]), R_CopyImageFromSurface(this->surface[2])};

    R_SetImageFilter(this->texture[0], R_FILTER_NEAREST);
    R_SetImageFilter(this->texture[1], R_FILTER_NEAREST);
    R_SetImageFilter(this->texture[2], R_FILTER_NEAREST);
}

void BackgroundObject::Init() {
    for (size_t i = 0; i < layers.size(); i++) {
        layers[i]->init();
    }
}

void NewBackgroundObject(std::string name, u32 solid, ME::LuaWrapper::LuaRef table) {
    auto &L = Scripting::get_singleton_ptr()->Lua->s_lua;
    std::vector<ME::LuaWrapper::LuaRef> b = table;
    std::vector<ME::ref<BackgroundLayer>> Layers;

    for (auto &c : b) {
        METADOT_BUG(std::format("NewBackgroundObject {0}, [{1:.2} {2:.2} {3:.2} {4:.2}]", c["name"].get<std::string>().c_str(), c["p1"].get<f32>(), c["p2"].get<f32>(), c["x1"].get<f32>(),
                                c["x2"].get<f32>())
                            .c_str());
        Layers.push_back(ME::create_ref<BackgroundLayer>(LoadTexture(c["name"].get<std::string>().c_str()), c["p1"].get<f32>(), c["p2"].get<f32>(), c["x1"].get<f32>(), c["x2"].get<f32>()));
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
