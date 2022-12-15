// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METADOT_SHADERS_HPP_
#define _METADOT_SHADERS_HPP_

#include "engine/sdl_wrapper.h"

#include "engine/renderer/renderer_gpu.h"
#include "engine/filesystem.h"

#include <cstdlib>
#include <string>
#include <vector>

// Based on https://github.com/grimfang4/sdl-gpu/blob/master/demos/simple-shader/main.c (MIT License)

// Loads a shader and prepends version/compatibility info before compiling it.
U32 METAENGINE_Shaders_LoadShader(R_ShaderEnum shader_type,
                                     const char *filename);
R_ShaderBlock METAENGINE_Shaders_LoadShaderProgram(
        U32 *p, const char *vertex_shader_file, const char *fragment_shader_file);
void METAENGINE_Shaders_FreeShader(U32 p);

class ShaderBase {
public:
    U32 shader;
    R_ShaderBlock block;

    ShaderBase(const char *vertex_shader_file, const char *fragment_shader_file) {
        shader = 0;
        block = METAENGINE_Shaders_LoadShaderProgram(&shader, vertex_shader_file,
                                                     fragment_shader_file);
    }

    ~ShaderBase() { METAENGINE_Shaders_FreeShader(shader); }

    virtual void prepare() = 0;

    void activate() { R_ActivateShaderProgram(shader, &block); }
};

class WaterFlowPassShader : public ShaderBase {
public:
    bool dirty = false;

    WaterFlowPassShader()
        : ShaderBase(METADOT_RESLOC("data/shaders/common.vert"),
                     METADOT_RESLOC("data/shaders/waterFlow.frag")){};

    void prepare() {}

    void update(int w, int h) {
        int res_loc = R_GetUniformLocation(shader, "resolution");

        float res[2] = {(float) w, (float) h};
        R_SetUniformfv(res_loc, 2, 1, res);
    }
};

class WaterShader : public ShaderBase {
public:
    WaterShader()
        : ShaderBase(METADOT_RESLOC("data/shaders/common.vert"),
                     METADOT_RESLOC("data/shaders/water.frag")){};

    void prepare() {}

    void update(float t, int w, int h, R_Image *maskImg, int mask_x, int mask_y,
                int mask_w, int mask_h, int scale, R_Image *flowImg, int overlay,
                bool showFlow, bool pixelated) {
        int time_loc = R_GetUniformLocation(shader, "time");
        int res_loc = R_GetUniformLocation(shader, "resolution");
        int mask_loc = R_GetUniformLocation(shader, "mask");
        int mask_pos_loc = R_GetUniformLocation(shader, "maskPos");
        int mask_size_loc = R_GetUniformLocation(shader, "maskSize");
        int scale_loc = R_GetUniformLocation(shader, "scale");
        int flowTex_loc = R_GetUniformLocation(shader, "flowTex");
        int overlay_loc = R_GetUniformLocation(shader, "overlay");
        int showFlow_loc = R_GetUniformLocation(shader, "showFlow");
        int pixelated_loc = R_GetUniformLocation(shader, "pixelated");

        R_SetUniformf(time_loc, t);

        float res[2] = {(float) w, (float) h};
        R_SetUniformfv(res_loc, 2, 1, res);

        R_SetShaderImage(maskImg, mask_loc, 1);

        float res2[2] = {(float) mask_x, (float) mask_y};
        R_SetUniformfv(mask_pos_loc, 2, 1, res2);
        float res3[2] = {(float) mask_w, (float) mask_h};
        R_SetUniformfv(mask_size_loc, 2, 1, res3);

        R_SetUniformf(scale_loc, scale);

        R_SetShaderImage(flowImg, flowTex_loc, 2);

        R_SetUniformi(overlay_loc, overlay);
        R_SetUniformi(showFlow_loc, showFlow);
        R_SetUniformi(pixelated_loc, pixelated);
    }
};

class NewLightingShader : public ShaderBase {
public:
    float lastLx = 0.0;
    float lastLy = 0.0;
    float lastQuality = 0.0;
    float lastInside = 0.0;
    bool lastSimpleMode = false;
    bool lastEmissionEnabled = false;
    bool lastDitheringEnabled = false;

    NewLightingShader()
        : ShaderBase(METADOT_RESLOC("data/shaders/common.vert"),
                     METADOT_RESLOC("data/shaders/newLighting.frag")){};

    void prepare() {}

    void setSimpleMode(bool simpleMode) {
        int simpleOnly_loc = R_GetUniformLocation(shader, "simpleOnly");
        R_SetUniformi(simpleOnly_loc, simpleMode);

        lastSimpleMode = simpleMode;
    }

    void setEmissionEnabled(bool emissionEnabled) {
        int emission_loc = R_GetUniformLocation(shader, "emission");
        R_SetUniformi(emission_loc, emissionEnabled);

        lastEmissionEnabled = emissionEnabled;
    }

    void setDitheringEnabled(bool ditheringEnabled) {
        int dithering_loc = R_GetUniformLocation(shader, "dithering");
        R_SetUniformi(dithering_loc, ditheringEnabled);

        lastDitheringEnabled = ditheringEnabled;
    }

    void setQuality(float quality) {
        int lightingQuality_loc = R_GetUniformLocation(shader, "lightingQuality");
        R_SetUniformf(lightingQuality_loc, quality);

        lastQuality = quality;
    }

    void setInside(float inside) {
        int inside_loc = R_GetUniformLocation(shader, "inside");
        R_SetUniformf(inside_loc, inside);

        lastInside = inside;
    }

    void setBounds(float minX, float minY, float maxX, float maxY) {
        int minX_loc = R_GetUniformLocation(shader, "minX");
        int minY_loc = R_GetUniformLocation(shader, "minY");
        int maxX_loc = R_GetUniformLocation(shader, "maxX");
        int maxY_loc = R_GetUniformLocation(shader, "maxY");

        R_SetUniformf(minX_loc, minX);
        R_SetUniformf(minY_loc, minY);
        R_SetUniformf(maxX_loc, maxX);
        R_SetUniformf(maxY_loc, maxY);
    }

    void update(R_Image *tex, R_Image *emit, float x, float y) {
        int txrmap_loc = R_GetUniformLocation(shader, "txrmap");
        int emitmap_loc = R_GetUniformLocation(shader, "emitmap");
        int txrsize_loc = R_GetUniformLocation(shader, "texSize");
        int t0_loc = R_GetUniformLocation(shader, "t0");

        lastLx = x;
        lastLy = y;

        float res[2] = {x, y};
        R_SetUniformfv(t0_loc, 2, 1, res);

        float tres[2] = {(float) tex->w, (float) tex->h};
        R_SetUniformfv(txrsize_loc, 2, 1, tres);

        R_SetShaderImage(tex, txrmap_loc, 1);
        R_SetShaderImage(emit, emitmap_loc, 2);
    }
};

class FireShader : public ShaderBase {
public:
    FireShader()
        : ShaderBase(METADOT_RESLOC("data/shaders/common.vert"),
                     METADOT_RESLOC("data/shaders/fire.frag")){};

    void prepare() {}

    void update(R_Image *tex) {
        int firemap_loc = R_GetUniformLocation(shader, "firemap");
        int txrsize_loc = R_GetUniformLocation(shader, "texSize");

        float tres[2] = {(float) tex->w, (float) tex->h};
        R_SetUniformfv(txrsize_loc, 2, 1, tres);

        R_SetShaderImage(tex, firemap_loc, 1);
    }
};

class Fire2Shader : public ShaderBase {
public:
    Fire2Shader()
        : ShaderBase(METADOT_RESLOC("data/shaders/common.vert"),
                     METADOT_RESLOC("data/shaders/fire2.frag")){};

    void prepare() {}

    void update(R_Image *tex) {
        int firemap_loc = R_GetUniformLocation(shader, "firemap");
        int txrsize_loc = R_GetUniformLocation(shader, "texSize");

        float tres[2] = {(float) tex->w, (float) tex->h};
        R_SetUniformfv(txrsize_loc, 2, 1, tres);

        R_SetShaderImage(tex, firemap_loc, 1);
    }
};

struct ShaderWorker
{
    WaterShader *waterShader = nullptr;
    WaterFlowPassShader *waterFlowPassShader = nullptr;
    NewLightingShader *newLightingShader = nullptr;
    float newLightingShader_insideDes = 0.0f;
    float newLightingShader_insideCur = 0.0f;
    FireShader *fireShader = nullptr;
    Fire2Shader *fire2Shader = nullptr;

    void LoadShaders();
};

#endif
