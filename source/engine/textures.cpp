// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "textures.hpp"

#include <string.h>

#include "engine/core/base_memory.h"
#include "engine/core/core.hpp"
#include "engine/core/io/filesystem.h"
#include "engine/core/sdl_wrapper.h"
#include "engine/engine.hpp"
#include "engine/renderer/renderer_gpu.h"
#include "libs/external/stb_image.h"

#define CUTE_ASEPRITE_IMPLEMENTATION
#include "libs/cute/cute_aseprite.h"

namespace ME {

Texture::Texture(const std::string &path) {
    if (ME_fs_exists(METADOT_RESLOC(path))) {

    } else {
        METADOT_ERROR("Unable to load texture ", path);
    }
}

Texture::Texture(C_Surface *sur, bool init_image) noexcept {
    ME_ASSERT(sur);
    m_surface = sur;

    if (init_image) {
        // 复制surface到可渲染图像
        m_image = R_CopyImageFromSurface(m_surface);

        // 设置默认过滤
        R_SetImageFilter(m_image, R_FILTER_NEAREST);
    }
}

Texture::~Texture() {
    if (m_image) R_FreeImage(m_image);
    SDL_FreeSurface(m_surface);
}

void InitTexture(TexturePack &tex) {
    tex.testTexture = LoadTexture("data/assets/textures/test.png");
    tex.dirt1Texture = LoadTexture("data/assets/textures/testDirt.png");
    tex.stone1Texture = LoadTexture("data/assets/textures/testStone.png");
    tex.smoothStone = LoadTexture("data/assets/textures/smooth_stone.png");
    tex.cobbleStone = LoadTexture("data/assets/textures/cobble_stone.png");
    tex.flatCobbleStone = LoadTexture("data/assets/textures/flat_cobble_stone.png");
    tex.smoothDirt = LoadTexture("data/assets/textures/smooth_dirt.png");
    tex.cobbleDirt = LoadTexture("data/assets/textures/cobble_dirt.png");
    tex.flatCobbleDirt = LoadTexture("data/assets/textures/flat_cobble_dirt.png");
    tex.softDirt = LoadTexture("data/assets/textures/soft_dirt.png");
    tex.cloud = LoadTexture("data/assets/textures/cloud.png");
    tex.gold = LoadTexture("data/assets/textures/gold.png");
    tex.goldMolten = LoadTexture("data/assets/textures/moltenGold.png");
    tex.goldSolid = LoadTexture("data/assets/textures/solidGold.png");
    tex.iron = LoadTexture("data/assets/textures/iron.png");
    tex.obsidian = LoadTexture("data/assets/textures/obsidian.png");
    tex.caveBG = LoadTextureInternal("data/assets/backgrounds/testCave.png", SDL_PIXELFORMAT_ARGB8888, false);

    // Test aseprite
    tex.testAse = LoadAsepriteTexture("data/assets/textures/Sprite-0003.ase", false);

    tex.testVacuum = LoadTexture("data/assets/objects/testVacuum.png");
    tex.testBucket = LoadTexture("data/assets/objects/testBucket.png");
    tex.testBucketFilled = LoadTexture("data/assets/objects/testBucket_fill.png");
    tex.testPickaxe = LoadTexture("data/assets/objects/testPickaxe.png");
    tex.testHammer = LoadTexture("data/assets/objects/testHammer.png");
}

void EndTexture(TexturePack &tex) {
    tex.testTexture.reset();
    tex.dirt1Texture.reset();
    tex.stone1Texture.reset();
    tex.smoothStone.reset();
    tex.cobbleStone.reset();
    tex.flatCobbleStone.reset();
    tex.smoothDirt.reset();
    tex.cobbleDirt.reset();
    tex.flatCobbleDirt.reset();
    tex.softDirt.reset();
    tex.cloud.reset();
    tex.gold.reset();
    tex.goldMolten.reset();
    tex.goldSolid.reset();
    tex.iron.reset();
    tex.obsidian.reset();
    tex.caveBG.reset();

    tex.testAse.reset();

    tex.testVacuum.reset();
    tex.testBucket.reset();
    tex.testBucketFilled.reset();
    tex.testPickaxe.reset();
    tex.testHammer.reset();
}

TextureRef LoadTexture(const std::string &path) { return LoadTextureInternal(path, SDL_PIXELFORMAT_ARGB8888); }

TextureRef LoadTextureInternal(const std::string &path, u32 pixelFormat, bool init_image) {

    // 可以在这里找到SDL相关函数
    // https://wiki.libsdl.org/SDL_CreateRGBSurfaceFrom

    // 设置要求 stb_image 输出的颜色格式
    // 如果不想/不需要 alpha 通道 则使用 STBI_rgb 而这里默认启用 alpha 通道
    int req_format = STBI_rgb_alpha;
    int width, height, orig_format;
    unsigned char *data = stbi_load(METADOT_RESLOC(path), &width, &height, &orig_format, req_format);
    if (data == NULL) {
        METADOT_ERROR("Loading image failed: %s %s", stbi_failure_reason(), METADOT_RESLOC(path));
    }

    // 设置 RGB(A) 字节数组的像素格式颜色掩码
    // 这里仅支持 STBI_rgb (3) 和 STBI_rgb_alpha (4)
    u32 rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int shift = (req_format == STBI_rgb) ? 8 : 0;
    rmask = 0xff000000 >> shift;
    gmask = 0x00ff0000 >> shift;
    bmask = 0x0000ff00 >> shift;
    amask = 0x000000ff >> shift;
#else  // 小端 x86
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
    C_Surface *loadedSurface_converted = SDL_ConvertSurfaceFormat(loadedSurface, pixelFormat, 0);

    SDL_FreeSurface(loadedSurface);

    ME_ASSERT(loadedSurface_converted);

    TextureRef tex = create_ref<Texture>(loadedSurface_converted, init_image);

    stbi_image_free(data);

    return tex;
}

C_Surface *ScaleSurface(C_Surface *src, f32 x, f32 y) {
    C_Surface *dest = SDL_CreateRGBSurface(src->flags, src->w * x, src->h * y, src->format->BitsPerPixel, src->format->Rmask, src->format->Gmask, src->format->Bmask, src->format->Amask);

    C_Rect srcR;
    srcR.w = src->w;
    srcR.h = src->h;

    C_Rect dstR;
    dstR.w = dest->w;
    dstR.h = dest->h;

    SDL_FillRect(dest, &dstR, 0x00000000);
    SDL_SetSurfaceBlendMode(src,
                            SDL_BLENDMODE_NONE);  // override instead of overlap (prevents transparent things darkening)
    SDL_BlitScaled(src, &srcR, dest, &dstR);

    return src;
}

TextureRef LoadAsepriteTexture(const std::string &path, bool init_image) {

    ase_t *ase = cute_aseprite_load_from_file(METADOT_RESLOC(path), NULL);
    if (NULL == ase) {
        METADOT_ERROR("Unable to load ase ", path);
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

    METADOT_BUG(std::format("Aseprite {0} {1}", ase->frame_count, ase->palette.entry_count).c_str());

    C_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(frame->pixels, ase->w * ase->frame_count, ase->h, bpp * 8, bpp * ase->w * ase->frame_count, pixel_format);

    C_Surface *surface_converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);

    SDL_FreeSurface(surface);

    if (!surface_converted) METADOT_ERROR("Surface could not be created!, ", SDL_GetError());

    SDL_SetPaletteColors(surface_converted->format->palette, (SDL_Color *)&ase->palette.entries, 0, ase->palette.entry_count);
    // SDL_SetColorKey(surface, SDL_TRUE, ase->color_profile);

    ME_ASSERT(surface_converted);

    TextureRef tex = create_ref<Texture>(surface_converted, init_image);

    cute_aseprite_free(ase);

    return tex;
}

void RenderTextureRect(TextureRef tex, R_Target *target, int x, int y, MErect *clip) {
    MErect dst;
    dst.x = x;
    dst.y = y;
    if (clip != nullptr) {
        dst.w = clip->w;
        dst.h = clip->h;
    }
    auto image = R_CopyImageFromSurfaceRect(tex->surface(), clip);
    ME_ASSERT(image);
    R_BlitRect(image, NULL, target, &dst);
}
}  // namespace ME