// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_BACKGROUND_HPP
#define ME_BACKGROUND_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/core/sdl_wrapper.h"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/lua_wrapper.hpp"
#include "game_datastruct.hpp"
#include "textures.hpp"

class BackgroundLayer {
public:
    TextureRef tex;

    C_Surface *surface[3];
    R_Image *texture[3];
    f32 parralaxX;
    f32 parralaxY;
    f32 moveX;
    f32 moveY;

public:
    BackgroundLayer(TextureRef tex, f32 parralaxX, f32 parralaxY, f32 moveX, f32 moveY) noexcept;
    ~BackgroundLayer();
};

using BackgroundLayerRef = ME::ref<BackgroundLayer>;

class BackgroundObject {
public:
    u32 solid;
    std::vector<BackgroundLayerRef> layers;
    BackgroundObject(u32 solid, std::vector<BackgroundLayerRef> layers) : solid(std::move(solid)), layers(std::move(layers)){};
    // BackgroundObject(u32 solid, std::vector<BackgroundLayer> layers) : solid(std::move(solid)) {
    //     for (auto layer : layers) {
    //         this->layers.push_back(ME::create_ref<BackgroundLayer>(layer));
    //     }
    // };
    //  BackgroundObject(BackgroundObject &) = default;
    ~BackgroundObject();
    void Init();
};

class BackgroundSystem : public IGameSystem {
public:
    std::unordered_map<std::string, BackgroundObject *> m_backgrounds;

    // static BackgroundObject TEST_OVERWORLD;

    void Push(std::string name, BackgroundObject *bg);
    BackgroundObject *Get(std::string name);

    REGISTER_SYSTEM(BackgroundSystem)

    void create() override;
    void destory() override;
    void reload() override;
    void registerLua(ME::LuaWrapper::State &s_lua) override;

public:
    void draw();
};

#endif
