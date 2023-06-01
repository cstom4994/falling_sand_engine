// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_BACKGROUND_HPP
#define ME_BACKGROUND_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/core.hpp"
#include "core/sdl_wrapper.h"
#include "game_datastruct.hpp"
#include "game_resources.hpp"
#include "renderer/renderer_gpu.h"
#include "scripting/lua_wrapper.hpp"

struct BackgroundLayer {
    std::vector<C_Surface *> surface;
    std::vector<R_Image *> texture;
    Texture *tex;
    f32 parralaxX;
    f32 parralaxY;
    f32 moveX;
    f32 moveY;
    BackgroundLayer(Texture *texture, f32 parallaxX, f32 parallaxY, f32 moveX, f32 moveY);
    ~BackgroundLayer();
    void init();
};

class BackgroundObject {
public:
    u32 solid;
    std::vector<ME::ref<BackgroundLayer>> layers;
    explicit BackgroundObject(u32 solid, std::vector<ME::ref<BackgroundLayer>> layers) : solid(std::move(solid)), layers(std::move(layers)){};
    void Init();
};

class BackgroundSystem : public IGameSystem {
public:
    std::unordered_map<std::string, BackgroundObject *> m_backgrounds;

    void Push(std::string name, BackgroundObject *bg);
    BackgroundObject *Get(std::string name);

    REGISTER_SYSTEM(BackgroundSystem)

    void Create() override;
    void Destory() override;
    void Reload() override;
    void RegisterLua(LuaWrapper::State &s_lua) override;
};

#endif
