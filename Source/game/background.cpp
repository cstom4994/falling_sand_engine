// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "background.hpp"

#include <algorithm>

#include "core/global.hpp"
#include "engine/memory.hpp"
#include "game/game.hpp"
#include "game/game_resources.hpp"

BackgroundLayer::BackgroundLayer(C_Surface *texture, F32 parallaxX, F32 parallaxY, F32 moveX, F32 moveY) {
    this->surface = {ScaleSurface(texture, 1, 1), ScaleSurface(texture, 2, 2), ScaleSurface(texture, 3, 3)};
    this->parralaxX = parallaxX;
    this->parralaxY = parallaxY;
    this->moveX = moveX;
    this->moveY = moveY;
}

void BackgroundLayer::init() {
    this->texture = {R_CopyImageFromSurface(this->surface[0]), R_CopyImageFromSurface(this->surface[1]), R_CopyImageFromSurface(this->surface[2])};

    R_SetImageFilter(this->texture[0], R_FILTER_NEAREST);
    R_SetImageFilter(this->texture[1], R_FILTER_NEAREST);
    R_SetImageFilter(this->texture[2], R_FILTER_NEAREST);
}

void Background::init() {
    for (size_t i = 0; i < layers.size(); i++) {
        layers[i].init();
    }
}

void Backgrounds::Push(std::string name, Background *bg) { m_backgrounds.insert(std::make_pair(name, bg)); }

Background *Backgrounds::Get(std::string name) {
    METADOT_ASSERT(m_backgrounds[name], "background is not exist");
    return m_backgrounds[name];
}

void Backgrounds::Load() {
    METADOT_INFO("Loading backgrounds...");

    std::vector<BackgroundLayer> testOverworldLayers = {BackgroundLayer(LoadTextureInternal("data/assets/backgrounds/TestOverworld/layer2.png", SDL_PIXELFORMAT_ARGB8888)->surface, 0.125, 0.125, 1, 0),
                                                        BackgroundLayer(LoadTextureInternal("data/assets/backgrounds/TestOverworld/layer3.png", SDL_PIXELFORMAT_ARGB8888)->surface, 0.25, 0.25, 0, 0),
                                                        BackgroundLayer(LoadTextureInternal("data/assets/backgrounds/TestOverworld/layer4.png", SDL_PIXELFORMAT_ARGB8888)->surface, 0.375, 0.375, 4, 0),
                                                        BackgroundLayer(LoadTextureInternal("data/assets/backgrounds/TestOverworld/layer5.png", SDL_PIXELFORMAT_ARGB8888)->surface, 0.5, 0.5, 0, 0)};

    METADOT_NEW(C, global.game->GameIsolate_.backgrounds, Backgrounds);
    METADOT_CREATE(C, bg, Background, 0x7EAFCB, testOverworldLayers);
    global.game->GameIsolate_.backgrounds->Push("TEST_OVERWORLD", bg);
    global.game->GameIsolate_.backgrounds->Get("TEST_OVERWORLD")->init();
}

void Backgrounds::Unload() {
    for (auto &[name, bg] : m_backgrounds) METADOT_DELETE(C, bg, Background);
}