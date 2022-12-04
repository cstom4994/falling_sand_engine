// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_CHUNK_HPP_
#define _METADOT_CHUNK_HPP_

#include "Core/Const.hpp"
#include "GameDataStruct.hpp"
#include "GameScriptingWrap.hpp"
#include "Materials.hpp"
#include "RigidBody.hpp"

#include <fstream>
#include <iostream>
#include <utility>

typedef struct
{
    UInt16 index;
    UInt32 color;
    int32_t temperature;
} MaterialInstanceData;

class Chunk {
    std::string fname;

public:
    int x;
    int y;
    bool hasMeta = false;
    // in order for a chunk to execute phase generationPhase+1, all surrounding chunks must be at least generationPhase
    int8_t generationPhase = 0;
    bool pleaseDelete = false;

    explicit Chunk(int x, int y, char *worldName)
        : x(std::move(x)), y(std::move(y)),
          fname(std::move(std::string(worldName) + "/chunks/chunk_" + std::to_string(x) + "_" +
                          std::to_string(y))){};
    Chunk() : Chunk(0, 0, (char *) "chunks"){};
    ~Chunk();

    void loadMeta();

    //static MaterialInstanceData* readBuf;
    void read();
    void write(MaterialInstance *tiles, MaterialInstance *layer2, UInt32 *background);
    bool hasFile();

    bool hasTileCache = false;
    MaterialInstance *tiles = nullptr;
    MaterialInstance *layer2 = nullptr;
    UInt32 *background = nullptr;
    std::vector<Biome *> biomes = {nullptr};
    std::vector<b2PolygonShape> polys = {};
    RigidBody *rb = nullptr;
};

class ChunkReadyToMerge {
public:
    int cx;
    int cy;
    MaterialInstance *tiles;
    ChunkReadyToMerge(int cx, int cy, MaterialInstance *tiles) {
        this->cx = cx;
        this->cy = cy;
        this->tiles = tiles;
    }
    ChunkReadyToMerge() : ChunkReadyToMerge(0, 0, nullptr){};
};

#endif