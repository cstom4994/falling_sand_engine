// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_POPULATORS_CPP_
#define _METADOT_POPULATORS_CPP_

#include "Populators.hpp"
#include "Structures.hpp"

#ifndef INC_Textures
#include "Game/Textures.hpp"
#endif// !INC_Textures

#include "Game/InEngine.h"

// #include "Populator.hpp"
// #include "Game/Textures.hpp"
// #include <string>
// #include "Structures.hpp"

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
// 							if (tiles[(x + dx) + (y + dy) * CHUNK_W].mat.physicsType == PhysicsType::AIR || (tiles[(x + dx) + (y + dy) * CHUNK_W].mat.physicsType == PhysicsType::SAND && tiles[(x + dx) + (y + dy) * CHUNK_W].mat.id != Materials::SOFT_DIRT.id)) {
// 								tiles[x + y * CHUNK_W] = Tiles::createCobbleStone(px, py);
// 								goto nextTile;
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
// 							if (tiles[(x + dx) + (y + dy) * CHUNK_W].mat.physicsType == PhysicsType::AIR || (tiles[(x + dx) + (y + dy) * CHUNK_W].mat.physicsType == PhysicsType::SAND && tiles[(x + dx) + (y + dy) * CHUNK_W].mat.id != Materials::SOFT_DIRT.id)) {
// 								tiles[x + y * CHUNK_W] = Tiles::createCobbleDirt(px, py);
// 								goto nextTile;
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
// 		Structure st = Structure(Textures::LoadTexture(m, SDL_PIXELFORMAT_ARGB8888), Materials::CLOUD);
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

class TestPhase1Populator : public Populator {
public:
    int getPhase() { return 1; }

    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world) {
        for (int x = 0; x < 10; x++) {
            for (int y = 0; y < 10; y++) {
                chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0xff0000);
            }
        }

        return {};
    }
};

class TestPhase2Populator : public Populator {
public:
    int getPhase() { return 2; }

    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world) {
        for (int x = 0; x < 10; x++) {
            for (int y = 0; y < 10; y++) {
                chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0x00ff00);
            }
        }

        return {};
    }
};

class TestPhase3Populator : public Populator {
public:
    int getPhase() { return 3; }

    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world) {
        for (int x = 0; x < 10; x++) {
            for (int y = 0; y < 10; y++) {
                chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0x0000ff);
            }
        }

        return {};
    }
};

class TestPhase4Populator : public Populator {
public:
    int getPhase() { return 4; }

    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world) {
        for (int x = 0; x < 10; x++) {
            for (int y = 0; y < 10; y++) {
                chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0xffff00);
            }
        }

        return {};
    }
};

class TestPhase5Populator : public Populator {
public:
    int getPhase() { return 5; }

    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world) {
        for (int x = 0; x < 10; x++) {
            for (int y = 0; y < 10; y++) {
                chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0xff00ff);
            }
        }

        return {};
    }
};

class TestPhase6Populator : public Populator {
public:
    int getPhase() { return 6; }

    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world) {
        for (int x = 0; x < 10; x++) {
            for (int y = 0; y < 10; y++) {
                chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0x00ffff);
            }
        }

        return {};
    }
};

class TestPhase0Populator : public Populator {
public:
    int getPhase() { return 0; }

    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk *area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk ch, World *world) {
        for (int x = 10; x < 20; x++) {
            for (int y = 10; y < 20; y++) {
                chunk[x + y * CHUNK_W] = MaterialInstance(&Materials::GENERIC_SOLID, 0xffffff);
            }
        }

        return {};
    }
};

class CavePopulator : public Populator {
public:
    int getPhase() { return 0; }

    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk **area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk *ch, World *world) {

        if (ch->y < 0) return {};
        for (int x = 0; x < CHUNK_W; x++) {
            for (int y = 0; y < CHUNK_H; y++) {
                int px = x + ch->x * CHUNK_W;
                int py = y + ch->y * CHUNK_H;
                if (chunk[x + y * CHUNK_W].mat->physicsType == PhysicsType::SOLID &&
                    chunk[x + y * CHUNK_W].mat->id != Materials::CLOUD.id) {
                    double n = (world->noise.GetPerlin(px * 1.5, py * 1.5, 3802) + 1) / 2;
                    double n2 = (world->noise.GetPerlin(px / 3.0, py / 3.0, 6213) + 1) / 2;
                    double ndetail =
                            (world->noise.GetPerlin(px * 8.0, py * 8.0, 5319) + 1) / 2 * 0.08;

                    if (n2 + n + ndetail < std::fmin(0.95, (py) / 1000.0)) {
                        double nlav = world->noise.GetPerlin(px / 4.0, py / 4.0, 7018);
                        if (nlav > 0.45) {
                            chunk[x + y * CHUNK_W] = rand() % 3 == 0
                                                             ? (ch->y > 15 ? Tiles::createLava()
                                                                           : Tiles::createWater())
                                                             : Tiles::NOTHING;
                        } else {
                            chunk[x + y * CHUNK_W] = Tiles::NOTHING;
                        }
                    } else {
                        double n3 = world->noise.GetPerlin(px / 64.0, py / 64.0, 9828);
                        if (n3 - 0.25 > py / 1000.0) { chunk[x + y * CHUNK_W] = Tiles::NOTHING; }
                    }
                }
            }
        }
        return {};
    }
};

class CobblePopulator : public Populator {
public:
    int getPhase() { return 1; }

    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk **area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk *ch, World *world) {

        if (ch->y < 0) return {};

        //int dist = 8 + (world->noise.GetNoise(px * 4, py * 4, 3323) + 1) / 2 * 12;
        //int dist2 = dist - 6;

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

                if (area[chx + chy * 3]->tiles[(dxx) + (dyy) *CHUNK_W].mat->physicsType ==
                            PhysicsType::AIR ||
                    (area[chx + chy * 3]->tiles[(dxx) + (dyy) *CHUNK_W].mat->physicsType ==
                             PhysicsType::SAND &&
                     area[chx + chy * 3]->tiles[(dxx) + (dyy) *CHUNK_W].mat->id !=
                             Materials::SOFT_DIRT.id)) {
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
                                if (area[1 + 1 * 3]->tiles[(sdxx) + (sdyy) *CHUNK_W].mat->id ==
                                            Materials::SMOOTH_STONE.id ||
                                    area[1 + 1 * 3]->tiles[(sdxx) + (sdyy) *CHUNK_W].mat->id ==
                                            Materials::FLAT_COBBLE_STONE.id) {

                                    chunk[sdxx + sdyy * CHUNK_W] =
                                            Tiles::createCobbleStone(sx + dx, sy + dy);

                                } else if (area[1 + 1 * 3]->tiles[(sdxx) + (sdyy) *CHUNK_W]
                                                           .mat->id == Materials::SMOOTH_DIRT.id ||
                                           area[1 + 1 * 3]->tiles[(sdxx) + (sdyy) *CHUNK_W]
                                                           .mat->id ==
                                                   Materials::FLAT_COBBLE_DIRT.id) {

                                    chunk[sdxx + sdyy * CHUNK_W] =
                                            Tiles::createCobbleDirt(sx + dx, sy + dy);
                                }
                            } else {
                                if (area[1 + 1 * 3]->tiles[(sdxx) + (sdyy) *CHUNK_W].mat->id ==
                                    Materials::SMOOTH_STONE.id) {

                                    chunk[sdxx + sdyy * CHUNK_W] = Tiles::create(
                                            &Materials::FLAT_COBBLE_STONE, sx + dx, sy + dy);

                                } else if (area[1 + 1 * 3]
                                                   ->tiles[(sdxx) + (sdyy) *CHUNK_W]
                                                   .mat->id == Materials::SMOOTH_DIRT.id) {

                                    chunk[sdxx + sdyy * CHUNK_W] = Tiles::create(
                                            &Materials::FLAT_COBBLE_DIRT, sx + dx, sy + dy);
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
};

class OrePopulator : public Populator {
public:
    int getPhase() { return 0; }

    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk **area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk *ch, World *world) {

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
};

class TreePopulator : public Populator {
public:
    int getPhase() { return 1; }

    std::vector<PlacedStructure> apply(MaterialInstance *chunk, MaterialInstance *layer2,
                                       Chunk **area, bool *dirty, int tx, int ty, int tw, int th,
                                       Chunk *ch, World *world) {
        if (ch->y < 0 || ch->y > 3) return {};
        int x = rand() % (CHUNK_W / 2) + (CHUNK_W / 4);
        if (area[1 + 2 * 3]->tiles[x + 0 * CHUNK_W].mat->id == Materials::SOFT_DIRT.id) return {};

        for (int y = 0; y < CHUNK_H; y++) {
            if (area[1 + 2 * 3]->tiles[x + y * CHUNK_W].mat->id == Materials::SOFT_DIRT.id) {
                int px = x + ch->x * CHUNK_W;
                int py = y + (ch->y + 1) * CHUNK_W;
                /*Structure tree = Structures::makeTree1(*world, px, py);
                px -= tree.w / 2;
                py -= tree.h - 2;*/

                /*for (int tx = 0; tx < tree.w; tx++) {
                    for (int ty = 0; ty < tree.h; ty++) {
                        int chx = (int)floor((tx + px) / (float)CHUNK_W) + 1 - ch.x;
                        int chy = (int)floor((ty + py) / (float)CHUNK_H) + 1 - ch.y;
                        if (chx < 0 || chy < 0 || chx > 2 || chy > 2) continue;
                        int dxx = (CHUNK_W + ((tx + px) % CHUNK_W)) % CHUNK_W;
                        int dyy = (CHUNK_H + ((ty + py) % CHUNK_H)) % CHUNK_H;
                        if (tree.tiles[tx + ty * tree.w].mat->physicsType != PhysicsType::AIR && area[chx + chy * 3].tiles[dxx + dyy * CHUNK_W].mat->physicsType == PhysicsType::AIR && area[chx + chy * 3].layer2[dxx + dyy * CHUNK_W].mat->physicsType == PhysicsType::AIR) {
                            area[chx + chy * 3].layer2[dxx + dyy * CHUNK_W] = tree.tiles[tx + ty * tree.w];
                            dirty[chx + chy * 3] = true;
                        }
                    }
                }*/

                char buff[40];
                //snprintf(buff, sizeof(buff), "data/assets/objects/tree%d.png", rand() % 8 + 1);
                snprintf(buff, sizeof(buff), "data/assets/objects/tree1.png");
                //snprintf(buff, sizeof(buff), "data/assets/objects/testTree.png");
                std::string buffAsStdStr = buff;
                C_Surface *tex = Textures::LoadTexture(buffAsStdStr.c_str());

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
                    if (((METADOT_GET_PIXEL(tex, texX, tex->h - 1) >> 24) & 0xff) != 0x00) {
                        rb->weldX = texX;
                        rb->weldY = tex->h - 1;
                        break;
                    }
                }
                world->rigidBodies.push_back(rb);
                world->updateRigidBodyHitbox(rb);

                return {};
            }
        }
        return {};
    }
};

#endif