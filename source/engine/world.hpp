// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_WORLD_HPP
#define ME_WORLD_HPP

#include <deque>
#include <future>
#include <unordered_map>
#include <vector>

#include "chunk.hpp"
#include "engine/audio/audio.h"
#include "engine/core/const.h"
#include "engine/core/macros.hpp"
#include "engine/ecs/ecs.hpp"
#include "engine/physics/box2d.h"
#include "engine/utils/utility.hpp"
#include "game/player.hpp"
#include "game_basic.hpp"
#include "game_datastruct.hpp"
#include "libs/fastnoise/fastnoise.h"
#include "libs/parallel_hashmap/phmap.h"

class Populator;
class WorldGenerator;
class Player;
struct CellData;

class LoadChunkParams {
public:
    int x;
    int y;
    bool populate;
    long long time;
    LoadChunkParams(int x, int y, bool populate, long long time) {
        this->x = x;
        this->y = y;
        this->populate = populate;
        this->time = time;
    }
};

struct WorldMeta {
    std::string worldName;
    std::string lastOpenedVersion;
    time_t lastOpenedTime = 0;

    static WorldMeta loadWorldMeta(std::string worldFileName, bool noSaveLoad = false);

    bool save(std::string worldFileName);
};
METADOT_STRUCT(WorldMeta, worldName, lastOpenedVersion, lastOpenedTime);

class WorldSystem {
public:
    static ME::thread_pool tickPool;
    static ME::thread_pool tickVisitedPool;
    static ME::thread_pool updateRigidBodyHitboxPool;
};

class World {
public:
    std::string worldName = "";
    WorldMeta metadata{};
    bool noSaveLoad = false;

    R_Target *target = nullptr;

    ~World();

    struct {
        std::vector<CellData *> cells;
        std::vector<RigidBody *> rigidBodies;
        std::vector<RigidBody *> worldRigidBodies;
        std::vector<std::vector<MEvec2>> worldMeshes;
        std::vector<std::vector<MEvec2>> worldTris;

        std::vector<LoadChunkParams> toLoad;
        std::vector<std::future<Chunk *>> readyToReadyToMerge;

        std::deque<Chunk *> readyToMerge;  // deque, but std::vector should work

        std::vector<PlacedStructure> structures;
        std::vector<MEvec2> distributedPoints;
        phmap::flat_hash_map<int, phmap::flat_hash_map<int, Chunk *>> chunkCache;
        std::vector<Populator *> populators;

        ME::ECS::entity_id player;
    };

    struct {
        ME::ECS::registry registry;
    };

    ME::ECS::registry &Reg() { return registry; }

    bool *hasPopulator = nullptr;
    int highestPopulator = 0;
    FastNoise noise;
    ME::Audio *audioEngine = nullptr;

    MaterialInstance *tiles = nullptr;
    f32 *flowX = nullptr;
    f32 *flowY = nullptr;
    f32 *prevFlowX = nullptr;
    f32 *prevFlowY = nullptr;
    MaterialInstance *layer2 = nullptr;
    u32 *background = nullptr;
    u16 width = 0;
    u16 height = 0;
    int tickCt = 0;

    R_Image *fireTex = nullptr;
    bool *tickVisited1 = nullptr;
    bool *tickVisited2 = nullptr;
    i32 *newTemps = nullptr;
    bool needToTickGeneration = false;

    bool *dirty = nullptr;
    bool *active = nullptr;
    bool *lastActive = nullptr;
    bool *layer2Dirty = nullptr;
    bool *backgroundDirty = nullptr;
    ME_rect loadZone;
    ME_rect lastLoadZone{};
    ME_rect tickZone{};
    ME_rect meshZone{};
    ME_rect lastMeshZone{};
    ME_rect lastMeshLoadZone{};

    MEvec2 gravity{};
    b2World *b2world = nullptr;
    RigidBody *staticBody = nullptr;

    void init(std::string worldPath, u16 w, u16 h, R_Target *renderer, ME::Audio *audioEngine, WorldGenerator *generator);
    void init(std::string worldPath, u16 w, u16 h, R_Target *target, ME::Audio *audioEngine);
    MaterialInstance getTile(int x, int y);
    void setTile(int x, int y, MaterialInstance type);
    MaterialInstance getTileLayer2(int x, int y);
    void setTileLayer2(int x, int y, MaterialInstance type);
    void tick();
    void tickTemperature();
    void frame();
    void tickCells();
    void renderCells(unsigned char **texture);
    void tickObjectBounds();
    void tickObjects();
    void tickObjectsMesh();
    void tickChunks();
    void tickChunkGeneration();
    void addCell(CellData *cell);
    void explosion(int x, int y, int radius);
    RigidBody *makeRigidBody(b2BodyType type, f32 x, f32 y, f32 angle, b2PolygonShape shape, f32 density, f32 friction, C_Surface *texture);
    RigidBody *makeRigidBodyMulti(b2BodyType type, f32 x, f32 y, f32 angle, std::vector<b2PolygonShape> shape, f32 density, f32 friction, C_Surface *texture);
    void updateRigidBodyHitbox(RigidBody *rb);
    void updateChunkMesh(Chunk *chunk);
    void updateWorldMesh();
    void queueLoadChunk(int cx, int cy, bool populate, bool render);
    Chunk *loadChunk(Chunk *, bool populate, bool render);
    void unloadChunk(Chunk *ch);
    void writeChunkToDisk(Chunk *ch);
    void chunkSaveCache(Chunk *ch);
    WorldGenerator *gen = nullptr;
    void generateChunk(Chunk *ch);
    Biome *getBiomeAt(int x, int y);
    Biome *getBiomeAt(Chunk *ch, int x, int y);
    void addStructure(PlacedStructure str);
    MEvec2 getNearestPoint(f32 x, f32 y);
    std::vector<MEvec2> getPointsWithin(f32 x, f32 y, f32 w, f32 h);
    Chunk *getChunk(int cx, int cy);
    void populateChunk(Chunk *ch, int phase, bool render);
    void tickEntities(R_Target *target);
    void forLine(int x0, int y0, int x1, int y1, std::function<bool(int)> fn);
    void forLineCornered(int x0, int y0, int x1, int y1, std::function<bool(int)> fn);
    RigidBody *physicsCheck(int x, int y);
    void physicsCheck_flood(int x, int y, bool *visited, int *count, u32 *cols, int *minX, int *maxX, int *minY, int *maxY);
    void saveWorld();
    bool isC2Ground(f32 x, f32 y);
    bool isPlayerInWorld();
    std::tuple<WorldEntity *, Player *> getHostPlayer();
};

#endif
