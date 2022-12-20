// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_GAMERESOURCES_H_
#define _METADOT_GAMERESOURCES_H_

#include "core/core.h"
#include "engine/sdl_wrapper.h"

typedef struct TexturePack {
    C_Surface *testTexture;
    C_Surface *dirt1Texture;
    C_Surface *stone1Texture;
    C_Surface *smoothStone;
    C_Surface *cobbleStone;
    C_Surface *flatCobbleStone;
    C_Surface *smoothDirt;
    C_Surface *cobbleDirt;
    C_Surface *flatCobbleDirt;
    C_Surface *softDirt;
    C_Surface *cloud;
    C_Surface *gold;
    C_Surface *goldMolten;
    C_Surface *goldSolid;
    C_Surface *iron;
    C_Surface *obsidian;
    C_Surface *caveBG;
    C_Surface *testAse;
} TexturePack;

void InitTexture(TexturePack *tex);
void EndTexture(TexturePack *tex);

C_Surface *LoadTexture(const char *path);
C_Surface *LoadTextureInternal(const char *path, U32 pixelFormat);
C_Surface *LoadAseprite(const char *path);
C_Surface *ScaleTexture(C_Surface *, float x, float y);
C_Surface *LoadTextureData(const char *path);

#endif
