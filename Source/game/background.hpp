// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_BACKGROUND_HPP_
#define _METADOT_BACKGROUND_HPP_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/core.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/sdl_wrapper.h"
#include "game/game_datastruct.hpp"
#include "game/game_resources.hpp"
#include "scripting/lua_wrapper.hpp"

struct BackgroundLayer {
    std::vector<C_Surface *> surface;
    std::vector<R_Image *> texture;
    Texture *tex;
    F32 parralaxX;
    F32 parralaxY;
    F32 moveX;
    F32 moveY;
    BackgroundLayer(Texture *texture, F32 parallaxX, F32 parallaxY, F32 moveX, F32 moveY);
    ~BackgroundLayer();
    void init();
};

class BackgroundObject {
public:
    U32 solid;
    std::vector<std::shared_ptr<BackgroundLayer>> layers;
    explicit BackgroundObject(U32 solid, std::vector<std::shared_ptr<BackgroundLayer>> layers) : solid(std::move(solid)), layers(std::move(layers)){};
    void Init();
};

class BackgroundSystem : public IGameSystem {
public:
    std::unordered_map<std::string, BackgroundObject *> m_backgrounds;

    void Push(std::string name, BackgroundObject *bg);
    BackgroundObject *Get(std::string name);

    void Create() override;
    void Destory() override;
    void RegisterLua(LuaWrapper::State &s_lua) override;
};

#endif
