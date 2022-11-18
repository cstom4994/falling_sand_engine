// Copyright(c) 2022, KaoruXun All rights reserved.


#include "Game/InEngine.h"

#include "Game/Textures.hpp"

#include "Game/FileSystem.hpp"
#include "Game/DebugImpl.hpp"

#include "external/stb_image.h"

C_Surface *Textures::testTexture = nullptr;
C_Surface *Textures::dirt1Texture = nullptr;
C_Surface *Textures::stone1Texture = nullptr;
C_Surface *Textures::smoothStone = nullptr;
C_Surface *Textures::cobbleStone = nullptr;
C_Surface *Textures::flatCobbleStone = nullptr;
C_Surface *Textures::smoothDirt = nullptr;
C_Surface *Textures::cobbleDirt = nullptr;
C_Surface *Textures::flatCobbleDirt = nullptr;
C_Surface *Textures::softDirt = nullptr;
C_Surface *Textures::cloud = nullptr;
C_Surface *Textures::gold = nullptr;
C_Surface *Textures::goldMolten = nullptr;
C_Surface *Textures::goldSolid = nullptr;
C_Surface *Textures::iron = nullptr;
C_Surface *Textures::obsidian = nullptr;
C_Surface *Textures::caveBG = nullptr;

void Textures::initTexture() {
    testTexture = Textures::loadTexture("data/assets/textures/test.png");
    dirt1Texture = Textures::loadTexture("data/assets/textures/testDirt.png");
    stone1Texture = Textures::loadTexture("data/assets/textures/testStone.png");
    smoothStone = Textures::loadTexture("data/assets/textures/smooth_stone_128x.png");
    cobbleStone = Textures::loadTexture("data/assets/textures/cobble_stone_128x.png");
    flatCobbleStone = Textures::loadTexture("data/assets/textures/flat_cobble_stone_128x.png");
    smoothDirt = Textures::loadTexture("data/assets/textures/smooth_dirt_128x.png");
    cobbleDirt = Textures::loadTexture("data/assets/textures/cobble_dirt_128x.png");
    flatCobbleDirt = Textures::loadTexture("data/assets/textures/flat_cobble_dirt_128x.png");
    softDirt = Textures::loadTexture("data/assets/textures/soft_dirt.png");
    cloud = Textures::loadTexture("data/assets/textures/cloud.png");
    gold = Textures::loadTexture("data/assets/textures/gold.png");
    goldMolten = Textures::loadTexture("data/assets/textures/moltenGold.png");
    goldSolid = Textures::loadTexture("data/assets/textures/solidGold.png");
    iron = Textures::loadTexture("data/assets/textures/iron.png");
    obsidian = Textures::loadTexture("data/assets/textures/obsidian.png");
    caveBG = Textures::loadTexture("data/assets/backgrounds/testCave.png");
}

C_Surface *Textures::loadTexture(std::string path) {
    return loadTexture(path, SDL_PIXELFORMAT_ARGB8888);
}

C_Surface *Textures::loadTexture(std::string path, Uint32 pixelFormat) {

    // https://wiki.libsdl.org/SDL_CreateRGBSurfaceFrom

    // the color format you request stb_image to output,
    // use STBI_rgb if you don't want/need the alpha channel
    int req_format = STBI_rgb_alpha;
    int width, height, orig_format;
    unsigned char *data = stbi_load(METADOT_RESLOC_STR(path), &width, &height, &orig_format, req_format);
    if (data == NULL) {
        std::cout << "Loading image failed: " << stbi_failure_reason() << " ::" << METADOT_RESLOC_STR(path) << std::endl;
    }

    // Set up the pixel format color masks for RGB(A) byte arrays.
    // Only STBI_rgb (3) and STBI_rgb_alpha (4) are supported here!
    Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int shift = (req_format == STBI_rgb) ? 8 : 0;
    rmask = 0xff000000 >> shift;
    gmask = 0x00ff0000 >> shift;
    bmask = 0x0000ff00 >> shift;
    amask = 0x000000ff >> shift;
#else// little endian, like x86
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = (req_format == STBI_rgb) ? 0 : 0xff000000;
#endif

    int depth, pitch;
    if (req_format == STBI_rgb) {
        depth = 24;
        pitch = 3 * width;// 3 bytes per pixel * pixels per row
    } else {              // STBI_rgb_alpha (RGBA)
        depth = 32;
        pitch = 4 * width;
    }


    C_Surface *loadedSurface = SDL_CreateRGBSurfaceFrom((void *) data, width, height, depth, pitch,
                                                          rmask, gmask, bmask, amask);

    //stbi_image_free(data);

    METADOT_ASSERT_E(loadedSurface);

    return loadedSurface;
}

C_Surface *Textures::scaleTexture(C_Surface *src, float x, float y) {
    C_Surface *dest = SDL_CreateRGBSurface(src->flags, src->w * x, src->h * y, src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);

    SDL_Rect *srcR = new SDL_Rect();
    srcR->w = src->w;
    srcR->h = src->h;

    SDL_Rect *dstR = new SDL_Rect();
    dstR->w = dest->w;
    dstR->h = dest->h;

    SDL_FillRect(dest, dstR, 0x00000000);

    SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_NONE);// override instead of overlap (prevents transparent things darkening)

    SDL_BlitScaled(src, srcR, dest, dstR);

    delete srcR;
    delete dstR;

    return dest;
}
