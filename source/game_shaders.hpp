// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_GAMESHADERS_HPP_
#define _METADOT_GAMESHADERS_HPP_

#include "core/core.hpp"
#include "engine/engine_shaders.hpp"
#include "game_datastruct.hpp"

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

class BlurShader : public ShaderBase {
public:
    void Update(R_Image *tex);
};

class UntexturedShader : public ShaderBase {
public:
    GLuint VBO;
    // GLuint modelViewProjection_loc;
    // GLuint vertex_loc;
    // GLuint color_loc;
    void Update(float mvp[], GLfloat gldata[]);
};

class ShaderWorkerSystem : public IGameSystem {
public:
    WaterShader *waterShader = nullptr;
    WaterFlowPassShader *waterFlowPassShader = nullptr;
    NewLightingShader *newLightingShader = nullptr;
    FireShader *fireShader = nullptr;
    Fire2Shader *fire2Shader = nullptr;
    BlurShader *blurShader = nullptr;
    UntexturedShader *untexturedShader = nullptr;

    REGISTER_SYSTEM(ShaderWorkerSystem)

    void Create() override;
    void Destory() override;
    void Reload() override;
    void RegisterLua(LuaWrapper::State &s_lua) override;
};

#endif