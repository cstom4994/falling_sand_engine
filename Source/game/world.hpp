// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_WORLD_HPP_
#define _METADOT_WORLD_HPP_

#include <deque>
#include <future>
#include <unordered_map>
#include <vector>

#include "chunk.hpp"
#include "core/const.h"
#include "core/macros.h"
#include "core/threadpool.hpp"
#include "engine/Noise.h"
#include "engine/audio.hpp"
#include "engine/internal/builtin_box2d.h"
#include "engine/renderer/renderer_gpu.h"
#include "game_datastruct.hpp"
#include "game_scriptingwrap.hpp"
#include "libs/sparsehash/dense_hash_map.h"

class Populator;
class WorldGenerator;
class Player;

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
    long lastOpenedTime = 0;

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
        std::vector<Particle *> particles;
        std::vector<RigidBody *> rigidBodies;
        std::vector<std::vector<b2Vec2>> worldMeshes;
        std::vector<std::vector<b2Vec2>> worldTris;
        std::vector<RigidBody *> worldRigidBodies;

        std::vector<LoadChunkParams> toLoad;
        std::vector<std::future<Chunk *>> readyToReadyToMerge;
        std::deque<Chunk *> readyToMerge;

        std::vector<PlacedStructure> structures;
        std::vector<b2Vec2> distributedPoints;
        google::dense_hash_map<int, google::dense_hash_map<int, Chunk *>> chunkCache;
        // std::unordered_map<int, std::unordered_map<int, Chunk*>> chunkCache;
        std::vector<Populator *> populators;
        std::vector<WorldEntity *> entities;
        Player *player = nullptr;
    } WorldIsolate_;

    bool *hasPopulator = nullptr;
    int highestPopulator = 0;
    FastNoise noise;
    Audio *audioEngine = nullptr;

    MaterialInstance *tiles = nullptr;
    float *flowX = nullptr;
    float *flowY = nullptr;
    float *prevFlowX = nullptr;
    float *prevFlowY = nullptr;
    MaterialInstance *layer2 = nullptr;
    U32 *background = nullptr;
    uint16_t width = 0;
    uint16_t height = 0;
    int tickCt = 0;
    static ThreadPool *tickPool;
    static ThreadPool *tickVisitedPool;
    static ThreadPool *updateRigidBodyHitboxPool;
    static ThreadPool *loadChunkPool;

    R_Image *fireTex = nullptr;
    bool *tickVisited1 = nullptr;
    bool *tickVisited2 = nullptr;
    int32_t *newTemps = nullptr;
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

    b2Vec2 gravity{};
    b2World *b2world = nullptr;
    RigidBody *staticBody = nullptr;

    void init(std::string worldPath, uint16_t w, uint16_t h, R_Target *renderer, Audio *audioEngine, WorldGenerator *generator);
    void init(std::string worldPath, uint16_t w, uint16_t h, R_Target *target, Audio *audioEngine);
    MaterialInstance getTile(int x, int y);
    void setTile(int x, int y, MaterialInstance type);
    MaterialInstance getTileLayer2(int x, int y);
    void setTileLayer2(int x, int y, MaterialInstance type);
    void tick();
    void tickTemperature();
    void frame();
    void tickParticles();
    void renderParticles(unsigned char **texture);
    void tickObjectBounds();
    void tickObjects();
    void tickObjectsMesh();
    void tickChunks();
    void tickChunkGeneration();
    void addParticle(Particle *particle);
    void explosion(int x, int y, int radius);
    RigidBody *makeRigidBody(b2BodyType type, float x, float y, float angle, b2PolygonShape shape, float density, float friction, C_Surface *texture);
    RigidBody *makeRigidBodyMulti(b2BodyType type, float x, float y, float angle, std::vector<b2PolygonShape> shape, float density, float friction, C_Surface *texture);
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
    b2Vec2 getNearestPoint(float x, float y);
    std::vector<b2Vec2> getPointsWithin(float x, float y, float w, float h);
    Chunk *getChunk(int cx, int cy);
    void populateChunk(Chunk *ch, int phase, bool render);
    void tickEntities(R_Target *target);
    void forLine(int x0, int y0, int x1, int y1, std::function<bool(int)> fn);
    void forLineCornered(int x0, int y0, int x1, int y1, std::function<bool(int)> fn);
    RigidBody *physicsCheck(int x, int y);
    void physicsCheck_flood(int x, int y, bool *visited, int *count, U32 *cols, int *minX, int *maxX, int *minY, int *maxY);
    void saveWorld();
};

#endif
