// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_CHUNK_HPP_
#define _METADOT_CHUNK_HPP_

#include <fstream>
#include <iostream>
#include <utility>

#include "core/const.h"
#include "core/core.h"
#include "core/cpp/static_relfection.hpp"
#include "game_datastruct.hpp"
#include "game_scriptingwrap.hpp"

typedef struct {
    U16 index;
    U32 color;
    I32 temperature;
} MaterialInstanceData;

// Chunk data structure
typedef struct Chunk {
    std::string fname;

    int x;
    int y;
    bool hasMeta = false;
    // in order for a chunk to execute phase generationPhase+1, all surrounding chunks must be at least generationPhase
    I8 generationPhase = 0;
    bool pleaseDelete = false;

    bool hasTileCache = false;
    MaterialInstance *tiles = nullptr;
    MaterialInstance *layer2 = nullptr;
    U32 *background = nullptr;
    MetaEngine::vector<Biome *> biomes = {nullptr};
    MetaEngine::vector<b2PolygonShape> polys = {};
    RigidBody *rb = nullptr;
} Chunk;

template <>
struct MetaEngine::StaticRefl::TypeInfo<Chunk> : TypeInfoBase<Chunk> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("fname"), &Chunk::fname},
            Field{TSTR("x"), &Chunk::x},
            Field{TSTR("y"), &Chunk::y},
            Field{TSTR("hasMeta"), &Chunk::hasMeta},
            Field{TSTR("generationPhase"), &Chunk::generationPhase},
            Field{TSTR("pleaseDelete"), &Chunk::pleaseDelete},
            Field{TSTR("hasTileCache"), &Chunk::hasTileCache},
            Field{TSTR("tiles"), &Chunk::tiles},
            Field{TSTR("layer2"), &Chunk::layer2},
            Field{TSTR("background"), &Chunk::background},
            Field{TSTR("biomes"), &Chunk::biomes},
            Field{TSTR("polys"), &Chunk::polys},
            Field{TSTR("rb"), &Chunk::rb},
    };
};

// Initialize a chunk
void Chunk_Init(Chunk *_struct, int x, int y, char *worldName);
// Uninitialize a chunk
void Chunk_Delete(Chunk *_struct);
// Check chunk's meta data
void Chunk_loadMeta(Chunk *_struct);

// static MaterialInstanceData* readBuf;
void Chunk_read(Chunk *_struct);
void Chunk_write(Chunk *_struct, MaterialInstance *tiles, MaterialInstance *layer2, U32 *background);
bool Chunk_hasFile(Chunk *_struct);

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
