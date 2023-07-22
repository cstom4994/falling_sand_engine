// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_GENERATOR_WORLD_H
#define ME_GENERATOR_WORLD_H

#include "engine/core/global.hpp"
#include "game.hpp"
#include "game_datastruct.hpp"

namespace ME {

class ScriptingWorldGenerator : public WorldGenerator {

    void generateChunk(world *world, Chunk *ch) override;
    std::vector<Populator *> getPopulators() override;
};

class MaterialTestGenerator : public WorldGenerator {
    void generateChunk(world *world, Chunk *ch) override;
    std::vector<Populator *> getPopulators() override;
};

class DefaultGenerator : public WorldGenerator {

    int getBaseHeight(world *world, int x, Chunk *ch);
    int getHeight(world *world, int x, Chunk *ch);

    void generateChunk(world *world, Chunk *ch) override;

    std::vector<Populator *> getPopulators() override;
};

}  // namespace ME

#endif
