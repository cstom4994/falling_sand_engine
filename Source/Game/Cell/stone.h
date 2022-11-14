#pragma once

#include "types.h"
#include "cell.h"

// Utility function to create sand particles
Cell CreateStone()
{
    int vx = (short)GetRandomValue(-1, 1);
    float random = (float)GetRandomValue(70, 90) / 100.f;
    Color base = GRAY;
    Color col = Color{ (unsigned char)(random * base.r), (unsigned char)(random * base.g),(unsigned char)(random * base.b), 255 };
    return Cell(CellType::Stone, col, vx, 1);
};

void ProcessStone(CellularData& data, CellMap& map)
{
    // Copy data
    Cell cell = map[data.x][data.y];
    map[data.x][data.y] = CreateAir();
    // Apply motion equations
    // FIXME particles going too fast might pass through others
    // Apply line tracing collision later
    int targetx = std::clamp(data.x + cell._vx, 0, mapSize - 1);
    int targety = std::clamp(data.y + cell._vy, 0, mapSize - 1);

    Cell target = map[targetx][targety];
    if (target._id == CellType::Water || target._id == CellType::Lava) {
        target.clock = data.clock;
        map[data.x][data.y] = target;
    } else if (map[targetx][targety]._id != CellType::Air) {
        // Collision
        targetx = data.x;
        targety = data.y;
        // FIXME momentum where
    }
    cell._vx *= GetRandomValue(-1, 1);
    cell.clock = data.clock;
    map[targetx][targety] = cell;
};
