// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_BACKGROUND_HPP_
#define _METADOT_BACKGROUND_HPP_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/core.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/sdl_wrapper.h"

class BackgroundLayer {
public:
    std::vector<C_Surface *> surface;
    std::vector<R_Image *> texture;
    F32 parralaxX;
    F32 parralaxY;
    F32 moveX;
    F32 moveY;
    BackgroundLayer(C_Surface *texture, F32 parallaxX, F32 parallaxY, F32 moveX, F32 moveY);
    void init();
};

class Background {
public:
    U32 solid;
    std::vector<BackgroundLayer> layers;
    explicit Background(U32 solid, std::vector<BackgroundLayer> layers) : solid(std::move(solid)), layers(std::move(layers)){};
    void init();
};

struct Backgrounds {

    std::unordered_map<std::string, Background *> m_backgrounds;

    void Push(std::string name, Background *bg);
    Background *Get(std::string name);

    void Load();
    void Unload();
};

#endif
