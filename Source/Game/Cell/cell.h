#pragma once

#include "Libs/raylib/raylib.h"
#include "Libs/raylib/raymath.h"

#include "types.h"

// Should be a data only class
class Cell {
public:
    // Everything is public because i trust you all fellas
    CellType _id;
    Color _col;
    int _vx, _vy;
    unsigned short clock;

    Cell(CellType type = CellType::Air, Color color = RAYWHITE, short vx = 0, short vy = 0)
        : _id(type), _col(color), _vx(vx), _vy(vy), clock(0)
    {
    };

    static const int SIZE = 4;

    static int RoundToNearestSize(int pos)
    {
        //if (pos < 0) pos = 0; // Limit
        return round(pos / SIZE) * SIZE;
    }

    static Vector2 RoundToNearestSize(int x, int y, int padding = 0)
    {
        Vector2 result = Vector2{ 0,0 };
        int correctedx = round(x - padding * (x / SIZE));
        int correctedy = round(y - padding * (y / SIZE));
        result.x = RoundToNearestSize(correctedx);
        result.y = RoundToNearestSize(correctedy);
        return result;
    };

    static Vector2 RoundToNearestSize(Vector2 pos, int padding = 0)
    {
        return RoundToNearestSize(pos.x, pos.y, padding);
    };

    virtual void ProcessCell(CellularData& data, CellMap& map, const CellMap& prevGeneration) { return; };

};

Cell CreateAir()
{
    return Cell();
};

void ProcessAir(CellularData&, CellMap&)
{
    return;
}

CellularData IsTypeAdjacent(CellType type, CellularData location, CellMap& map)
{
    for (int i = -1; i <= 1; i++) {
        if (location.x + i < 0 || location.x + i >= mapSize) continue;
        for (int j = -1; j <= 1; j++) {
            if (location.y + j < 0 || location.y + j >= mapSize) continue;
            if (i == 0 && j == 0) continue;
            Cell& cell = map[location.x + i][location.y + j];
            if (cell._id == type) {
                return CellularData{ location.x + i, location.y + j, cell.clock };
            }
        }
    }

    return nullData;
}