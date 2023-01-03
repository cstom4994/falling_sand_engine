// Copyright(c) 2022, KaoruXun All rights reserved.

#include "game_datastruct.hpp"

#include <string>

#include "core/core.h"
#include "core/core.hpp"
#include "core/debug_impl.hpp"
#include "core/global.hpp"
#include "core/macros.h"
#include "engine/filesystem.h"
#include "engine/internal/builtin_box2d.h"
#include "engine/reflectionflat.hpp"
#include "engine/renderer/renderer_utils.h"
#include "game/chunk.hpp"
#include "game/game.hpp"
#include "game/game_datastruct.hpp"
#include "game/game_resources.hpp"
#include "game/game_ui.hpp"
#include "game/utils.hpp"
#include "scripting/lua_wrapper.hpp"
#include "world.hpp"

GameData GameData_;

void ReleaseGameData() {
    for (auto b : GameData_.biome_container) {
        if (static_cast<bool>(b)) delete b;
    }
}

WorldEntity::WorldEntity(bool isplayer) : is_player(isplayer) {}

WorldEntity::~WorldEntity() {
    // if (static_cast<bool>(rb)) delete rb;
}

#pragma region Material

Material::Material(int id, std::string name, int physicsType, int slipperyness, U8 alpha, float density, int iterations, int emit, U32 emitColor, U32 color) {
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
Material Materials::GENERIC_AIR = Material(nMaterials++, "_AIR", PhysicsType::AIR, 0, 255, 0, 0, 16, 0);
Material Materials::GENERIC_SOLID = Material(nMaterials++, "_SOLID", PhysicsType::SOLID, 0, 255, 1, 0, 0, 0);
Material Materials::GENERIC_SAND = Material(nMaterials++, "_SAND", PhysicsType::SAND, 20, 255, 10, 2, 0, 0);
Material Materials::GENERIC_LIQUID = Material(nMaterials++, "_LIQUID", PhysicsType::SOUP, 0, 255, 1.5, 3, 0, 0);
Material Materials::GENERIC_GAS = Material(nMaterials++, "_GAS", PhysicsType::GAS, 0, 255, -1, 1, 0, 0);
Material Materials::GENERIC_PASSABLE = Material(nMaterials++, "_PASSABLE", PhysicsType::PASSABLE, 0, 255, 0, 0, 0, 0);
Material Materials::GENERIC_OBJECT = Material(nMaterials++, "_OBJECT", PhysicsType::OBJECT, 0, 255, 1000.0, 0, 0, 0);

Material Materials::TEST_SAND = Material(nMaterials++, "Test Sand", PhysicsType::SAND, 20, 255, 10, 2, 0, 0);
Material Materials::TEST_TEXTURED_SAND = Material(nMaterials++, "Test Textured Sand", PhysicsType::SAND, 20, 255, 10, 2, 0, 0);
Material Materials::TEST_LIQUID = Material(nMaterials++, "Test Liquid", PhysicsType::SOUP, 0, 255, 1.5, 4, 0, 0);

Material Materials::STONE = Material(nMaterials++, "Stone", PhysicsType::SOLID, 0, 1, 0);
Material Materials::GRASS = Material(nMaterials++, "Grass", PhysicsType::SAND, 20, 12, 1);
Material Materials::DIRT = Material(nMaterials++, "Dirt", PhysicsType::SAND, 8, 15, 1);

Material Materials::SMOOTH_STONE = Material(nMaterials++, "Stone", PhysicsType::SOLID, 0, 1, 0);
Material Materials::COBBLE_STONE = Material(nMaterials++, "Cobblestone", PhysicsType::SOLID, 0, 1, 0);
Material Materials::SMOOTH_DIRT = Material(nMaterials++, "Ground", PhysicsType::SOLID, 0, 1, 0);
Material Materials::COBBLE_DIRT = Material(nMaterials++, "Hard Ground", PhysicsType::SOLID, 0, 1, 0);
Material Materials::SOFT_DIRT = Material(nMaterials++, "Dirt", PhysicsType::SOLID, 0, 15, 2);

Material Materials::WATER = Material(nMaterials++, "Water", PhysicsType::SOUP, 0, 0x80, 1.5, 6, 40, 0x3000AFB5);
Material Materials::LAVA = Material(nMaterials++, "Lava", PhysicsType::SOUP, 0, 0xC0, 2, 1, 40, 0xFFFF6900);

Material Materials::CLOUD = Material(nMaterials++, "Cloud", PhysicsType::SOLID, 0, 127, 1, 0);

Material Materials::GOLD_ORE = Material(nMaterials++, "Gold Ore", PhysicsType::SAND, 20, 255, 20, 2, 8, 0x804000);
Material Materials::GOLD_MOLTEN = Material(nMaterials++, "Molten Gold", PhysicsType::SOUP, 0, 255, 20, 2, 8, 0x6FFF9B40);
Material Materials::GOLD_SOLID = Material(nMaterials++, "Solid Gold", PhysicsType::SOLID, 0, 255, 20, 2, 8, 0);

Material Materials::IRON_ORE = Material(nMaterials++, "Iron Ore", PhysicsType::SAND, 20, 255, 20, 2, 8, 0x7F442F);

Material Materials::OBSIDIAN = Material(nMaterials++, "Obsidian", PhysicsType::SOLID, 0, 255, 1, 0, 0, 0);
Material Materials::STEAM = Material(nMaterials++, "Steam", PhysicsType::GAS, 0, 255, -1, 1, 0, 0);

Material Materials::SOFT_DIRT_SAND = Material(nMaterials++, "Soft Dirt Sand", PhysicsType::SAND, 8, 15, 2);

Material Materials::FIRE = Material(nMaterials++, "Fire", PhysicsType::PASSABLE, 0, 255, 20, 1, 0, 0);

Material Materials::FLAT_COBBLE_STONE = Material(nMaterials++, "Flat Cobblestone", PhysicsType::SOLID, 0, 1, 0);
Material Materials::FLAT_COBBLE_DIRT = Material(nMaterials++, "Flat Hard Ground", PhysicsType::SOLID, 0, 1, 0);

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

        U32 rgb = rand() % 255;
        rgb = (rgb << 8) + rand() % 255;
        rgb = (rgb << 8) + rand() % 255;

        int type = rand() % 2 == 0 ? (rand() % 2 == 0 ? PhysicsType::SAND : PhysicsType::GAS) : PhysicsType::SOUP;
        float dens = 0;
        if (type == PhysicsType::SAND) {
            dens = 5 + (rand() % 1000) / 1000.0;
        } else if (type == PhysicsType::SOUP) {
            dens = 4 + (rand() % 1000) / 1000.0;
        } else if (type == PhysicsType::GAS) {
            dens = 3 + (rand() % 1000) / 1000.0;
        }
        randMats[i] = Material(nMaterials++, buff, type, 10, type == PhysicsType::SAND ? 255 : (rand() % 192 + 63), dens, rand() % 4 + 1, 0, 0, rgb);
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

MaterialInstance::MaterialInstance(Material *mat, U32 color, int32_t temperature) {
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
    U32 rgb = 220;
    rgb = (rgb << 8) + 155 + rand() % 30;
    rgb = (rgb << 8) + 100;
    return MaterialInstance(&Materials::TEST_SAND, rgb);
}

MaterialInstance Tiles::createTestTexturedSand(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->testTexture;

    int tx = x % tex->w;
    int ty = y % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);
    return MaterialInstance(&Materials::TEST_TEXTURED_SAND, rgb);
}

MaterialInstance Tiles::createTestLiquid() {
    U32 rgb = 0;
    rgb = (rgb << 8) + 0;
    rgb = (rgb << 8) + 255;
    return MaterialInstance(&Materials::TEST_LIQUID, rgb);
}

MaterialInstance Tiles::createStone(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->cobbleStone;

    int tx = x % tex->w;
    int ty = y % tex->h;

    U8 *pixel = (U8 *)tex->pixels;

    pixel += ((ty)*tex->pitch) + ((tx) * sizeof(U32));
    U32 rgb = *((U32 *)pixel);

    return MaterialInstance(&Materials::STONE, rgb);
}

MaterialInstance Tiles::createGrass() {
    U32 rgb = 40;
    rgb = (rgb << 8) + 120 + rand() % 20;
    rgb = (rgb << 8) + 20;
    return MaterialInstance(&Materials::GRASS, rgb);
}

MaterialInstance Tiles::createDirt() {
    U32 rgb = 60 + rand() % 10;
    rgb = (rgb << 8) + 40;
    rgb = (rgb << 8) + 20;
    return MaterialInstance(&Materials::DIRT, rgb);
}

MaterialInstance Tiles::createSmoothStone(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->smoothStone;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::SMOOTH_STONE, rgb);
}

MaterialInstance Tiles::createCobbleStone(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->cobbleStone;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::COBBLE_STONE, rgb);
}

MaterialInstance Tiles::createSmoothDirt(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->smoothDirt;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::SMOOTH_DIRT, rgb);
}

MaterialInstance Tiles::createCobbleDirt(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->cobbleDirt;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::COBBLE_DIRT, rgb);
}

MaterialInstance Tiles::createSoftDirt(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->softDirt;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::SOFT_DIRT, rgb);
}

MaterialInstance Tiles::createWater() {
    U32 rgb = 0x00B69F;

    return MaterialInstance(&Materials::WATER, rgb, -1023);
}

MaterialInstance Tiles::createLava() {
    U32 rgb = 0xFF7C00;

    return MaterialInstance(&Materials::LAVA, rgb, 1024);
}

MaterialInstance Tiles::createCloud(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->cloud;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::CLOUD, rgb);
}

MaterialInstance Tiles::createGold(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->gold;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::GOLD_ORE, rgb);
}

MaterialInstance Tiles::createIron(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->iron;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::IRON_ORE, rgb);
}

MaterialInstance Tiles::createObsidian(int x, int y) {
    C_Surface *tex = global.game->GameIsolate_.texturepack->obsidian;

    int tx = (tex->w + (x % tex->w)) % tex->w;
    int ty = (tex->h + (y % tex->h)) % tex->h;

    U32 rgb = R_GET_PIXEL(tex, tx, ty);

    return MaterialInstance(&Materials::OBSIDIAN, rgb);
}

MaterialInstance Tiles::createSteam() { return MaterialInstance(&Materials::STEAM, 0x666666); }

MaterialInstance Tiles::createFire() {

    U32 rgb = 255;
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
        C_Surface *tex = global.game->GameIsolate_.texturepack->goldMolten;

        int tx = (tex->w + (x % tex->w)) % tex->w;
        int ty = (tex->h + (y % tex->h)) % tex->h;

        U32 rgb = R_GET_PIXEL(tex, tx, ty);

        return MaterialInstance(&Materials::GOLD_MOLTEN, rgb);
    } else if (mat->id == Materials::GOLD_SOLID.id) {
        C_Surface *tex = global.game->GameIsolate_.texturepack->goldSolid;

        int tx = (tex->w + (x % tex->w)) % tex->w;
        int ty = (tex->h + (y % tex->h)) % tex->h;

        U32 rgb = R_GET_PIXEL(tex, tx, ty);

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
        C_Surface *tex = global.game->GameIsolate_.texturepack->flatCobbleStone;

        int tx = (tex->w + (x % tex->w)) % tex->w;
        int ty = (tex->h + (y % tex->h)) % tex->h;

        U32 rgb = R_GET_PIXEL(tex, tx, ty);

        return MaterialInstance(&Materials::FLAT_COBBLE_STONE, rgb);
    } else if (mat->id == Materials::FLAT_COBBLE_DIRT.id) {
        C_Surface *tex = global.game->GameIsolate_.texturepack->flatCobbleDirt;

        int tx = (tex->w + (x % tex->w)) % tex->w;
        int ty = (tex->h + (y % tex->h)) % tex->h;

        U32 rgb = R_GET_PIXEL(tex, tx, ty);

        return MaterialInstance(&Materials::FLAT_COBBLE_DIRT, rgb);
    }

    return MaterialInstance(mat, mat->color);
}

#pragma endregion Material

Item::Item() {}

Item::~Item() {
    R_FreeImage(texture);
    SDL_FreeSurface(surface);
}

Item *Item::makeItem(U8 flags, RigidBody *rb) {
    Item *i;

    if (rb->item != NULL) {
        i = rb->item;
        i->surface = rb->surface;
        i->texture = rb->texture;
    } else {
        i = new Item();
        i->flags = flags;
        i->surface = rb->surface;
        i->texture = rb->texture;
    }

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

            U32 color = R_GET_PIXEL(tex, x, y);

            // SDL_Color rgb;
            // U32 data = getpixel(tex, x, y);
            // SDL_GetRGB(data, tex->format, &rgb.r, &rgb.g, &rgb.b);

            // U32 j = data;

            // if (rgb.a > (U8)0)
            // {
            //     if (j - 1 > maxN) maxN = j - 1;
            //     fill.resize(maxN);
            //     fill[j - 1] = { x,y };
            // }

            if (((color >> 32) & 0xff) > 0) {
                U32 n = color & 0x00ffffff;

                // U32 t = (n * 100) / col;
                U32 t = n;

                fill[t - 1] = {x, y};
                if (t - 1 > maxN) maxN = t - 1;
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
                    tiles[x + y * texture->w] = Tiles::NOTHING;
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
            tiles[tx + ty * w] = Tiles::NOTHING;
        }
    }

    int trunk = 3 + rand() % 2;

    float cx = w / 2;
    float dcx = (((rand() % 10) / 10.0) - 0.5) / 3.0;
    for (int ty = h - 1; ty > 20; ty--) {
        int bw = trunk + std::max((ty - h + 10) / 3, 0);
        for (int xx = -bw; xx <= bw; xx++) {
            tiles[((int)cx + xx - (int)(dcx * (h - 30))) + ty * w] = MaterialInstance(&Materials::GENERIC_PASSABLE, xx >= 2 ? 0x683600 : 0x7C4000);
        }
        cx += dcx;
    }

    for (int theta = 0; theta < 360; theta += 1) {
        double p = world.noise.GetPerlin(std::cos(theta * 3.1415 / 180.0) * 4 + x, std::sin(theta * 3.1415 / 180.0) * 4 + y, 2652);
        float r = 15 + (float)p * 6;
        for (float d = 0; d < r; d += 0.5) {
            int tx = cx - (int)(dcx * (h - 30)) + d * std::cos(theta * 3.1415 / 180.0);
            int ty = 20 + d * std::sin(theta * 3.1415 / 180.0);
            if (tx >= 0 && ty >= 0 && tx < w && ty < h) {
                tiles[tx + ty * w] = MaterialInstance(&Materials::GENERIC_PASSABLE, r - d < 0.5f ? 0x00aa00 : 0x00ff00);
            }
        }
    }

    int nBranches = rand() % 3;
    bool side = rand() % 2;  // false = right, true = left
    for (int i = 0; i < nBranches; i++) {
        int yPos = 20 + (h - 20) / 3 * (i + 1) + rand() % 10;
        float tilt = ((rand() % 10) / 10.0 - 0.5) * 8;
        int len = 10 + rand() % 5;
        for (int xx = 0; xx < len; xx++) {
            int tx = (int)(w / 2 + dcx * (h - yPos)) + (side ? 1 : -1) * (xx + 2) - (int)(dcx * (h - 30));
            int th = 3 * (1 - (xx / (float)len));
            for (int yy = -th; yy <= th; yy++) {
                int ty = yPos + yy + (xx / (float)len * tilt);
                if (tx >= 0 && ty >= 0 && tx < w && ty < h) {
                    tiles[tx + ty * w] = MaterialInstance(&Materials::GENERIC_PASSABLE, yy >= 2 ? 0x683600 : 0x7C4000);
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
    return Structure(LoadTexture(buffAsStdStr.c_str()), Materials::GENERIC_PASSABLE);
}

PlacedStructure::PlacedStructure(Structure base, int x, int y) {
    this->base = base;
    this->x = x;
    this->y = y;
}

// #include "Populator.hpp"
// #include "game/Textures.hpp"
// #include <string>
// #include "structures.hpp"

// std::vector<PlacedStructure> Populator::apply(MaterialInstance* tiles, Chunk ch, World world){

// 	for (int x = 0; x < CHUNK_W; x++) {
// 		for (int y = 0; y < CHUNK_H; y++) {
// 			int px = x + ch.x * CHUNK_W;
// 			int py = y + ch.y * CHUNK_H;
// 			if (tiles[x + y * CHUNK_W].mat.physicsType == PhysicsType::SOLID && tiles[x + y * CHUNK_W].mat.id != Materials::CLOUD.id) {
// 				double n = world.perlin.noise(px / 64.0, py / 64.0, 3802);
// 				double n2 = world.perlin.noise(px / 150.0, py / 150.0, 6213);
// 				double ndetail = world.perlin.noise(px / 16.0, py / 16.0, 5319) * 0.1;
// 				if (n2 + n + ndetail < std::fmin(0.95, (py) / 1000.0)) {
// 					double nlav = world.perlin.noise(px / 250.0, py / 250.0, 7018);
// 					if (nlav > 0.7) {
// 						tiles[x + y * CHUNK_W] = rand() % 3 == 0 ? (ch.y > 5 ? Tiles::createLava() : Tiles::createWater()) : Tiles::NOTHING;
// 					}
// 					else {
// 						tiles[x + y * CHUNK_W] = Tiles::NOTHING;
// 					}
// 				}
// 				else {
// 					double n3 = world.perlin.noise(px / 64.0, py / 64.0, 9828);
// 					if (n3 - 0.25 > py / 1000.0) {
// 						tiles[x + y * CHUNK_W] = Tiles::NOTHING;
// 					}
// 				}
// 			}

// 			if (tiles[x + y * CHUNK_W].mat.id == Materials::SMOOTH_STONE.id) {
// 				double n = world.perlin.noise(px / 48.0, py / 48.0, 5124);
// 				if (n < 0.25) tiles[x + y * CHUNK_W] = Tiles::createIron(px, py);
// 			}

// 			if (tiles[x + y * CHUNK_W].mat.id == Materials::SMOOTH_STONE.id) {
// 				double n = world.perlin.noise(px / 32.0, py / 32.0, 7513);
// 				if (n < 0.20) tiles[x + y * CHUNK_W] = Tiles::createGold(px, py);
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
// * CHUNK_W].mat.id != Materials::SOFT_DIRT.id)) { 								tiles[x + y * CHUNK_W] = Tiles::createCobbleStone(px, py); 								goto nextTile;
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
// * CHUNK_W].mat.id != Materials::SOFT_DIRT.id)) { 								tiles[x + y * CHUNK_W] = Tiles::createCobbleDirt(px, py); 								goto nextTile;
// 							}
// 						}
// 					}
// 				}
// 			}

// 		nextTile: {}

// 		}
// 	}

// 	std::vector<PlacedStructure> structs;
// 	//if (ch.x % 2 == 0) {
// 	//	TileProperties* str = new TileProperties[100 * 50];
// 	//	for (int x = 0; x < 20; x++) {
// 	//		for (int y = 0; y < 8; y++) {
// 	//			str[x + y * 20] = Tiles::TEST_SOLID;
// 	//		}
// 	//	}
// 	//	PlacedStructure* ps = new PlacedStructure(Structure(20, 8, str), ch.x * CHUNK_W - 10, ch.y * CHUNK_H - 4);
// 	//	//world.addParticle(new Particle(Tiles::TEST_SAND, ch.x * CHUNK_W, ch.y * CHUNK_H, 0, 0, 0, 1));
// 	//	structs.push_back(*ps);
// 	//	//std::cout << "placestruct " << world.structures.size() << std::endl;
// 	//}

// 	//if (ch.x % 2 == 0) {
// 	//	TileProperties* str = new TileProperties[100 * 50];
// 	//	for (int x = 0; x < 100; x++) {
// 	//		for (int y = 0; y < 50; y++) {
// 	//			if (x == 0 || x == 99 || y == 0 || y == 49) {
// 	//				str[x + y * 100] = Tiles::TEST_SOLID;
// 	//			}else {
// 	//				str[x + y * 100] = Tiles::NOTHING;
// 	//			}
// 	//		}
// 	//	}
// 	//	PlacedStructure* ps = new PlacedStructure(Structure(100, 50, str), ch.x * CHUNK_W - 50, ch.y * CHUNK_H - 25);
// 	//	//world.addParticle(new Particle(Tiles::TEST_SAND, ch.x * CHUNK_W, ch.y * CHUNK_H, 0, 0, 0, 1));
// 	//	structs.push_back(*ps);
// 	//	//std::cout << "placestruct " << world.structures.size() << std::endl;
// 	//}

// 	if (ch.y < 2 && rand() % 2 == 0) {
// 		//TileProperties* str = new TileProperties[100 * 50];
// 		int posX = ch.x * CHUNK_W + (rand() % CHUNK_W);
// 		int posY = ch.y * CHUNK_H + (rand() % CHUNK_H);
// 		/*for (int x = 0; x < 100; x++) {
// 			for (int y = 0; y < 50; y++) {
// 				str[x + y * 100] = Tiles::createCloud(x + posX + ch.x * CHUNK_W, y + posY + ch.y * CHUNK_H);
// 			}
// 		}*/
// 		std::string m = "data/assets/objects/cloud_";
// 		m.append(std::to_string(rand() % 11));
// 		m.append(".png");
// 		Structure st = Structure(LoadTexture(m, SDL_PIXELFORMAT_ARGB8888), Materials::CLOUD);
// 		PlacedStructure* ps = new PlacedStructure(st, posX, posY);
// 		//world.addParticle(new Particle(Tiles::TEST_SAND, ch.x * CHUNK_W, ch.y * CHUNK_H, 0, 0, 0, 1));
// 		structs.push_back(*ps);
// 		//std::cout << "placestruct " << world.structures.size() << std::endl;
// 	}

// 	float treePointsScale = 2000;
// 	std::vector<b2Vec2> treePts = world.getPointsWithin((ch.x - 1) * CHUNK_W / treePointsScale, (ch.y - 1) * CHUNK_H / treePointsScale, CHUNK_W * 3 / treePointsScale, CHUNK_H * 3 / treePointsScale);
// 	Structure tree = Structures::makeTree1(world, ch.x * CHUNK_W, ch.y * CHUNK_H);
// 	std::cout << treePts.size() << std::endl;
// 	for (int i = 0; i < treePts.size(); i++) {
// 		int px = treePts[i].x * treePointsScale - ch.x * CHUNK_W;
// 		int py = treePts[i].y * treePointsScale - ch.y * CHUNK_H;

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

std::vector<PlacedStructure> TestPhase1Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0xff0000);
        }
    }

    return {};
}

std::vector<PlacedStructure> TestPhase2Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0x00ff00);
        }
    }

    return {};
}

std::vector<PlacedStructure> TestPhase3Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0x0000ff);
        }
    }

    return {};
}

std::vector<PlacedStructure> TestPhase4Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0xffff00);
        }
    }

    return {};
}

std::vector<PlacedStructure> TestPhase5Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0xff00ff);
        }
    }

    return {};
}

std::vector<PlacedStructure> TestPhase6Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 0; x < 10; x++) {
        for (int y = 0; y < 10; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0x00ffff);
        }
    }

    return {};
}

std::vector<PlacedStructure> TestPhase0Populator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk *area, bool *dirty, int tx, int ty, int tw, int th, Chunk ch, World *world) {
    for (int x = 10; x < 20; x++) {
        for (int y = 10; y < 20; y++) {
            chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0xffffff);
        }
    }

    return {};
}

std::vector<PlacedStructure> CavePopulator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world) {

    if (ch->y < 0) return {};
    for (int x = 0; x < CHUNK_W; x++) {
        for (int y = 0; y < CHUNK_H; y++) {
            int px = x + ch->x * CHUNK_W;
            int py = y + ch->y * CHUNK_H;
            if (chunk[x + y * CHUNK_W].mat->physicsType == PhysicsType::SOLID && chunk[x + y * CHUNK_W].mat->id != Materials::CLOUD.id) {
                double n = (world->noise.GetPerlin(px * 1.5, py * 1.5, 3802) + 1) / 2;
                double n2 = (world->noise.GetPerlin(px / 3.0, py / 3.0, 6213) + 1) / 2;
                double ndetail = (world->noise.GetPerlin(px * 8.0, py * 8.0, 5319) + 1) / 2 * 0.08;

                if (n2 + n + ndetail < std::fmin(0.95, (py) / 1000.0)) {
                    double nlav = world->noise.GetPerlin(px / 4.0, py / 4.0, 7018);
                    if (nlav > 0.45) {
                        chunk[x + y * CHUNK_W] = rand() % 3 == 0 ? (ch->y > 15 ? Tiles::createLava() : Tiles::createWater()) : Tiles::NOTHING;
                    } else {
                        chunk[x + y * CHUNK_W] = Tiles::NOTHING;
                    }
                } else {
                    double n3 = world->noise.GetPerlin(px / 64.0, py / 64.0, 9828);
                    if (n3 - 0.25 > py / 1000.0) {
                        chunk[x + y * CHUNK_W] = Tiles::NOTHING;
                    }
                }
            }
        }
    }
    return {};
}

std::vector<PlacedStructure> CobblePopulator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world) {

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
                (area[chx + chy * 3]->tiles[(dxx) + (dyy)*CHUNK_W].mat->physicsType == PhysicsType::SAND && area[chx + chy * 3]->tiles[(dxx) + (dyy)*CHUNK_W].mat->id != Materials::SOFT_DIRT.id)) {
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
                            if (area[1 + 1 * 3]->tiles[(sdxx) + (sdyy)*CHUNK_W].mat->id == Materials::SMOOTH_STONE.id ||
                                area[1 + 1 * 3]->tiles[(sdxx) + (sdyy)*CHUNK_W].mat->id == Materials::FLAT_COBBLE_STONE.id) {

                                chunk[sdxx + sdyy * CHUNK_W] = Tiles::createCobbleStone(sx + dx, sy + dy);

                            } else if (area[1 + 1 * 3]->tiles[(sdxx) + (sdyy)*CHUNK_W].mat->id == Materials::SMOOTH_DIRT.id ||
                                       area[1 + 1 * 3]->tiles[(sdxx) + (sdyy)*CHUNK_W].mat->id == Materials::FLAT_COBBLE_DIRT.id) {

                                chunk[sdxx + sdyy * CHUNK_W] = Tiles::createCobbleDirt(sx + dx, sy + dy);
                            }
                        } else {
                            if (area[1 + 1 * 3]->tiles[(sdxx) + (sdyy)*CHUNK_W].mat->id == Materials::SMOOTH_STONE.id) {

                                chunk[sdxx + sdyy * CHUNK_W] = Tiles::create(&Materials::FLAT_COBBLE_STONE, sx + dx, sy + dy);

                            } else if (area[1 + 1 * 3]->tiles[(sdxx) + (sdyy)*CHUNK_W].mat->id == Materials::SMOOTH_DIRT.id) {

                                chunk[sdxx + sdyy * CHUNK_W] = Tiles::create(&Materials::FLAT_COBBLE_DIRT, sx + dx, sy + dy);
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

std::vector<PlacedStructure> OrePopulator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world) {

    if (ch->y < 0) return {};
    for (int x = 0; x < CHUNK_W; x++) {
        for (int y = 0; y < CHUNK_H; y++) {
            int px = x + ch->x * CHUNK_W;
            int py = y + ch->y * CHUNK_H;

            MaterialInstance prop = chunk[x + y * CHUNK_W];
            if (chunk[x + y * CHUNK_W].mat->id == Materials::SMOOTH_STONE.id) {
                double n = (world->noise.GetNoise(px * 1.7, py * 1.7, 5124) + 1) / 2;
                if (n < 0.25) chunk[x + y * CHUNK_W] = Tiles::createIron(px, py);
            }

            if (chunk[x + y * CHUNK_W].mat->id == Materials::SMOOTH_STONE.id) {
                double n = (world->noise.GetNoise(px * 2, py * 2, 7513) + 1) / 2;
                if (n < 0.20) chunk[x + y * CHUNK_W] = Tiles::createGold(px, py);
            }
        }
    }

    return {};
}

std::vector<PlacedStructure> TreePopulator::apply(MaterialInstance *chunk, MaterialInstance *layer2, Chunk **area, bool *dirty, int tx, int ty, int tw, int th, Chunk *ch, World *world) {
    if (ch->y < 0 || ch->y > 3) return {};
    int x = (rand() % (CHUNK_W / 2) + (CHUNK_W / 4)) * 1;
    if (area[1 + 2 * 3]->tiles[x + 0 * CHUNK_W].mat->id == Materials::SOFT_DIRT.id) return {};

    for (int y = 0; y < CHUNK_H; y++) {
        if (area[1 + 2 * 3]->tiles[x + y * CHUNK_W].mat->id == Materials::SOFT_DIRT.id) {
            int px = x + ch->x * CHUNK_W;
            int py = y + (ch->y + 1) * CHUNK_W;

            // Structure tree = Structures::makeTree1(*world, px, py);
            // px -= tree.w / 2;
            // py -= tree.h - 2;

            // for (int tx = 0; tx < tree.w; tx++) {
            //     for (int ty = 0; ty < tree.h; ty++) {
            //         int chx = (int) floor((tx + px) / (float) CHUNK_W) + 1 - ch->x;
            //         int chy = (int) floor((ty + py) / (float) CHUNK_H) + 1 - ch->y;
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
            C_Surface *tex = LoadTexture(buff);

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

RigidBody::RigidBody(b2Body *body) { this->body = body; }

RigidBody::~RigidBody() {
    // if (item) delete item;
}

void Player::render(R_Target *target, int ofsX, int ofsY) {
    if (heldItem != NULL) {
        int scaleEnt = global.game->GameIsolate_.globaldef.hd_objects ? global.game->GameIsolate_.globaldef.hd_objects_size : 1;

        R_Rect *ir = new R_Rect{(float)(int)(ofsX + x + hw / 2.0 - heldItem->surface->w), (float)(int)(ofsY + y + hh / 2.0 - heldItem->surface->h / 2), (float)heldItem->surface->w,
                                (float)heldItem->surface->h};
        float fx = (float)(int)(-ir->x + ofsX + x + hw / 2.0);
        float fy = (float)(int)(-ir->y + ofsY + y + hh / 2.0);
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

b2Vec2 rotate_point2(float cx, float cy, float angle, b2Vec2 p);

void Player::setItemInHand(Item *item, World *world) {
    RigidBody *r;
    if (heldItem != NULL) {
        b2PolygonShape ps;
        ps.SetAsBox(1, 1);

        float angle = holdAngle;

        b2Vec2 pt = rotate_point2(0, 0, angle * 3.1415 / 180.0, {(float)(heldItem->surface->w / 2.0), (float)(heldItem->surface->h / 2.0)});

        r = world->makeRigidBody(b2_dynamicBody, x + hw / 2 + world->loadZone.x - pt.x + 16 * cos((holdAngle + 180) * 3.1415f / 180.0f),
                                 y + hh / 2 + world->loadZone.y - pt.y + 16 * sin((holdAngle + 180) * 3.1415f / 180.0f), angle, ps, 1, 0.3, heldItem->surface);

        //  0 -> -w/2 -h/2
        // 90 ->  w/2 -h/2
        // 180 ->  w/2  h/2
        // 270 -> -w/2  h/2

        float strength = 10;
        int time = Time::millis() - startThrow;

        if (time > 1000) time = 1000;

        strength += time / 1000.0 * 30;

        r->body->SetLinearVelocity({(float)(strength * (float)cos((holdAngle + 180) * 3.1415f / 180.0f)), (float)(strength * (float)sin((holdAngle + 180) * 3.1415f / 180.0f)) - 10});

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

Player::Player() : WorldEntity(true) {}

Player::~Player() {
    if (heldItem) delete heldItem;
}

b2Vec2 rotate_point2(float cx, float cy, float angle, b2Vec2 p) {
    float s = sin(angle);
    float c = cos(angle);

    // translate to origin
    p.x -= cx;
    p.y -= cy;

    // rotate
    float xn = p.x * c - p.y * s;
    float yn = p.x * s + p.y * c;

    // translate back
    return b2Vec2(xn + cx, yn + cy);
}

void InitGlobalDEF(GlobalDEF *_struct, bool openDebugUIs) {

    auto L = global.scripts->LuaRuntime;
    auto GlobalDEF = (*L->GetWrapper())["global_def"];

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

    } else {
        METADOT_ERROR("GlobalDEF WAS NULL");
    }

    GameUI::DebugDrawUI::visible = openDebugUIs;
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