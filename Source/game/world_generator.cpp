// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_GENERATOR_WORLD_CPP_
#define _METADOT_GENERATOR_WORLD_CPP_

#include "core/global.hpp"
#include "engine/renderer/renderer_utils.h"
#include "game/game.hpp"
#include "game_datastruct.hpp"

#pragma region MaterialTestGenerator

class MaterialTestGenerator : public WorldGenerator {
    void generateChunk(World *world, Chunk *ch) override {
        MaterialInstance *prop = new MaterialInstance[CHUNK_W * CHUNK_H];
        MaterialInstance *layer2 = new MaterialInstance[CHUNK_W * CHUNK_H];
        U32 *background = new U32[CHUNK_W * CHUNK_H];
        Material *mat;

        while (true) {
            mat = global.GameData_.materials_container[rand() % global.GameData_.materials_container.size()];
            if (mat->id >= 31 && (mat->physicsType == PhysicsType::SAND || mat->physicsType == PhysicsType::SOUP)) break;
        }

        for (int x = 0; x < CHUNK_W; x++) {
            int px = x + ch->x * CHUNK_W;

            for (int y = 0; y < CHUNK_H; y++) {
                background[x + y * CHUNK_W] = 0x00000000;
                int py = y + ch->y * CHUNK_W;

                if (py > 400 && py <= 450) {
                    prop[x + y * CHUNK_W] = TilesCreateCobbleStone(px, py);
                } else if (ch->y == 1 && ch->x >= 1 && ch->x <= 4) {
                    if (x < 8 || y < 8 || x >= CHUNK_H - 8 || (y >= CHUNK_W - 8 && (x < 60 || x >= 68))) {
                        prop[x + y * CHUNK_W] = TilesCreateCobbleDirt(px, py);
                    } else if (y > CHUNK_H * 0.75) {
                        prop[x + y * CHUNK_W] = TilesCreate(mat, px, py);
                    } else {
                        prop[x + y * CHUNK_W] = Tiles_NOTHING;
                    }
                } else if (ch->x == 1 && py <= 400 && py > 300 && x < (py - 300)) {
                    prop[x + y * CHUNK_W] = TilesCreateCobbleStone(px, py);
                } else if (ch->x == 4 && py <= 400 && py > 300 && (CHUNK_W - x) < (py - 300)) {
                    prop[x + y * CHUNK_W] = TilesCreateCobbleStone(px, py);
                } else {
                    prop[x + y * CHUNK_W] = Tiles_NOTHING;
                }

                layer2[x + y * CHUNK_W] = Tiles_NOTHING;
            }
        }

        ch->tiles = prop;
        ch->layer2 = layer2;
        ch->background = background;
    }

    std::vector<Populator *> getPopulators() override { return {}; }
};

#pragma endregion MaterialTestGenerator

#pragma region DefaultGenerator

#define BIOMEGETID(_c) global.game->GameSystem_.gameScriptwrap.BiomeGet(_c)->id

class DefaultGenerator : public WorldGenerator {

    int getBaseHeight(World *world, int x, Chunk *ch) {

        if (nullptr == ch) {
            return 0;
        }

        Biome *b = world->getBiomeAt(ch, x, ch->y * CHUNK_H);

        if (b->id == BIOMEGETID("DEFAULT")) {
            // return 0;
            return (int)(world->height / 2 + ((world->noise.GetPerlin((F32)(x / 10.0), 0, 15))) * 100);
        } else if (b->id == BIOMEGETID("PLAINS")) {
            // return 10;
            return (int)(world->height / 2 + ((world->noise.GetPerlin((F32)(x / 10.0), 0, 15))) * 25);
        } else if (b->id == BIOMEGETID("FOREST")) {
            // return 20;
            return (int)(world->height / 2 + ((world->noise.GetPerlin((F32)(x / 10.0), 0, 15))) * 100);
        } else if (b->id == BIOMEGETID("MOUNTAINS")) {
            // return 30;
            return (int)(world->height / 2 + ((world->noise.GetPerlin((F32)(x / 10.0), 0, 15))) * 250);
        }

        return 0;
    }

    int getHeight(World *world, int x, Chunk *ch) {

        int baseH = getBaseHeight(world, x, ch);

        Biome *b = world->getBiomeAt(x, 0);

        if (b->id == BIOMEGETID("DEFAULT")) {
            baseH += (int)(((world->noise.GetPerlin((F32)(x * 1), 0, 30) / 2.0) + 0.5) * 15 + (((world->noise.GetPerlin((F32)(x * 5), 0, 30) / 2.0) + 0.5) - 0.5) * 2);
        } else if (b->id == BIOMEGETID("PLAINS")) {
            baseH += (int)(((world->noise.GetPerlin((F32)(x * 1), 0, 30) / 2.0) + 0.5) * 6 + ((world->noise.GetPerlin((F32)(x * 5), 0, 30) / 2.0) - 0.5) * 2);
        } else if (b->id == BIOMEGETID("FOREST")) {
            baseH += (int)(((world->noise.GetPerlin((F32)(x * 1), 0, 30) / 2.0) + 0.5) * 15 + ((world->noise.GetPerlin((F32)(x * 5), 0, 30) / 2.0) - 0.5) * 2);
        } else if (b->id == BIOMEGETID("MOUNTAINS")) {
            baseH += (int)(((world->noise.GetPerlin((F32)(x * 1), 0, 30) / 2.0) + 0.5) * 20 + ((world->noise.GetPerlin((F32)(x * 5), 0, 30) / 2.0) - 0.5) * 4);
        }

        return baseH;
    }

    void generateChunk(World *world, Chunk *ch) override {
        MaterialInstance *prop = new MaterialInstance[CHUNK_W * CHUNK_H];
        MaterialInstance *layer2 = new MaterialInstance[CHUNK_W * CHUNK_H];
        U32 *background = new U32[CHUNK_W * CHUNK_H];
        // std::cout << "generate " << cx << " " << cy << std::endl;
        /*for (int x = 0; x < CHUNK_W; x++) {
            for (int y = 0; y < CHUNK_H; y++) {
                prop[x + y * CHUNK_W] = (x + y)%2 == 0 ? Tiles_TEST_SOLID : TilesCreateSmoothStone(x, y);
            }
        }*/

        /*for (int x = 0; x < CHUNK_W; x++) {
            for (int y = 0; y < CHUNK_H; y++) {
                prop[x + y * CHUNK_W] = (cx + cy) % 2 == 0 ? Tiles_TEST_SOLID : TilesCreateSmoothStone(x, y);
            }
        }*/

        /*for (int x = 0; x < CHUNK_W; x++) {
            for (int y = 0; y < CHUNK_H; y++) {
                prop[x + y * CHUNK_W] = TileProperties(-1, PhysicsType::SOLID, (x + y * width)/16, 0, 0);
            }
        }*/

        for (int x = 0; x < CHUNK_W; x++) {
            int px = x + ch->x * CHUNK_W;

            int surf = getHeight(world, px, ch);

            for (int y = 0; y < CHUNK_H; y++) {
                background[x + y * CHUNK_W] = 0x00000000;
                int py = y + ch->y * CHUNK_W;
                Biome *b = world->getBiomeAt(px, py);

                if (b->id == BIOMEGETID("TEST_1")) {
                    prop[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xffe00000);
                } else if (b->id == BIOMEGETID("TEST_2")) {
                    prop[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xff00ff00);
                } else if (b->id == BIOMEGETID("TEST_3")) {
                    prop[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xff0000ff);
                } else if (b->id == BIOMEGETID("TEST_4")) {
                    prop[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xffff00ff);
                }

                if (b->id == BIOMEGETID("TEST_1_2")) {
                    prop[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xffFF6600);
                } else if (b->id == BIOMEGETID("TEST_2_2")) {
                    prop[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xff00FFBF);
                } else if (b->id == BIOMEGETID("TEST_3_2")) {
                    prop[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xff005DFF);
                } else if (b->id == BIOMEGETID("TEST_4_2")) {
                    prop[x + y * CHUNK_W] = MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xffC200FF);
                }
                // continue;

                if (b->id == BIOMEGETID("DEFAULT")) {
                    if (py > surf) {
                        int tx = (global.game->GameIsolate_.texturepack->caveBG->surface->w + (px % global.game->GameIsolate_.texturepack->caveBG->surface->w)) %
                                 global.game->GameIsolate_.texturepack->caveBG->surface->w;
                        int ty = (global.game->GameIsolate_.texturepack->caveBG->surface->h + (py % global.game->GameIsolate_.texturepack->caveBG->surface->h)) %
                                 global.game->GameIsolate_.texturepack->caveBG->surface->h;
                        background[x + y * CHUNK_W] = R_GET_PIXEL(global.game->GameIsolate_.texturepack->caveBG->surface, tx % global.game->GameIsolate_.texturepack->caveBG->surface->w,
                                                                  ty % global.game->GameIsolate_.texturepack->caveBG->surface->h);
                        F64 thru = std::fmin(std::fmax(0, abs(surf - py) / 150.0), 1);

                        F64 n = (world->noise.GetPerlin(px * 4.0, py * 4.0, 2960) / 2.0 + 0.5) - 0.1;
                        F64 n2 = ((world->noise.GetPerlin(px * 2.0, py * 2.0, 8923) / 2.0 + 0.5) * 0.9 + (world->noise.GetPerlin(px * 8.0, py * 8.0, 7526) / 2.0 + 0.5) * 0.1) - 0.1;
                        prop[x + y * CHUNK_W] = (n * (1 - thru) + n2 * thru) < 0.5 ? TilesCreateSmoothStone(px, py) : TilesCreateSmoothDirt(px, py);
                    } else if (py > surf - 64) {
                        F64 n = ((world->noise.GetPerlin(px * 4.0, py * 4.0, 0) / 2.0 + 0.5) + 0.4) / 2.0;
                        prop[x + y * CHUNK_W] = n < abs((surf - 64) - py) / 64.0 ? TilesCreateSmoothDirt(px, py) : TilesCreateSoftDirt(px, py);
                    } else if (py > surf - 65) {
                        if (rand() % 2 == 0) prop[x + y * CHUNK_W] = TilesCreateGrass();
                    } else {
                        prop[x + y * CHUNK_W] = Tiles_NOTHING;
                    }

                    layer2[x + y * CHUNK_W] = Tiles_NOTHING;
                } else if (b->id == BIOMEGETID("PLAINS")) {
                    if (py > surf) {
                        int tx = (global.game->GameIsolate_.texturepack->caveBG->surface->w + (px % global.game->GameIsolate_.texturepack->caveBG->surface->h)) %
                                 global.game->GameIsolate_.texturepack->caveBG->surface->h;
                        int ty = (global.game->GameIsolate_.texturepack->caveBG->surface->h + (py % global.game->GameIsolate_.texturepack->caveBG->surface->h)) %
                                 global.game->GameIsolate_.texturepack->caveBG->surface->h;
                        U8 *pixel = (U8 *)global.game->GameIsolate_.texturepack->caveBG->surface->pixels;
                        pixel += ((ty % global.game->GameIsolate_.texturepack->caveBG->surface->h) * global.game->GameIsolate_.texturepack->caveBG->surface->pitch) +
                                 ((tx % global.game->GameIsolate_.texturepack->caveBG->surface->w) * sizeof(U32));
                        background[x + y * CHUNK_W] = *((U32 *)pixel);
                        F64 thru = std::fmin(std::fmax(0, abs(surf - py) / 150.0), 1);

                        F64 n = (world->noise.GetPerlin(px * 4.0, py * 4.0, 2960) / 2.0 + 0.5) - 0.1;
                        F64 n2 = ((world->noise.GetPerlin(px * 2.0, py * 2.0, 8923) / 2.0 + 0.5) * 0.9 + (world->noise.GetPerlin(px * 8.0, py * 8.0, 7526) / 2.0 + 0.5) * 0.1) - 0.1;
                        prop[x + y * CHUNK_W] = (n * (1 - thru) + n2 * thru) < 0.5 ? TilesCreateSmoothStone(px, py) : TilesCreateSmoothDirt(px, py);
                    } else if (py > surf - 64) {
                        F64 n = ((world->noise.GetPerlin(px * 4.0, py * 4.0, 0) / 2.0 + 0.5) + 0.4) / 2.0;
                        prop[x + y * CHUNK_W] = n < abs((surf - 64) - py) / 64.0 ? TilesCreateSmoothDirt(px, py) : MaterialInstance(&MaterialsList::GENERIC_SOLID, 0xff0000);
                    } else if (py > surf - 65) {
                        if (rand() % 2 == 0) prop[x + y * CHUNK_W] = TilesCreateGrass();
                    } else {
                        prop[x + y * CHUNK_W] = Tiles_NOTHING;
                    }

                    layer2[x + y * CHUNK_W] = Tiles_NOTHING;
                } else if (b->id == BIOMEGETID("MOUNTAINS")) {
                    if (py > surf) {
                        int tx = (global.game->GameIsolate_.texturepack->caveBG->surface->w + (px % global.game->GameIsolate_.texturepack->caveBG->surface->h)) %
                                 global.game->GameIsolate_.texturepack->caveBG->surface->h;
                        int ty = (global.game->GameIsolate_.texturepack->caveBG->surface->h + (py % global.game->GameIsolate_.texturepack->caveBG->surface->h)) %
                                 global.game->GameIsolate_.texturepack->caveBG->surface->h;
                        U8 *pixel = (U8 *)global.game->GameIsolate_.texturepack->caveBG->surface->pixels;
                        pixel += ((ty % global.game->GameIsolate_.texturepack->caveBG->surface->h) * global.game->GameIsolate_.texturepack->caveBG->surface->pitch) +
                                 ((tx % global.game->GameIsolate_.texturepack->caveBG->surface->w) * sizeof(U32));
                        background[x + y * CHUNK_W] = *((U32 *)pixel);
                        F64 thru = std::fmin(std::fmax(0, abs(surf - py) / 150.0), 1);

                        F64 n = (world->noise.GetPerlin(px * 4.0, py * 4.0, 2960) / 2.0 + 0.5) - 0.1;
                        F64 n2 = ((world->noise.GetPerlin(px * 2.0, py * 2.0, 8923) / 2.0 + 0.5) * 0.9 + (world->noise.GetPerlin(px * 8.0, py * 8.0, 7526) / 2.0 + 0.5) * 0.1) - 0.1;
                        prop[x + y * CHUNK_W] = (n * (1 - thru) + n2 * thru) < 0.5 ? TilesCreateSmoothStone(px, py) : TilesCreateSmoothDirt(px, py);
                    } else if (py > surf - 64) {
                        F64 n = ((world->noise.GetPerlin(px * 4.0, py * 4.0, 0) / 2.0 + 0.5) + 0.4) / 2.0;
                        prop[x + y * CHUNK_W] = n < abs((surf - 64) - py) / 64.0 ? TilesCreateSmoothDirt(px, py) : MaterialInstance(&MaterialsList::GENERIC_SOLID, 0x00ff00);
                    } else if (py > surf - 65) {
                        if (rand() % 2 == 0) prop[x + y * CHUNK_W] = TilesCreateGrass();
                    } else {
                        prop[x + y * CHUNK_W] = Tiles_NOTHING;
                    }

                    layer2[x + y * CHUNK_W] = Tiles_NOTHING;
                } else if (b->id == BIOMEGETID("FOREST")) {
                    if (py > surf) {
                        int tx = (global.game->GameIsolate_.texturepack->caveBG->surface->w + (px % global.game->GameIsolate_.texturepack->caveBG->surface->h)) %
                                 global.game->GameIsolate_.texturepack->caveBG->surface->h;
                        int ty = (global.game->GameIsolate_.texturepack->caveBG->surface->h + (py % global.game->GameIsolate_.texturepack->caveBG->surface->h)) %
                                 global.game->GameIsolate_.texturepack->caveBG->surface->h;
                        U8 *pixel = (U8 *)global.game->GameIsolate_.texturepack->caveBG->surface->pixels;
                        pixel += ((ty % global.game->GameIsolate_.texturepack->caveBG->surface->h) * global.game->GameIsolate_.texturepack->caveBG->surface->pitch) +
                                 ((tx % global.game->GameIsolate_.texturepack->caveBG->surface->w) * sizeof(U32));
                        background[x + y * CHUNK_W] = *((U32 *)pixel);
                        F64 thru = std::fmin(std::fmax(0, abs(surf - py) / 150.0), 1);

                        F64 n = (world->noise.GetPerlin(px * 4.0, py * 4.0, 2960) / 2.0 + 0.5) - 0.1;
                        F64 n2 = ((world->noise.GetPerlin(px * 2.0, py * 2.0, 8923) / 2.0 + 0.5) * 0.9 + (world->noise.GetPerlin(px * 8.0, py * 8.0, 7526) / 2.0 + 0.5) * 0.1) - 0.1;
                        prop[x + y * CHUNK_W] = (n * (1 - thru) + n2 * thru) < 0.5 ? TilesCreateSmoothStone(px, py) : TilesCreateSmoothDirt(px, py);
                    } else if (py > surf - 64) {
                        F64 n = ((world->noise.GetPerlin(px * 4.0, py * 4.0, 0) / 2.0 + 0.5) + 0.4) / 2.0;
                        prop[x + y * CHUNK_W] = n < abs((surf - 64) - py) / 64.0 ? TilesCreateSmoothDirt(px, py) : MaterialInstance(&MaterialsList::GENERIC_SOLID, 0x0000ff);
                    } else if (py > surf - 65) {
                        if (rand() % 2 == 0) prop[x + y * CHUNK_W] = TilesCreateGrass();
                    } else {
                        prop[x + y * CHUNK_W] = Tiles_NOTHING;
                    }

                    layer2[x + y * CHUNK_W] = Tiles_NOTHING;
                }
            }
        }

        ch->tiles = prop;
        ch->layer2 = layer2;
        ch->background = background;
    }

    std::vector<Populator *> getPopulators() override { return {new CavePopulator(), new OrePopulator(), new CobblePopulator(), new TreePopulator()}; }
};

#undef BIOMEGETID

#pragma endregion

#endif
