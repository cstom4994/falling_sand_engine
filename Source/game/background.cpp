// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "background.hpp"

#include <algorithm>
#include <memory>

#include "core/global.hpp"
#include "engine/memory.hpp"
#include "game/game.hpp"
#include "game/game_resources.hpp"
#include "renderer/renderer_gpu.h"

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
    if (this->tex) Eng_DestroyTexture(this->tex);
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

void BackgroundSystem::Push(std::string name, BackgroundObject *bg) { m_backgrounds.insert(std::make_pair(name, bg)); }

BackgroundObject *BackgroundSystem::Get(std::string name) {
    METADOT_ASSERT(m_backgrounds[name], "background is not exist");
    return m_backgrounds[name];
}

void BackgroundSystem::Create() {
    std::vector<std::shared_ptr<BackgroundLayer>> testOverworldLayers = {
            std::make_shared<BackgroundLayer>(LoadTextureInternal("data/assets/backgrounds/TestOverworld/layer2.png", SDL_PIXELFORMAT_ARGB8888), 0.125, 0.125, 1, 0),
            std::make_shared<BackgroundLayer>(LoadTextureInternal("data/assets/backgrounds/TestOverworld/layer3.png", SDL_PIXELFORMAT_ARGB8888), 0.25, 0.25, 0, 0),
            std::make_shared<BackgroundLayer>(LoadTextureInternal("data/assets/backgrounds/TestOverworld/layer4.png", SDL_PIXELFORMAT_ARGB8888), 0.375, 0.375, 4, 0),
            std::make_shared<BackgroundLayer>(LoadTextureInternal("data/assets/backgrounds/TestOverworld/layer5.png", SDL_PIXELFORMAT_ARGB8888), 0.5, 0.5, 0, 0)};

    METADOT_CREATE(C, bg, BackgroundObject, 0x7EAFCB, testOverworldLayers);
    global.game->GameIsolate_.backgrounds->Push("TEST_OVERWORLD", bg);
    global.game->GameIsolate_.backgrounds->Get("TEST_OVERWORLD")->Init();
}

void BackgroundSystem::Destory() {
    for (auto &[name, bg] : m_backgrounds) METADOT_DELETE(C, bg, BackgroundObject);
}
void BackgroundSystem::RegisterLua() {}
