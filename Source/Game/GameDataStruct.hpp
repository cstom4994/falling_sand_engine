// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GAMEDATASTRUCT_HPP_
#define _METADOT_GAMEDATASTRUCT_HPP_

#include "Engine/Internal/BuiltinBox2d.h"
#include "Engine/RendererGPU.h"
#include "Materials.hpp"
#include "Structures.hpp"

#include <functional>
#include <string>

struct Chunk;
struct Populator;
struct RigidBody;
struct World;

struct Biome
{
    int id;
    std::string name;
    explicit Biome(std::string name, int id) : name(std::move(name)), id(std::move(id)){};
};

struct WorldGenerator
{
    virtual void generateChunk(World *world, Chunk *ch) = 0;
    virtual std::vector<Populator *> getPopulators() = 0;
};

struct Particle
{
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
    explicit Particle(MaterialInstance tile, float x, float y, float vx, float vy, float ax,
                      float ay)
        : tile(std::move(tile)), x(x), y(y), vx(vx), vy(vy), ax(ax), ay(ay) {}
    Particle(const Particle &part)
        : tile(part.tile), x(part.x), y(part.y), vx(part.vx), vy(part.vy), ax(part.ax),
          ay(part.ay) {}
};

struct Entity
{
    float x = 0;
    float y = 0;
    float vx = 0;
    float vy = 0;
    int hw = 14;
    int hh = 26;
    bool ground = false;
    RigidBody *rb = nullptr;
    b2Body *body = nullptr;

    virtual void render(METAENGINE_Render_Target *target, int ofsX, int ofsY);
    virtual void renderLQ(METAENGINE_Render_Target *target, int ofsX, int ofsY);
    Entity();
    ~Entity();
};

struct SettingsBase
{
    static bool draw_frame_graph;
    static bool draw_background;
    static bool draw_background_grid;
    static bool draw_load_zones;
    static bool draw_physics_debug;
    static bool draw_b2d_shape;
    static bool draw_b2d_joint;
    static bool draw_b2d_aabb;
    static bool draw_b2d_pair;
    static bool draw_b2d_centerMass;
    static bool draw_chunk_state;
    static bool draw_debug_stats;
    static bool draw_material_info;
    static bool draw_detailed_material_info;
    static bool draw_uinode_bounds;
    static bool draw_temperature_map;
    static bool draw_cursor;

    static bool ui_tweak;
    static bool ui_code_editor;

    static bool draw_shaders;
    static int water_overlay;
    static bool water_showFlow;
    static bool water_pixelated;
    static float lightingQuality;
    static bool draw_light_overlay;
    static bool simpleLighting;
    static bool lightingEmission;
    static bool lightingDithering;

    static bool tick_world;
    static bool tick_box2d;
    static bool tick_temperature;
    static bool hd_objects;

    static int hd_objects_size;

    static int networkMode;
    static std::string server_ip;
    static int server_port;
};

struct Settings : SettingsBase
{
    Settings() {
        // LINK_PROPERTY(ui_tweak, &ui_tweak);
        // LINK_PROPERTY(ui_code_editor, &ui_code_editor);
    }
};

struct Populator
{
    virtual int getPhase() = 0;
    virtual std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                               Chunk **area, bool *dirty, int tx, int ty, int tw,
                                               int th, Chunk *ch, World *world) = 0;
};

#pragma region Populators

struct TestPhase1Populator : public Populator
{
    int getPhase() { return 1; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world);
};

struct TestPhase2Populator : public Populator
{
    int getPhase() { return 2; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world);
};

struct TestPhase3Populator : public Populator
{
    int getPhase() { return 3; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world);
};

struct TestPhase4Populator : public Populator
{
    int getPhase() { return 4; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world);
};

struct TestPhase5Populator : public Populator
{
    int getPhase() { return 5; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world);
};

struct TestPhase6Populator : public Populator
{
    int getPhase() { return 6; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world);
};

struct TestPhase0Populator : public Populator
{
    int getPhase() { return 0; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world);
};

struct CavePopulator : public Populator
{
    int getPhase() { return 0; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk **area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk *ch, World *world);
};

struct CobblePopulator : public Populator
{
    int getPhase() { return 1; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk **area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk *ch, World *world);
};

struct OrePopulator : public Populator
{
    int getPhase() { return 0; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk **area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk *ch, World *world);
};

struct TreePopulator : public Populator
{
    int getPhase() { return 1; }
    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk **area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk *ch, World *world);
};

#pragma endregion Populators

#endif