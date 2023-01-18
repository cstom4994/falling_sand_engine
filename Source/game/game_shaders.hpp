// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_GAMESHADERS_HPP_
#define _METADOT_GAMESHADERS_HPP_

#include "core/core.hpp"
#include "engine/engine_shaders.hpp"
#include "game/game_datastruct.hpp"

class WaterFlowPassShader : public ShaderBase {
public:
    bool dirty;

    void Update(int w, int h);
};

class WaterShader : public ShaderBase {
public:
    void Update(F32 t, int w, int h, R_Image *maskImg, int mask_x, int mask_y, int mask_w, int mask_h, int scale, R_Image *flowImg, int overlay, bool showFlow, bool pixelated);
};

class NewLightingShader : public ShaderBase {
public:
    F32 lastLx;
    F32 lastLy;
    F32 lastQuality;
    F32 lastInside;
    bool lastSimpleMode;
    bool lastEmissionEnabled;
    bool lastDitheringEnabled;

    F32 insideDes;
    F32 insideCur;

    void SetSimpleMode(bool simpleMode);
    void SetEmissionEnabled(bool emissionEnabled);
    void SetDitheringEnabled(bool ditheringEnabled);
    void SetQuality(F32 quality);
    void SetInside(F32 inside);
    void SetBounds(F32 minX, F32 minY, F32 maxX, F32 maxY);

    void Update(R_Image *tex, R_Image *emit, F32 x, F32 y);
};

class FireShader : public ShaderBase {
public:
    void Update(R_Image *tex);
};

class Fire2Shader : public ShaderBase {
public:
    void Update(R_Image *tex);
};

class ShaderWorkerSystem : IGameSystem {
public:
    WaterShader *waterShader;
    WaterFlowPassShader *waterFlowPassShader;
    NewLightingShader *newLightingShader;
    FireShader *fireShader;
    Fire2Shader *fire2Shader;

    void Create() override;
    void Destory() override;
    void RegisterLua(LuaWrapper::State &s_lua) override;
};

#endif