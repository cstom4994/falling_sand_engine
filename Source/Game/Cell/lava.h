#pragma once

#include "types.h"
#include "cell.h"

Cell CreateLava()
{
    int vx = (short)GetRandomValue(-1, 1);
    float random = (float)GetRandomValue(70, 90) / 100.f;
    Color base = RED;
    Color col = Color{ (unsigned char)(random * base.r), (unsigned char)(random * base.g),(unsigned char)(random * base.b), 255 };
    return Cell(CellType::Lava, col, vx, 1);
}

void ProcessLava(CellularData& data, CellMap& map)
{
    Cell cell = map[data.x][data.y];
    
    int targetx = std::clamp(data.x + cell._vx, 0, mapSize - 1);
    int targety = std::clamp(data.y + cell._vy, 0, mapSize - 1);

    CellularData adjSand = IsTypeAdjacent(CellType::Sand, data, map);
    if (adjSand != nullData) {
        map[adjSand.x][adjSand.y] = CreateLava();
        map[adjSand.x][adjSand.y].clock = data.clock;
        return;
    }

    // Is water around
    CellularData adjWater = IsTypeAdjacent(CellType::Water, data, map);
    if (adjWater != nullData && adjWater.clock >= cell.clock) {
        // Remove water
        map[adjWater.x][adjWater.y] = CreateAir();
        // Transform into stone
        map[data.x][data.y] = CreateStone();
        map[data.x][data.y].clock = data.clock;
        return;
    }
    if (map[targetx][targety]._id != CellType::Air) {
        if (map[data.x][targety]._id == CellType::Air) {
            targetx = data.x;
        } else if (map[targetx][data.y]._id == CellType::Air) {
            // Attempt going sideways
            targety = data.y;
        } else {
            // stay put
            targetx = data.x;
            targety = data.y;
            cell._vx = GetRandomValue(-1, 1);
        }
    } else {
        // simulate air current?
        cell._vx = GetRandomValue(0, 100) <= 30 ? GetRandomValue(-1, 1) : cell._vx;
    }

    cell.clock = data.clock;
    map[data.x][data.y] = CreateAir();
    map[targetx][targety] = cell;
}
