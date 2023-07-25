// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_GAMEDATASTRUCT_HPP
#define ME_GAMEDATASTRUCT_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/core/mathlib.hpp"
#include "engine/core/sdl_wrapper.h"
#include "engine/ecs/ecs.hpp"
#include "engine/meta/static_relfection.hpp"
#include "engine/physics/inc/physics2d.h"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/lua_wrapper.hpp"
#include "engine/utils/type.hpp"
#include "game_basic.hpp"

struct ImGuiContext;

namespace ME {

struct Chunk;
struct Populator;
struct world;
struct RigidBody;
struct R_Target;
struct CellData;
struct Biome;
struct Material;
struct Player;
struct game;

#define RegisterFunctions(name, func)    \
    Meta::AnyFunction any_##func{&func}; \
    GAME()->HostData.Functions.insert(std::make_pair(#name, any_##func))

#define GetFunctions(name) GAME()->HostData.Functions[name]

class WorldEntity {
public:
    std::string name;

    f32 x = 0;
    f32 y = 0;
    f32 vx = 0;
    f32 vy = 0;
    int hw = 14;
    int hh = 26;
    bool ground = false;
    RigidBody *rb = nullptr;
    bool is_player = false;

    WorldEntity(const WorldEntity &) = default;

    WorldEntity(bool isplayer, f32 x, f32 y, f32 vx, f32 vy, int hw, int hh, RigidBody *rb, std::string n = "unknown")
        : is_player(isplayer), x(x), y(y), vx(vx), vy(vy), hw(hw), hh(hh), rb(rb), name(n) {}
    ~WorldEntity() {
        // if (static_cast<bool>(rb)) delete rb;
    }
};

template <>
struct meta::static_refl::TypeInfo<WorldEntity> : TypeInfoBase<WorldEntity> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("x"), &Type::x},
            Field{TSTR("y"), &Type::y},
            Field{TSTR("vx"), &Type::vx},
            Field{TSTR("vy"), &Type::vy},
            Field{TSTR("hw"), &Type::hw},
            Field{TSTR("hh"), &Type::hh},
            Field{TSTR("ground"), &Type::ground},

            // Field{TSTR("body"), &Type::body},
            Field{TSTR("rb"), &Type::rb},
            Field{TSTR("is_player"), &Type::is_player},
    };
};

void ReleaseGameData();

#pragma region Material

enum PhysicsType {
    AIR = 0,
    SOLID = 1,
    SAND = 2,
    SOUP = 3,
    GAS = 4,
    PASSABLE = 5,
    OBJECT = 5,
};

#define INTERACT_NONE 0
#define INTERACT_TRANSFORM_MATERIAL 1  // id, radius
#define INTERACT_SPAWN_MATERIAL 2      // id, radius
#define EXPLODE 3                      // radius

#define REACT_TEMPERATURE_BELOW 4  // temperature, id
#define REACT_TEMPERATURE_ABOVE 5  // temperature, id

// 一些材料数据类型
using mat_id = u32;
using mat_instance_id = mat_id;
using world_background_col = u32;
using mat_temperature = i16;

// 材料相互作用数据
struct MaterialInteraction {
    int type = INTERACT_NONE;
    mat_temperature data1 = 0;  // 温度
    mat_id data2 = 0;           // 材料索引ID
    int ofsX = 0;
    int ofsY = 0;
};

template <>
struct meta::static_refl::TypeInfo<MaterialInteraction> : TypeInfoBase<MaterialInteraction> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("type"), &MaterialInteraction::type}, Field{TSTR("data1"), &MaterialInteraction::data1}, Field{TSTR("data2"), &MaterialInteraction::data2},
            Field{TSTR("ofsX"), &MaterialInteraction::ofsX}, Field{TSTR("ofsY"), &MaterialInteraction::ofsY},
    };
};

// 材料类型
struct Material {
    std::string name;
    std::string index_name;

    mat_id id = 0;
    int physicsType = 0;
    u8 alpha = 0;
    f32 density = 0;
    int iterations = 0;
    int emit = 0;
    u32 emitColor = 0;
    u32 color = 0;
    u32 addTemp = 0;
    f32 conductionSelf = 1.0;
    f32 conductionOther = 1.0;

    bool is_scriptable = false;

    bool interact = false;
    int *nInteractions = nullptr;

    std::vector<MaterialInteraction> *interactions = nullptr;

    bool react = false;
    int nReactions = 0;

    std::vector<MaterialInteraction> reactions;

    int slipperyness = 1;

    Material(mat_id id, std::string name, std::string index_name, PhysicsType physicsType, int slipperyness, u8 alpha, f32 density, int iterations, int emit, u32 emitColor, u32 color);
    Material(mat_id id, std::string name, std::string index_name, PhysicsType physicsType, int slipperyness, u8 alpha, f32 density, int iterations, int emit, u32 emitColor)
        : Material(id, name, index_name, physicsType, slipperyness, alpha, density, iterations, emit, emitColor, 0xffffffff) {}
    Material(mat_id id, std::string name, std::string index_name, PhysicsType physicsType, int slipperyness, u8 alpha, f32 density, int iterations)
        : Material(id, name, index_name, physicsType, slipperyness, alpha, density, iterations, 0, 0) {}
    Material(mat_id id, std::string name, std::string index_name, PhysicsType physicsType, int slipperyness, f32 density, int iterations)
        : Material(id, name, index_name, physicsType, slipperyness, 0xff, density, iterations) {}
    Material() : Material(0, "Air", "", PhysicsType::AIR, 4, 0, 0) {}
};

struct MaterialsList {
    std::unordered_map<int, Material> ScriptableMaterials;
    Material GENERIC_AIR;
    Material GENERIC_SOLID;
    Material GENERIC_SAND;
    Material GENERIC_LIQUID;
    Material GENERIC_GAS;
    Material GENERIC_PASSABLE;
    Material GENERIC_OBJECT;
    Material STONE;
    Material GRASS;
    Material DIRT;
    Material SMOOTH_STONE;
    Material COBBLE_STONE;
    Material SMOOTH_DIRT;
    Material COBBLE_DIRT;
    Material SOFT_DIRT;
    Material WATER;
    Material LAVA;
    Material CLOUD;
    Material GOLD_ORE;
    Material GOLD_MOLTEN;
    Material GOLD_SOLID;
    Material IRON_ORE;
    Material OBSIDIAN;
    Material STEAM;
    Material SOFT_DIRT_SAND;
    Material FIRE;
    Material FLAT_COBBLE_STONE;
    Material FLAT_COBBLE_DIRT;
};

void InitMaterials();
void RegisterMaterial(int s_id, std::string name, std::string index_name, int physicsType, int slipperyness, u8 alpha, f32 density, int iterations, int emit, u32 emitColor, u32 color);
void PushMaterials();

// 材料实例
class MaterialInstance {
public:
    mat_instance_id id = 0;
    Material *mat;

    u32 color;
    mat_temperature temperature;
    bool moved = false;
    f32 fluidAmount = 2.0f;
    f32 fluidAmountDiff = 0.0f;
    u8 settleCount = 0;

    MaterialInstance(Material *mat, u32 color, mat_temperature temperature);
    MaterialInstance(Material *mat, u32 color) : MaterialInstance(mat, color, 0){};
    MaterialInstance();
    inline bool operator==(const MaterialInstance &other) { return this->mat->id == other.mat->id; }
};

static_assert(sizeof(MaterialInstance) == 40);

template <>
struct meta::static_refl::TypeInfo<MaterialInstance> : TypeInfoBase<MaterialInstance> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {};
};

struct GameData {
    i32 ofsX = 0;
    i32 ofsY = 0;

    f32 plPosX = 0;
    f32 plPosY = 0;

    f32 camX = 0;
    f32 camY = 0;

    f32 desCamX = 0;
    f32 desCamY = 0;

    f32 freeCamX = 0;
    f32 freeCamY = 0;

    std::map<std::string, Biome> biome_container;

    std::vector<Material *> materials_container;
    i32 materials_count;
    Material **materials_array;

    std::vector<MaterialInstance> mat_instance_container;
    MaterialInstance *mat_instance_array;

    // 材料数据结构
    MaterialsList materials_list;

    struct {
        std::unordered_map<std::string, meta::any_function> Functions;
    } HostData;
};

extern MaterialInstance Tiles_NOTHING;
extern MaterialInstance Tiles_TEST_SOLID;
extern MaterialInstance Tiles_TEST_SAND;
extern MaterialInstance Tiles_TEST_LIQUID;
extern MaterialInstance Tiles_TEST_GAS;
extern MaterialInstance Tiles_OBJECT;

MaterialInstance TilesCreateTestSand();
MaterialInstance TilesCreateTestTexturedSand(int x, int y);
MaterialInstance TilesCreateTestLiquid();
MaterialInstance TilesCreateStone(int x, int y);
MaterialInstance TilesCreateGrass();
MaterialInstance TilesCreateDirt();
MaterialInstance TilesCreateSmoothStone(int x, int y);
MaterialInstance TilesCreateCobbleStone(int x, int y);
MaterialInstance TilesCreateSmoothDirt(int x, int y);
MaterialInstance TilesCreateCobbleDirt(int x, int y);
MaterialInstance TilesCreateSoftDirt(int x, int y);
MaterialInstance TilesCreateWater();
MaterialInstance TilesCreateLava();
MaterialInstance TilesCreateCloud(int x, int y);
MaterialInstance TilesCreateGold(int x, int y);
MaterialInstance TilesCreateIron(int x, int y);
MaterialInstance TilesCreateObsidian(int x, int y);
MaterialInstance TilesCreateSteam();
MaterialInstance TilesCreateFire();
MaterialInstance TilesCreate(mat_id id, int x, int y);
// MaterialInstance TilesCreate(mat_id id, int x, int y, int test);

#pragma endregion Material

class Structure {
public:
    MaterialInstance *tiles;
    int w;
    int h;

    Structure(int w, int h, MaterialInstance *tiles);
    Structure(C_Surface *texture, Material templ);
    Structure() = default;
};

class world;

class Structures {
public:
    static Structure makeTree(world world, int x, int y);
    static Structure makeTree1(world world, int x, int y);
};

class PlacedStructure {
public:
    Structure base;
    int x;
    int y;

    PlacedStructure(Structure base, int x, int y);
    PlacedStructure(const PlacedStructure &p2) {
        this->base = Structure(base);
        this->x = x;
        this->y = y;
    }
};

class Biome {
public:
    int id = -1;
    explicit Biome(int id) : /*name(std::move(name)),*/ id(std::move(id)){};

    Biome() = default;
    Biome(const Biome &) = default;

public:
    static Biome biomeGet(std::string name);
    static ME_INLINE int biomeGetID(std::string name) { return biomeGet(name).id; }
    static void createBiome(std::string name, int id);
};

template <>
struct meta::static_refl::TypeInfo<Biome> : TypeInfoBase<Biome> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("id"), &Type::id},
            // Field{TSTR("name"), &Type::name},
    };
};

struct WorldGenerator {
    virtual void generateChunk(world *world, Chunk *ch) = 0;
    virtual std::vector<Populator *> getPopulators() = 0;
};

struct Populator {
    virtual int getPhase() = 0;
    virtual std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, world *world) = 0;
};

#pragma region Populators

struct TestPhase1Populator : public Populator {
    int getPhase() { return 1; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, world *world);
};

struct TestPhase2Populator : public Populator {
    int getPhase() { return 2; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, world *world);
};

struct TestPhase3Populator : public Populator {
    int getPhase() { return 3; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, world *world);
};

struct TestPhase4Populator : public Populator {
    int getPhase() { return 4; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, world *world);
};

struct TestPhase5Populator : public Populator {
    int getPhase() { return 5; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, world *world);
};

struct TestPhase6Populator : public Populator {
    int getPhase() { return 6; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, world *world);
};

struct TestPhase0Populator : public Populator {
    int getPhase() { return 0; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, world *world);
};

struct CavePopulator : public Populator {
    int getPhase() { return 0; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, world *world);
};

struct CobblePopulator : public Populator {
    int getPhase() { return 1; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, world *world);
};

struct OrePopulator : public Populator {
    int getPhase() { return 0; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, world *world);
};

struct TreePopulator : public Populator {
    int getPhase() { return 1; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, world *world);
};

#pragma endregion Populators

template <>
struct meta::static_refl::TypeInfo<Material> : TypeInfoBase<Material> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("name"), &Material::name},
            Field{TSTR("index_name"), &Material::index_name},
            Field{TSTR("id"), &Material::id},
            Field{TSTR("is_scriptable"), &Material::is_scriptable},
            Field{TSTR("physicsType"), &Material::physicsType},
            Field{TSTR("alpha"), &Material::alpha},
            Field{TSTR("density"), &Material::density},
            Field{TSTR("iterations"), &Material::iterations},
            Field{TSTR("emit"), &Material::emit},
            Field{TSTR("emitColor"), &Material::emitColor},
            Field{TSTR("color"), &Material::color},
            Field{TSTR("addTemp"), &Material::addTemp},
            Field{TSTR("conductionSelf"), &Material::conductionSelf},
            Field{TSTR("conductionOther"), &Material::conductionOther},
            Field{TSTR("interact"), &Material::interact},
            Field{TSTR("nInteractions"), &Material::nInteractions},
            // Field{TSTR("interactions"), &Material::interactions},
            Field{TSTR("react"), &Material::react},
            Field{TSTR("nReactions"), &Material::nReactions},
            // Field{TSTR("reactions"), &Material::reactions},
            Field{TSTR("slipperyness"), &Material::slipperyness},
    };
};

extern GameData g_game_data;

ME_INLINE GameData *GAME() { return &g_game_data; }

}  // namespace ME

ME_GUI_DEFINE_BEGIN(template <>, ME::WorldEntity)
ME::meta::static_refl::TypeInfo<ME::WorldEntity>::ForEachVarOf(var, [&](const auto &field, auto &&value) { ImGui::Auto(value, std::string(field.name)); });
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN(template <>, ME::Material)
ME::meta::static_refl::TypeInfo<ME::Material>::ForEachVarOf(var, [](auto field, auto &&value) {
    static_assert(std::is_lvalue_reference_v<decltype(value)>);
    ImGui::Auto(value, std::string(field.name));
});
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN(template <>, ME::MaterialInstance)
ImGui::Text("MaterialInstance: %d\n", var.id);
ImGui::Auto(*var.mat, "Material");  // 这里 var.mat 是指针 所以用*操作符给auto
ME_GUI_DEFINE_END

ME_GUI_DEFINE_BEGIN(template <>, ME::Biome)
// ImGui::Text("Name: %s", var.name.c_str());
ImGui::Text("ID: %d", var.id);
ME_GUI_DEFINE_END

#endif
