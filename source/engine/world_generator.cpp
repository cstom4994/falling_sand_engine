// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "world_generator.h"

#include "engine/core/global.hpp"
#include "engine/utils/random.hpp"
#include "game.hpp"
#include "game_datastruct.hpp"

#pragma region MaterialTestGenerator

void MaterialTestGenerator::generateChunk(World *world, Chunk *ch) {
    MaterialInstance *prop = new MaterialInstance[CHUNK_W * CHUNK_H];
    MaterialInstance *layer2 = new MaterialInstance[CHUNK_W * CHUNK_H];
    u32 *background = new u32[CHUNK_W * CHUNK_H];
    Material *mat;

    while (true) {
        mat = GAME()->materials_container[rand() % GAME()->materials_container.size()];
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

std::vector<Populator *> MaterialTestGenerator::getPopulators() { return {}; }

#pragma endregion MaterialTestGenerator

#pragma region DefaultGenerator

int DefaultGenerator::getBaseHeight(World *world, int x, Chunk *ch) {

    if (nullptr == ch) {
        return 0;
    }

    Biome *b = world->getBiomeAt(ch, x, ch->y * CHUNK_H);

    if (b->id == Biome::biomeGetID("DEFAULT")) {
        // return 0;
        return (int)(world->height / 2 + ((world->noise.GetPerlin((f32)(x / 10.0), 0, 15))) * 100);
    } else if (b->id == Biome::biomeGetID("PLAINS")) {
        // return 10;
        return (int)(world->height / 2 + ((world->noise.GetPerlin((f32)(x / 10.0), 0, 15))) * 25);
    } else if (b->id == Biome::biomeGetID("FOREST")) {
        // return 20;
        return (int)(world->height / 2 + ((world->noise.GetPerlin((f32)(x / 10.0), 0, 15))) * 100);
    } else if (b->id == Biome::biomeGetID("MOUNTAINS")) {
        // return 30;
        return (int)(world->height / 2 + ((world->noise.GetPerlin((f32)(x / 10.0), 0, 15))) * 250);
    }

    return 0;
}

int DefaultGenerator::getHeight(World *world, int x, Chunk *ch) {

    int baseH = getBaseHeight(world, x, ch);

    Biome *b = world->getBiomeAt(x, 0);

    if (b->id == Biome::biomeGetID("DEFAULT")) {
        baseH += (int)(((world->noise.GetPerlin((f32)(x * 1), 0, 30) / 2.0) + 0.5) * 15 + (((world->noise.GetPerlin((f32)(x * 5), 0, 30) / 2.0) + 0.5) - 0.5) * 2);
    } else if (b->id == Biome::biomeGetID("PLAINS")) {
        baseH += (int)(((world->noise.GetPerlin((f32)(x * 1), 0, 30) / 2.0) + 0.5) * 6 + ((world->noise.GetPerlin((f32)(x * 5), 0, 30) / 2.0) - 0.5) * 2);
    } else if (b->id == Biome::biomeGetID("FOREST")) {
        baseH += (int)(((world->noise.GetPerlin((f32)(x * 1), 0, 30) / 2.0) + 0.5) * 15 + ((world->noise.GetPerlin((f32)(x * 5), 0, 30) / 2.0) - 0.5) * 2);
    } else if (b->id == Biome::biomeGetID("MOUNTAINS")) {
        baseH += (int)(((world->noise.GetPerlin((f32)(x * 1), 0, 30) / 2.0) + 0.5) * 20 + ((world->noise.GetPerlin((f32)(x * 5), 0, 30) / 2.0) - 0.5) * 4);
    }

    return baseH;
}

void DefaultGenerator::generateChunk(World *world, Chunk *ch) {
    MaterialInstance *prop = new MaterialInstance[CHUNK_W * CHUNK_H];
    MaterialInstance *layer2 = new MaterialInstance[CHUNK_W * CHUNK_H];
    u32 *background = new u32[CHUNK_W * CHUNK_H];

    // METADOT_BUG(std::format("DefaultGenerator generateChunk {0} {1}", ch->x, ch->y).c_str());

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

    // for (int x = 0; x < CHUNK_W; x++) {
    //     for (int y = 0; y < CHUNK_H; y++) {
    //         prop[x + y * CHUNK_W] = TileProperties(-1, PhysicsType::SOLID, (x + y * width)/16, 0, 0);
    //     }
    // }

#if 1

    for (int x = 0; x < CHUNK_W; x++) {
        int px = x + ch->x * CHUNK_W;

        int surf = getHeight(world, px, ch);

        for (int y = 0; y < CHUNK_H; y++) {
            background[x + y * CHUNK_W] = 0x00000000;
            int py = y + ch->y * CHUNK_W;
            Biome *b = world->getBiomeAt(px, py);

            // std::cout << "DefaultGenerator generate " << ch->x << " " << ch->y << " Biome: " << b->name << std::endl;

            if (b->id == Biome::biomeGetID("TEST_1")) {
                prop[x + y * CHUNK_W] = MaterialInstance(&GAME()->materials_list.GENERIC_SOLID, 0xffe00000);
            } else if (b->id == Biome::biomeGetID("TEST_2")) {
                prop[x + y * CHUNK_W] = MaterialInstance(&GAME()->materials_list.GENERIC_SOLID, 0xff00ff00);
            } else if (b->id == Biome::biomeGetID("TEST_3")) {
                prop[x + y * CHUNK_W] = MaterialInstance(&GAME()->materials_list.GENERIC_SOLID, 0xff0000ff);
            } else if (b->id == Biome::biomeGetID("TEST_4")) {
                prop[x + y * CHUNK_W] = MaterialInstance(&GAME()->materials_list.GENERIC_SOLID, 0xffff00ff);
            }

            if (b->id == Biome::biomeGetID("TEST_1_2")) {
                prop[x + y * CHUNK_W] = MaterialInstance(&GAME()->materials_list.GENERIC_SOLID, 0xffFF6600);
            } else if (b->id == Biome::biomeGetID("TEST_2_2")) {
                prop[x + y * CHUNK_W] = MaterialInstance(&GAME()->materials_list.GENERIC_SOLID, 0xff00FFBF);
            } else if (b->id == Biome::biomeGetID("TEST_3_2")) {
                prop[x + y * CHUNK_W] = MaterialInstance(&GAME()->materials_list.GENERIC_SOLID, 0xff005DFF);
            } else if (b->id == Biome::biomeGetID("TEST_4_2")) {
                prop[x + y * CHUNK_W] = MaterialInstance(&GAME()->materials_list.GENERIC_SOLID, 0xffC200FF);
            }
            // continue;

            if (b->id == Biome::biomeGetID("DEFAULT")) {
                if (py > surf) {
                    int tx = (global.game->Iso.texturepack.caveBG->surface()->w + (px % global.game->Iso.texturepack.caveBG->surface()->w)) % global.game->Iso.texturepack.caveBG->surface()->w;
                    int ty = (global.game->Iso.texturepack.caveBG->surface()->h + (py % global.game->Iso.texturepack.caveBG->surface()->h)) % global.game->Iso.texturepack.caveBG->surface()->h;
                    background[x + y * CHUNK_W] =
                            R_GET_PIXEL(global.game->Iso.texturepack.caveBG->surface(), tx % global.game->Iso.texturepack.caveBG->surface()->w, ty % global.game->Iso.texturepack.caveBG->surface()->h);
                    f64 thru = std::fmin(std::fmax(0, abs(surf - py) / 150.0), 1);

                    f64 n = (world->noise.GetPerlin(px * 4.0, py * 4.0, 2960) / 2.0 + 0.5) - 0.1;
                    f64 n2 = ((world->noise.GetPerlin(px * 2.0, py * 2.0, 8923) / 2.0 + 0.5) * 0.9 + (world->noise.GetPerlin(px * 8.0, py * 8.0, 7526) / 2.0 + 0.5) * 0.1) - 0.1;
                    prop[x + y * CHUNK_W] = (n * (1 - thru) + n2 * thru) < 0.5 ? TilesCreateSmoothStone(px, py) : TilesCreateSmoothDirt(px, py);
                } else if (py > surf - 64) {
                    f64 n = ((world->noise.GetPerlin(px * 4.0, py * 4.0, 0) / 2.0 + 0.5) + 0.4) / 2.0;
                    prop[x + y * CHUNK_W] = n < abs((surf - 64) - py) / 64.0 ? TilesCreateSmoothDirt(px, py) : TilesCreateSoftDirt(px, py);
                } else if (py > surf - 65) {
                    if (rand() % 2 == 0) prop[x + y * CHUNK_W] = TilesCreateGrass();
                } else {
                    prop[x + y * CHUNK_W] = Tiles_NOTHING;
                }

                layer2[x + y * CHUNK_W] = Tiles_NOTHING;
            } else if (b->id == Biome::biomeGetID("PLAINS")) {
                if (py > surf) {
                    int tx = (global.game->Iso.texturepack.caveBG->surface()->w + (px % global.game->Iso.texturepack.caveBG->surface()->h)) % global.game->Iso.texturepack.caveBG->surface()->h;
                    int ty = (global.game->Iso.texturepack.caveBG->surface()->h + (py % global.game->Iso.texturepack.caveBG->surface()->h)) % global.game->Iso.texturepack.caveBG->surface()->h;
                    u8 *pixel = (u8 *)global.game->Iso.texturepack.caveBG->surface()->pixels;
                    pixel += ((ty % global.game->Iso.texturepack.caveBG->surface()->h) * global.game->Iso.texturepack.caveBG->surface()->pitch) +
                             ((tx % global.game->Iso.texturepack.caveBG->surface()->w) * sizeof(u32));
                    background[x + y * CHUNK_W] = *((u32 *)pixel);
                    f64 thru = std::fmin(std::fmax(0, abs(surf - py) / 150.0), 1);

                    f64 n = (world->noise.GetPerlin(px * 4.0, py * 4.0, 2960) / 2.0 + 0.5) - 0.1;
                    f64 n2 = ((world->noise.GetPerlin(px * 2.0, py * 2.0, 8923) / 2.0 + 0.5) * 0.9 + (world->noise.GetPerlin(px * 8.0, py * 8.0, 7526) / 2.0 + 0.5) * 0.1) - 0.1;
                    prop[x + y * CHUNK_W] = (n * (1 - thru) + n2 * thru) < 0.5 ? TilesCreateSmoothStone(px, py) : TilesCreateSmoothDirt(px, py);
                } else if (py > surf - 64) {
                    f64 n = ((world->noise.GetPerlin(px * 4.0, py * 4.0, 0) / 2.0 + 0.5) + 0.4) / 2.0;
                    prop[x + y * CHUNK_W] = n < abs((surf - 64) - py) / 64.0 ? TilesCreateSmoothDirt(px, py) : MaterialInstance(&GAME()->materials_list.GENERIC_SOLID, 0xff0000);
                } else if (py > surf - 65) {
                    if (rand() % 2 == 0) prop[x + y * CHUNK_W] = TilesCreateGrass();
                } else {
                    prop[x + y * CHUNK_W] = Tiles_NOTHING;
                }

                layer2[x + y * CHUNK_W] = Tiles_NOTHING;
            } else if (b->id == Biome::biomeGetID("MOUNTAINS")) {
                if (py > surf) {
                    int tx = (global.game->Iso.texturepack.caveBG->surface()->w + (px % global.game->Iso.texturepack.caveBG->surface()->h)) % global.game->Iso.texturepack.caveBG->surface()->h;
                    int ty = (global.game->Iso.texturepack.caveBG->surface()->h + (py % global.game->Iso.texturepack.caveBG->surface()->h)) % global.game->Iso.texturepack.caveBG->surface()->h;
                    u8 *pixel = (u8 *)global.game->Iso.texturepack.caveBG->surface()->pixels;
                    pixel += ((ty % global.game->Iso.texturepack.caveBG->surface()->h) * global.game->Iso.texturepack.caveBG->surface()->pitch) +
                             ((tx % global.game->Iso.texturepack.caveBG->surface()->w) * sizeof(u32));
                    background[x + y * CHUNK_W] = *((u32 *)pixel);
                    f64 thru = std::fmin(std::fmax(0, abs(surf - py) / 150.0), 1);

                    f64 n = (world->noise.GetPerlin(px * 4.0, py * 4.0, 2960) / 2.0 + 0.5) - 0.1;
                    f64 n2 = ((world->noise.GetPerlin(px * 2.0, py * 2.0, 8923) / 2.0 + 0.5) * 0.9 + (world->noise.GetPerlin(px * 8.0, py * 8.0, 7526) / 2.0 + 0.5) * 0.1) - 0.1;
                    prop[x + y * CHUNK_W] = (n * (1 - thru) + n2 * thru) < 0.5 ? TilesCreateSmoothStone(px, py) : TilesCreateSmoothDirt(px, py);
                } else if (py > surf - 64) {
                    f64 n = ((world->noise.GetPerlin(px * 4.0, py * 4.0, 0) / 2.0 + 0.5) + 0.4) / 2.0;
                    prop[x + y * CHUNK_W] = n < abs((surf - 64) - py) / 64.0 ? TilesCreateSmoothDirt(px, py) : MaterialInstance(&GAME()->materials_list.GENERIC_SOLID, 0x00ff00);
                } else if (py > surf - 65) {
                    if (rand() % 2 == 0) prop[x + y * CHUNK_W] = TilesCreateGrass();
                } else {
                    prop[x + y * CHUNK_W] = Tiles_NOTHING;
                }

                layer2[x + y * CHUNK_W] = Tiles_NOTHING;
            } else if (b->id == Biome::biomeGetID("FOREST")) {
                if (py > surf) {
                    int tx = (global.game->Iso.texturepack.caveBG->surface()->w + (px % global.game->Iso.texturepack.caveBG->surface()->h)) % global.game->Iso.texturepack.caveBG->surface()->h;
                    int ty = (global.game->Iso.texturepack.caveBG->surface()->h + (py % global.game->Iso.texturepack.caveBG->surface()->h)) % global.game->Iso.texturepack.caveBG->surface()->h;
                    u8 *pixel = (u8 *)global.game->Iso.texturepack.caveBG->surface()->pixels;
                    pixel += ((ty % global.game->Iso.texturepack.caveBG->surface()->h) * global.game->Iso.texturepack.caveBG->surface()->pitch) +
                             ((tx % global.game->Iso.texturepack.caveBG->surface()->w) * sizeof(u32));
                    background[x + y * CHUNK_W] = *((u32 *)pixel);
                    f64 thru = std::fmin(std::fmax(0, abs(surf - py) / 150.0), 1);

                    f64 n = (world->noise.GetPerlin(px * 4.0, py * 4.0, 2960) / 2.0 + 0.5) - 0.1;
                    f64 n2 = ((world->noise.GetPerlin(px * 2.0, py * 2.0, 8923) / 2.0 + 0.5) * 0.9 + (world->noise.GetPerlin(px * 8.0, py * 8.0, 7526) / 2.0 + 0.5) * 0.1) - 0.1;
                    prop[x + y * CHUNK_W] = (n * (1 - thru) + n2 * thru) < 0.5 ? TilesCreateSmoothStone(px, py) : TilesCreateSmoothDirt(px, py);
                } else if (py > surf - 64) {
                    f64 n = ((world->noise.GetPerlin(px * 4.0, py * 4.0, 0) / 2.0 + 0.5) + 0.4) / 2.0;
                    prop[x + y * CHUNK_W] = n < abs((surf - 64) - py) / 64.0 ? TilesCreateSmoothDirt(px, py) : MaterialInstance(&GAME()->materials_list.GENERIC_SOLID, 0x0000ff);
                } else if (py > surf - 65) {
                    if (rand() % 2 == 0) prop[x + y * CHUNK_W] = TilesCreateGrass();
                } else {
                    prop[x + y * CHUNK_W] = Tiles_NOTHING;
                }

                layer2[x + y * CHUNK_W] = Tiles_NOTHING;
            }

            // prop[x + y * CHUNK_W] = Tiles_NOTHING;
            // layer2[x + y * CHUNK_W] = Tiles_NOTHING;
        }
    }
#else

    for (int x = 0; x < CHUNK_W; x++) {
        int px = x + ch->x * CHUNK_W;

        int surf = getHeight(world, px, ch);

        for (int y = 0; y < CHUNK_H; y++) {
            background[x + y * CHUNK_W] = 0x00000000;
            int py = y + ch->y * CHUNK_W;
            // Biome *b = world->getBiomeAt(px, py);
            Biome *b = BiomeGet("DEFAULT");

            ME_ASSERT(b->id >= 0);

            // std::cout << "DefaultGenerator generate " << ch->x << " " << ch->y << " Biome: " << b->name << std::endl;

            if (py > 400 && py <= 450) {
                prop[x + y * CHUNK_W] = TilesCreateCobbleStone(px, py);
            } else if (ch->y == 1 && ch->x >= 1 && ch->x <= 4) {
                if (x < 8 || y < 8 || x >= CHUNK_H - 8 || (y >= CHUNK_W - 8 && (x < 60 || x >= 68))) {
                    prop[x + y * CHUNK_W] = TilesCreateCobbleDirt(px, py);
                } else if (y > CHUNK_H * 0.75) {
                    prop[x + y * CHUNK_W] = TilesCreateCobbleDirt(px, py);
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

#endif

    ch->tiles = prop;
    ch->layer2 = layer2;
    ch->background = background;
}

std::vector<Populator *> DefaultGenerator::getPopulators() {
    return {new CavePopulator(), new OrePopulator(), new CobblePopulator()};
    // return {new CavePopulator(), new OrePopulator(), new CobblePopulator(), new TreePopulator()};
}

#pragma endregion

void ScriptingWorldGenerator::generateChunk(World *world, Chunk *ch) {}

std::vector<Populator *> ScriptingWorldGenerator::getPopulators() { return {}; }

double wang_seed = 0;
static CLGMRandom random(wang_seed);

int myrand() { return random.Random(0, 2147483645 - 1); }

#define STB_HBWANG_RAND() myrand()

#define STB_HBWANG_IMPLEMENTATION
#include "libs/external/stb_herringbone_wang_tile.h"
#include "libs/external/stb_image.h"
#include "libs/external/stb_image_write.h"

using namespace std::string_literals;
void genwang(std::string filename, unsigned char *data, int xs, int ys, int w, int h) {
    stbhw_tileset ts;
    if (xs < 1 || xs > 1000) {
        fprintf(stderr, "xsize invalid or out of range\n");
        exit(1);
    }
    if (ys < 1 || ys > 1000) {
        fprintf(stderr, "ysize invalid or out of range\n");
        exit(1);
    }

    stbhw_build_tileset_from_image(&ts, data, w * 3, w, h);
    // allocate a buffer to create the final image to
    int yimg = ys + 4;
    auto buff = static_cast<unsigned char *>(malloc(3 * xs * yimg));
    stbhw_generate_image(&ts, NULL, buff, xs * 3, xs, yimg);
    stbi_write_png(filename.c_str(), xs, yimg, 3, buff, xs * 3);
    stbhw_free_tileset(&ts);
    free(buff);
}

int test_wang() {

    // mapgen {tile-file} {xsize} {ysize} {seed} [n]

    int xs = 128;
    int ys = 128;

    int n = 1;

    int w, h;

    unsigned char *data = stbi_load("data/assets/textures/wang_test.png", &w, &h, NULL, 3);

    ME_ASSERT(data);

    printf("Output size: %dx%d\n", xs, ys);

    auto seed = std::stoull("123456");
    printf("Using seed: %llu\n", seed);
    wang_seed = seed;
    {
        CLGMRandom rnd(seed);

        // int num = seed + xs + 11 * (xs / -11) - 12 * (seed / 12);
        int num = xs % 11 + seed % 12;
        printf("Some number: %d\n", num);
        while (num-- > 0) {
            rnd.Next();
        }

        for (int i = 0; i < n; ++i) {
            double newSeed = rnd.Next() * 2147483645.0;
            printf("Adjusted seed: %f\n", newSeed);
            random.SetSeed(newSeed);

            auto filename = std::string("output_wang");
            filename = filename.substr(0, filename.size() - 4);
            filename = filename + std::to_string(seed) + "#" + std::to_string(i) + ".png";

            genwang(filename, data, xs, ys, w, h);
        }
    }

    free(data);

    return 0;
}