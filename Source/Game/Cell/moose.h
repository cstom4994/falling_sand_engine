#pragma once

#include "types.h"
#include "cell.h"

// Moose coffee
Cell CreateMoose()
{
    float random = (float)GetRandomValue(70, 90) / 100.f;
    Color base = ORANGE;
    Color _col = Color{ (unsigned char)(random * base.r), (unsigned char)(random * base.g),(unsigned char)(random * base.b), 255 };
    int vx = GetRandomValue(-1, 1);// ? -1 : 1;
    return Cell(CellType::Moose, _col, vx, -1);
};

void ProcessMoose(CellularData& data, CellMap& map)
{
    /* Imagine this prevGeneration:
     * [Moose] [Air] [Moose]
     * In the next step both will merge into the same air cell
     * Issue here is collisions with two moving particles will never work
     * in current state. This is why buffering just won't work
     * For reference, The powder toy directly manipulates the map
     * https://github.com/The-Powder-Toy/The-Powder-Toy/blob/master/src/simulation/Simulation.cpp#L3141
     * it also stores actual data in a separate buffer to avoid moving 65 MB worth of data every frame
     * and to solve particles racing the scanline
     */

     // Copy data
    Cell cell = map[data.x][data.y];
    // Apply motion equations
    // FIXME particles going too fast might pass through others
    // Apply line tracing collision later
    int targetx = std::clamp(data.x + cell._vx, 0, mapSize - 1);
    int targety = std::clamp(data.y - 1, 0, mapSize - 1);

    // If desired position is occupied
    if (map[targetx][targety]._id != CellType::Air) {
        cell._vx = GetRandomValue(-1, 1);
        if (map[data.x][targety]._id == CellType::Air) {
            // Attempt going up
            targetx = data.x;
        } else if (map[targetx][data.y]._id == CellType::Air) {
            // Attempt going sideways
            targety = data.y;
        } else {
            // stay put
            targetx = data.x;
            targety = data.y;
        }
    } else {
        // simulate air current?
        cell._vx = GetRandomValue(0, 100) <= 30 ? GetRandomValue(-1, 1) : cell._vx;
    }

    map[data.x][data.y] = CreateAir();
    cell.clock = data.clock;
    map[targetx][targety] = cell;
};
