// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_GENERATOR_WORLD_H
#define ME_GENERATOR_WORLD_H

#include "engine/core/global.hpp"
#include "game.hpp"
#include "game_datastruct.hpp"

class ScriptingWorldGenerator : public WorldGenerator {

    void generateChunk(World *world, Chunk *ch) override;
    std::vector<Populator *> getPopulators() override;
};

class MaterialTestGenerator : public WorldGenerator {
    void generateChunk(World *world, Chunk *ch) override;
    std::vector<Populator *> getPopulators() override;
};

class DefaultGenerator : public WorldGenerator {

    int getBaseHeight(World *world, int x, Chunk *ch);
    int getHeight(World *world, int x, Chunk *ch);

    void generateChunk(World *world, Chunk *ch) override;

    std::vector<Populator *> getPopulators() override;
};

#endif
