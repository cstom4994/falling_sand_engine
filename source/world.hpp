// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_WORLD_HPP_
#define _METADOT_WORLD_HPP_

#include <deque>
#include <future>
#include <unordered_map>
#include <vector>

#include "Noise.h"
#include "audio/audio.h"
#include "chunk.hpp"
#include "core/const.h"
#include "core/cpp/vector.hpp"
#include "core/macros.h"
#include "core/threadpool.hpp"
#include "ecs/ecs.hpp"
#include "game_basic.hpp"
#include "game_datastruct.hpp"
#include "physics/box2d.h"
#include "libs/parallel_hashmap/phmap.h"
#include "renderer/renderer_gpu.h"

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

    static WorldMeta loadWorldMeta(std::string worldFileName);
    bool save(std::string worldFileName);
};
METADOT_STRUCT(WorldMeta, worldName, lastOpenedVersion, lastOpenedTime);

class World {
public:
    std::string worldName = "";
    WorldMeta metadata{};
    bool noSaveLoad = false;

    R_Target *target = nullptr;

    ~World();

    struct {
        MetaEngine::vector<CellData *> cells;
        MetaEngine::vector<RigidBody *> rigidBodies;
        MetaEngine::vector<RigidBody *> worldRigidBodies;
        MetaEngine::vector<MetaEngine::vector<vec2>> worldMeshes;
        MetaEngine::vector<MetaEngine::vector<vec2>> worldTris;

        MetaEngine::vector<LoadChunkParams> toLoad;
        MetaEngine::vector<std::future<Chunk *>> readyToReadyToMerge;

        MetaEngine::vector<Chunk *> readyToMerge;  // deque, but MetaEngine::vector should work

        MetaEngine::vector<PlacedStructure> structures;
        MetaEngine::vector<vec2> distributedPoints;
        phmap::flat_hash_map<int, phmap::flat_hash_map<int, Chunk *>> chunkCache;
        MetaEngine::vector<Populator *> populators;

        MetaEngine::ECS::entity_id player;
    };

    struct {
        MetaEngine::ECS::registry registry;
    };

    MetaEngine::ECS::registry &Reg() { return registry; }

    bool *hasPopulator = nullptr;
    int highestPopulator = 0;
    FastNoise noise;
    Audio *audioEngine = nullptr;

    MaterialInstance *tiles = nullptr;
    F32 *flowX = nullptr;
    F32 *flowY = nullptr;
    F32 *prevFlowX = nullptr;
    F32 *prevFlowY = nullptr;
    MaterialInstance *layer2 = nullptr;
    U32 *background = nullptr;
    U16 width = 0;
    U16 height = 0;
    int tickCt = 0;
    static ThreadPool *tickPool;
    static ThreadPool *tickVisitedPool;
    static ThreadPool *updateRigidBodyHitboxPool;
    static ThreadPool *loadChunkPool;

    R_Image *fireTex = nullptr;
    bool *tickVisited1 = nullptr;
    bool *tickVisited2 = nullptr;
    I32 *newTemps = nullptr;
    bool needToTickGeneration = false;

    bool *dirty = nullptr;
    bool *active = nullptr;
    bool *lastActive = nullptr;
    bool *layer2Dirty = nullptr;
    bool *backgroundDirty = nullptr;
    C_Rect loadZone{};
    C_Rect lastLoadZone{};
    C_Rect tickZone{};
    C_Rect meshZone{};
    C_Rect lastMeshZone{};
    C_Rect lastMeshLoadZone{};

    vec2 gravity{};
    b2World *b2world = nullptr;
    RigidBody *staticBody = nullptr;

    void init(std::string worldPath, U16 w, U16 h, R_Target *renderer, Audio *audioEngine, WorldGenerator *generator);
    void init(std::string worldPath, U16 w, U16 h, R_Target *target, Audio *audioEngine);
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
    RigidBody *makeRigidBody(b2BodyType type, F32 x, F32 y, F32 angle, b2PolygonShape shape, F32 density, F32 friction, C_Surface *texture);
    RigidBody *makeRigidBodyMulti(b2BodyType type, F32 x, F32 y, F32 angle, MetaEngine::vector<b2PolygonShape> shape, F32 density, F32 friction, C_Surface *texture);
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
    vec2 getNearestPoint(F32 x, F32 y);
    MetaEngine::vector<vec2> getPointsWithin(F32 x, F32 y, F32 w, F32 h);
    Chunk *getChunk(int cx, int cy);
    void populateChunk(Chunk *ch, int phase, bool render);
    void tickEntities(R_Target *target);
    void forLine(int x0, int y0, int x1, int y1, std::function<bool(int)> fn);
    void forLineCornered(int x0, int y0, int x1, int y1, std::function<bool(int)> fn);
    RigidBody *physicsCheck(int x, int y);
    void physicsCheck_flood(int x, int y, bool *visited, int *count, U32 *cols, int *minX, int *maxX, int *minY, int *maxY);
    void saveWorld();
    bool isC2Ground(F32 x, F32 y);
    bool isPlayerInWorld();
    std::tuple<WorldEntity *, Player *> getHostPlayer();
};

#endif
