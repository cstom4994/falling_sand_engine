// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GAMEDATASTRUCT_HPP_
#define _METADOT_GAMEDATASTRUCT_HPP_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/core.hpp"
#include "engine/internal/builtin_box2d.h"
#include "engine/math.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "engine/scripting/lua_wrapper.hpp"
#include "engine/sdl_wrapper.h"

struct Chunk;
struct Populator;
struct World;
struct RigidBody;
struct b2Body;
struct R_Target;
struct Particle;

struct WorldEntity {
    float x = 0;
    float y = 0;
    float vx = 0;
    float vy = 0;
    int hw = 14;
    int hh = 26;
    bool ground = false;
    RigidBody *rb = nullptr;
    b2Body *body = nullptr;
    bool is_player = false;

    virtual void render(R_Target *target, int ofsX, int ofsY);
    virtual void renderLQ(R_Target *target, int ofsX, int ofsY);
    WorldEntity(bool isplayer);
    ~WorldEntity();
};

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

class Material {
public:
    std::string name;
    int id = 0;
    int physicsType = 0;
    U8 alpha = 0;
    float density = 0;
    int iterations = 0;
    int emit = 0;
    U32 emitColor = 0;
    U32 color = 0;
    U32 addTemp = 0;
    float conductionSelf = 1.0;
    float conductionOther = 1.0;

    bool interact = false;
    int *nInteractions = nullptr;
    std::vector<MaterialInteraction> *interactions = nullptr;
    bool react = false;
    int nReactions = 0;
    std::vector<MaterialInteraction> reactions;

    int slipperyness = 1;

    Material(int id, std::string name, int physicsType, int slipperyness, U8 alpha, float density, int iterations, int emit, U32 emitColor, U32 color);
    Material(int id, std::string name, int physicsType, int slipperyness, U8 alpha, float density, int iterations, int emit, U32 emitColor)
        : Material(id, name, physicsType, slipperyness, alpha, density, iterations, emit, emitColor, 0xffffffff){};
    Material(int id, std::string name, int physicsType, int slipperyness, U8 alpha, float density, int iterations) : Material(id, name, physicsType, slipperyness, alpha, density, iterations, 0, 0){};
    Material(int id, std::string name, int physicsType, int slipperyness, float density, int iterations) : Material(id, name, physicsType, slipperyness, 0xff, density, iterations){};
    Material() : Material(0, "Air", PhysicsType::AIR, 4, 0, 0){};
};

class Materials {
public:
    static int nMaterials;
    static std::vector<Material *> MATERIALS;
    static Material **MATERIALS_ARRAY;

    static Material GENERIC_AIR;
    static Material GENERIC_SOLID;
    static Material GENERIC_SAND;
    static Material GENERIC_LIQUID;
    static Material GENERIC_GAS;
    static Material GENERIC_PASSABLE;
    static Material GENERIC_OBJECT;
    static Material TEST_SAND;
    static Material TEST_TEXTURED_SAND;
    static Material TEST_LIQUID;
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

    static void Init();
};

class MaterialInstance {
public:
    static int _curID;

    Material *mat;
    U32 color;
    int32_t temperature;
    uint32_t id = 0;
    bool moved = false;
    float fluidAmount = 2.0f;
    float fluidAmountDiff = 0.0f;
    uint8_t settleCount = 0;

    MaterialInstance(Material *mat, U32 color, int32_t temperature);
    MaterialInstance(Material *mat, U32 color) : MaterialInstance(mat, color, 0){};
    MaterialInstance() : MaterialInstance(&Materials::GENERIC_AIR, 0x000000, 0){};
    bool operator==(const MaterialInstance &other);
};

class Tiles {
public:
    static const MaterialInstance NOTHING;
    static const MaterialInstance TEST_SOLID;
    static const MaterialInstance TEST_SAND;
    static const MaterialInstance TEST_LIQUID;
    static const MaterialInstance TEST_GAS;
    static const MaterialInstance OBJECT;

    static MaterialInstance createTestSand();
    static MaterialInstance createTestTexturedSand(int x, int y);
    static MaterialInstance createTestLiquid();

    static MaterialInstance createStone(int x, int y);
    static MaterialInstance createGrass();
    static MaterialInstance createDirt();

    static MaterialInstance createSmoothStone(int x, int y);
    static MaterialInstance createCobbleStone(int x, int y);
    static MaterialInstance createSmoothDirt(int x, int y);
    static MaterialInstance createCobbleDirt(int x, int y);

    static MaterialInstance createSoftDirt(int x, int y);

    static MaterialInstance createWater();
    static MaterialInstance createLava();

    static MaterialInstance createCloud(int x, int y);
    static MaterialInstance createGold(int x, int y);
    static MaterialInstance createIron(int x, int y);

    static MaterialInstance createObsidian(int x, int y);

    static MaterialInstance createSteam();

    static MaterialInstance createFire();

    static MaterialInstance create(Material *mat, int x, int y);
};

#pragma endregion Material

class ItemFlags {
public:
    static const U8 RIGIDBODY = 0b00000001;
    static const U8 FLUID_CONTAINER = 0b00000010;
    static const U8 TOOL = 0b00000100;
    static const U8 CHISEL = 0b00001000;
    static const U8 HAMMER = 0b00010000;
    static const U8 VACUUM = 0b00100000;
};

class Item {
public:
    U8 flags = 0;

    void setFlag(U8 f) { flags |= f; }

    bool getFlag(U8 f) { return flags & f; }

    C_Surface *surface = nullptr;
    R_Image *texture = nullptr;
    int pivotX = 0;
    int pivotY = 0;
    float breakSize = 16;
    std::vector<MaterialInstance> carry;
    std::vector<U16Point> fill;
    uint16_t capacity = 0;

    std::vector<Particle *> vacuumParticles;

    Item();
    ~Item();

    static Item *makeItem(U8 flags, RigidBody *rb);
    void loadFillTexture(C_Surface *tex);
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

struct EntityComponents {};

struct Biome {
    int id;
    std::string name;
    explicit Biome(std::string name, int id) : name(std::move(name)), id(std::move(id)){};
};

struct WorldGenerator {
    virtual void generateChunk(World *world, Chunk *ch) = 0;
    virtual std::vector<Populator *> getPopulators() = 0;
};

struct Particle {
    MaterialInstance tile{};
    float x = 0;
    float y = 0;
    float vx = 0;
    float vy = 0;
    float ax = 0;
    float ay = 0;
    float targetX = 0;
    float targetY = 0;
    float targetForce = 0;
    bool phase = false;
    bool temporary = false;
    int lifetime = 0;
    int fadeTime = 60;
    unsigned short inObjectState = 0;
    std::function<void()> killCallback = []() {};
    explicit Particle(MaterialInstance tile, float x, float y, float vx, float vy, float ax, float ay) : tile(std::move(tile)), x(x), y(y), vx(vx), vy(vy), ax(ax), ay(ay) {}
    Particle(const Particle &part) : tile(part.tile), x(part.x), y(part.y), vx(part.vx), vy(part.vy), ax(part.ax), ay(part.ay) {}
};

struct GlobalDEF {
    bool draw_frame_graph;
    bool draw_background;
    bool draw_background_grid;
    bool draw_load_zones;
    bool draw_physics_debug;
    bool draw_b2d_shape;
    bool draw_b2d_joint;
    bool draw_b2d_aabb;
    bool draw_b2d_pair;
    bool draw_b2d_centerMass;
    bool draw_chunk_state;
    bool draw_debug_stats;
    bool draw_material_info;
    bool draw_detailed_material_info;
    bool draw_uinode_bounds;
    bool draw_temperature_map;
    bool draw_cursor;

    bool ui_tweak;

    bool draw_shaders;
    int water_overlay;
    bool water_showFlow;
    bool water_pixelated;
    float lightingQuality;
    bool draw_light_overlay;
    bool simpleLighting;
    bool lightingEmission;
    bool lightingDithering;

    bool tick_world;
    bool tick_box2d;
    bool tick_temperature;
    bool hd_objects;

    int hd_objects_size;
};
METADOT_STRUCT(GlobalDEF, draw_frame_graph, draw_background, draw_background_grid, draw_load_zones, draw_physics_debug, draw_b2d_shape, draw_b2d_joint, draw_b2d_aabb, draw_b2d_pair,
               draw_b2d_centerMass, draw_chunk_state, draw_debug_stats, draw_material_info, draw_detailed_material_info, draw_uinode_bounds, draw_temperature_map, draw_cursor, ui_tweak, draw_shaders,
               water_overlay, water_showFlow, water_pixelated, lightingQuality, draw_light_overlay, simpleLighting, lightingEmission, lightingDithering, tick_world, tick_box2d, tick_temperature,
               hd_objects, hd_objects_size);

void InitGlobalDEF(GlobalDEF *_struct, bool openDebugUIs);
void LoadGlobalDEF(std::string globaldef_src);
void SaveGlobalDEF(std::string globaldef_src);

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
    float hover = 0;

    Item *item = nullptr;

    RigidBody(b2Body *body);
    ~RigidBody();
};

#pragma endregion Rigidbody

#pragma region Player

class Player : public WorldEntity {
public:
    Item *heldItem = nullptr;
    float holdAngle = 0;
    long long startThrow = 0;
    bool holdHammer = false;
    bool holdVacuum = false;
    int hammerX = 0;
    int hammerY = 0;

    void render(R_Target *target, int ofsX, int ofsY) override;
    void renderLQ(R_Target *target, int ofsX, int ofsY) override;
    void setItemInHand(Item *item, World *world);

    Player();
    ~Player();
};

#pragma endregion Player

#endif