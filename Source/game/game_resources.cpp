// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "game_resources.hpp"

#include <string.h>

#include "core/alloc.h"
#include "core/core.h"
#include "engine/filesystem.h"
#include "engine/renderer/renderer_gpu.h"
#include "engine/sdl_wrapper.h"
#include "libs/Ase_Loader.h"
#include "libs/external/stb_image.h"

void InitTexture(TexturePack *tex) {
    METADOT_ASSERT_E(tex);

    tex->testTexture = LoadTexture("data/assets/textures/test.png");
    tex->dirt1Texture = LoadTexture("data/assets/textures/testDirt.png");
    tex->stone1Texture = LoadTexture("data/assets/textures/testStone.png");
    tex->smoothStone = LoadTexture("data/assets/textures/smooth_stone_128x.png");
    tex->cobbleStone = LoadTexture("data/assets/textures/cobble_stone_128x.png");
    tex->flatCobbleStone = LoadTexture("data/assets/textures/flat_cobble_stone_128x.png");
    tex->smoothDirt = LoadTexture("data/assets/textures/smooth_dirt_128x.png");
    tex->cobbleDirt = LoadTexture("data/assets/textures/cobble_dirt_128x.png");
    tex->flatCobbleDirt = LoadTexture("data/assets/textures/flat_cobble_dirt_128x.png");
    tex->softDirt = LoadTexture("data/assets/textures/soft_dirt.png");
    tex->cloud = LoadTexture("data/assets/textures/cloud.png");
    tex->gold = LoadTexture("data/assets/textures/gold.png");
    tex->goldMolten = LoadTexture("data/assets/textures/moltenGold.png");
    tex->goldSolid = LoadTexture("data/assets/textures/solidGold.png");
    tex->iron = LoadTexture("data/assets/textures/iron.png");
    tex->obsidian = LoadTexture("data/assets/textures/obsidian.png");
    tex->caveBG = LoadTexture("data/assets/backgrounds/testCave.png");
}

void EndTexture(TexturePack *tex) { METADOT_ASSERT_E(tex); }

C_Surface *LoadTextureData(const char *path) { return LoadTextureInternal(path, SDL_PIXELFORMAT_ARGB8888); }

C_Surface *LoadTexture(const char *path) { return LoadTextureInternal(path, SDL_PIXELFORMAT_ARGB8888); }

C_Surface *LoadTextureInternal(const char *path, U32 pixelFormat) {

    // https://wiki.libsdl.org/SDL_CreateRGBSurfaceFrom

    // the color format you request stb_image to output,
    // use STBI_rgb if you don't want/need the alpha channel
    int req_format = STBI_rgb_alpha;
    int width, height, orig_format;
    unsigned char *data = stbi_load(METADOT_RESLOC(path), &width, &height, &orig_format, req_format);
    if (data == NULL) {
        METADOT_ERROR("Loading image failed: %s %s", stbi_failure_reason(), METADOT_RESLOC(path));
    }

    // Set up the pixel format color masks for RGB(A) byte arrays.
    // Only STBI_rgb (3) and STBI_rgb_alpha (4) are supported here!
    U32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int shift = (req_format == STBI_rgb) ? 8 : 0;
    rmask = 0xff000000 >> shift;
    gmask = 0x00ff0000 >> shift;
    bmask = 0x0000ff00 >> shift;
    amask = 0x000000ff >> shift;
#else  // little endian, like x86
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = (req_format == STBI_rgb) ? 0 : 0xff000000;
#endif

    int depth, pitch;
    if (req_format == STBI_rgb) {
        depth = 24;
        pitch = 3 * width;  // 3 bytes per pixel * pixels per row
    } else {                // STBI_rgb_alpha (RGBA)
        depth = 32;
        pitch = 4 * width;
    }

    C_Surface *loadedSurface = SDL_CreateRGBSurfaceFrom((void *)data, width, height, depth, pitch, rmask, gmask, bmask, amask);

    // stbi_image_free(data);

    METADOT_ASSERT_E(loadedSurface);

    return loadedSurface;
}

C_Surface *ScaleTexture(C_Surface *src, F32 x, F32 y) {
    C_Surface *dest = SDL_CreateRGBSurface(src->flags, src->w * x, src->h * y, src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);

    C_Rect *srcR = (C_Rect *)gc_malloc(&gc, sizeof(C_Rect));
    srcR->w = src->w;
    srcR->h = src->h;

    C_Rect *dstR = (C_Rect *)gc_malloc(&gc, sizeof(C_Rect));
    dstR->w = dest->w;
    dstR->h = dest->h;

    SDL_FillRect(dest, dstR, 0x00000000);

    SDL_SetSurfaceBlendMode(src,
                            SDL_BLENDMODE_NONE);  // override instead of overlap (prevents transparent things darkening)

    SDL_BlitScaled(src, srcR, dest, dstR);

    gc_free(&gc, srcR);
    gc_free(&gc, dstR);

    return dest;
}

C_Surface *LoadAseprite(const char *path) {

    Ase_Output *ase = Ase_Load(METADOT_RESLOC(path));

    if (NULL == ase) return nullptr;

    SDL_PixelFormatEnum pixel_format;
    if (ase->bpp == 1) {
        pixel_format = SDL_PIXELFORMAT_INDEX8;
    } else if (ase->bpp == 4) {
        pixel_format = SDL_PIXELFORMAT_RGBA32;
    } else {
        METADOT_ERROR("Test %d BPP not supported!", ase->bpp);
    }

    C_Surface *surface =
            SDL_CreateRGBSurfaceWithFormatFrom(ase->pixels, ase->frame_width * ase->num_frames, ase->frame_height, ase->bpp * 8, ase->bpp * ase->frame_width * ase->num_frames, pixel_format);
    if (!surface) METADOT_ERROR("Surface could not be created!, %s", SDL_GetError());
    SDL_SetPaletteColors(surface->format->palette, (SDL_Color *)&ase->palette.entries, 0, ase->palette.num_entries);
    SDL_SetColorKey(surface, SDL_TRUE, ase->palette.color_key);

    return surface;
}