// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_GAMERESOURCES_HPP
#define ME_GAMERESOURCES_HPP

#include "core/core.hpp"
#include "core/sdl_wrapper.h"
#include "engine/engine.h"
#include "renderer/renderer_gpu.h"

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
void RenderTextureRect(Texture *tex, R_Target *target, int x, int y, metadot_rect *clip = nullptr);

#endif
