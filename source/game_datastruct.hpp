// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_GAMEDATASTRUCT_HPP
#define ME_GAMEDATASTRUCT_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/core.hpp"
#include "core/cpp/type.hpp"
#include "core/mathlib.hpp"
#include "core/sdl_wrapper.h"
#include "ecs/ecs.hpp"
#include "game_basic.hpp"
#include "meta/static_relfection.hpp"
#include "physics/box2d.h"
#include "renderer/renderer_gpu.h"
#include "scripting/lua_wrapper.hpp"

struct Chunk;
struct Populator;
struct World;
struct RigidBody;
struct b2Body;
struct R_Target;
struct CellData;
struct Biome;
struct Material;
struct Player;
struct Game;
struct ImGuiContext;

#define RegisterFunctions(name, func)    \
    Meta::AnyFunction any_##func{&func}; \
    global.GameData_.HostData.Functions.insert(std::make_pair(#name, any_##func))

#define GetFunctions(name) global.GameData_.HostData.Functions[name]

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

    static std::vector<Biome *> biome_container;
    static std::vector<Material *> materials_container;
    static i32 materials_count;
    static Material **materials_array;

    struct {
        std::unordered_map<std::string, ME::meta::any_function> Functions;
    } HostData;
};

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
struct ME::meta::static_refl::TypeInfo<WorldEntity> : TypeInfoBase<WorldEntity> {
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

ME_GUI_DEFINE_BEGIN(template <>, WorldEntity)
ME::meta::static_refl::TypeInfo<WorldEntity>::ForEachVarOf(var, [&](const auto &field, auto &&value) { ImGui::Auto(value, std::string(field.name)); });
ME_GUI_DEFINE_END

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

struct MaterialInteraction {
    int type = INTERACT_NONE;
    int data1 = 0;
    int data2 = 0;
    int ofsX = 0;
    int ofsY = 0;
};

template <>
struct ME::meta::static_refl::TypeInfo<MaterialInteraction> : TypeInfoBase<MaterialInteraction> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("type"), &MaterialInteraction::type}, Field{TSTR("data1"), &MaterialInteraction::data1}, Field{TSTR("data2"), &MaterialInteraction::data2},
            Field{TSTR("ofsX"), &MaterialInteraction::ofsX}, Field{TSTR("ofsY"), &MaterialInteraction::ofsY},
    };
};

struct Material {
    std::string name;
    std::string index_name;

    int id = 0;
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

    Material(int id, std::string name, std::string index_name, PhysicsType physicsType, int slipperyness, u8 alpha, f32 density, int iterations, int emit, u32 emitColor, u32 color);
    Material(int id, std::string name, std::string index_name, PhysicsType physicsType, int slipperyness, u8 alpha, f32 density, int iterations, int emit, u32 emitColor)
        : Material(id, name, index_name, physicsType, slipperyness, alpha, density, iterations, emit, emitColor, 0xffffffff) {}
    Material(int id, std::string name, std::string index_name, PhysicsType physicsType, int slipperyness, u8 alpha, f32 density, int iterations)
        : Material(id, name, index_name, physicsType, slipperyness, alpha, density, iterations, 0, 0) {}
    Material(int id, std::string name, std::string index_name, PhysicsType physicsType, int slipperyness, f32 density, int iterations)
        : Material(id, name, index_name, physicsType, slipperyness, 0xff, density, iterations) {}
    Material() : Material(0, "Air", "", PhysicsType::AIR, 4, 0, 0) {}
};

template <>
struct ME::meta::static_refl::TypeInfo<Material> : TypeInfoBase<Material> {
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

ME_GUI_DEFINE_BEGIN(template <>, Material)
ME::meta::static_refl::TypeInfo<Material>::ForEachVarOf(var, [](auto field, auto &&value) {
    static_assert(std::is_lvalue_reference_v<decltype(value)>);
    ImGui::Auto(value, std::string(field.name));
});
ME_GUI_DEFINE_END

struct MaterialsList {
    static std::unordered_map<int, Material> ScriptableMaterials;
    static Material GENERIC_AIR;
    static Material GENERIC_SOLID;
    static Material GENERIC_SAND;
    static Material GENERIC_LIQUID;
    static Material GENERIC_GAS;
    static Material GENERIC_PASSABLE;
    static Material GENERIC_OBJECT;
    static Material STONE;
    static Material GRASS;
    static Material DIRT;
    static Material SMOOTH_STONE;
    static Material COBBLE_STONE;
    static Material SMOOTH_DIRT;
    static Material COBBLE_DIRT;
    static Material SOFT_DIRT;
    static Material WATER;
    static Material LAVA;
    static Material CLOUD;
    static Material GOLD_ORE;
    static Material GOLD_MOLTEN;
    static Material GOLD_SOLID;
    static Material IRON_ORE;
    static Material OBSIDIAN;
    static Material STEAM;
    static Material SOFT_DIRT_SAND;
    static Material FIRE;
    static Material FLAT_COBBLE_STONE;
    static Material FLAT_COBBLE_DIRT;
};

void InitMaterials();
void RegisterMaterial(int s_id, std::string name, std::string index_name, int physicsType, int slipperyness, u8 alpha, f32 density, int iterations, int emit, u32 emitColor, u32 color);
void PushMaterials();

class MaterialInstance {
public:
    Material *mat;
    u32 color;
    i32 temperature;
    u32 id = 0;
    bool moved = false;
    f32 fluidAmount = 2.0f;
    f32 fluidAmountDiff = 0.0f;
    u8 settleCount = 0;

    MaterialInstance(Material *mat, u32 color, i32 temperature);
    MaterialInstance(Material *mat, u32 color) : MaterialInstance(mat, color, 0){};
    MaterialInstance() : MaterialInstance(&MaterialsList::GENERIC_AIR, 0x000000, 0){};
    inline bool operator==(const MaterialInstance &other) { return this->mat->id == other.mat->id; }
};

template <>
struct ME::meta::static_refl::TypeInfo<MaterialInstance> : TypeInfoBase<MaterialInstance> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {};
};

ME_GUI_DEFINE_BEGIN(template <>, MaterialInstance)
ImGui::Text("MaterialInstance:\n%d", var.id);
ImGui::Auto(var.mat, "Material");
ME_GUI_DEFINE_END

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
MaterialInstance TilesCreate(Material *mat, int x, int y);
MaterialInstance TilesCreate(int id, int x, int y);

#pragma endregion Material

MAKE_ENUM_FLAGS(ItemFlags, int){
        ItemFlags_None = 1 << 0,   ItemFlags_Rigidbody = 1 << 1, ItemFlags_Fluid_Container = 1 << 2, ItemFlags_Tool = 1 << 3,
        ItemFlags_Chisel = 1 << 4, ItemFlags_Hammer = 1 << 5,    ItemFlags_Vacuum = 1 << 6,
};

typedef enum EnumPlayerHoldType {
    None = 0,
    Hammer = 1,
    Vacuum,
} EnumPlayerHoldType;

template <>
struct ME::meta::static_refl::TypeInfo<EnumPlayerHoldType> : TypeInfoBase<EnumPlayerHoldType> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("None"), Type::None},
            Field{TSTR("Hammer"), Type::Hammer},
            Field{TSTR("Vacuum"), Type::Vacuum},
    };
};

ME_GUI_DEFINE_BEGIN(template <>, EnumPlayerHoldType)
ImGui::Text("EnumPlayerHoldType: %s", ME::meta::static_refl::TypeInfo<EnumPlayerHoldType>::fields.NameOfValue(var).data());
ME_GUI_DEFINE_END

class Item {
public:
    std::string name;

    ItemFlags flags = ItemFlags::ItemFlags_None;

    void setFlag(ItemFlags f) { flags |= f; }
    bool getFlag(ItemFlags f) { return static_cast<bool>(flags & f); }

    C_Surface *surface = nullptr;
    R_Image *texture = nullptr;

    int pivotX = 0;
    int pivotY = 0;
    f32 breakSize = 16;
    std::vector<MaterialInstance> carry;
    std::vector<U16Point> fill;
    u16 capacity = 0;

    std::vector<CellData *> vacuumCells = {};

    Item(const Item &p) = default;

    Item();
    ~Item();

    static Item *makeItem(ItemFlags flags, RigidBody *rb, std::string n = "unknown");
    void loadFillTexture(C_Surface *tex);
};

template <>
struct ME::meta::static_refl::TypeInfo<Item> : TypeInfoBase<Item> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {Field{TSTR("name"), &Type::name}, Field{TSTR("pivotX"), &Type::pivotX}, Field{TSTR("pivotY"), &Type::pivotY}, Field{TSTR("breakSize"), &Type::breakSize},
                                         Field{TSTR("capacity"), &Type::capacity}};
};

ME_GUI_DEFINE_BEGIN(template <>, Item)
ME::meta::static_refl::TypeInfo<Item>::ForEachVarOf(var, [&](const auto &field, auto &&value) { ImGui::Auto(value, std::string(field.name)); });
ME_GUI_DEFINE_END

using ItemLuaPtr = ME::ref<Item>;

struct ItemBinding : public LuaWrapper::PodBind::Binding<ItemBinding, Item> {
    static constexpr const char *class_name = "Item";
    static luaL_Reg *members() { static luaL_Reg members[] = {{"setFlag", setFlag}, {"getFlag", getFlag}, {nullptr, nullptr}}; }
    // Lua constructor
    static int create(lua_State *L) {
        LuaWrapper::PodBind::CheckArgCount(L, 2);
        const char *name = luaL_checkstring(L, 1);
        int age = luaL_checkinteger(L, 2);
        ItemLuaPtr sp = ME::create_ref<Item>();
        push(L, sp);
        return 1;
    }
    // Bind functions
    static int setFlag(lua_State *L) {
        LuaWrapper::PodBind::CheckArgCount(L, 2);
        ItemLuaPtr t = fromStack(L, 1);
        t->setFlag((ItemFlags)lua_tointeger(L, 2));
        return 0;
    }
    static int getFlag(lua_State *L) {
        LuaWrapper::PodBind::CheckArgCount(L, 2);
        ItemLuaPtr t = fromStack(L, 1);
        lua_pushboolean(L, t->getFlag((ItemFlags)lua_tointeger(L, 2)));
        return 1;
    }
};

class Structure {
public:
    MaterialInstance *tiles;
    int w;
    int h;

    Structure(int w, int h, MaterialInstance *tiles);
    Structure(C_Surface *texture, Material templ);
    Structure() = default;
};

class World;

class Structures {
public:
    static Structure makeTree(World world, int x, int y);
    static Structure makeTree1(World world, int x, int y);
};

class PlacedStructure {
public:
    Structure base;
    int x;
    int y;

    PlacedStructure(Structure base, int x, int y);
    // PlacedStructure(const PlacedStructure &p2) { this->base = Structure(base); this->x = x; this->y = y; }
};

class Biome {
public:
    int id;
    std::string name;
    explicit Biome(std::string name, int id) : name(std::move(name)), id(std::move(id)){};
};

template <>
struct ME::meta::static_refl::TypeInfo<Biome> : TypeInfoBase<Biome> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("id"), &Type::id},
            Field{TSTR("name"), &Type::name},
    };
};

ME_GUI_DEFINE_BEGIN(template <>, Biome)
ImGui::Text("Name: %s", var.name.c_str());
ImGui::Text("ID: %d", var.id);
ME_GUI_DEFINE_END

struct WorldGenerator {
    virtual void generateChunk(World *world, Chunk *ch) = 0;
    virtual std::vector<Populator *> getPopulators() = 0;
};

struct Populator {
    virtual int getPhase() = 0;
    virtual std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world) = 0;
};

#pragma region Populators

struct TestPhase1Populator : public Populator {
    int getPhase() { return 1; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world);
};

struct TestPhase2Populator : public Populator {
    int getPhase() { return 2; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world);
};

struct TestPhase3Populator : public Populator {
    int getPhase() { return 3; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world);
};

struct TestPhase4Populator : public Populator {
    int getPhase() { return 4; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world);
};

struct TestPhase5Populator : public Populator {
    int getPhase() { return 5; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world);
};

struct TestPhase6Populator : public Populator {
    int getPhase() { return 6; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world);
};

struct TestPhase0Populator : public Populator {
    int getPhase() { return 0; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world);
};

struct CavePopulator : public Populator {
    int getPhase() { return 0; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world);
};

struct CobblePopulator : public Populator {
    int getPhase() { return 1; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world);
};

struct OrePopulator : public Populator {
    int getPhase() { return 0; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world);
};

struct TreePopulator : public Populator {
    int getPhase() { return 1; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world);
};

#pragma endregion Populators

#pragma region Rigidbody

class Item;

class RigidBody {
public:
    std::string name;

    b2Body *body = nullptr;
    C_Surface *surface = nullptr;
    R_Image *texture = nullptr;

    int matWidth = 0;
    int matHeight = 0;
    MaterialInstance *tiles = nullptr;

    // hitbox needs update
    bool needsUpdate = false;

    // surface needs to be converted to texture
    bool texNeedsUpdate = false;

    int weldX = -1;
    int weldY = -1;
    bool back = false;
    std::list<TPPLPoly> outline;
    std::list<TPPLPoly> outline2;
    f32 hover = 0;

    Item *item = nullptr;

    RigidBody(b2Body *body, std::string name = "unknown");
    ~RigidBody();
};

template <>
struct ME::meta::static_refl::TypeInfo<RigidBody> : TypeInfoBase<RigidBody> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("matWidth"), &Type::matWidth},
            Field{TSTR("matHeight"), &Type::matHeight},
            Field{TSTR("needsUpdate"), &Type::needsUpdate},
            Field{TSTR("texNeedsUpdate"), &Type::texNeedsUpdate},
            Field{TSTR("weldX"), &Type::weldX},
            Field{TSTR("weldY"), &Type::weldY},
            Field{TSTR("back"), &Type::back},
            Field{TSTR("hover"), &Type::hover},
            // Field{TSTR("item"), &Type::item},
    };
};

ME_GUI_DEFINE_BEGIN(template <>, RigidBody)
ImGui::Text("RigidBody: %s", var.name.c_str());
ImGui::Text("matWidth: %d", var.matWidth);
ImGui::Text("matHeight: %d", var.matHeight);
ImGui::Text("weldX: %d", var.weldX);
ImGui::Text("weldY: %d", var.weldY);
ImGui::Text("needsUpdate: %s", BOOL_STRING(var.needsUpdate));
ImGui::Text("texNeedsUpdate: %s", BOOL_STRING(var.texNeedsUpdate));
ME_GUI_DEFINE_END

using RigidBodyPtr = ME::ref<RigidBody>;

struct RigidBodyBinding : public LuaWrapper::PodBind::Binding<RigidBodyBinding, RigidBody> {
    static constexpr const char *class_name = "RigidBody";

    static luaL_Reg *members() {
        static luaL_Reg members[] = {{nullptr, nullptr}};
        return members;
    }

    static LuaWrapper::PodBind::bind_properties *properties() {
        static LuaWrapper::PodBind::bind_properties properties[] = {{"name", get_name, set_name}, {nullptr, nullptr, nullptr}};
        return properties;
    }

    // Lua constructor
    static int create(lua_State *L) {
        std::cout << "Create called\n";
        LuaWrapper::PodBind::CheckArgCount(L, 2);
        b2Body *body = (b2Body *)lua_touserdata(L, 1);
        const char *name = luaL_checkstring(L, 2);
        RigidBodyPtr sp = ME::create_ref<RigidBody>(body, name);
        push(L, sp);
        return 1;
    }

    // Method glue functions
    //

    // static int walk(lua_State *L) {
    //     LuaWrapper::PodBind::CheckArgCount(L, 1);
    //     RigidBodyPtr a = fromStack(L, 1);
    //     a->walk();
    //     return 0;
    // }

    // static int setName(lua_State *L) {
    //     LuaWrapper::PodBind::CheckArgCount(L, 2);
    //     RigidBodyPtr a = fromStack(L, 1);
    //     const char *name = lua_tostring(L, 2);
    //     a->setName(name);
    //     return 0;
    // }

    // Propertie getters and setters

    // 1 - class metatable
    // 2 - key
    static int get_name(lua_State *L) {
        LuaWrapper::PodBind::CheckArgCount(L, 2);
        RigidBodyPtr a = fromStack(L, 1);
        lua_pushstring(L, a->name.c_str());
        return 1;
    }

    // 1 - class metatable
    // 2 - key
    // 3 - value
    static int set_name(lua_State *L) {
        LuaWrapper::PodBind::CheckArgCount(L, 3);
        RigidBodyPtr a = fromStack(L, 1);
        a->name = lua_tostring(L, 3);
        return 0;
    }
};

#pragma endregion Rigidbody

#pragma region Player

struct Controlable {};

class Player {
public:
    Item *heldItem = nullptr;
    f32 holdAngle = 0;
    i64 startThrow = 0;
    EnumPlayerHoldType holdtype = None;
    int hammerX = 0;
    int hammerY = 0;

    void render(WorldEntity *we, R_Target *target, int ofsX, int ofsY);
    void renderLQ(WorldEntity *we, R_Target *target, int ofsX, int ofsY);
    void setItemInHand(WorldEntity *we, Item *item, World *world);

    Player(const Player &p) = default;

    Player();
    ~Player();
};

template <>
struct ME::meta::static_refl::TypeInfo<Player> : TypeInfoBase<Player> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("heldItem"), &Type::heldItem},
            Field{TSTR("holdAngle"), &Type::holdAngle},
            Field{TSTR("startThrow"), &Type::startThrow},
            Field{TSTR("holdtype"), &Type::holdtype},
            Field{TSTR("hammerX"), &Type::hammerX},
            Field{TSTR("hammerY"), &Type::hammerY},

            Field{TSTR("render"), static_cast<void (Type::*)(WorldEntity *we, R_Target *target, int ofsX, int ofsY) /* const */>(&Type::render)},
            Field{TSTR("renderLQ"), static_cast<void (Type::*)(WorldEntity *we, R_Target *target, int ofsX, int ofsY) /* const */>(&Type::renderLQ)},
            Field{TSTR("setItemInHand"), static_cast<void (Type::*)(WorldEntity *we, Item *item, World *world) /* const */>(&Type::setItemInHand)},
    };
};

ME_GUI_DEFINE_BEGIN(template <>, Player)
ME::meta::static_refl::TypeInfo<Player>::ForEachVarOf(var, [&](const auto &field, auto &&value) { ImGui::Auto(value, std::string(field.name)); });
ME_GUI_DEFINE_END

struct move_player_event {
    f32 dt;
    f32 thruTick;
    Game *g;
};

struct entity_update_event {
    Game *g;
};

class ControableSystem : public MetaEngine::ECS::system<move_player_event> {
public:
    void process(MetaEngine::ECS::registry &world, const move_player_event &evt) override;
};

class WorldEntitySystem : public MetaEngine::ECS::system<entity_update_event> {
public:
    void process(MetaEngine::ECS::registry &world, const entity_update_event &evt) override;
};

#pragma endregion Player

#endif
