// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Background.hpp"
#include "Core/Global.hpp"
#include "Engine/Memory.hpp"
#include "Game/Game.hpp"
#include "Game/InEngine.h"
#include "Game/Textures.hpp"

#include <algorithm>

BackgroundLayer::BackgroundLayer(C_Surface *texture, float parallaxX, float parallaxY, float moveX,
                                 float moveY) {
    this->surface = {Textures::scaleTexture(texture, 1, 1), Textures::scaleTexture(texture, 2, 2),
                     Textures::scaleTexture(texture, 3, 3)};
    this->parralaxX = parallaxX;
    this->parralaxY = parallaxY;
    this->moveX = moveX;
    this->moveY = moveY;
}

void BackgroundLayer::init() {
    this->texture = {METAENGINE_Render_CopyImageFromSurface(this->surface[0]),
                     METAENGINE_Render_CopyImageFromSurface(this->surface[1]),
                     METAENGINE_Render_CopyImageFromSurface(this->surface[2])};

    METAENGINE_Render_SetImageFilter(this->texture[0], METAENGINE_Render_FILTER_NEAREST);
    METAENGINE_Render_SetImageFilter(this->texture[1], METAENGINE_Render_FILTER_NEAREST);
    METAENGINE_Render_SetImageFilter(this->texture[2], METAENGINE_Render_FILTER_NEAREST);
}

void Background::init() {
    for (size_t i = 0; i < layers.size(); i++) { layers[i].init(); }
}

void Backgrounds::Push(std::string name, Background *bg) {
    m_backgrounds.insert(std::make_pair(name, bg));
}

Background *Backgrounds::Get(std::string name) {
    METADOT_ASSERT(m_backgrounds[name], "background is not exist");
    return m_backgrounds[name];
}

void Backgrounds::Load() {
    METADOT_INFO("Loading backgrounds...");

    std::vector<BackgroundLayer> testOverworldLayers = {
            BackgroundLayer(
                    Textures::LoadTexture("data/assets/backgrounds/TestOverworld/layer2.png",
                                          SDL_PIXELFORMAT_ARGB8888),
                    0.125, 0.125, 1, 0),
            BackgroundLayer(
                    Textures::LoadTexture("data/assets/backgrounds/TestOverworld/layer3.png",
                                          SDL_PIXELFORMAT_ARGB8888),
                    0.25, 0.25, 0, 0),
            BackgroundLayer(
                    Textures::LoadTexture("data/assets/backgrounds/TestOverworld/layer4.png",
                                          SDL_PIXELFORMAT_ARGB8888),
                    0.375, 0.375, 4, 0),
            BackgroundLayer(
                    Textures::LoadTexture("data/assets/backgrounds/TestOverworld/layer5.png",
                                          SDL_PIXELFORMAT_ARGB8888),
                    0.5, 0.5, 0, 0)};

    METADOT_NEW(C, global.game->GameIsolate_.backgrounds, Backgrounds);
    METADOT_CREATE(C, bg, Background, 0x7EAFCB, testOverworldLayers);
    global.game->GameIsolate_.backgrounds->Push("TEST_OVERWORLD", bg);
    global.game->GameIsolate_.backgrounds->Get("TEST_OVERWORLD")->init();
}

void Backgrounds::Unload() {
    for (auto &[name, bg]: m_backgrounds) METADOT_DELETE(C, bg, Background);
}