// Copyright(c) 2022, KaoruXun All rights reserved.

#include "game_shaders.hpp"

#pragma region Shaders

void WaterFlowPassShader__update(struct WaterFlowPassShader *shader_t, int w, int h) {
    int res_loc = R_GetUniformLocation(shader_t->sb.shader, "resolution");

    float res[2] = {(float)w, (float)h};
    R_SetUniformfv(res_loc, 2, 1, res);
}

void WaterShader__update(struct WaterShader *shader_t, float t, int w, int h, R_Image *maskImg, int mask_x, int mask_y, int mask_w, int mask_h, int scale, R_Image *flowImg, int overlay, bool showFlow,
                         bool pixelated) {
    int time_loc = R_GetUniformLocation(shader_t->sb.shader, "time");
    int res_loc = R_GetUniformLocation(shader_t->sb.shader, "resolution");
    int mask_loc = R_GetUniformLocation(shader_t->sb.shader, "mask");
    int mask_pos_loc = R_GetUniformLocation(shader_t->sb.shader, "maskPos");
    int mask_size_loc = R_GetUniformLocation(shader_t->sb.shader, "maskSize");
    int scale_loc = R_GetUniformLocation(shader_t->sb.shader, "scale");
    int flowTex_loc = R_GetUniformLocation(shader_t->sb.shader, "flowTex");
    int overlay_loc = R_GetUniformLocation(shader_t->sb.shader, "overlay");
    int showFlow_loc = R_GetUniformLocation(shader_t->sb.shader, "showFlow");
    int pixelated_loc = R_GetUniformLocation(shader_t->sb.shader, "pixelated");

    R_SetUniformf(time_loc, t);

    float res[2] = {(float)w, (float)h};
    R_SetUniformfv(res_loc, 2, 1, res);

    R_SetShaderImage(maskImg, mask_loc, 1);

    float res2[2] = {(float)mask_x, (float)mask_y};
    R_SetUniformfv(mask_pos_loc, 2, 1, res2);
    float res3[2] = {(float)mask_w, (float)mask_h};
    R_SetUniformfv(mask_size_loc, 2, 1, res3);

    R_SetUniformf(scale_loc, scale);

    R_SetShaderImage(flowImg, flowTex_loc, 2);

    R_SetUniformi(overlay_loc, overlay);
    R_SetUniformi(showFlow_loc, showFlow);
    R_SetUniformi(pixelated_loc, pixelated);
}

void NewLightingShader__setSimpleMode(struct NewLightingShader *shader_t, bool simpleMode) {
    int simpleOnly_loc = R_GetUniformLocation(shader_t->sb.shader, "simpleOnly");
    R_SetUniformi(simpleOnly_loc, simpleMode);

    shader_t->lastSimpleMode = simpleMode;
}

void NewLightingShader__setEmissionEnabled(struct NewLightingShader *shader_t, bool emissionEnabled) {
    int emission_loc = R_GetUniformLocation(shader_t->sb.shader, "emission");
    R_SetUniformi(emission_loc, emissionEnabled);

    shader_t->lastEmissionEnabled = emissionEnabled;
}

void NewLightingShader__setDitheringEnabled(struct NewLightingShader *shader_t, bool ditheringEnabled) {
    int dithering_loc = R_GetUniformLocation(shader_t->sb.shader, "dithering");
    R_SetUniformi(dithering_loc, ditheringEnabled);

    shader_t->lastDitheringEnabled = ditheringEnabled;
}

void NewLightingShader__setQuality(struct NewLightingShader *shader_t, float quality) {
    int lightingQuality_loc = R_GetUniformLocation(shader_t->sb.shader, "lightingQuality");
    R_SetUniformf(lightingQuality_loc, quality);

    shader_t->lastQuality = quality;
}

void NewLightingShader__setInside(struct NewLightingShader *shader_t, float inside) {
    int inside_loc = R_GetUniformLocation(shader_t->sb.shader, "inside");
    R_SetUniformf(inside_loc, inside);

    shader_t->lastInside = inside;
}

void NewLightingShader__setBounds(struct NewLightingShader *shader_t, float minX, float minY, float maxX, float maxY) {
    int minX_loc = R_GetUniformLocation(shader_t->sb.shader, "minX");
    int minY_loc = R_GetUniformLocation(shader_t->sb.shader, "minY");
    int maxX_loc = R_GetUniformLocation(shader_t->sb.shader, "maxX");
    int maxY_loc = R_GetUniformLocation(shader_t->sb.shader, "maxY");

    R_SetUniformf(minX_loc, minX);
    R_SetUniformf(minY_loc, minY);
    R_SetUniformf(maxX_loc, maxX);
    R_SetUniformf(maxY_loc, maxY);
}

void NewLightingShader__update(struct NewLightingShader *shader_t, R_Image *tex, R_Image *emit, float x, float y) {
    int txrmap_loc = R_GetUniformLocation(shader_t->sb.shader, "txrmap");
    int emitmap_loc = R_GetUniformLocation(shader_t->sb.shader, "emitmap");
    int txrsize_loc = R_GetUniformLocation(shader_t->sb.shader, "texSize");
    int t0_loc = R_GetUniformLocation(shader_t->sb.shader, "t0");

    shader_t->lastLx = x;
    shader_t->lastLy = y;

    float res[2] = {x, y};
    R_SetUniformfv(t0_loc, 2, 1, res);

    float tres[2] = {(float)tex->w, (float)tex->h};
    R_SetUniformfv(txrsize_loc, 2, 1, tres);

    R_SetShaderImage(tex, txrmap_loc, 1);
    R_SetShaderImage(emit, emitmap_loc, 2);
}

void FireShader__update(struct FireShader *shader_t, R_Image *tex) {
    int firemap_loc = R_GetUniformLocation(shader_t->sb.shader, "firemap");
    int txrsize_loc = R_GetUniformLocation(shader_t->sb.shader, "texSize");

    float tres[2] = {(float)tex->w, (float)tex->h};
    R_SetUniformfv(txrsize_loc, 2, 1, tres);

    R_SetShaderImage(tex, firemap_loc, 1);
}

void Fire2Shader__update(struct Fire2Shader *shader_t, R_Image *tex) {
    int firemap_loc = R_GetUniformLocation(shader_t->sb.shader, "firemap");
    int txrsize_loc = R_GetUniformLocation(shader_t->sb.shader, "texSize");

    float tres[2] = {(float)tex->w, (float)tex->h};
    R_SetUniformfv(txrsize_loc, 2, 1, tres);

    R_SetShaderImage(tex, firemap_loc, 1);
}

#pragma endregion Shaders

void LoadShaders(ShaderWorker *shaderworker) {
    EndShaders(shaderworker);

    shaderworker->waterShader = (struct WaterShader *)malloc(sizeof(struct WaterShader));
    shaderworker->waterFlowPassShader = (struct WaterFlowPassShader *)malloc(sizeof(struct WaterFlowPassShader));
    shaderworker->newLightingShader = (struct NewLightingShader *)malloc(sizeof(struct NewLightingShader));
    shaderworker->fireShader = (struct FireShader *)malloc(sizeof(struct FireShader));
    shaderworker->fire2Shader = (struct Fire2Shader *)malloc(sizeof(struct Fire2Shader));

    shaderworker->waterShader->sb.vertex_shader_file = METADOT_RESLOC("data/shaders/common.vert");
    shaderworker->waterShader->sb.fragment_shader_file = METADOT_RESLOC("data/shaders/water.frag");
    shaderworker->waterFlowPassShader->sb.vertex_shader_file = METADOT_RESLOC("data/shaders/common.vert");
    shaderworker->waterFlowPassShader->sb.fragment_shader_file = METADOT_RESLOC("data/shaders/waterFlow.frag");
    shaderworker->newLightingShader->sb.vertex_shader_file = METADOT_RESLOC("data/shaders/common.vert");
    shaderworker->newLightingShader->sb.fragment_shader_file = METADOT_RESLOC("data/shaders/newLighting.frag");
    shaderworker->fireShader->sb.vertex_shader_file = METADOT_RESLOC("data/shaders/common.vert");
    shaderworker->fireShader->sb.fragment_shader_file = METADOT_RESLOC("data/shaders/fire.frag");
    shaderworker->fire2Shader->sb.vertex_shader_file = METADOT_RESLOC("data/shaders/common.vert");
    shaderworker->fire2Shader->sb.fragment_shader_file = METADOT_RESLOC("data/shaders/fire2.frag");

    shaderworker->waterFlowPassShader->dirty = false;

    shaderworker->newLightingShader->lastLx = 0.0;
    shaderworker->newLightingShader->lastLy = 0.0;
    shaderworker->newLightingShader->lastQuality = 0.0;
    shaderworker->newLightingShader->lastInside = 0.0;
    shaderworker->newLightingShader->lastSimpleMode = false;
    shaderworker->newLightingShader->lastEmissionEnabled = false;
    shaderworker->newLightingShader->lastDitheringEnabled = false;

    ShaderInit(&shaderworker->waterShader->sb);
    ShaderInit(&shaderworker->waterFlowPassShader->sb);
    ShaderInit(&shaderworker->newLightingShader->sb);
    ShaderInit(&shaderworker->fireShader->sb);
    ShaderInit(&shaderworker->fire2Shader->sb);
}

void EndShaders(ShaderWorker *shaderworker) {
    if (shaderworker->waterShader) {
        ShaderUnload(&shaderworker->waterShader->sb);
        free(shaderworker->waterShader);
    }
    if (shaderworker->waterFlowPassShader) {
        ShaderUnload(&shaderworker->waterFlowPassShader->sb);
        free(shaderworker->waterFlowPassShader);
    }
    if (shaderworker->newLightingShader) {
        ShaderUnload(&shaderworker->newLightingShader->sb);
        free(shaderworker->newLightingShader);
    }
    if (shaderworker->fireShader) {
        ShaderUnload(&shaderworker->fireShader->sb);
        free(shaderworker->fireShader);
    }
    if (shaderworker->fire2Shader) {
        ShaderUnload(&shaderworker->fire2Shader->sb);
        free(shaderworker->fire2Shader);
    }
}
