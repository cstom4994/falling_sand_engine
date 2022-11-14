#pragma once

//#include "cell.h"
#include "sand.h"

Cell CreateStatic()
{
    unsigned char random = (unsigned char)GetRandomValue(0, 225);
    //float random = (float)GetRandomValue(70, 90) / 100.f;
    //_col = Color{ (unsigned char)(random * GOLD.r), (unsigned char)(random * GOLD.g),(unsigned char)(random * (float)GOLD.b), 255 };
    Color _col = Color{ random, random, random, 255 };
    return Cell(CellType::Static, _col, 0, 1);
};

void ProcessStatic(CellularData& data, CellMap& map)
{
    Cell cell = map[data.x][data.y];
    unsigned char random = (unsigned char)GetRandomValue(0, 225);
    cell._col = Color{ random, random, random, 255 };
    // Apply motion equations
    // FIXME particles going too fast might pass through others
    // Apply line tracing collision later
    int targetx = std::clamp(data.x + cell._vx, 0, mapSize - 1);
    int targety = std::clamp(data.y + cell._vy, 0, mapSize - 1);

    //if (target.y >= prevGeneration.size() || prevGeneration[target.x][target.y]._id != CellType::Air) {
    if (map[targetx][targety]._id != CellType::Air) {
        // Collision
        targetx = data.x;
        targety = data.y;
        // FIXME momentum where
    }
    map[data.x][data.y] = CreateAir();
    cell.clock = data.clock;
    map[targetx][targety] = cell;
};
