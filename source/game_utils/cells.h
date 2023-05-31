
#pragma once

#include <string>
#include <vector>

#include "core/core.hpp"
#include "core/mathlib.hpp"
#include "game_datastruct.hpp"
#include "game_resources.hpp"
#include "renderer/gpu.hpp"

struct CellData {

    MaterialInstance tile{};
    F32 x = 0;
    F32 y = 0;
    F32 vx = 0;
    F32 vy = 0;
    F32 ax = 0;
    F32 ay = 0;
    F32 targetX = 0;
    F32 targetY = 0;
    F32 targetForce = 0;
    bool phase = false;
    bool temporary = false;
    int lifetime = 0;
    int fadeTime = 60;
    U8 inObjectState = 0;
    std::function<void()> killCallback = []() {};

    explicit CellData(MaterialInstance tile, F32 x, F32 y, F32 vx, F32 vy, F32 ax, F32 ay) : tile(std::move(tile)), x(x), y(y), vx(vx), vy(vy), ax(ax), ay(ay) {}
    CellData(const CellData& part) : tile(part.tile), x(part.x), y(part.y), vx(part.vx), vy(part.vy), ax(part.ax), ay(part.ay) {}
};
