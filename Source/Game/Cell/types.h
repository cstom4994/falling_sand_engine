#pragma once

#include <vector>
#include <algorithm>

#define CellMap std::vector<std::vector<Cell>>

const int mapSize = 150;
const int padding = 0;

typedef struct CellularData {
    int x;
    int y;
    unsigned clock;

    bool operator==(const CellularData& other) const
    {
        return (other.x == x) && (other.y == y) && (other.clock == clock);
    }

    bool operator!=(const CellularData& other) const
    {
        return !(*this == other);
    }
} CellularData;

const CellularData nullData = { -1,-1, 0 };

enum class CellType : unsigned short int {
    Air,
    Sand,
    Static,
    Spray,
    Moose, // ???
    Water,
    Lava,
    Stone,
    END_TYPE // keep this at the end
};

char* Cellnames[] = {
    "Air",
    "Sand",
    "Static",
    "Spray",
    "Moose",
    "Water",
    "Lava",
    "Stone",
    "None"
};
