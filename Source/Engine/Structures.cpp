// Copyright(c) 2019 - 2022, KaoruXun All rights reserved.


#include "Structures.hpp"
#include "Macros.hpp"
#include "world.hpp"

#ifndef INC_Textures
#include "Textures.hpp"
#endif

#undef min
#undef max

#include <algorithm>


Structure::Structure(int w, int h, MaterialInstance *tiles) {
    this->w = w;
    this->h = h;
    this->tiles = tiles;
}

Structure::Structure(SDL_Surface *texture, Material mat) {
    MaterialInstance *tiles = new MaterialInstance[texture->w * texture->h];
    for (int x = 0; x < texture->w; x++) {
        for (int y = 0; y < texture->h; y++) {
            Uint32 color = METADOT_GET_PIXEL(texture, x, y);
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
            tiles[((int) cx + xx - (int) (dcx * (h - 30))) + ty * w] = MaterialInstance(&Materials::GENERIC_PASSABLE, xx >= 2 ? 0x683600 : 0x7C4000);
        }
        cx += dcx;
    }

    for (int theta = 0; theta < 360; theta += 1) {
        double p = world.noise.GetPerlin(std::cos(theta * 3.1415 / 180.0) * 4 + x, std::sin(theta * 3.1415 / 180.0) * 4 + y, 2652);
        float r = 15 + (float) p * 6;
        for (float d = 0; d < r; d += 0.5) {
            int tx = cx - (int) (dcx * (h - 30)) + d * std::cos(theta * 3.1415 / 180.0);
            int ty = 20 + d * std::sin(theta * 3.1415 / 180.0);
            if (tx >= 0 && ty >= 0 && tx < w && ty < h) {
                tiles[tx + ty * w] = MaterialInstance(&Materials::GENERIC_PASSABLE, r - d < 0.5f ? 0x00aa00 : 0x00ff00);
            }
        }
    }

    int nBranches = rand() % 3;
    bool side = rand() % 2;// false = right, true = left
    for (int i = 0; i < nBranches; i++) {
        int yPos = 20 + (h - 20) / 3 * (i + 1) + rand() % 10;
        float tilt = ((rand() % 10) / 10.0 - 0.5) * 8;
        int len = 10 + rand() % 5;
        for (int xx = 0; xx < len; xx++) {
            int tx = (int) (w / 2 + dcx * (h - yPos)) + (side ? 1 : -1) * (xx + 2) - (int) (dcx * (h - 30));
            int th = 3 * (1 - (xx / (float) len));
            for (int yy = -th; yy <= th; yy++) {
                int ty = yPos + yy + (xx / (float) len * tilt);
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
    return Structure(Textures::loadTexture(buffAsStdStr.c_str()), Materials::GENERIC_PASSABLE);
}

PlacedStructure::PlacedStructure(Structure base, int x, int y) {
    this->base = base;
    this->x = x;
    this->y = y;
}
