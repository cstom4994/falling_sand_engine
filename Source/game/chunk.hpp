// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_CHUNK_HPP_
#define _METADOT_CHUNK_HPP_

#include <fstream>
#include <iostream>
#include <tuple>
#include <utility>

#include "core/const.h"
#include "core/core.h"
#include "core/cpp/static_relfection.hpp"
#include "engine/code_reflection.hpp"
#include "engine/reflectionflat.hpp"
#include "game_basic.hpp"
#include "game_datastruct.hpp"

typedef struct {
    U16 index;
    U32 color;
    I32 temperature;
} MaterialInstanceData;
template <>
struct MetaEngine::StaticRefl::TypeInfo<MaterialInstanceData> : TypeInfoBase<MaterialInstanceData> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("index"), &Type::index},
            Field{TSTR("color"), &Type::color},
            Field{TSTR("temperature"), &Type::temperature},
    };
};

// Chunk data structure
typedef struct Chunk {
    std::string pack_filename;

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
            Field{TSTR("pack_filename"), &Type::pack_filename}, Field{TSTR("x"), &Type::x}, Field{TSTR("y"), &Type::y},

            Field{TSTR("hasMeta"), &Type::hasMeta, AttrList{Attr{TSTR("Meta::Msg"), std::tuple{1.0f, 2.0f}}}},
            // Field{TSTR("generationPhase"), &Chunk::generationPhase},
            Field{TSTR("pleaseDelete"), &Type::pleaseDelete},
            // Field{TSTR("hasTileCache"), &Chunk::hasTileCache},
            // Field{TSTR("tiles"), &Chunk::tiles},
            // Field{TSTR("layer2"), &Chunk::layer2},
            Field{TSTR("background"), &Type::background},
            // Field{TSTR("biomes"), &Chunk::biomes},
            // Field{TSTR("polys"), &Chunk::polys},
            // Field{TSTR("rb"), &Chunk::rb},
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
