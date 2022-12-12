// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Materials.hpp"
#include "Core/Macros.h"
#include "Game/GameResources.hpp"
#include "Engine/Renderer/RendererUtils.h"

Material::Material(int id, std::string name, int physicsType, int slipperyness, UInt8 alpha,
                   float density, int iterations, int emit, UInt32 emitColor, UInt32 color) {
    this->name = name;
    this->id = id;
    this->physicsType = physicsType;
    this->slipperyness = slipperyness;
    this->alpha = alpha;
    this->density = density;
    this->iterations = iterations;
    this->emit = emit;
    this->emitColor = emitColor;
    this->color = color;
}

int Materials::nMaterials = 0;
Material Materials::GENERIC_AIR =
        Material(nMaterials++, "_AIR", PhysicsType::AIR, 0, 255, 0, 0, 16, 0);
Material Materials::GENERIC_SOLID =
        Material(nMaterials++, "_SOLID", PhysicsType::SOLID, 0, 255, 1, 0, 0, 0);
Material Materials::GENERIC_SAND =
        Material(nMaterials++, "_SAND", PhysicsType::SAND, 20, 255, 10, 2, 0, 0);
Material Materials::GENERIC_LIQUID =
        Material(nMaterials++, "_LIQUID", PhysicsType::SOUP, 0, 255, 1.5, 3, 0, 0);
Material Materials::GENERIC_GAS =
        Material(nMaterials++, "_GAS", PhysicsType::GAS, 0, 255, -1, 1, 0, 0);
Material Materials::GENERIC_PASSABLE =
        Material(nMaterials++, "_PASSABLE", PhysicsType::PASSABLE, 0, 255, 0, 0, 0, 0);
Material Materials::GENERIC_OBJECT =
        Material(nMaterials++, "_OBJECT", PhysicsType::OBJECT, 0, 255, 1000.0, 0, 0, 0);

Material Materials::TEST_SAND =
        Material(nMaterials++, "Test Sand", PhysicsType::SAND, 20, 255, 10, 2, 0, 0);
Material Materials::TEST_TEXTURED_SAND =
        Material(nMaterials++, "Test Textured Sand", PhysicsType::SAND, 20, 255, 10, 2, 0, 0);
Material Materials::TEST_LIQUID =
        Material(nMaterials++, "Test Liquid", PhysicsType::SOUP, 0, 255, 1.5, 4, 0, 0);

Material Materials::STONE = Material(nMaterials++, "Stone", PhysicsType::SOLID, 0, 1, 0);
Material Materials::GRASS = Material(nMaterials++, "Grass", PhysicsType::SAND, 20, 12, 1);
Material Materials::DIRT = Material(nMaterials++, "Dirt", PhysicsType::SAND, 8, 15, 1);

Material Materials::SMOOTH_STONE = Material(nMaterials++, "Stone", PhysicsType::SOLID, 0, 1, 0);
Material Materials::COBBLE_STONE =
        Material(nMaterials++, "Cobblestone", PhysicsType::SOLID, 0, 1, 0);
Material Materials::SMOOTH_DIRT = Material(nMaterials++, "Ground", PhysicsType::SOLID, 0, 1, 0);
Material Materials::COBBLE_DIRT =
        Material(nMaterials++, "Hard Ground", PhysicsType::SOLID, 0, 1, 0);
Material Materials::SOFT_DIRT = Material(nMaterials++, "Dirt", PhysicsType::SOLID, 0, 15, 2);

Material Materials::WATER =
        Material(nMaterials++, "Water", PhysicsType::SOUP, 0, 0x80, 1.5, 6, 40, 0x3000AFB5);
Material Materials::LAVA =
        Material(nMaterials++, "Lava", PhysicsType::SOUP, 0, 0xC0, 2, 1, 40, 0xFFFF6900);

Material Materials::CLOUD = Material(nMaterials++, "Cloud", PhysicsType::SOLID, 0, 127, 1, 0);

Material Materials::GOLD_ORE =
        Material(nMaterials++, "Gold Ore", PhysicsType::SAND, 20, 255, 20, 2, 8, 0x804000);
Material Materials::GOLD_MOLTEN =
        Material(nMaterials++, "Molten Gold", PhysicsType::SOUP, 0, 255, 20, 2, 8, 0x6FFF9B40);
Material Materials::GOLD_SOLID =
        Material(nMaterials++, "Solid Gold", PhysicsType::SOLID, 0, 255, 20, 2, 8, 0);

Material Materials::IRON_ORE =
        Material(nMaterials++, "Iron Ore", PhysicsType::SAND, 20, 255, 20, 2, 8, 0x7F442F);

Material Materials::OBSIDIAN =
        Material(nMaterials++, "Obsidian", PhysicsType::SOLID, 0, 255, 1, 0, 0, 0);
Material Materials::STEAM = Material(nMaterials++, "Steam", PhysicsType::GAS, 0, 255, -1, 1, 0, 0);

Material Materials::SOFT_DIRT_SAND =
        Material(nMaterials++, "Soft Dirt Sand", PhysicsType::SAND, 8, 15, 2);

Material Materials::FIRE =
        Material(nMaterials++, "Fire", PhysicsType::PASSABLE, 0, 255, 20, 1, 0, 0);

Material Materials::FLAT_COBBLE_STONE =
        Material(nMaterials++, "Flat Cobblestone", PhysicsType::SOLID, 0, 1, 0);
Material Materials::FLAT_COBBLE_DIRT =
        Material(nMaterials++, "Flat Hard Ground", PhysicsType::SOLID, 0, 1, 0);

std::vector<Material *> Materials::MATERIALS;
Material **Materials::MATERIALS_ARRAY;

void Materials::Init() {

    Materials::GENERIC_AIR.conductionSelf = 0.8;
    Materials::GENERIC_AIR.conductionOther = 0.8;

    Materials::LAVA.conductionSelf = 0.5;
    Materials::LAVA.conductionOther = 0.7;
    Materials::LAVA.addTemp = 2;

    Materials::COBBLE_STONE.conductionSelf = 0.01;
    Materials::COBBLE_STONE.conductionOther = 0.4;

    Materials::COBBLE_DIRT.conductionSelf = 1.0;
    Materials::COBBLE_DIRT.conductionOther = 1.0;

    Materials::GOLD_ORE.conductionSelf = 1.0;
    Materials::GOLD_ORE.conductionOther = 1.0;

    Materials::GOLD_MOLTEN.conductionSelf = 1.0;
    Materials::GOLD_MOLTEN.conductionOther = 1.0;

#define REGISTER(material) MATERIALS.insert(MATERIALS.begin() + material.id, &material);
    REGISTER(GENERIC_AIR);
    REGISTER(GENERIC_SOLID);
    REGISTER(GENERIC_SAND);
    REGISTER(GENERIC_LIQUID);
    REGISTER(GENERIC_GAS);
    REGISTER(GENERIC_PASSABLE);
    REGISTER(GENERIC_OBJECT);
    REGISTER(TEST_SAND);
    REGISTER(TEST_TEXTURED_SAND);
    REGISTER(TEST_LIQUID);
    REGISTER(STONE);
    REGISTER(GRASS);
    REGISTER(DIRT);
    REGISTER(SMOOTH_STONE);
    REGISTER(COBBLE_STONE);
    REGISTER(SMOOTH_DIRT);
    REGISTER(COBBLE_DIRT);
    REGISTER(SOFT_DIRT);
    REGISTER(WATER);
    REGISTER(LAVA);
    REGISTER(CLOUD);
    REGISTER(GOLD_ORE);
    REGISTER(GOLD_MOLTEN);
    REGISTER(GOLD_SOLID);
    REGISTER(IRON_ORE);
    REGISTER(OBSIDIAN);
    REGISTER(STEAM);
    REGISTER(SOFT_DIRT_SAND);
    REGISTER(FIRE);
    REGISTER(FLAT_COBBLE_STONE);
    REGISTER(FLAT_COBBLE_DIRT);

    Material *randMats = new Material[10];
    for (int i = 0; i < 10; i++) {
        char buff[10];
        snprintf(buff, sizeof(buff), "Mat_%d", i);
        std::string buffAsStdStr = buff;

        UInt32 rgb = rand() % 255;
        rgb = (rgb << 8) + rand() % 255;
        rgb = (rgb << 8) + rand() % 255;

        int type = rand() % 2 == 0 ? (rand() % 2 == 0 ? PhysicsType::SAND : PhysicsType::GAS)
                                   : PhysicsType::SOUP;
        float dens = 0;
        if (type == PhysicsType::SAND) {
            dens = 5 + (rand() % 1000) / 1000.0;
        } else if (type == PhysicsType::SOUP) {
            dens = 4 + (rand() % 1000) / 1000.0;
        } else if (type == PhysicsType::GAS) {
            dens = 3 + (rand() % 1000) / 1000.0;
        }
        randMats[i] = Material(nMaterials++, buff, type, 10,
                               type == PhysicsType::SAND ? 255 : (rand() % 192 + 63), dens,
                               rand() % 4 + 1, 0, 0, rgb);
        REGISTER(randMats[i]);
    }

    for (int j = 0; j < MATERIALS.size(); j++) {
        MATERIALS[j]->interact = false;
        MATERIALS[j]->interactions = new std::vector<MaterialInteraction>[MATERIALS.size()];
        MATERIALS[j]->nInteractions = new int[MATERIALS.size()];

        for (int k = 0; k < MATERIALS.size(); k++) {
            MATERIALS[j]->interactions[k] = {};
            MATERIALS[j]->nInteractions[k] = 0;
        }

        MATERIALS[j]->react = false;
        MATERIALS[j]->nReactions = 0;
        MATERIALS[j]->reactions = {};
    }

    for (int i = 0; i < 10; i++) {
        Material *mat = MATERIALS[randMats[i].id];
        mat->interact = false;
        int interactions = rand() % 3 + 1;
        for (int j = 0; j < interactions; j++) {
            while (true) {
                Material imat = randMats[rand() % 10];
                if (imat.id != mat->id) {
                    MATERIALS[mat->id]->nInteractions[imat.id]++;

                    MaterialInteraction inter;
                    inter.type = rand() % 2 + 1;

                    if (inter.type == INTERACT_TRANSFORM_MATERIAL) {
                        inter.data1 = randMats[rand() % 10].id;
                        inter.data2 = rand() % 4;
                        inter.ofsX = rand() % 5 - 2;
                        inter.ofsY = rand() % 5 - 2;
                    } else if (inter.type == INTERACT_SPAWN_MATERIAL) {
                        inter.data1 = randMats[rand() % 10].id;
                        inter.data2 = rand() % 4;
                        inter.ofsX = rand() % 5 - 2;
                        inter.ofsY = rand() % 5 - 2;
                    }

                    MATERIALS[mat->id]->interactions[imat.id].push_back(inter);

                    break;
                }
            }
        }
    }

    /*MATERIALS[WATER.id]->interact = true;
    MATERIALS[WATER.id]->nInteractions[LAVA.id] = 2;
    MATERIALS[WATER.id]->interactions[LAVA.id].push_back({ INTERACT_TRANSFORM_MATERIAL, OBSIDIAN.id, 3, 0, 0 });
    MATERIALS[WATER.id]->interactions[LAVA.id].push_back(MaterialInteraction{ INTERACT_SPAWN_MATERIAL, STEAM.id, 0, 0, 0 });*/

    MATERIALS[LAVA.id]->react = true;
    MATERIALS[LAVA.id]->nReactions = 1;
    MATERIALS[LAVA.id]->reactions.push_back({REACT_TEMPERATURE_BELOW, 512, OBSIDIAN.id});

    MATERIALS[WATER.id]->react = true;
    MATERIALS[WATER.id]->nReactions = 1;
    MATERIALS[WATER.id]->reactions.push_back({REACT_TEMPERATURE_ABOVE, 128, STEAM.id});

    MATERIALS[GOLD_ORE.id]->react = true;
    MATERIALS[GOLD_ORE.id]->nReactions = 1;
    MATERIALS[GOLD_ORE.id]->reactions.push_back({REACT_TEMPERATURE_ABOVE, 512, GOLD_MOLTEN.id});

    MATERIALS[GOLD_MOLTEN.id]->react = true;
    MATERIALS[GOLD_MOLTEN.id]->nReactions = 1;
    MATERIALS[GOLD_MOLTEN.id]->reactions.push_back({REACT_TEMPERATURE_BELOW, 128, GOLD_SOLID.id});

    MATERIALS_ARRAY = MATERIALS.data();

#undef REGISTER
}

int MaterialInstance::_curID = 1;

MaterialInstance::MaterialInstance(Material *mat, UInt32 color, int32_t temperature) {
    this->id = _curID++;
    this->mat = mat;
    this->color = color;
    this->temperature = temperature;
}

bool MaterialInstance::operator==(const MaterialInstance &other) { return this->id == other.id; }

const MaterialInstance Tiles::NOTHING = MaterialInstance(&Materials::GENERIC_AIR, 0x000000);
const MaterialInstance Tiles::TEST_SOLID = MaterialInstance(&Materials::GENERIC_SOLID, 0xff0000);
const MaterialInstance Tiles::TEST_SAND = MaterialInstance(&Materials::GENERIC_SAND, 0xffff00);
const MaterialInstance Tiles::TEST_LIQUID = MaterialInstance(&Materials::GENERIC_LIQUID, 0x0000ff);
const MaterialInstance Tiles::TEST_GAS = MaterialInstance(&Materials::GENERIC_GAS, 0x800080);
const MaterialInstance Tiles::OBJECT = MaterialInstance(&Materials::GENERIC_OBJECT, 0x00ff00);

MaterialInstance Tiles::createTestSand() {
    UInt32 rgb = 220;
    rgb = (rgb << 8) + 155 + rand() % 30;
    rgb = (rgb << 8) + 100;
    return MaterialInstance(&Materials::TEST_SAND, rgb);
}

MaterialInstance Tiles::createTestTexturedSand(int x, int y) {
    C_Surface *tex = Textures::testTexture;

    int tx = x % tex->w;
    int ty = y % tex->h;

    UInt32 rgb = R_GET_PIXEL(tex, tx, ty);
    return MaterialInstance(&Materials::TEST_TEXTURED_SAND, rgb);
}

MaterialInstance Tiles::createTestLiquid() {
    UInt32 rgb = 0;
    rgb = (rgb << 8) + 0;
    rgb = (rgb << 8) + 255;
    return MaterialInstance(&Materials::TEST_LIQUID, rgb);
}

MaterialInstance Tiles::createStone(int x, int y) {
    C_Surface *tex = Textures::cobbleStone;

    int tx = x % tex->w;
    int ty = y % tex->h;

    UInt8 *pixel = (UInt8 *) tex->pixels;

    pixel += ((ty) *tex->pitch) + ((tx) * sizeof(UInt32));
    UInt32 rgb = *((UInt32 *) pixel);

    return MaterialInstance(&Materials::STONE, rgb);
}

MaterialInstance Tiles::createGrass() {
    UInt32 rgb = 40;
    rgb = (rgb << 8) + 120 + rand() % 20;
    rgb = (rgb << 8) + 20;
    return MaterialInstance(&Materials::GRASS, rgb);
}

MaterialInstance Tiles::createDirt() {
    UInt32 rgb = 60 + rand() % 10;
    rgb = (rgb << 8) + 40;
    rgb = (rgb << 8) + 20;
    return MaterialInstance(&Materials::DIRT, rgb);
}

MaterialInstance Tiles::createSmoothStone(int x, int y) {
    C_Surface *tex = Textures::smoothStone;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::SMOOTH_STONE, rgb);
}

MaterialInstance Tiles::createCobbleStone(int x, int y) {
    C_Surface *tex = Textures::cobbleStone;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::COBBLE_STONE, rgb);
}

MaterialInstance Tiles::createSmoothDirt(int x, int y) {
    C_Surface *tex = Textures::smoothDirt;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::SMOOTH_DIRT, rgb);
}

MaterialInstance Tiles::createCobbleDirt(int x, int y) {
    C_Surface *tex = Textures::cobbleDirt;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::COBBLE_DIRT, rgb);
}

MaterialInstance Tiles::createSoftDirt(int x, int y) {
    C_Surface *tex = Textures::softDirt;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::SOFT_DIRT, rgb);
}

MaterialInstance Tiles::createWater() {
    UInt32 rgb = 0x00B69F;

    return MaterialInstance(&Materials::WATER, rgb, -1023);
}

MaterialInstance Tiles::createLava() {
    UInt32 rgb = 0xFF7C00;

    return MaterialInstance(&Materials::LAVA, rgb, 1024);
}

MaterialInstance Tiles::createCloud(int x, int y) {
    C_Surface *tex = Textures::cloud;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::CLOUD, rgb);
}

MaterialInstance Tiles::createGold(int x, int y) {
    C_Surface *tex = Textures::gold;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::GOLD_ORE, rgb);
}

MaterialInstance Tiles::createIron(int x, int y) {
    C_Surface *tex = Textures::iron;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::IRON_ORE, rgb);
}

MaterialInstance Tiles::createObsidian(int x, int y) {
    C_Surface *tex = Textures::obsidian;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::OBSIDIAN, rgb);
}

MaterialInstance Tiles::createSteam() { return MaterialInstance(&Materials::STEAM, 0x666666); }

MaterialInstance Tiles::createFire() {

    UInt32 rgb = 255;
    rgb = (rgb << 8) + 100 + rand() % 50;
    rgb = (rgb << 8) + 50;

    return MaterialInstance(&Materials::FIRE, rgb);
}

MaterialInstance Tiles::create(Material *mat, int x, int y) {
    if (mat->id == Materials::TEST_SAND.id) {
        return createTestSand();
    } else if (mat->id == Materials::TEST_TEXTURED_SAND.id) {
        return createTestTexturedSand(x, y);
    } else if (mat->id == Materials::TEST_LIQUID.id) {
        return createTestLiquid();
    } else if (mat->id == Materials::STONE.id) {
        return createStone(x, y);
    } else if (mat->id == Materials::GRASS.id) {
        return createGrass();
    } else if (mat->id == Materials::DIRT.id) {
        return createDirt();
    } else if (mat->id == Materials::SMOOTH_STONE.id) {
        return createSmoothStone(x, y);
    } else if (mat->id == Materials::COBBLE_STONE.id) {
        return createCobbleStone(x, y);
    } else if (mat->id == Materials::SMOOTH_DIRT.id) {
        return createSmoothDirt(x, y);
    } else if (mat->id == Materials::COBBLE_DIRT.id) {
        return createCobbleDirt(x, y);
    } else if (mat->id == Materials::SOFT_DIRT.id) {
        return createSoftDirt(x, y);
    } else if (mat->id == Materials::WATER.id) {
        return createWater();
    } else if (mat->id == Materials::LAVA.id) {
        return createLava();
    } else if (mat->id == Materials::CLOUD.id) {
        return createCloud(x, y);
    } else if (mat->id == Materials::GOLD_ORE.id) {
        return createGold(x, y);
    } else if (mat->id == Materials::GOLD_MOLTEN.id) {
        C_Surface *tex = Textures::goldMolten;

        int tx = (tex->w + (x % tex->w)) % tex->w;
        int ty = (tex->h + (y % tex->h)) % tex->h;

        UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

        return MaterialInstance(&Materials::GOLD_MOLTEN, rgb);
    } else if (mat->id == Materials::GOLD_SOLID.id) {
        C_Surface *tex = Textures::goldSolid;

        int tx = (tex->w + (x % tex->w)) % tex->w;
        int ty = (tex->h + (y % tex->h)) % tex->h;

        UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

        return MaterialInstance(&Materials::GOLD_SOLID, rgb);
    } else if (mat->id == Materials::IRON_ORE.id) {
        return createIron(x, y);
    } else if (mat->id == Materials::OBSIDIAN.id) {
        return createObsidian(x, y);
    } else if (mat->id == Materials::STEAM.id) {
        return createSteam();
    } else if (mat->id == Materials::FIRE.id) {
        return createFire();
    } else if (mat->id == Materials::FLAT_COBBLE_STONE.id) {
        C_Surface *tex = Textures::flatCobbleStone;

        int tx = (tex->w + (x % tex->w)) % tex->w;
        int ty = (tex->h + (y % tex->h)) % tex->h;

        UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

        return MaterialInstance(&Materials::FLAT_COBBLE_STONE, rgb);
    } else if (mat->id == Materials::FLAT_COBBLE_DIRT.id) {
        C_Surface *tex = Textures::flatCobbleDirt;

        int tx = (tex->w + (x % tex->w)) % tex->w;
        int ty = (tex->h + (y % tex->h)) % tex->h;

        UInt32 rgb = R_GET_PIXEL(tex, tx, ty);

        return MaterialInstance(&Materials::FLAT_COBBLE_DIRT, rgb);
    }

    return MaterialInstance(mat, mat->color);
}
