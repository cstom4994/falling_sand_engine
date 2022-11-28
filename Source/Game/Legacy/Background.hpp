// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Engine/Platforms/SDLWrapper.hpp"
#include "Engine/Render/renderer_gpu.h"
#include "Core/Core.hpp"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class BackgroundLayer {
public:
    std::vector<C_Surface *> surface;
    std::vector<METAENGINE_Render_Image *> texture;
    float parralaxX;
    float parralaxY;
    float moveX;
    float moveY;
    BackgroundLayer(C_Surface *texture, float parallaxX, float parallaxY, float moveX, float moveY);
    void init();
};

class Background {
public:
    UInt32 solid;
    std::vector<BackgroundLayer> layers;
    explicit Background(UInt32 solid, std::vector<BackgroundLayer> layers) : solid(std::move(solid)), layers(std::move(layers)){};
    void init();
};

class Backgrounds {
private:
    std::unordered_map<std::string, Background *> m_backgrounds;

public:
    Backgrounds() = default;
    ~Backgrounds();

    void Push(std::string name, Background *bg);
    Background *Get(std::string name);
    //static Background TEST_OVERWORLD;
};
