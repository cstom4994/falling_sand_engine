// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_TEXTURES_HPP
#define ME_TEXTURES_HPP

#include "engine/core/core.hpp"
#include "engine/core/sdl_wrapper.h"
#include "engine/engine.h"
#include "engine/renderer/renderer_gpu.h"

// 贴图类
class Texture {
private:
    C_Surface *m_surface = nullptr;
    R_Image *m_image = nullptr;

public:
    Texture(const std::string &path);
    Texture(C_Surface *sur) noexcept;
    ~Texture();

    C_Surface *surface() const { return m_surface; }
    R_Image *image() const { return m_image; }
};

// 贴图引用
using TextureRef = ME::ref<Texture>;

struct TexturePack {
    TextureRef testTexture;
    TextureRef dirt1Texture;
    TextureRef stone1Texture;
    TextureRef smoothStone;
    TextureRef cobbleStone;
    TextureRef flatCobbleStone;
    TextureRef smoothDirt;
    TextureRef cobbleDirt;
    TextureRef flatCobbleDirt;
    TextureRef softDirt;
    TextureRef cloud;
    TextureRef gold;
    TextureRef goldMolten;
    TextureRef goldSolid;
    TextureRef iron;
    TextureRef obsidian;
    TextureRef caveBG;
    TextureRef testAse;

    TextureRef testVacuum;
    TextureRef testHammer;
    TextureRef testPickaxe;
    TextureRef testBucket;
    TextureRef testBucketFilled;
};

void InitTexture(TexturePack &tex);
void EndTexture(TexturePack &tex);

TextureRef LoadTexture(const std::string &path);
TextureRef LoadTextureInternal(const std::string &path, u32 pixelFormat);
C_Surface *ScaleSurface(C_Surface *src, f32 x, f32 y);
TextureRef LoadAsepriteTexture(const std::string &path);
TextureRef LoadTextureData(const std::string &path);
void RenderTextureRect(TextureRef tex, R_Target *target, int x, int y, MErect *clip = nullptr);

#endif
