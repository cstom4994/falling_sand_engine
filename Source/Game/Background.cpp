// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Game/InEngine.h"

#include "Background.hpp"
#include "Game/Textures.hpp"
#include <algorithm>

#include "Engine/Memory.hpp"
#include "Game/InEngine.h"

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

Backgrounds::~Backgrounds() {
    for (auto &[name, bg]: m_backgrounds) METADOT_DELETE(C, bg, Background);
}