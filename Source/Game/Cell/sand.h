#pragma once

#include "types.h"
#include "cell.h"

// Utility function to create sand particles
Cell CreateSand()
{
    float random = (float)GetRandomValue(70, 90) / 100.f;
    Color _col = Color{ (unsigned char)(random * GOLD.r), (unsigned char)(random * GOLD.g),(unsigned char)(random * (float)GOLD.b), 255 };
    int vx = (short)GetRandomValue(-1, 1);
    return Cell(CellType::Sand, _col, vx, 1);
};

void ProcessSand(CellularData& data, CellMap& map)
{
    // Copy data
    Cell cell = map[data.x][data.y];
    cell.clock = data.clock;
    map[data.x][data.y] = CreateAir();

    int targetx = std::clamp(data.x + cell._vx, 0, mapSize - 1);
    int targety = std::clamp(data.y + cell._vy, 0, mapSize - 1);

    Cell adjWater = map[targetx][targety];
    if (adjWater._id == CellType::Water) {
        // Displace water
        // Not proud of this
        adjWater.clock = data.clock;
        map[data.x][data.y] = adjWater;
    } else if (map[targetx][targety]._id != CellType::Air) {
        // Collision
        targetx = data.x;
        targety = data.y;
        // FIXME momentum where
    }
    cell._vx *= GetRandomValue(-1, 1);
    map[targetx][targety] = cell;
};
