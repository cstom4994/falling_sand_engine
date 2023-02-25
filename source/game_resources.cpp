// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "game_resources.hpp"

#include <string.h>

#include "core/sdl_wrapper.h"
#include "core/alloc.hpp"
#include "core/core.h"
#include "core/io/filesystem.h"
#include "core/sdl_wrapper.h"
#include "engine/engine.h"
#include "libs/external/stb_image.h"
#include "renderer/renderer_gpu.h"

#define CUTE_ASEPRITE_IMPLEMENTATION
#include "libs/cute/cute_aseprite.h"

IMPLENGINE();

void InitTexture(TexturePack *tex) {
    METADOT_ASSERT_E(tex);

    tex->testTexture = LoadTexture("data/assets/textures/test.png");
    tex->dirt1Texture = LoadTexture("data/assets/textures/testDirt.png");
    tex->stone1Texture = LoadTexture("data/assets/textures/testStone.png");
    tex->smoothStone = LoadTexture("data/assets/textures/smooth_stone.png");
    tex->cobbleStone = LoadTexture("data/assets/textures/cobble_stone.png");
    tex->flatCobbleStone = LoadTexture("data/assets/textures/flat_cobble_stone.png");
    tex->smoothDirt = LoadTexture("data/assets/textures/smooth_dirt.png");
    tex->cobbleDirt = LoadTexture("data/assets/textures/cobble_dirt.png");
    tex->flatCobbleDirt = LoadTexture("data/assets/textures/flat_cobble_dirt.png");
    tex->softDirt = LoadTexture("data/assets/textures/soft_dirt.png");
    tex->cloud = LoadTexture("data/assets/textures/cloud.png");
    tex->gold = LoadTexture("data/assets/textures/gold.png");
    tex->goldMolten = LoadTexture("data/assets/textures/moltenGold.png");
    tex->goldSolid = LoadTexture("data/assets/textures/solidGold.png");
    tex->iron = LoadTexture("data/assets/textures/iron.png");
    tex->obsidian = LoadTexture("data/assets/textures/obsidian.png");
    tex->caveBG = LoadTexture("data/assets/backgrounds/testCave.png");
}

void EndTexture(TexturePack *tex) {
    METADOT_ASSERT_E(tex);

    DestroyTexture(tex->testTexture);
    DestroyTexture(tex->dirt1Texture);
    DestroyTexture(tex->stone1Texture);
    DestroyTexture(tex->smoothStone);
    DestroyTexture(tex->cobbleStone);
    DestroyTexture(tex->flatCobbleStone);
    DestroyTexture(tex->smoothDirt);
    DestroyTexture(tex->cobbleDirt);
    DestroyTexture(tex->flatCobbleDirt);
    DestroyTexture(tex->softDirt);
    DestroyTexture(tex->cloud);
    DestroyTexture(tex->gold);
    DestroyTexture(tex->goldMolten);
    DestroyTexture(tex->goldSolid);
    DestroyTexture(tex->iron);
    DestroyTexture(tex->obsidian);
    DestroyTexture(tex->caveBG);
    DestroyTexture(tex->testAse);
}

Texture *CreateTexture(C_Surface *surface) {
    Texture *tex = new Texture;
    tex->surface = surface;
    return tex;
}

void DestroyTexture(Texture *tex) {
    METADOT_ASSERT_E(tex);
    if (tex->surface) SDL_FreeSurface(tex->surface);
    delete tex;
}

Texture *LoadTextureData(const char *path) { return LoadTextureInternal(path, SDL_PIXELFORMAT_ARGB8888); }

Texture *LoadTexture(const char *path) { return LoadTextureInternal(path, SDL_PIXELFORMAT_ARGB8888); }

Texture *LoadTextureInternal(const char *path, U32 pixelFormat) {

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

    C_Surface *loadedSurface = nullptr;

    loadedSurface = SDL_CreateRGBSurfaceFrom((void *)data, width, height, depth, pitch, rmask, gmask, bmask, amask);
    loadedSurface = SDL_ConvertSurfaceFormat(loadedSurface, pixelFormat, 0);

    METADOT_ASSERT_E(loadedSurface);

    Texture *tex = CreateTexture(loadedSurface);

    stbi_image_free(data);

    return tex;
}

C_Surface *ScaleSurface(C_Surface *src, F32 x, F32 y) {
    C_Surface *dest = SDL_CreateRGBSurface(src->flags, src->w * x, src->h * y, src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);

    C_Rect *srcR = new C_Rect;
    srcR->w = src->w;
    srcR->h = src->h;

    C_Rect *dstR = new C_Rect;
    dstR->w = dest->w;
    dstR->h = dest->h;

    SDL_FillRect(dest, dstR, 0x00000000);
    SDL_SetSurfaceBlendMode(src,
                            SDL_BLENDMODE_NONE);  // override instead of overlap (prevents transparent things darkening)
    SDL_BlitScaled(src, srcR, dest, dstR);

    delete srcR;
    delete dstR;
    src = dest;
    return src;
}

Texture *LoadAsepriteTexture(const char *path) {

    ase_t *ase = cute_aseprite_load_from_file(METADOT_RESLOC(path), NULL);
    if (NULL == ase) {
        METADOT_ERROR("Unable to load ase %s", path);
        return nullptr;
    }

    ase_frame_t *frame = ase->frames;

    SDL_PixelFormatEnum pixel_format;
    // if (ase->bpp == 1) {
    //     pixel_format = SDL_PIXELFORMAT_INDEX8;
    // } else if (ase->bpp == 4) {
    //     pixel_format = SDL_PIXELFORMAT_RGBA32;
    // } else {
    //     METADOT_ERROR("Aseprite %d BPP not supported!", ase->bpp);
    // }
    pixel_format = SDL_PIXELFORMAT_RGBA32;
    int bpp = 4;

    METADOT_BUG("Aseprite %d %d", ase->frame_count, ase->palette.entry_count);

    C_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(frame->pixels, ase->w * ase->frame_count, ase->h, bpp * 8, bpp * ase->w * ase->frame_count, pixel_format);
    surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
    if (!surface) METADOT_ERROR("Surface could not be created!, %s", SDL_GetError());
    SDL_SetPaletteColors(surface->format->palette, (SDL_Color *)&ase->palette.entries, 0, ase->palette.entry_count);
    // SDL_SetColorKey(surface, SDL_TRUE, ase->color_profile);

    Texture *tex = CreateTexture(surface);

    cute_aseprite_free(ase);

    return tex;
}

void RenderTextureRect(Texture *tex, R_Target *target, int x, int y, metadot_rect *clip) {
    metadot_rect dst;
    dst.x = x;
    dst.y = y;
    if (clip != nullptr) {
        dst.w = clip->w;
        dst.h = clip->h;
    }
    auto image = R_CopyImageFromSurfaceRect(tex->surface, clip);
    METADOT_ASSERT_E(image);
    R_BlitRect(image, NULL, target, &dst);
}