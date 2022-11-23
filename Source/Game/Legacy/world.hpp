// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_WORLD_HPP_
#define _METADOT_WORLD_HPP_


#include "Chunk.hpp"

#define INC_World

#include "Game/Macros.hpp"

#include "Networking.hpp"
#include "Particle.hpp"
#include <box2d/b2_math.h>
#include <box2d/b2_world.h>
#include <deque>
#include <vector>
#ifndef INC_RigidBody
#include "RigidBody.hpp"
#endif
#include "Libs/FastNoise/FastNoise.h"
#include "Libs/sparsehash/dense_hash_map.h"
#include "Structures.hpp"
#include <future>
#include <unordered_map>
#ifndef INC_Player
#include "Player.hpp"
#endif
#include "Engine/AudioEngine/AudioEngine.h"

#include "Libs/ctpl_stl.h"
#include "Libs/veque.hpp"

#include "Engine/Render/renderer_gpu.h"

#ifndef INC_Biome
#include "Biome.hpp"
#endif// !INC_Biome


#ifndef INC_WorldGenerator
#include "WorldGenerator.hpp"
#endif

#include "Game/Const.hpp"

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

class WorldMeta {
public:
    std::string worldName;
    std::string lastOpenedVersion;
    int64_t lastOpenedTime = 0;

    static WorldMeta loadWorldMeta(std::string worldFileName);
    bool save(std::string worldFileName);
};


class World {
public:
    std::string worldName = "";
    WorldMeta metadata{};
    bool noSaveLoad = false;

    METAENGINE_Render_Target *target = nullptr;

    MaterialInstance *tiles = nullptr;
    float *flowX = nullptr;
    float *flowY = nullptr;
    float *prevFlowX = nullptr;
    float *prevFlowY = nullptr;
    MaterialInstance *layer2 = nullptr;
    UInt32 *background = nullptr;
    veque::veque<Particle *> particles;
    uint16_t width = 0;
    uint16_t height = 0;
    void init(std::string worldPath, uint16_t w, uint16_t h, METAENGINE_Render_Target *renderer, CAudioEngine *audioEngine, int netMode, WorldGenerator *generator);
    void init(std::string worldPath, uint16_t w, uint16_t h, METAENGINE_Render_Target *renderer, CAudioEngine *audioEngine, int netMode);
    MaterialInstance getTile(int x, int y);
    void setTile(int x, int y, MaterialInstance type);
    MaterialInstance getTileLayer2(int x, int y);
    void setTileLayer2(int x, int y, MaterialInstance type);
    int tickCt = 0;
    static ctpl::thread_pool *tickPool;
    static ctpl::thread_pool *tickVisitedPool;
    static ctpl::thread_pool *updateRigidBodyHitboxPool;
    static ctpl::thread_pool *loadChunkPool;

    METAENGINE_Render_Image *fireTex = nullptr;
    bool *tickVisited1 = nullptr;
    bool *tickVisited2 = nullptr;

    void tick();

    void tickTemperature();
    int32_t *newTemps = nullptr;
    void frame();
    void tickParticles();
    void renderParticles(unsigned char **texture);
    void tickObjectBounds();
    void tickObjects();
    void tickObjectsMesh();
    void tickChunks();
    void tickChunkGeneration();
    bool needToTickGeneration = false;
    void addParticle(Particle *particle);
    void explosion(int x, int y, int radius);
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
    std::vector<RigidBody *> rigidBodies;
    RigidBody *staticBody = nullptr;

    RigidBody *makeRigidBody(b2BodyType type, float x, float y, float angle, b2PolygonShape shape, float density, float friction, C_Surface *texture);
    RigidBody *makeRigidBodyMulti(b2BodyType type, float x, float y, float angle, std::vector<b2PolygonShape> shape, float density, float friction, C_Surface *texture);
    void updateRigidBodyHitbox(RigidBody *rb);

    std::vector<veque::veque<b2Vec2>> worldMeshes;
    std::vector<veque::veque<b2Vec2>> worldTris;
    void updateChunkMesh(Chunk *chunk);
    void updateWorldMesh();
    std::vector<RigidBody *> worldRigidBodies;

    std::vector<LoadChunkParams> toLoad;
    std::vector<std::future<Chunk *>> readyToReadyToMerge;
    std::deque<Chunk *> readyToMerge;
    void queueLoadChunk(int cx, int cy, bool populate, bool render);
    Chunk *loadChunk(Chunk *, bool populate, bool render);
    void unloadChunk(Chunk *ch);
    void writeChunkToDisk(Chunk *ch);
    void chunkSaveCache(Chunk *ch);
    WorldGenerator *gen = nullptr;
    void generateChunk(Chunk *ch);
    Biome *getBiomeAt(int x, int y);
    Biome *getBiomeAt(Chunk *ch, int x, int y);

    FastNoise noise;

    std::vector<PlacedStructure> structures;
    void addStructure(PlacedStructure str);

    veque::veque<b2Vec2> distributedPoints;
    b2Vec2 getNearestPoint(float x, float y);
    veque::veque<b2Vec2> getPointsWithin(float x, float y, float w, float h);

    Chunk *getChunk(int cx, int cy);
    google::dense_hash_map<int, google::dense_hash_map<int, Chunk *>> chunkCache;
    //std::unordered_map<int, std::unordered_map<int, Chunk*>> chunkCache;

    std::vector<Populator *> populators;
    bool *hasPopulator = nullptr;
    int highestPopulator = 0;
    void populateChunk(Chunk *ch, int phase, bool render);

    std::vector<Entity *> entities;
    Player *player = nullptr;
    void tickEntities(METAENGINE_Render_Target *target);

    CAudioEngine *audioEngine = nullptr;

    void forLine(int x0, int y0, int x1, int y1, std::function<bool(int)> fn);
    void forLineCornered(int x0, int y0, int x1, int y1, std::function<bool(int)> fn);

    RigidBody *physicsCheck(int x, int y);
    void physicsCheck_flood(int x, int y, bool *visited, int *count, uint32 *cols, int *minX, int *maxX, int *minY, int *maxY);

    ~World();
};

#endif