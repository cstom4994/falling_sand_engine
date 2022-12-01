// Copyright(c) 2022, KaoruXun All rights reserved.

#define INC_WorldGenerator

#ifndef INC_World
#include "world.hpp"
#endif

#include "Populators.hpp"

class World;
class Populator;

class WorldGenerator {
public:
    virtual void generateChunk(World *world, Chunk *ch) = 0;
    virtual std::vector<Populator *> getPopulators() = 0;
};
