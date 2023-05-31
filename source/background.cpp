// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "background.hpp"

#include <algorithm>
#include <memory>
#include <vector>

#include "core/core.h"
#include "core/global.hpp"

#include "game.hpp"
#include "game_resources.hpp"
#include "renderer/renderer_gpu.h"
#include "scripting/lua/lua_wrapper.hpp"

BackgroundLayer::BackgroundLayer(Texture *texture, F32 parallaxX, F32 parallaxY, F32 moveX, F32 moveY) {
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

void NewBackgroundObject(std::string name, U32 solid, LuaWrapper::LuaRef table) {
    auto &L = Scripting::GetSingletonPtr()->Lua->s_lua;
    std::vector<LuaWrapper::LuaRef> b = table;
    std::vector<MetaEngine::Ref<BackgroundLayer>> Layers;

    for (auto &c : b) {
        METADOT_BUG("NewBackgroundObject %s, [%f %f %f %f]", c["name"].get<std::string>().c_str(), c["p1"].get<F32>(), c["p2"].get<F32>(), c["x1"].get<F32>(), c["x2"].get<F32>());
        Layers.push_back(MetaEngine::CreateRef<BackgroundLayer>(LoadTexture(c["name"].get<std::string>().c_str()), c["p1"].get<F32>(), c["p2"].get<F32>(), c["x1"].get<F32>(), c["x2"].get<F32>()));
    }

    METADOT_CREATE(C, bg, BackgroundObject, solid, Layers);

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
    auto &L = Scripting::GetSingletonPtr()->Lua->s_lua;

    this->RegisterLua(L);

    L["OnBackgroundLoad"]();

    
}

void BackgroundSystem::Destory() {
    for (auto &[name, bg] : m_backgrounds) METADOT_DELETE(C, bg, BackgroundObject);
}
void BackgroundSystem::Reload() {
    
}
void BackgroundSystem::RegisterLua(LuaWrapper::State &s_lua) { s_lua["NewBackgroundObject"] = LuaWrapper::function(NewBackgroundObject); }
