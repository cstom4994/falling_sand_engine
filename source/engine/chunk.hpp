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
#include "engine/physics/box2d/inc/box2d.h"
#include "game/player.hpp"
#include "game_basic.hpp"
#include "game_datastruct.hpp"
#include "reflectionflat.hpp"

namespace ME {

// MaterialInstance并不会完全地存储在磁盘中
// 这里选择部分数据
// MaterialInstanceData是会存储在磁盘中的MaterialInstance数据结构
typedef struct {
    mat_instance_id index;
    u32 color;
    mat_temperature temperature;
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

    // TODO: 23/7/22 我在思考是否可以把区块的background数据换成一个整体的位图来存储，那样效率应该会更高
    u32 *background = nullptr;

    // 区块群系优化为id查表
    std::vector<int> biomes_id{};
    std::vector<b2PolygonShape> polys{};
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

    // 粗略计算区块占用内存字节
    u64 get_chunk_size();
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

ME_GUI_DEFINE_BEGIN(template <>, b2PolygonShape)
ImGui::Auto("b2PolygonShape");
ME_GUI_DEFINE_END

#endif
