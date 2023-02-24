// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_GAMERESOURCES_HPP_
#define _METADOT_GAMERESOURCES_HPP_

#include "core/core.h"
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
Texture *LoadTextureInternal(const char *path, U32 pixelFormat);
C_Surface *ScaleSurface(C_Surface *src, F32 x, F32 y);
Texture *LoadAsepriteTexture(const char *path);
Texture *LoadTextureData(const char *path);
void RenderTextureRect(Texture *tex, R_Target *target, int x, int y, metadot_rect *clip = nullptr);

#endif
