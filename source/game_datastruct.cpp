// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "game_datastruct.hpp"

#include <string>
#include <string_view>

#include "chunk.hpp"
#include "core/core.h"
#include "core/core.hpp"
#include "core/cpp/utils.hpp"
#include "core/debug_impl.hpp"
#include "core/global.hpp"
#include "core/macros.h"
#include "filesystem.h"
#include "game.hpp"
#include "game_resources.hpp"
#include "game_ui.hpp"
#include "internal/builtin_box2d.h"
#include "reflectionflat.hpp"
#include "renderer/renderer_utils.h"
#include "scripting/lua_wrapper.hpp"
#include "world.hpp"

MetaEngine::vector<Biome *> GameData::biome_container;
MetaEngine::vector<Material *> GameData::materials_container;
I32 GameData::materials_count = 0;
Material **GameData::materials_array;

void ReleaseGameData() {
    for (auto b : global.GameData_.biome_container) {
        if (static_cast<bool>(b)) delete b;
    }
}

WorldEntity::WorldEntity(bool isplayer, std::string n) : is_player(isplayer), name(n) {}

WorldEntity::~WorldEntity() {
    // if (static_cast<bool>(rb)) delete rb;
}

#pragma region Material

Material::Material(int id, std::string name, std::string index_name, int physicsType, int slipperyness, U8 alpha, F32 density, int iterations, int emit, U32 emitColor, U32 color) {
    this->name = name;
    this->index_name = index_name;
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

// REFLECT_CLASS(Material);
// CLASS_META_DATA(META_DATA_DESCRIPTION, "Material data");
// REFLECT_MEMBER(name);
// REFLECT_MEMBER(index_name);
// REFLECT_MEMBER(id);
// REFLECT_MEMBER(physicsType);
// REFLECT_MEMBER(alpha);
// REFLECT_MEMBER(density);
// REFLECT_MEMBER(iterations);
// REFLECT_MEMBER(emit);
// REFLECT_MEMBER(emitColor);
// REFLECT_MEMBER(color);
// REFLECT_MEMBER(addTemp);
// REFLECT_MEMBER(conductionSelf);
// REFLECT_MEMBER(conductionOther);
// REFLECT_MEMBER(interact);
// REFLECT_MEMBER(nInteractions);
// REFLECT_MEMBER(interactions);
// REFLECT_MEMBER(react);
// REFLECT_MEMBER(nReactions);
// REFLECT_MEMBER(reactions);
// REFLECT_MEMBER(slipperyness);
// REFLECT_END(Material);

#pragma region MATERIALSLIST

#define INITMATERIAL(_index, _id, _name, _physics, _s, _a, _d, _i, _e, _c) Material MaterialsList::_index = Material(_id, _name, #_index, _physics, _s, _a, _d, _i, _e, _c)

// Basic materials
INITMATERIAL(GENERIC_AIR, GameData::materials_count++, "_AIR", PhysicsType::AIR, 0, 255, 0, 0, 16, 0);
INITMATERIAL(GENERIC_SOLID, GameData::materials_count++, "_SOLID", PhysicsType::SOLID, 0, 255, 1, 0, 0, 0);
INITMATERIAL(GENERIC_SAND, GameData::materials_count++, "_SAND", PhysicsType::SAND, 20, 255, 10, 2, 0, 0);
INITMATERIAL(GENERIC_LIQUID, GameData::materials_count++, "_LIQUID", PhysicsType::SOUP, 0, 255, 1.5, 3, 0, 0);
INITMATERIAL(GENERIC_GAS, GameData::materials_count++, "_GAS", PhysicsType::GAS, 0, 255, -1, 1, 0, 0);
INITMATERIAL(GENERIC_PASSABLE, GameData::materials_count++, "_PASSABLE", PhysicsType::PASSABLE, 0, 255, 0, 0, 0, 0);
INITMATERIAL(GENERIC_OBJECT, GameData::materials_count++, "_OBJECT", PhysicsType::OBJECT, 0, 255, 1000.0, 0, 0, 0);

#undef INITMATERIAL

// Test materials
Material MaterialsList::TEST_SAND = Material(GameData::materials_count++, "Test Sand", "TEST_SAND", PhysicsType::SAND, 20, 255, 10, 2, 0, 0);
Material MaterialsList::TEST_TEXTURED_SAND = Material(GameData::materials_count++, "Test Textured Sand", "TEST_TEXTURED_SAND", PhysicsType::SAND, 20, 255, 10, 2, 0, 0);
Material MaterialsList::TEST_LIQUID = Material(GameData::materials_count++, "Test Liquid", "TEST_LIQUID", PhysicsType::SOUP, 0, 255, 1.5, 4, 0, 0);

// Game contents
Material MaterialsList::STONE = Material(GameData::materials_count++, "Stone", "STONE", PhysicsType::SOLID, 0, 1, 0);
Material MaterialsList::GRASS = Material(GameData::materials_count++, "Grass", "GRASS", PhysicsType::SAND, 20, 12, 1);
Material MaterialsList::DIRT = Material(GameData::materials_count++, "Dirt", "DIRT", PhysicsType::SAND, 8, 15, 1);

Material MaterialsList::SMOOTH_STONE = Material(GameData::materials_count++, "Stone", "SMOOTH_STONE", PhysicsType::SOLID, 0, 1, 0);
Material MaterialsList::COBBLE_STONE = Material(GameData::materials_count++, "Cobblestone", "COBBLE_STONE", PhysicsType::SOLID, 0, 1, 0);
Material MaterialsList::SMOOTH_DIRT = Material(GameData::materials_count++, "Ground", "SMOOTH_DIRT", PhysicsType::SOLID, 0, 1, 0);
Material MaterialsList::COBBLE_DIRT = Material(GameData::materials_count++, "Hard Ground", "COBBLE_DIRT", PhysicsType::SOLID, 0, 1, 0);
Material MaterialsList::SOFT_DIRT = Material(GameData::materials_count++, "Dirt", "SOFT_DIRT", PhysicsType::SOLID, 0, 15, 2);

Material MaterialsList::WATER = Material(GameData::materials_count++, "Water", "WATER", PhysicsType::SOUP, 0, 0x80, 1.5, 6, 40, 0x3000AFB5);
Material MaterialsList::LAVA = Material(GameData::materials_count++, "Lava", "LAVA", PhysicsType::SOUP, 0, 0xC0, 2, 1, 40, 0xFFFF6900);

Material MaterialsList::CLOUD = Material(GameData::materials_count++, "Cloud", "CLOUD", PhysicsType::SOLID, 0, 127, 1, 0);

Material MaterialsList::GOLD_ORE = Material(GameData::materials_count++, "Gold Ore", "GOLD_ORE", PhysicsType::SAND, 20, 255, 20, 2, 8, 0x804000);
Material MaterialsList::GOLD_MOLTEN = Material(GameData::materials_count++, "Molten Gold", "GOLD_MOLTEN", PhysicsType::SOUP, 0, 255, 20, 2, 8, 0x6FFF9B40);
Material MaterialsList::GOLD_SOLID = Material(GameData::materials_count++, "Solid Gold", "GOLD_SOLID", PhysicsType::SOLID, 0, 255, 20, 2, 8, 0);

Material MaterialsList::IRON_ORE = Material(GameData::materials_count++, "Iron Ore", "IRON_ORE", PhysicsType::SAND, 20, 255, 20, 2, 8, 0x7F442F);

Material MaterialsList::OBSIDIAN = Material(GameData::materials_count++, "Obsidian", "OBSIDIAN", PhysicsType::SOLID, 0, 255, 1, 0, 0, 0);
Material MaterialsList::STEAM = Material(GameData::materials_count++, "Steam", "STEAM", PhysicsType::GAS, 0, 255, -1, 1, 0, 0);

Material MaterialsList::SOFT_DIRT_SAND = Material(GameData::materials_count++, "Soft Dirt Sand", "SOFT_DIRT_SAND", PhysicsType::SAND, 8, 15, 2);

Material MaterialsList::FIRE = Material(GameData::materials_count++, "Fire", "FIRE", PhysicsType::PASSABLE, 0, 255, 20, 1, 0, 0);

Material MaterialsList::FLAT_COBBLE_STONE = Material(GameData::materials_count++, "Flat Cobblestone", "FLAT_COBBLE_STONE", PhysicsType::SOLID, 0, 1, 0);
Material MaterialsList::FLAT_COBBLE_DIRT = Material(GameData::materials_count++, "Flat Hard Ground", "FLAT_COBBLE_DIRT", PhysicsType::SOLID, 0, 1, 0);

#pragma endregion MATERIALSLIST

void InitMaterials() {

    MaterialsList::GENERIC_AIR.conductionSelf = 0.8;
    MaterialsList::GENERIC_AIR.conductionOther = 0.8;

    MaterialsList::LAVA.conductionSelf = 0.5;
    MaterialsList::LAVA.conductionOther = 0.7;
    MaterialsList::LAVA.addTemp = 2;

    MaterialsList::COBBLE_STONE.conductionSelf = 0.01;
    MaterialsList::COBBLE_STONE.conductionOther = 0.4;

    MaterialsList::COBBLE_DIRT.conductionSelf = 1.0;
    MaterialsList::COBBLE_DIRT.conductionOther = 1.0;

    MaterialsList::GOLD_ORE.conductionSelf = 1.0;
    MaterialsList::GOLD_ORE.conductionOther = 1.0;

    MaterialsList::GOLD_MOLTEN.conductionSelf = 1.0;
    MaterialsList::GOLD_MOLTEN.conductionOther = 1.0;

#define REGISTER(_m) global.GameData_.materials_container.insert(global.GameData_.materials_container.begin() + _m.id, &_m)

    REGISTER(MaterialsList::GENERIC_AIR);
    REGISTER(MaterialsList::GENERIC_SOLID);
    REGISTER(MaterialsList::GENERIC_SAND);
    REGISTER(MaterialsList::GENERIC_LIQUID);
    REGISTER(MaterialsList::GENERIC_GAS);
    REGISTER(MaterialsList::GENERIC_PASSABLE);
    REGISTER(MaterialsList::GENERIC_OBJECT);
    REGISTER(MaterialsList::TEST_SAND);
    REGISTER(MaterialsList::TEST_TEXTURED_SAND);
    REGISTER(MaterialsList::TEST_LIQUID);
    REGISTER(MaterialsList::STONE);
    REGISTER(MaterialsList::GRASS);
    REGISTER(MaterialsList::DIRT);
    REGISTER(MaterialsList::SMOOTH_STONE);
    REGISTER(MaterialsList::COBBLE_STONE);
    REGISTER(MaterialsList::SMOOTH_DIRT);
    REGISTER(MaterialsList::COBBLE_DIRT);
    REGISTER(MaterialsList::SOFT_DIRT);
    REGISTER(MaterialsList::WATER);
    REGISTER(MaterialsList::LAVA);
    REGISTER(MaterialsList::CLOUD);
    REGISTER(MaterialsList::GOLD_ORE);
    REGISTER(MaterialsList::GOLD_MOLTEN);
    REGISTER(MaterialsList::GOLD_SOLID);
    REGISTER(MaterialsList::IRON_ORE);
    REGISTER(MaterialsList::OBSIDIAN);
    REGISTER(MaterialsList::STEAM);
    REGISTER(MaterialsList::SOFT_DIRT_SAND);
    REGISTER(MaterialsList::FIRE);
    REGISTER(MaterialsList::FLAT_COBBLE_STONE);
    REGISTER(MaterialsList::FLAT_COBBLE_DIRT);

    Material *randMats = new Material[10];
    for (int i = 0; i < 10; i++) {
        char buff[10];
        snprintf(buff, sizeof(buff), "Mat_%d", i);
        std::string buffAsStdStr = buff;

        U32 rgb = rand() % 255;
        rgb = (rgb << 8) + rand() % 255;
        rgb = (rgb << 8) + rand() % 255;

        int type = rand() % 2 == 0 ? (rand() % 2 == 0 ? PhysicsType::SAND : PhysicsType::GAS) : PhysicsType::SOUP;
        F32 dens = 0;
        if (type == PhysicsType::SAND) {
            dens = 5 + (rand() % 1000) / 1000.0;
        } else if (type == PhysicsType::SOUP) {
            dens = 4 + (rand() % 1000) / 1000.0;
        } else if (type == PhysicsType::GAS) {
            dens = 3 + (rand() % 1000) / 1000.0;
        }
        randMats[i] = Material(GameData::materials_count++, buff, buff, type, 10, type == PhysicsType::SAND ? 255 : (rand() % 192 + 63), dens, rand() % 4 + 1, 0, 0, rgb);
        REGISTER(randMats[i]);
    }

    for (int j = 0; j < GameData::materials_container.size(); j++) {
        GameData::materials_container[j]->interact = false;
        GameData::materials_container[j]->interactions = new MetaEngine::vector<MaterialInteraction>[GameData::materials_container.size()];
        GameData::materials_container[j]->nInteractions = new int[GameData::materials_container.size()];

        for (int k = 0; k < GameData::materials_container.size(); k++) {
            GameData::materials_container[j]->interactions[k] = {};
            GameData::materials_container[j]->nInteractions[k] = 0;
        }

        GameData::materials_container[j]->react = false;
        GameData::materials_container[j]->nReactions = 0;
        GameData::materials_container[j]->reactions = {};
    }

    for (int i = 0; i < 10; i++) {
        Material *mat = GameData::materials_container[randMats[i].id];
        mat->interact = false;
        int interactions = rand() % 3 + 1;
        for (int j = 0; j < interactions; j++) {
            while (true) {
                Material imat = randMats[rand() % 10];
                if (imat.id != mat->id) {
                    GameData::materials_container[mat->id]->nInteractions[imat.id]++;

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

                    GameData::materials_container[mat->id]->interactions[imat.id].push_back(inter);

                    break;
                }
            }
        }
    }

    /*GameData::materials_container[WATER.id]->interact = true;
    GameData::materials_container[WATER.id]->nInteractions[LAVA.id] = 2;
    GameData::materials_container[WATER.id]->interactions[LAVA.id].push_back({ INTERACT_TRANSFORM_MATERIAL, OBSIDIAN.id, 3, 0, 0 });
    GameData::materials_container[WATER.id]->interactions[LAVA.id].push_back(MaterialInteraction{ INTERACT_SPAWN_MATERIAL, STEAM.id, 0, 0, 0 });*/

    GameData::materials_container[MaterialsList::LAVA.id]->react = true;
    GameData::materials_container[MaterialsList::LAVA.id]->nReactions = 1;
    GameData::materials_container[MaterialsList::LAVA.id]->reactions.push_back({REACT_TEMPERATURE_BELOW, 512, MaterialsList::OBSIDIAN.id});

    GameData::materials_container[MaterialsList::WATER.id]->react = true;
    GameData::materials_container[MaterialsList::WATER.id]->nReactions = 1;
    GameData::materials_container[MaterialsList::WATER.id]->reactions.push_back({REACT_TEMPERATURE_ABOVE, 128, MaterialsList::STEAM.id});

    GameData::materials_container[MaterialsList::GOLD_ORE.id]->react = true;
    GameData::materials_container[MaterialsList::GOLD_ORE.id]->nReactions = 1;
    GameData::materials_container[MaterialsList::GOLD_ORE.id]->reactions.push_back({REACT_TEMPERATURE_ABOVE, 512, MaterialsList::GOLD_MOLTEN.id});

    GameData::materials_container[MaterialsList::GOLD_MOLTEN.id]->react = true;
    GameData::materials_container[MaterialsList::GOLD_MOLTEN.id]->nReactions = 1;
    GameData::materials_container[MaterialsList::GOLD_MOLTEN.id]->reactions.push_back({REACT_TEMPERATURE_BELOW, 128, MaterialsList::GOLD_SOLID.id});

    global.GameData_.materials_array = GameData::materials_container.data();

#undef REGISTER

    // Just test
    MetaEngine::StaticRefl::TypeInfo<Material>::fields.ForEach([](const auto &field) { METADOT_DBG(field.name); });
}

int MaterialInstance::_curID = 1;

MaterialInstance::MaterialInstance(Material *mat, U32 color, I32 temperature) {
    this->id = _curID++;
    this->mat = mat;
    this->color = color;
    this->temperature = temperature;
}

bool MaterialInstance::operator==(const MaterialInstance &other) { return this->id == other.id; }

MaterialInstance Tiles_NOTHING = MaterialInstance(&MaterialsList::GENERIC_AIR, 0x000000);
MaterialInstance Tiles_TEST_SOLID = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xff0000);
MaterialInstance Tiles_TEST_SAND = MaterialInstance(&MaterialsList::GENERIC_SAND, 0xffff00);
MaterialInstance Tiles_TEST_LIQUID = MaterialInstance(&MaterialsList::GENERIC_LIQUID, 0x0000ff);
MaterialInstance Tiles_TEST_GAS = MaterialInstance(&MaterialsList::GENERIC_GAS, 0x800080);
MaterialInstance Tiles_OBJECT = MaterialInstance(&MaterialsList::GENERIC_OBJECT, 0x00ff00);

MaterialInstance TilesCreateTestSand() {
    U32 rgb = 220;
    rgb = (rgb << 8) + 155 + rand() % 30;
    rgb = (rgb << 8) + 100;
    return MaterialInstance(&MaterialsList::TEST_SAND, rgb);
}

MaterialInstance TilesCreateTestTexturedSand(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->testTexture->surface;

    int tx = x % tex->w;
    int ty = y % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);
    return MaterialInstance(&MaterialsList::TEST_TEXTURED_SAND, rgb);
}

MaterialInstance TilesCreateTestLiquid() {
    U32 rgb = 0;
    rgb = (rgb << 8) + 0;
    rgb = (rgb << 8) + 255;
    return MaterialInstance(&MaterialsList::TEST_LIQUID, rgb);
}

MaterialInstance TilesCreateStone(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->cobbleStone->surface;

    int tx = x % tex->w;
    int ty = y % tex->h;

    U8 *pixel = (U8 *)tex->pixels;

    pixel += ((ty)*tex->pitch) + ((tx) * sizeof(U32));
    U32 rgb = *((U32 *)pixel);

    return MaterialInstance(&MaterialsList::STONE, rgb);
}

MaterialInstance TilesCreateGrass() {
    U32 rgb = 40;
    rgb = (rgb << 8) + 120 + rand() % 20;
    rgb = (rgb << 8) + 20;
    return MaterialInstance(&MaterialsList::GRASS, rgb);
}

MaterialInstance TilesCreateDirt() {
    U32 rgb = 60 + rand() % 10;
    rgb = (rgb << 8) + 40;
    rgb = (rgb << 8) + 20;
    return MaterialInstance(&MaterialsList::DIRT, rgb);
}

MaterialInstance TilesCreateSmoothStone(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->smoothStone->surface;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&MaterialsList::SMOOTH_STONE, rgb);
}

MaterialInstance TilesCreateCobbleStone(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->cobbleStone->surface;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&MaterialsList::COBBLE_STONE, rgb);
}

MaterialInstance TilesCreateSmoothDirt(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->smoothDirt->surface;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&MaterialsList::SMOOTH_DIRT, rgb);
}

MaterialInstance TilesCreateCobbleDirt(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->cobbleDirt->surface;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&MaterialsList::COBBLE_DIRT, rgb);
}

MaterialInstance TilesCreateSoftDirt(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->softDirt->surface;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&MaterialsList::SOFT_DIRT, rgb);
}

MaterialInstance TilesCreateWater() {
    U32 rgb = 0x00B69F;

    return MaterialInstance(&MaterialsList::WATER, rgb, -1023);
}

MaterialInstance TilesCreateLava() {
    U32 rgb = 0xFF7C00;

    return MaterialInstance(&MaterialsList::LAVA, rgb, 1024);
}

MaterialInstance TilesCreateCloud(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->cloud->surface;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&MaterialsList::CLOUD, rgb);
}

MaterialInstance TilesCreateGold(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->gold->surface;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&MaterialsList::GOLD_ORE, rgb);
}

MaterialInstance TilesCreateIron(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->iron->surface;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&MaterialsList::IRON_ORE, rgb);
}

MaterialInstance TilesCreateObsidian(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->obsidian->surface;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&MaterialsList::OBSIDIAN, rgb);
}

MaterialInstance TilesCreateSteam() { return MaterialInstance(&MaterialsList::STEAM, 0x666666); }

MaterialInstance TilesCreateFire() {

    U32 rgb = 255;
    rgb = (rgb << 8) + 100 + rand() % 50;
    rgb = (rgb << 8) + 50;

    return MaterialInstance(&MaterialsList::FIRE, rgb);
}

MaterialInstance TilesCreate(Material *mat, int x, int y) {
    if (mat->id == MaterialsList::TEST_SAND.id) {
        return TilesCreateTestSand();
    } else if (mat->id == MaterialsList::TEST_TEXTURED_SAND.id) {
        return TilesCreateTestTexturedSand(x, y);
    } else if (mat->id == MaterialsList::TEST_LIQUID.id) {
        return TilesCreateTestLiquid();
    } else if (mat->id == MaterialsList::STONE.id) {
        return TilesCreateStone(x, y);
    } else if (mat->id == MaterialsList::GRASS.id) {
        return TilesCreateGrass();
    } else if (mat->id == MaterialsList::DIRT.id) {
        return TilesCreateDirt();
    } else if (mat->id == MaterialsList::SMOOTH_STONE.id) {
        return TilesCreateSmoothStone(x, y);
    } else if (mat->id == MaterialsList::COBBLE_STONE.id) {
        return TilesCreateCobbleStone(x, y);
    } else if (mat->id == MaterialsList::SMOOTH_DIRT.id) {
        return TilesCreateSmoothDirt(x, y);
    } else if (mat->id == MaterialsList::COBBLE_DIRT.id) {
        return TilesCreateCobbleDirt(x, y);
    } else if (mat->id == MaterialsList::SOFT_DIRT.id) {
        return TilesCreateSoftDirt(x, y);
    } else if (mat->id == MaterialsList::WATER.id) {
        return TilesCreateWater();
    } else if (mat->id == MaterialsList::LAVA.id) {
        return TilesCreateLava();
    } else if (mat->id == MaterialsList::CLOUD.id) {
        return TilesCreateCloud(x, y);
    } else if (mat->id == MaterialsList::GOLD_ORE.id) {
        return TilesCreateGold(x, y);
    } else if (mat->id == MaterialsList::GOLD_MOLTEN.id) {
        C_Surface *tex = global.game->GameIsolate_.texturepack->goldMolten->surface;

        int tx = (tex->w + (x % tex->w)) % tex->w;
        int ty = (tex->h + (y % tex->h)) % tex->h;

        U32 rgb = R_GET_PIXEL(tex, tx, ty);

        return MaterialInstance(&MaterialsList::GOLD_MOLTEN, rgb);
    } else if (mat->id == MaterialsList::GOLD_SOLID.id) {
        C_Surface *tex = global.game->GameIsolate_.texturepack->goldSolid->surface;

        int tx = (tex->w + (x % tex->w)) % tex->w;
        int ty = (tex->h + (y % tex->h)) % tex->h;

        U32 rgb = R_GET_PIXEL(tex, tx, ty);

        return MaterialInstance(&MaterialsList::GOLD_SOLID, rgb);
    } else if (mat->id == MaterialsList::IRON_ORE.id) {
        return TilesCreateIron(x, y);
    } else if (mat->id == MaterialsList::OBSIDIAN.id) {
        return TilesCreateObsidian(x, y);
    } else if (mat->id == MaterialsList::STEAM.id) {
        return TilesCreateSteam();
    } else if (mat->id == MaterialsList::FIRE.id) {
        return TilesCreateFire();
    } else if (mat->id == MaterialsList::FLAT_COBBLE_STONE.id) {
        C_Surface *tex = global.game->GameIsolate_.texturepack->flatCobbleStone->surface;

        int tx = (tex->w + (x % tex->w)) % tex->w;
        int ty = (tex->h + (y % tex->h)) % tex->h;

        U32 rgb = R_GET_PIXEL(tex, tx, ty);

        return MaterialInstance(&MaterialsList::FLAT_COBBLE_STONE, rgb);
    } else if (mat->id == MaterialsList::FLAT_COBBLE_DIRT.id) {
        C_Surface *tex = global.game->GameIsolate_.texturepack->flatCobbleDirt->surface;

        int tx = (tex->w + (x % tex->w)) % tex->w;
        int ty = (tex->h + (y % tex->h)) % tex->h;

        U32 rgb = R_GET_PIXEL(tex, tx, ty);

        return MaterialInstance(&MaterialsList::FLAT_COBBLE_DIRT, rgb);
    }

    return MaterialInstance(mat, mat->color);
}

#pragma endregion Material

Item::Item() {}

Item::~Item() {
    R_FreeImage(texture);
    SDL_FreeSurface(surface);
}

Item *Item::makeItem(ItemFlags flags, RigidBody *rb, std::string n) {
    Item *i;

    if (rb->item != NULL) {
        i = rb->item;
    } else {
        i = new Item();
        i->flags = flags;
    }
    
    i->surface = rb->surface;
    i->texture = rb->texture;
    i->name = n;

    return i;
}

U32 getpixel(C_Surface *surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    U8 *p = (U8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch (bpp) {
        case 1:
            return *p;
            break;

        case 2:
            return *(U16 *)p;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
            break;

        case 4:
            return *(U32 *)p;
            break;

        default:
            return 0; /* shouldn't happen, but avoids warnings */
    }
}

void Item::loadFillTexture(C_Surface *tex) {
    fill.resize(capacity);
    U32 maxN = 0;
    for (U16 x = 0; x < tex->w; x++) {
        for (U16 y = 0; y < tex->h; y++) {
            U32 col = getpixel(tex, x, y);
            if (((col >> 24) & 0xff) > 0) {
                U32 n = col & 0x00ffffff;
                fill[n - 1] = {x, y};
                if (n - 1 > maxN) maxN = n - 1;
            }
        }
    }
    fill.resize(maxN);
}

Structure::Structure(int w, int h, MaterialInstance *tiles) {
    this->w = w;
    this->h = h;
    this->tiles = tiles;
}

Structure::Structure(C_Surface *texture, Material mat) {
    MaterialInstance *tiles = new MaterialInstance[texture->w * texture->h];
    for (int x = 0; x < texture->w; x++) {
        for (int y = 0; y < texture->h; y++) {
            U32 color = R_GET_PIXEL(texture, x, y);
            int alpha = 255;
            if (texture->format->format == SDL_PIXELFORMAT_ARGB8888) {
                alpha = (color >> 24) & 0xff;
                if (alpha == 0) {
                    tiles[x + y * texture->w] = Tiles_NOTHING;
                    continue;
                }
            }
            MaterialInstance prop = MaterialInstance(&mat, color);
            tiles[x + y * texture->w] = prop;
        }
    }
    this->w = texture->w;
    this->h = texture->h;
    this->tiles = tiles;
}

Structure Structures::makeTree(World world, int x, int y) {
    int w = 50 + rand() % 10;
    int h = 80 + rand() % 20;
    MaterialInstance *tiles = new MaterialInstance[w * h];

    for (int tx = 0; tx < w; tx++) {
        for (int ty = 0; ty < h; ty++) {
            tiles[tx + ty * w] = Tiles_NOTHING;
        }
    }

    int trunk = 3 + rand() % 2;

    F32 cx = w / 2;
    F32 dcx = (((rand() % 10) / 10.0) - 0.5) / 3.0;
    for (int ty = h - 1; ty > 20; ty--) {
        int bw = trunk + std::max((ty - h + 10) / 3, 0);
        for (int xx = -bw; xx <= bw; xx++) {
            tiles[((int)cx + xx - (int)(dcx * (h - 30))) + ty * w] = MaterialInstance(&MaterialsList::GENERIC_PASSABLE, xx >= 2 ? 0x683600 : 0x7C4000);
        }
        cx += dcx;
    }

    for (int theta = 0; theta < 360; theta += 1) {
        F64 p = world.noise.GetPerlin(std::cos(theta * 3.1415 / 180.0) * 4 + x, std::sin(theta * 3.1415 / 180.0) * 4 + y, 2652);
        F32 r = 15 + (F32)p * 6;
        for (F32 d = 0; d < r; d += 0.5) {
            int tx = cx - (int)(dcx * (h - 30)) + d * std::cos(theta * 3.1415 / 180.0);
            int ty = 20 + d * std::sin(theta * 3.1415 / 180.0);
            if (tx >= 0 && ty >= 0 && tx < w && ty < h) {
                tiles[tx + ty * w] = MaterialInstance(&MaterialsList::GENERIC_PASSABLE, r - d < 0.5f ? 0x00aa00 : 0x00ff00);
            }
        }
    }

    int nBranches = rand() % 3;
    bool side = rand() % 2;  // false = right, true = left
    for (int i = 0; i < nBranches; i++) {
        int yPos = 20 + (h - 20) / 3 * (i + 1) + rand() % 10;
        F32 tilt = ((rand() % 10) / 10.0 - 0.5) * 8;
        int len = 10 + rand() % 5;
        for (int xx = 0; xx < len; xx++) {
            int tx = (int)(w / 2 + dcx * (h - yPos)) + (side ? 1 : -1) * (xx + 2) - (int)(dcx * (h - 30));
            int th = 3 * (1 - (xx / (F32)len));
            for (int yy = -th; yy <= th; yy++) {
                int ty = yPos + yy + (xx / (F32)len * tilt);
                if (tx >= 0 && ty >= 0 && tx < w && ty < h) {
                    tiles[tx + ty * w] = MaterialInstance(&MaterialsList::GENERIC_PASSABLE, yy >= 2 ? 0x683600 : 0x7C4000);
                }
            }
        }
        side = !side;
    }

    return Structure(w, h, tiles);
}

Structure Structures::makeTree1(World world, int x, int y) {
    char buff[30];
    snprintf(buff, sizeof(buff), "data/assets/objects/tree%d.png", rand() % 8 + 1);
    std::string buffAsStdStr = buff;
    return Structure(LoadTexture(buffAsStdStr.c_str())->surface, MaterialsList::GENERIC_PASSABLE);
}

PlacedStructure::PlacedStructure(Structure base, int x, int y) {
    this->base = base;
    this->x = x;
    this->y = y;
}

// #include "Populator.hpp"
// #include "Textures.hpp"
// #include <string>
// #include "structures.hpp"

// MetaEngine::vector<PlacedStructure> Populator::apply(MaterialInstance* tiles, Chunk ch, World world){

// 	for (int x = 0; x < CHUNK_W; x++) {
// 		for (int y = 0; y < CHUNK_H; y++) {
// 			int px = x + ch.x * CHUNK_W;
// 			int py = y + ch.y * CHUNK_H;
// 			if (tiles[x + y * CHUNK_W].mat.physicsType == PhysicsType::SOLID && tiles[x + y * CHUNK_W].mat.id != Materials::CLOUD.id) {
// 				F64 n = world.perlin.noise(px / 64.0, py / 64.0, 3802);
// 				F64 n2 = world.perlin.noise(px / 150.0, py / 150.0, 6213);
// 				F64 ndetail = world.perlin.noise(px / 16.0, py / 16.0, 5319) * 0.1;
// 				if (n2 + n + ndetail < std::fmin(0.95, (py) / 1000.0)) {
// 					F64 nlav = world.perlin.noise(px / 250.0, py / 250.0, 7018);
// 					if (nlav > 0.7) {
// 						tiles[x + y * CHUNK_W] = rand() % 3 == 0 ? (ch.y > 5 ? TilesCreateLava() : TilesCreateWater()) : Tiles_NOTHING;
// 					}
// 					else {
// 						tiles[x + y * CHUNK_W] = Tiles_NOTHING;
// 					}
// 				}
// 				else {
// 					F64 n3 = world.perlin.noise(px / 64.0, py / 64.0, 9828);
// 					if (n3 - 0.25 > py / 1000.0) {
// 						tiles[x + y * CHUNK_W] = Tiles_NOTHING;
// 					}
// 				}
// 			}

// 			if (tiles[x + y * CHUNK_W].mat.id == Materials::SMOOTH_STONE.id) {
// 				F64 n = world.perlin.noise(px / 48.0, py / 48.0, 5124);
// 				if (n < 0.25) tiles[x + y * CHUNK_W] = TilesCreateIron(px, py);
// 			}

// 			if (tiles[x + y * CHUNK_W].mat.id == Materials::SMOOTH_STONE.id) {
// 				F64 n = world.perlin.noise(px / 32.0, py / 32.0, 7513);
// 				if (n < 0.20) tiles[x + y * CHUNK_W] = TilesCreateGold(px, py);
// 			}

// 			MaterialInstance prop = tiles[x + y * CHUNK_W];
// 			if (prop.mat.id == Materials::SMOOTH_STONE.id) {
// 				int dist = 6 + world.perlin.noise(px / 10.0, py / 10.0, 3323) * 5 + 5;
// 				for (int dx = -dist; dx <= dist; dx++) {
// 					for (int dy = -dist; dy <= dist; dy++) {
// 						if (x + dx >= 0 && x + dx < CHUNK_W && y + dy >= 0 && y + dy < CHUNK_H) {
// 							if (tiles[(x + dx) + (y + dy) * CHUNK_W].mat.physicsType == PhysicsType::AIR || (tiles[(x + dx) + (y + dy) * CHUNK_W].mat.physicsType == PhysicsType::SAND && tiles[(x + dx)
// + (y
// + dy)
// * CHUNK_W].mat.id != Materials::SOFT_DIRT.id)) { 								tiles[x + y * CHUNK_W] = TilesCreateCobbleStone(px, py); 								goto nextTile;
// 							}
// 						}
// 					}
// 				}
// 			}
// 			else if (prop.mat.id == Materials::SMOOTH_DIRT.id) {
// 				int dist = 6 + world.perlin.noise(px / 10.0, py / 10.0, 3323) * 5 + 5;
// 				for (int dx = -dist; dx <= dist; dx++) {
// 					for (int dy = -dist; dy <= dist; dy++) {
// 						if (x + dx >= 0 && x + dx < CHUNK_W && y + dy >= 0 && y + dy < CHUNK_H) {
// 							if (tiles[(x + dx) + (y + dy) * CHUNK_W].mat.physicsType == PhysicsType::AIR || (tiles[(x + dx) + (y + dy) * CHUNK_W].mat.physicsType == PhysicsType::SAND && tiles[(x + dx)
// + (y
// + dy)
// * CHUNK_W].mat.id != Materials::SOFT_DIRT.id)) { 								tiles[x + y * CHUNK_W] = TilesCreateCobbleDirt(px, py); 								goto nextTile;
// 							}
// 						}
// 					}
// 				}
// 			}

// 		nextTile: {}

// 		}
// 	}

// 	MetaEngine::vector<PlacedStructure> structs;
// 	//if (ch.x % 2 == 0) {
// 	//	TileProperties* str = new TileProperties[100 * 50];
// 	//	for (int x = 0; x < 20; x++) {
// 	//		for (int y = 0; y < 8; y++) {
// 	//			str[x + y * 20] = Tiles_TEST_SOLID;
// 	//		}
// 	//	}
// 	//	PlacedStructure* ps = new PlacedStructure(Structure(20, 8, str), ch.x * CHUNK_W - 10, ch.y * CHUNK_H - 4);
// 	//	//world.addParticle(new Particle(Tiles_TEST_SAND, ch.x * CHUNK_W, ch.y * CHUNK_H, 0, 0, 0, 1));
// 	//	structs.push_back(*ps);
// 	//	//std::cout << "placestruct " << world.structures.size() << std::endl;
// 	//}

// 	//if (ch.x % 2 == 0) {
// 	//	TileProperties* str = new TileProperties[100 * 50];
// 	//	for (int x = 0; x < 100; x++) {
// 	//		for (int y = 0; y < 50; y++) {
// 	//			if (x == 0 || x == 99 || y == 0 || y == 49) {
// 	//				str[x + y * 100] = Tiles_TEST_SOLID;
// 	//			}else {
// 	//				str[x + y * 100] = Tiles_NOTHING;
// 	//			}
// 	//		}
// 	//	}
// 	//	PlacedStructure* ps = new PlacedStructure(Structure(100, 50, str), ch.x * CHUNK_W - 50, ch.y * CHUNK_H - 25);
// 	//	//world.addParticle(new Particle(Tiles_TEST_SAND, ch.x * CHUNK_W, ch.y * CHUNK_H, 0, 0, 0, 1));
// 	//	structs.push_back(*ps);
// 	//	//std::cout << "placestruct " << world.structures.size() << std::endl;
// 	//}

// 	if (ch.y < 2 && rand() % 2 == 0) {
// 		//TileProperties* str = new TileProperties[100 * 50];
// 		int posX = ch.x * CHUNK_W + (rand() % CHUNK_W);
// 		int posY = ch.y * CHUNK_H + (rand() % CHUNK_H);
// 		/*for (int x = 0; x < 100; x++) {
// 			for (int y = 0; y < 50; y++) {
// 				str[x + y * 100] = TilesCreateCloud(x + posX + ch.x * CHUNK_W, y + posY + ch.y * CHUNK_H);
// 			}
// 		}*/
// 		std::string m = "data/assets/objects/cloud_";
// 		m.append(std::to_string(rand() % 11));
// 		m.append(".png");
// 		Structure st = Structure(LoadTexture(m, SDL_PIXELFORMAT_ARGB8888), Materials::CLOUD);
// 		PlacedStructure* ps = new PlacedStructure(st, posX, posY);
// 		//world.addParticle(new Particle(Tiles_TEST_SAND, ch.x * CHUNK_W, ch.y * CHUNK_H, 0, 0, 0, 1));
// 		structs.push_back(*ps);
// 		//std::cout << "placestruct " << world.structures.size() << std::endl;
// 	}

// 	F32 treePointsScale = 2000;
// 	MetaEngine::vector<b2Vec2> treePts = world.getPointsWithin((ch.x - 1) * CHUNK_W / treePointsScale, (ch.y - 1) * CHUNK_H / treePointsScale, CHUNK_W * 3 / treePointsScale, CHUNK_H * 3 /
// treePointsScale); 	Structure tree = Structures::makeTree1(world, ch.x * CHUNK_W, ch.y * CHUNK_H); 	std::cout << treePts.size() << std::endl; 	for (int i = 0; i < treePts.size(); i++) { 		int
// px = treePts[i].x * treePointsScale - ch.x * CHUNK_W; 		int py = treePts[i].y * treePointsScale - ch.y * CHUNK_H;

// 		for (int xx = 0; xx < tree.w; xx++) {
// 			for (int yy = 0; yy < tree.h; yy++) {
// 				if (px + xx >= 0 && px + xx < CHUNK_W && py + yy >= 0 && py + yy < CHUNK_H) {
// 					if (tree.tiles[xx + yy * tree.w].mat.physicsType != PhysicsType::AIR) {
// 						tiles[(px + xx) + (py + yy) * CHUNK_W] = tree.tiles[xx + yy * tree.w];
// 					}
// 				}
// 			}
// 		}
// 	}

// 	return structs;
// }

MetaEngine::vector<PlacedStructure> TestPhase1Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xff0000);
        }
    }

    return {};
}

MetaEngine::vector<PlacedStructure> TestPhase2Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0x00ff00);
        }
    }

    return {};
}

MetaEngine::vector<PlacedStructure> TestPhase3Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0x0000ff);
        }
    }

    return {};
}

MetaEngine::vector<PlacedStructure> TestPhase4Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xffff00);
        }
    }

    return {};
}

MetaEngine::vector<PlacedStructure> TestPhase5Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xff00ff);
        }
    }

    return {};
}

MetaEngine::vector<PlacedStructure> TestPhase6Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0x00ffff);
        }
    }

    return {};
}

MetaEngine::vector<PlacedStructure> TestPhase0Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 10; x < 20; x++) {
        for (int y = 10; y < 20; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xffffff);
        }
    }

    return {};
}

MetaEngine::vector<PlacedStructure> CavePopulator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world) {

    if (ch->y < 0) return {};
    for (int x = 0; x < CHUNK_W; x++) {
        for (int y = 0; y < CHUNK_H; y++) {
            int px = x + ch->x * CHUNK_W;
            int py = y + ch->y * CHUNK_H;
            if (chunk[x + y * CHUNK_W].mat->physicsType == PhysicsType::SOLID && chunk[x + y * CHUNK_W].mat->id != MaterialsList::CLOUD.id) {
                F64 n = (world->noise.GetPerlin(px * 1.5, py * 1.5, 3802) + 1) / 2;
                F64 n2 = (world->noise.GetPerlin(px / 3.0, py / 3.0, 6213) + 1) / 2;
                F64 ndetail = (world->noise.GetPerlin(px * 8.0, py * 8.0, 5319) + 1) / 2 * 0.08;

                if (n2 + n + ndetail < std::fmin(0.95, (py) / 1000.0)) {
                    F64 nlav = world->noise.GetPerlin(px / 4.0, py / 4.0, 7018);
                    if (nlav > 0.45) {
                        chunk[x + y * CHUNK_W] = rand() % 3 == 0 ? (ch->y > 15 ? TilesCreateLava() : TilesCreateWater()) : Tiles_NOTHING;
                    } else {
                        chunk[x + y * CHUNK_W] = Tiles_NOTHING;
                    }
                } else {
                    F64 n3 = world->noise.GetPerlin(px / 64.0, py / 64.0, 9828);
                    if (n3 - 0.25 > py / 1000.0) {
                        chunk[x + y * CHUNK_W] = Tiles_NOTHING;
                    }
                }
            }
        }
    }
    return {};
}

MetaEngine::vector<PlacedStructure> CobblePopulator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world) {

    if (ch->y < 0) return {};

    // int dist = 8 + (world->noise.GetNoise(px * 4, py * 4, 3323) + 1) / 2 * 12;
    // int dist2 = dist - 6;

    for (int sy = ty + CHUNK_H - 20; sy < ty + th - CHUNK_H + 20; sy++) {
        int gapx = 0;
        for (int sx = tx + CHUNK_W - 20; sx < tx + tw - CHUNK_W + 20; sx++) {
            int rx = sx - ch->x * CHUNK_W;
            int ry = sy - ch->y * CHUNK_H;

            int chx = 1;
            int chy = 1;
            int dxx = rx;
            int dyy = ry;
            if (dxx < 0) {
                chx--;
                dxx += CHUNK_W;
            } else if (dxx >= CHUNK_W) {
                chx++;
                dxx -= CHUNK_W;
            }

            if (dyy < 0) {
                chy--;
                dyy += CHUNK_H;
            } else if (dyy >= CHUNK_H) {
                chy++;
                dyy -= CHUNK_H;
            }

            if (area[chx + chy * 3]->tiles[(dxx) + (dyy)*CHUNK_W].mat->physicsType == PhysicsType::AIR ||
                (area[chx + chy * 3]->tiles[(dxx) + (dyy)*CHUNK_W].mat->physicsType == PhysicsType::SAND && area[chx + chy * 3]->tiles[(dxx) + (dyy)*CHUNK_W].mat->id != MaterialsList::SOFT_DIRT.id)) {
                if (gapx > 0) {
                    gapx--;
                    continue;
                }
                gapx = 4;

                int dist = 8 + (world->noise.GetNoise(sx * 4, sy * 4, 3323) + 1) / 2 * 12;
                int dist2 = dist - 6;

                for (int dx = -dist; dx <= dist; dx++) {
                    int sdxx = rx + dx;
                    if (sdxx < 0) continue;
                    if (sdxx >= CHUNK_W) break;
                    for (int dy = -dist; dy <= dist; dy++) {
                        int sdyy = ry + dy;
                        if (sdyy < 0) continue;
                        if (sdyy >= CHUNK_H) break;

                        if (dx >= -dist2 && dx <= dist2 && dy >= -dist2 && dy <= dist2) {
                            if (area[1 + 1 * 3]->tiles[(sdxx) + (sdyy)*CHUNK_W].mat->id == MaterialsList::SMOOTH_STONE.id ||
                                area[1 + 1 * 3]->tiles[(sdxx) + (sdyy)*CHUNK_W].mat->id == MaterialsList::FLAT_COBBLE_STONE.id) {

                                chunk[sdxx + sdyy * CHUNK_W] = TilesCreateCobbleStone(sx + dx, sy + dy);

                            } else if (area[1 + 1 * 3]->tiles[(sdxx) + (sdyy)*CHUNK_W].mat->id == MaterialsList::SMOOTH_DIRT.id ||
                                       area[1 + 1 * 3]->tiles[(sdxx) + (sdyy)*CHUNK_W].mat->id == MaterialsList::FLAT_COBBLE_DIRT.id) {

                                chunk[sdxx + sdyy * CHUNK_W] = TilesCreateCobbleDirt(sx + dx, sy + dy);
                            }
                        } else {
                            if (area[1 + 1 * 3]->tiles[(sdxx) + (sdyy)*CHUNK_W].mat->id == MaterialsList::SMOOTH_STONE.id) {

                                chunk[sdxx + sdyy * CHUNK_W] = TilesCreate(&MaterialsList::FLAT_COBBLE_STONE, sx + dx, sy + dy);

                            } else if (area[1 + 1 * 3]->tiles[(sdxx) + (sdyy)*CHUNK_W].mat->id == MaterialsList::SMOOTH_DIRT.id) {

                                chunk[sdxx + sdyy * CHUNK_W] = TilesCreate(&MaterialsList::FLAT_COBBLE_DIRT, sx + dx, sy + dy);
                            }
                        }
                    }
                }

            } else {
                gapx = 0;
            }
        }
    }

    return {};
}

MetaEngine::vector<PlacedStructure> OrePopulator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world) {

    if (ch->y < 0) return {};
    for (int x = 0; x < CHUNK_W; x++) {
        for (int y = 0; y < CHUNK_H; y++) {
            int px = x + ch->x * CHUNK_W;
            int py = y + ch->y * CHUNK_H;

            MaterialInstance prop = chunk[x + y * CHUNK_W];
            if (chunk[x + y * CHUNK_W].mat->id == MaterialsList::SMOOTH_STONE.id) {
                F64 n = (world->noise.GetNoise(px * 1.7, py * 1.7, 5124) + 1) / 2;
                if (n < 0.25) chunk[x + y * CHUNK_W] = TilesCreateIron(px, py);
            }

            if (chunk[x + y * CHUNK_W].mat->id == MaterialsList::SMOOTH_STONE.id) {
                F64 n = (world->noise.GetNoise(px * 2, py * 2, 7513) + 1) / 2;
                if (n < 0.20) chunk[x + y * CHUNK_W] = TilesCreateGold(px, py);
            }
        }
    }

    return {};
}

MetaEngine::vector<PlacedStructure> TreePopulator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world) {
    if (ch->y < 0 || ch->y > 3) return {};
    int x = (rand() % (CHUNK_W / 2) + (CHUNK_W / 4)) * 1;
    if (area[1 + 2 * 3]->tiles[x + 0 * CHUNK_W].mat->id == MaterialsList::SOFT_DIRT.id) return {};

    for (int y = 0; y < CHUNK_H; y++) {
        if (area[1 + 2 * 3]->tiles[x + y * CHUNK_W].mat->id == MaterialsList::SOFT_DIRT.id) {
            int px = x + ch->x * CHUNK_W;
            int py = y + (ch->y + 1) * CHUNK_W;

            // Structure tree = Structures::makeTree1(*world, px, py);
            // px -= tree.w / 2;
            // py -= tree.h - 2;

            // for (int tx = 0; tx < tree.w; tx++) {
            //     for (int ty = 0; ty < tree.h; ty++) {
            //         int chx = (int) floor((tx + px) / (F32) CHUNK_W) + 1 - ch->x;
            //         int chy = (int) floor((ty + py) / (F32) CHUNK_H) + 1 - ch->y;
            //         if (chx < 0 || chy < 0 || chx > 2 || chy > 2) continue;
            //         int dxx = (CHUNK_W + ((tx + px) % CHUNK_W)) % CHUNK_W;
            //         int dyy = (CHUNK_H + ((ty + py) % CHUNK_H)) % CHUNK_H;
            //         if (tree.tiles[tx + ty * tree.w].mat->physicsType != PhysicsType::AIR &&
            //             area[chx + chy * 3]->tiles[dxx + dyy * CHUNK_W].mat->physicsType ==
            //                     PhysicsType::AIR &&
            //             area[chx + chy * 3]->layer2[dxx + dyy * CHUNK_W].mat->physicsType ==
            //                     PhysicsType::AIR) {
            //             area[chx + chy * 3]->layer2[dxx + dyy * CHUNK_W] =
            //                     tree.tiles[tx + ty * tree.w];
            //             dirty[chx + chy * 3] = true;
            //         }
            //     }
            // }

            char buff[40];
            snprintf(buff, sizeof(buff), "data/assets/objects/tree%d.png", rand() % 8 + 1);
            C_Surface *tex = LoadTexture(buff)->surface;

            px -= tex->w / 2;
            py -= tex->h - 2;

            b2PolygonShape s;
            s.SetAsBox(1, 1);
            RigidBody *rb = world->makeRigidBody(b2_dynamicBody, px, py, 0, s, 1, 0.3, tex);
            for (int texX = 0; texX < tex->w; texX++) {
                b2Filter bf = {};
                bf.categoryBits = 0x0002;
                bf.maskBits = 0x0001;
                rb->body->GetFixtureList()[0].SetFilterData(bf);
                if (((R_GET_PIXEL(tex, texX, tex->h - 1) >> 24) & 0xff) != 0x00) {
                    rb->weldX = texX;
                    rb->weldY = tex->h - 1;
                    break;
                }
            }
            world->WorldIsolate_.rigidBodies.push_back(rb);
            world->updateRigidBodyHitbox(rb);

            return {};
        }
    }
    return {};
}

RigidBody::RigidBody(b2Body *body, std::string name) {
    this->body = body;
    this->name = std::string(name);
}

RigidBody::~RigidBody() {
    // if (item) delete item;
}

void Player::render(R_Target *target, int ofsX, int ofsY) {
    if (heldItem != NULL) {
        int scaleEnt = global.game->GameIsolate_.globaldef.hd_objects ? global.game->GameIsolate_.globaldef.hd_objects_size : 1;

        metadot_rect *ir = new metadot_rect{(F32)(int)(ofsX + x + hw / 2.0 - heldItem->surface->w), (F32)(int)(ofsY + y + hh / 2.0 - heldItem->surface->h / 2), (F32)heldItem->surface->w,
                                            (F32)heldItem->surface->h};
        F32 fx = (F32)(int)(-ir->x + ofsX + x + hw / 2.0);
        F32 fy = (F32)(int)(-ir->y + ofsY + y + hh / 2.0);
        fx -= heldItem->pivotX;
        ir->x += heldItem->pivotX;
        fy -= heldItem->pivotY;
        ir->y += heldItem->pivotY;
        R_SetShapeBlendMode(R_BlendPresetEnum::R_BLEND_ADD);
        // R_BlitTransformX(heldItem->texture, NULL, target, ir->x, ir->y, fp->x, fp->y, holdAngle, 1, 1);
        // SDL_RenderCopyExF(renderer, heldItem->texture, NULL, ir, holdAngle, fp, abs(holdAngle) > 90 ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);
        ir->x *= scaleEnt;
        ir->y *= scaleEnt;
        ir->w *= scaleEnt;
        ir->h *= scaleEnt;
        R_BlitRectX(heldItem->texture, NULL, target, ir, holdAngle, fx, fy, abs(holdAngle) > 90 ? R_FLIP_VERTICAL : R_FLIP_NONE);
        delete ir;
    }
}

void Player::renderLQ(R_Target *target, int ofsX, int ofsY) { R_Rectangle(target, x + ofsX, y + ofsY, x + ofsX + hw, y + ofsY + hh, {0xff, 0xff, 0xff, 0xff}); }

b2Vec2 rotate_point2(F32 cx, F32 cy, F32 angle, b2Vec2 p);

void Player::setItemInHand(Item *item, World *world) {
    RigidBody *r;
    if (heldItem != NULL) {
        b2PolygonShape ps;
        ps.SetAsBox(1, 1);

        F32 angle = holdAngle;

        b2Vec2 pt = rotate_point2(0, 0, angle * 3.1415 / 180.0, {(F32)(heldItem->surface->w / 2.0), (F32)(heldItem->surface->h / 2.0)});

        r = world->makeRigidBody(b2_dynamicBody, x + hw / 2 + world->loadZone.x - pt.x + 16 * cos((holdAngle + 180) * 3.1415f / 180.0f),
                                 y + hh / 2 + world->loadZone.y - pt.y + 16 * sin((holdAngle + 180) * 3.1415f / 180.0f), angle, ps, 1, 0.3, heldItem->surface);

        //  0 -> -w/2 -h/2
        // 90 ->  w/2 -h/2
        // 180 ->  w/2  h/2
        // 270 -> -w/2  h/2

        F32 strength = 10;
        int time = metadot_gettime() - startThrow;

        if (time > 1000) time = 1000;

        strength += time / 1000.0 * 30;

        r->body->SetLinearVelocity({(F32)(strength * (F32)cos((holdAngle + 180) * 3.1415f / 180.0f)), (F32)(strength * (F32)sin((holdAngle + 180) * 3.1415f / 180.0f)) - 10});

        b2Filter bf = {};
        bf.categoryBits = 0x0001;
        // bf.maskBits = 0x0000;
        r->body->GetFixtureList()[0].SetFilterData(bf);

        r->item = heldItem;
        world->WorldIsolate_.rigidBodies.push_back(r);
        world->updateRigidBodyHitbox(r);
        // SDL_DestroyTexture(heldItem->texture);
    }
    auto a = 7;
    heldItem = item;
}

Player::Player() : WorldEntity(true, "Player") {}

Player::~Player() {
    if (heldItem) delete heldItem;
}

b2Vec2 rotate_point2(F32 cx, F32 cy, F32 angle, b2Vec2 p) {
    F32 s = sin(angle);
    F32 c = cos(angle);

    // translate to origin
    p.x -= cx;
    p.y -= cy;

    // rotate
    F32 xn = p.x * c - p.y * s;
    F32 yn = p.x * s + p.y * c;

    // translate back
    return b2Vec2(xn + cx, yn + cy);
}

void InitGlobalDEF(GlobalDEF *_struct, bool openDebugUIs) {

    auto GlobalDEF = Scripts::GetSingletonPtr()->LuaCoreCpp->s_lua["global_def"];

    if (!GlobalDEF.isNilref()) {
        LoadLuaConfig(_struct, GlobalDEF, draw_frame_graph);
        LoadLuaConfig(_struct, GlobalDEF, draw_background);
        LoadLuaConfig(_struct, GlobalDEF, draw_background_grid);
        LoadLuaConfig(_struct, GlobalDEF, draw_load_zones);
        LoadLuaConfig(_struct, GlobalDEF, draw_physics_debug);
        LoadLuaConfig(_struct, GlobalDEF, draw_b2d_shape);
        LoadLuaConfig(_struct, GlobalDEF, draw_b2d_joint);
        LoadLuaConfig(_struct, GlobalDEF, draw_b2d_aabb);
        LoadLuaConfig(_struct, GlobalDEF, draw_b2d_pair);
        LoadLuaConfig(_struct, GlobalDEF, draw_b2d_centerMass);
        LoadLuaConfig(_struct, GlobalDEF, draw_chunk_state);
        LoadLuaConfig(_struct, GlobalDEF, draw_debug_stats);
        LoadLuaConfig(_struct, GlobalDEF, draw_material_info);
        LoadLuaConfig(_struct, GlobalDEF, draw_detailed_material_info);
        LoadLuaConfig(_struct, GlobalDEF, draw_uinode_bounds);
        LoadLuaConfig(_struct, GlobalDEF, draw_temperature_map);
        LoadLuaConfig(_struct, GlobalDEF, draw_cursor);
        LoadLuaConfig(_struct, GlobalDEF, ui_tweak);
        LoadLuaConfig(_struct, GlobalDEF, draw_shaders);
        LoadLuaConfig(_struct, GlobalDEF, water_overlay);
        LoadLuaConfig(_struct, GlobalDEF, water_showFlow);
        LoadLuaConfig(_struct, GlobalDEF, water_pixelated);
        LoadLuaConfig(_struct, GlobalDEF, lightingQuality);
        LoadLuaConfig(_struct, GlobalDEF, draw_light_overlay);
        LoadLuaConfig(_struct, GlobalDEF, simpleLighting);
        LoadLuaConfig(_struct, GlobalDEF, lightingEmission);
        LoadLuaConfig(_struct, GlobalDEF, lightingDithering);
        LoadLuaConfig(_struct, GlobalDEF, tick_world);
        LoadLuaConfig(_struct, GlobalDEF, tick_box2d);
        LoadLuaConfig(_struct, GlobalDEF, tick_temperature);
        LoadLuaConfig(_struct, GlobalDEF, hd_objects);
        LoadLuaConfig(_struct, GlobalDEF, hd_objects_size);
        LoadLuaConfig(_struct, GlobalDEF, draw_ui_debug);
        LoadLuaConfig(_struct, GlobalDEF, draw_imgui_debug);
        LoadLuaConfig(_struct, GlobalDEF, draw_profiler);

    } else {
        METADOT_ERROR("GlobalDEF WAS NULL");
    }

    gameUI.visible_debugdraw = openDebugUIs;
    _struct->draw_frame_graph = openDebugUIs;
    if (!openDebugUIs) {
        _struct->draw_background = true;
        _struct->draw_background_grid = false;
        _struct->draw_load_zones = false;
        _struct->draw_physics_debug = false;
        _struct->draw_chunk_state = false;
        _struct->draw_debug_stats = false;
        _struct->draw_detailed_material_info = false;
        _struct->draw_temperature_map = false;
    }

    METADOT_INFO("SettingsData loaded");
}

void LoadGlobalDEF(std::string globaldef_src) {}

void SaveGlobalDEF(std::string globaldef_src) {
    // std::string settings_data = "GetSettingsData = function()\nsettings_data = {}\n";
    // SaveLuaConfig(*_struct, "settings_data", settings_data);
    // settings_data += "return settings_data\nend";
    // std::ofstream o(globaldef_src);
    // o << settings_data;
}
