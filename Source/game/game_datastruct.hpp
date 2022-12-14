// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GAMEDATASTRUCT_HPP_
#define _METADOT_GAMEDATASTRUCT_HPP_

#include "structures.hpp"
#include "core/core.hpp"
#include "engine/entity_components/worldentity.hpp"
#include "materials.hpp"

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

struct Settings
{
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

    int networkMode;
    std::string server_ip;
    int server_port;

    void Init(bool openDebugUIs);
    void Load(std::string setting_file);
    void Save(std::string setting_file);
};
METADOT_STRUCT(Settings, draw_frame_graph, draw_background, draw_background_grid, draw_load_zones,
               draw_physics_debug, draw_b2d_shape, draw_b2d_joint, draw_b2d_aabb, draw_b2d_pair,
               draw_b2d_centerMass, draw_chunk_state, draw_debug_stats, draw_material_info,
               draw_detailed_material_info, draw_uinode_bounds, draw_temperature_map, draw_cursor,
               ui_tweak, draw_shaders, water_overlay, water_showFlow, water_pixelated,
               lightingQuality, draw_light_overlay, simpleLighting, lightingEmission,
               lightingDithering, tick_world, tick_box2d, tick_temperature, hd_objects,
               hd_objects_size);

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