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
#include "textures.hpp"

namespace ME {

BackgroundObject::~BackgroundObject() {
    for (size_t i = 0; i < layers.size(); i++) {
        layers[i].reset();
    }
}

void BackgroundObject::Init() {
    // for (size_t i = 0; i < layers.size(); i++) {
    //     InitBackgroundLayer(layers[i]);
    // }
}

void NewBackgroundObject(std::string name, u32 solid, lua_wrapper::LuaRef table) {
    auto &L = scripting::get_singleton_ptr()->s_lua;
    std::vector<lua_wrapper::LuaRef> b = table;
    std::vector<BackgroundLayerRef> Layers;

    for (auto &c : b) {
        METADOT_BUG(std::format("{0} {1}, [{2:.2} {3:.2} {4:.2} {5:.2}]", name, c["name"].get<std::string>().c_str(), c["p1"].get<f32>(), c["p2"].get<f32>(), c["x1"].get<f32>(), c["x2"].get<f32>())
                            .c_str());
        BackgroundLayerRef l = create_ref<BackgroundLayer>(LoadTexture(c["name"].get<std::string>().c_str()), c["p1"].get<f32>(), c["p2"].get<f32>(), c["x1"].get<f32>(), c["x2"].get<f32>());
        Layers.push_back(l);
    }

    BackgroundObject *bg = new BackgroundObject(solid, Layers);

    global.game->Iso.backgrounds->Push(name, bg);
    global.game->Iso.backgrounds->Get(name)->Init();
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

void BackgroundSystem::create() {

    // NewBackgroundObject("TEST_OVERWORLD");
    auto &L = scripting::get_singleton_ptr()->s_lua;

    this->registerLua(L);

    L["OnBackgroundLoad"]();
}

void BackgroundSystem::destory() {
    for (auto &[name, bg] : m_backgrounds) {
        delete bg;
    }
}
void BackgroundSystem::reload() {}

void BackgroundSystem::registerLua(lua_wrapper::State &s_lua) { s_lua["NewBackgroundObject"] = lua_wrapper::function(NewBackgroundObject); }

void BackgroundSystem::draw() {
    // 绘制背景贴图
    if (NULL == global.game->bg) global.game->bg = global.game->Iso.backgrounds->Get("TEST_OVERWORLD");
    if (NULL != global.game->bg && !global.game->bg->layers.empty() && global.game->Iso.globaldef.draw_background && the<engine>().eng()->render_scale <= ME_ARRAY_SIZE(global.game->bg->layers[0]->surface) &&
        global.game->Iso.world->loadZone.y > -5 * CHUNK_H) {
        R_SetShapeBlendMode(R_BLEND_SET);
        MEcolor col = {static_cast<u8>((global.game->bg->solid >> 16) & 0xff), static_cast<u8>((global.game->bg->solid >> 8) & 0xff), static_cast<u8>((global.game->bg->solid >> 0) & 0xff), 0xff};
        R_ClearColor(the<engine>().eng()->target, col);

        MErect dst;
        MErect src;

        f32 arX = (f32)the<engine>().eng()->windowWidth / (global.game->bg->layers[0]->surface[0]->w);
        f32 arY = (f32)the<engine>().eng()->windowHeight / (global.game->bg->layers[0]->surface[0]->h);

        f64 time = ME_gettime() / 1000.0;

        R_SetShapeBlendMode(R_BLEND_NORMAL);

        for (size_t i = 0; i < global.game->bg->layers.size(); i++) {
            BackgroundLayerRef bglayer = global.game->bg->layers[i];

            C_Surface *texture = bglayer->surface[(size_t)the<engine>().eng()->render_scale - 1];

            R_Image *tex = bglayer->texture[(size_t)the<engine>().eng()->render_scale - 1];
            R_SetBlendMode(tex, R_BLEND_NORMAL);

            int tw = texture->w;
            int th = texture->h;

            int iter = (int)ceil((f32)the<engine>().eng()->windowWidth / (tw)) + 1;
            for (int n = 0; n < iter; n++) {

                src.x = 0;
                src.y = 0;
                src.w = tw;
                src.h = th;

                dst.x = (((GAME()->ofsX + GAME()->camX) + global.game->Iso.world->loadZone.x * the<engine>().eng()->render_scale) + n * tw / bglayer->parralaxX) * bglayer->parralaxX +
                        global.game->Iso.world->width / 2.0f * the<engine>().eng()->render_scale - tw / 2.0f;
                dst.y = ((GAME()->ofsY + GAME()->camY) + global.game->Iso.world->loadZone.y * the<engine>().eng()->render_scale) * bglayer->parralaxY +
                        global.game->Iso.world->height / 2.0f * the<engine>().eng()->render_scale - th / 2.0f - the<engine>().eng()->windowHeight / 3.0f * (the<engine>().eng()->render_scale - 1);
                dst.w = (f32)tw;
                dst.h = (f32)th;

                dst.x += (f32)(the<engine>().eng()->render_scale * fmod(bglayer->moveX * time, tw));

                // TODO: optimize
                while (dst.x >= the<engine>().eng()->windowWidth - 10) dst.x -= (iter * tw);
                while (dst.x + dst.w < 0) dst.x += (iter * tw - 1);

                // TODO: optimize
                if (dst.x < 0) {
                    dst.w += dst.x;
                    src.x -= (int)dst.x;
                    src.w += (int)dst.x;
                    dst.x = 0;
                }

                if (dst.y < 0) {
                    dst.h += dst.y;
                    src.y -= (int)dst.y;
                    src.h += (int)dst.y;
                    dst.y = 0;
                }

                if (dst.x + dst.w >= the<engine>().eng()->windowWidth) {
                    src.w -= (int)((dst.x + dst.w) - the<engine>().eng()->windowWidth);
                    dst.w += the<engine>().eng()->windowWidth - (dst.x + dst.w);
                }

                if (dst.y + dst.h >= the<engine>().eng()->windowHeight) {
                    src.h -= (int)((dst.y + dst.h) - the<engine>().eng()->windowHeight);
                    dst.h += the<engine>().eng()->windowHeight - (dst.y + dst.h);
                }

                R_BlitRect(tex, &src, the<engine>().eng()->target, &dst);
            }
        }
    }
}

BackgroundLayer::BackgroundLayer(TextureRef tex, f32 parralaxX, f32 parralaxY, f32 moveX, f32 moveY) noexcept : tex(tex), parralaxX(parralaxX), parralaxY(parralaxY), moveX(moveX), moveY(moveY) {

    ME_ASSERT(tex);

    for (int i = 1; i <= 3; i++) {
        this->surface[i - 1] = ScaleSurface(tex->surface(), i, i);
        // layer->surface[i - 1] = texture->surface;
    }

    for (int i = 1; i <= 3; i++) {
        this->texture[i - 1] = R_CopyImageFromSurface(this->surface[i - 1]);
    }

    R_SetImageFilter(this->texture[0], R_FILTER_NEAREST);
    R_SetImageFilter(this->texture[1], R_FILTER_NEAREST);
    R_SetImageFilter(this->texture[2], R_FILTER_NEAREST);
}

BackgroundLayer::~BackgroundLayer() {
    for (auto &&image : this->texture) R_FreeImage(image);

    // 背景贴图不在贴图包中 这里将其释放
    if (this->tex) this->tex.reset();
}

}  // namespace ME