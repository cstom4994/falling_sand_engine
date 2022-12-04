// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GAMEDATASTRUCT_HPP_
#define _METADOT_GAMEDATASTRUCT_HPP_

#ifndef INC_Tiles
#include "Materials.hpp"
#endif// !INC_Tiles

#include <functional>

class Biome {
public:
    int id;
    std::string name;
    explicit Biome(std::string name, int id) : name(std::move(name)), id(std::move(id)){};
};

class Particle {
public:
    MaterialInstance tile{};
    float x = 0;
    float y = 0;
    float vx = 0;
    float vy = 0;
    float ax = 0;
    float ay = 0;
    float targetX = 0;
    float targetY = 0;
    float targetForce = 0;
    bool phase = false;
    bool temporary = false;
    int lifetime = 0;
    int fadeTime = 60;
    unsigned short inObjectState = 0;
    std::function<void()> killCallback = []() {};
    explicit Particle(MaterialInstance tile, float x, float y, float vx, float vy, float ax,
                      float ay)
        : tile(std::move(tile)), x(x), y(y), vx(vx), vy(vy), ax(ax), ay(ay) {}
    Particle(const Particle &part)
        : tile(part.tile), x(part.x), y(part.y), vx(part.vx), vy(part.vy), ax(part.ax),
          ay(part.ay) {}
};

#endif