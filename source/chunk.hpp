// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_CHUNK_HPP
#define ME_CHUNK_HPP

#include <fstream>
#include <iostream>
#include <tuple>
#include <utility>

#include "core/const.h"
#include "core/core.h"
#include "core/cpp/property.hpp"
#include "game_basic.hpp"
#include "game_datastruct.hpp"
#include "meta/reflection.hpp"
#include "meta/static_relfection.hpp"
#include "reflectionflat.hpp"

#define CHUNK_DATABASE_FORMAT "ssiiiiiiiBB"

typedef struct {
    U16 index;
    U32 color;
    I32 temperature;
} MaterialInstanceData;
template <>
struct ME::meta::static_refl::TypeInfo<MaterialInstanceData> : TypeInfoBase<MaterialInstanceData> {
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
    // bool hasMeta = false;

    Property<bool> hasMeta;

    // in order for a chunk to execute phase generationPhase+1, all surrounding chunks must be at least generationPhase
    I8 generationPhase = 0;
    bool pleaseDelete = false;

    bool hasTileCache = false;
    MaterialInstance *tiles = nullptr;
    MaterialInstance *layer2 = nullptr;
    U32 *background = nullptr;
    std::vector<Biome *> biomes = {nullptr};
    std::vector<b2PolygonShape> polys = {};
    RigidBody *rb = nullptr;
} Chunk;

template <>
struct ME::meta::static_refl::TypeInfo<Chunk> : TypeInfoBase<Chunk> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("pack_filename"), &Type::pack_filename},
            Field{TSTR("x"), &Type::x},
            Field{TSTR("y"), &Type::y},

            Field{TSTR("hasMeta"), &Type::hasMeta, AttrList{Attr{TSTR("Meta::Msg"), std::tuple{1.0f, 2.0f}}}},
            Field{TSTR("generationPhase"), &Chunk::generationPhase},
            Field{TSTR("pleaseDelete"), &Type::pleaseDelete},
            Field{TSTR("hasTileCache"), &Chunk::hasTileCache},
            Field{TSTR("tiles"), &Chunk::tiles},
            Field{TSTR("layer2"), &Chunk::layer2},
            Field{TSTR("background"), &Type::background},
            Field{TSTR("biomes"), &Chunk::biomes},
            Field{TSTR("polys"), &Chunk::polys},
            Field{TSTR("rb"), &Chunk::rb},
    };
};

METAENGINE_GUI_DEFINE_BEGIN(template <>, b2PolygonShape)
ImGui::Auto("b2PolygonShape");
METAENGINE_GUI_DEFINE_END

// Initialize a chunk
void ChunkInit(Chunk *_struct, int x, int y, char *worldName);
// Uninitialize a chunk
void ChunkDelete(Chunk *_struct);
// Check chunk's meta data
void ChunkLoadMeta(Chunk *_struct);

// static MaterialInstanceData* readBuf;
void ChunkRead(Chunk *_struct);
void ChunkWrite(Chunk *_struct, MaterialInstance *tiles, MaterialInstance *layer2, U32 *background);
bool ChunkHasFile(Chunk *_struct);

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
