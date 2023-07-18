// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_TEXTURES_HPP
#define ME_TEXTURES_HPP

#include "engine/core/core.hpp"
#include "engine/core/sdl_wrapper.h"
#include "engine/engine.h"
#include "engine/renderer/renderer_gpu.h"

typedef struct Texture {
    C_Surface *surface;
} Texture;

typedef struct TexturePack {
    Texture *testTexture;
    Texture *dirt1Texture;
    Texture *stone1Texture;
    Texture *smoothStone;
    Texture *cobbleStone;
    Texture *flatCobbleStone;
    Texture *smoothDirt;
    Texture *cobbleDirt;
    Texture *flatCobbleDirt;
    Texture *softDirt;
    Texture *cloud;
    Texture *gold;
    Texture *goldMolten;
    Texture *goldSolid;
    Texture *iron;
    Texture *obsidian;
    Texture *caveBG;
    Texture *testAse;

    Texture *testVacuum;
    Texture *testHammer;
    Texture *testPickaxe;
    Texture *testBucket;
} TexturePack;

void InitTexture(TexturePack *tex);
void EndTexture(TexturePack *tex);

Texture *CreateTexture(C_Surface *surface);
void DestroyTexture(Texture *tex);
Texture *LoadTexture(const char *path);
Texture *LoadTextureInternal(const char *path, u32 pixelFormat);
C_Surface *ScaleSurface(C_Surface *src, f32 x, f32 y);
Texture *LoadAsepriteTexture(const char *path);
Texture *LoadTextureData(const char *path);
void RenderTextureRect(Texture *tex, R_Target *target, int x, int y, MErect *clip = nullptr);

#endif
