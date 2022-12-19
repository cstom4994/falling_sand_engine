// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_MATERIALS_HPP_
#define _METADOT_MATERIALS_HPP_

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/core.hpp"
#include "engine/sdl_wrapper.h"

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

#endif
