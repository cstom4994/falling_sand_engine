// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_GAMESHADERS_HPP
#define ME_GAMESHADERS_HPP

#include "engine/core/core.hpp"
#include "engine/renderer/shaders.hpp"
#include "game_datastruct.hpp"

class CrtShader : public ShaderBase {
public:
    bool enable;

    void Update(int w, int h);

    ShaderBaseDecl();
};

class WaterFlowPassShader : public ShaderBase {
public:
    bool dirty;

    void Update(int w, int h);

    ShaderBaseDecl();
};

class WaterShader : public ShaderBase {
public:
    void Update(f32 t, int w, int h, R_Image *maskImg, int mask_x, int mask_y, int mask_w, int mask_h, int scale, R_Image *flowImg, int overlay, bool showFlow, bool pixelated);

    ShaderBaseDecl();
};

class NewLightingShader : public ShaderBase {
public:
    f32 lastLx;
    f32 lastLy;
    f32 lastQuality;
    f32 lastInside;
    bool lastSimpleMode;
    bool lastEmissionEnabled;
    bool lastDitheringEnabled;

    f32 insideDes;
    f32 insideCur;

    void SetSimpleMode(bool simpleMode);
    void SetEmissionEnabled(bool emissionEnabled);
    void SetDitheringEnabled(bool ditheringEnabled);
    void SetQuality(f32 quality);
    void SetInside(f32 inside);
    void SetBounds(f32 minX, f32 minY, f32 maxX, f32 maxY);

    void Update(R_Image *tex, R_Image *emit, f32 x, f32 y);

    ShaderBaseDecl();
};

class FireShader : public ShaderBase {
public:
    void Update(R_Image *tex);

    ShaderBaseDecl();
};

class Fire2Shader : public ShaderBase {
public:
    void Update(R_Image *tex);

    ShaderBaseDecl();
};

class BlurShader : public ShaderBase {
public:
    void Update(R_Image *tex);

    ShaderBaseDecl();
};

class UntexturedShader : public ShaderBase {
public:
    GLuint VBO;
    GLuint modelViewProjection_loc;
    GLuint vertex_loc;
    GLuint color_loc;
    void Update(float mvp[], GLfloat gldata[]);

    ShaderBaseDecl();
};

class ShaderWorkerSystem : public IGameSystem {
public:
    CrtShader *crtShader = nullptr;
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