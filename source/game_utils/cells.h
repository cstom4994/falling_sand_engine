
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
    f32 x = 0;
    f32 y = 0;
    f32 vx = 0;
    f32 vy = 0;
    f32 ax = 0;
    f32 ay = 0;
    f32 targetX = 0;
    f32 targetY = 0;
    f32 targetForce = 0;
    bool phase = false;
    bool temporary = false;
    int lifetime = 0;
    int fadeTime = 60;
    u8 inObjectState = 0;
    std::function<void()> killCallback = []() {};

    explicit CellData(MaterialInstance tile, f32 x, f32 y, f32 vx, f32 vy, f32 ax, f32 ay) : tile(std::move(tile)), x(x), y(y), vx(vx), vy(vy), ax(ax), ay(ay) {}
    CellData(const CellData& part) : tile(part.tile), x(part.x), y(part.y), vx(part.vx), vy(part.vy), ax(part.ax), ay(part.ay) {}
};
