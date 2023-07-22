// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_CHUNK_HPP
#define ME_CHUNK_HPP

#include <fstream>
#include <iostream>
#include <tuple>
#include <utility>

#include "engine/core/const.h"
#include "engine/core/core.hpp"
#include "engine/meta/reflection.hpp"
#include "engine/meta/static_relfection.hpp"
#include "game/player.hpp"
#include "game_basic.hpp"
#include "game_datastruct.hpp"
#include "reflectionflat.hpp"

#define CHUNK_DATABASE_FORMAT "ssiiiiiiiBB"

namespace ME {

typedef struct {
    u16 index;
    u32 color;
    i32 temperature;
} MaterialInstanceData;

template <>
struct meta::static_refl::TypeInfo<MaterialInstanceData> : TypeInfoBase<MaterialInstanceData> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("index"), &Type::index},
            Field{TSTR("color"), &Type::color},
            Field{TSTR("temperature"), &Type::temperature},
    };
};

// Chunk data structure
struct Chunk {
    std::string pack_filename;

    int x = 0;
    int y = 0;
    bool hasMeta = false;

    // in order for a chunk to execute phase generationPhase+1, all surrounding chunks must be at least generationPhase
    i8 generationPhase = 0;
    bool pleaseDelete = false;

    bool hasTileCache = false;
    MaterialInstance *tiles = nullptr;
    MaterialInstance *layer2 = nullptr;
    u32 *background = nullptr;

    // 区块群系优化为id查表
    std::vector<int> biomes_id{};
    std::vector<phy::Shape *> polys{};
    RigidBody *rb = nullptr;

    // Initialize a chunk
    void ChunkInit(int x, int y, const std::string &worldName);
    // Uninitialize a chunk
    void ChunkDelete();
    // Check chunk's meta data
    void ChunkLoadMeta();

    // static MaterialInstanceData* readBuf;
    void ChunkRead();
    void ChunkWrite(MaterialInstance *tiles, MaterialInstance *layer2, u32 *background);
    bool ChunkHasFile();
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

}  // namespace ME

template <>
struct ME::meta::static_refl::TypeInfo<ME::Chunk> : TypeInfoBase<Chunk> {
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
            Field{TSTR("biomes_id"), &Chunk::biomes_id},
            Field{TSTR("polys"), &Chunk::polys},
            Field{TSTR("rb"), &Chunk::rb},
    };
};

ME_GUI_DEFINE_BEGIN(template <>, ME::phy::Shape)
ImGui::Auto("ME::phy::Shape");
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN(template <>, ME::phy::Polygon)
ImGui::Auto("ME::phy::Polygon");
ME_GUI_DEFINE_END

#endif
