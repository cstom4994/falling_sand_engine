// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_GAMESHADERS_HPP_
#define _METADOT_GAMESHADERS_HPP_

#include "core/core.hpp"
#include "engine/engine_cpp.h"

struct WaterFlowPassShader {
    metadot_shader_base sb;

    bool dirty;
};

struct WaterShader {
    metadot_shader_base sb;
};

struct NewLightingShader {
    metadot_shader_base sb;

    float lastLx;
    float lastLy;
    float lastQuality;
    float lastInside;
    bool lastSimpleMode;
    bool lastEmissionEnabled;
    bool lastDitheringEnabled;
};

struct FireShader {
    metadot_shader_base sb;
};

struct Fire2Shader {
    metadot_shader_base sb;
};

void WaterFlowPassShader__update(struct WaterFlowPassShader *shader_t, int w, int h);
void WaterShader__update(struct WaterShader *shader_t, float t, int w, int h, R_Image *maskImg, int mask_x, int mask_y, int mask_w, int mask_h, int scale, R_Image *flowImg, int overlay, bool showFlow,
                         bool pixelated);
void NewLightingShader__setSimpleMode(struct NewLightingShader *shader_t, bool simpleMode);
void NewLightingShader__setEmissionEnabled(struct NewLightingShader *shader_t, bool emissionEnabled);
void NewLightingShader__setDitheringEnabled(struct NewLightingShader *shader_t, bool ditheringEnabled);
void NewLightingShader__setQuality(struct NewLightingShader *shader_t, float quality);
void NewLightingShader__setInside(struct NewLightingShader *shader_t, float inside);
void NewLightingShader__setBounds(struct NewLightingShader *shader_t, float minX, float minY, float maxX, float maxY);
void NewLightingShader__update(struct NewLightingShader *shader_t, R_Image *tex, R_Image *emit, float x, float y);
void FireShader__update(struct FireShader *shader_t, R_Image *tex);
void Fire2Shader__update(struct Fire2Shader *shader_t, R_Image *tex);

typedef struct ShaderWorker {
    struct WaterShader *waterShader;
    struct WaterFlowPassShader *waterFlowPassShader;
    struct NewLightingShader *newLightingShader;
    float newLightingShader_insideDes;
    float newLightingShader_insideCur;
    struct FireShader *fireShader;
    struct Fire2Shader *fire2Shader;
} ShaderWorker;

void LoadShaders(ShaderWorker *shaderworker);
void EndShaders(ShaderWorker *shaderworker);

#endif