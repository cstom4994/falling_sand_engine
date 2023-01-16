// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "game_shaders.hpp"

#pragma region Shaders

void WaterFlowPassShader::Update(int w, int h) {
    int res_loc = R_GetUniformLocation(this->shader, "resolution");

    F32 res[2] = {(F32)w, (F32)h};
    R_SetUniformfv(res_loc, 2, 1, res);
}

void WaterShader::Update(F32 t, int w, int h, R_Image *maskImg, int mask_x, int mask_y, int mask_w, int mask_h, int scale, R_Image *flowImg, int overlay, bool showFlow, bool pixelated) {
    int time_loc = R_GetUniformLocation(this->shader, "time");
    int res_loc = R_GetUniformLocation(this->shader, "resolution");
    int mask_loc = R_GetUniformLocation(this->shader, "mask");
    int mask_pos_loc = R_GetUniformLocation(this->shader, "maskPos");
    int mask_size_loc = R_GetUniformLocation(this->shader, "maskSize");
    int scale_loc = R_GetUniformLocation(this->shader, "scale");
    int flowTex_loc = R_GetUniformLocation(this->shader, "flowTex");
    int overlay_loc = R_GetUniformLocation(this->shader, "overlay");
    int showFlow_loc = R_GetUniformLocation(this->shader, "showFlow");
    int pixelated_loc = R_GetUniformLocation(this->shader, "pixelated");

    R_SetUniformf(time_loc, t);

    F32 res[2] = {(F32)w, (F32)h};
    R_SetUniformfv(res_loc, 2, 1, res);

    R_SetShaderImage(maskImg, mask_loc, 1);

    F32 res2[2] = {(F32)mask_x, (F32)mask_y};
    R_SetUniformfv(mask_pos_loc, 2, 1, res2);
    F32 res3[2] = {(F32)mask_w, (F32)mask_h};
    R_SetUniformfv(mask_size_loc, 2, 1, res3);

    R_SetUniformf(scale_loc, scale);

    R_SetShaderImage(flowImg, flowTex_loc, 2);

    R_SetUniformi(overlay_loc, overlay);
    R_SetUniformi(showFlow_loc, showFlow);
    R_SetUniformi(pixelated_loc, pixelated);
}

void NewLightingShader::SetSimpleMode(bool simpleMode) {
    int simpleOnly_loc = R_GetUniformLocation(this->shader, "simpleOnly");
    R_SetUniformi(simpleOnly_loc, simpleMode);

    this->lastSimpleMode = simpleMode;
}

void NewLightingShader::SetEmissionEnabled(bool emissionEnabled) {
    int emission_loc = R_GetUniformLocation(this->shader, "emission");
    R_SetUniformi(emission_loc, emissionEnabled);

    this->lastEmissionEnabled = emissionEnabled;
}

void NewLightingShader::SetDitheringEnabled(bool ditheringEnabled) {
    int dithering_loc = R_GetUniformLocation(this->shader, "dithering");
    R_SetUniformi(dithering_loc, ditheringEnabled);

    this->lastDitheringEnabled = ditheringEnabled;
}

void NewLightingShader::SetQuality(F32 quality) {
    int lightingQuality_loc = R_GetUniformLocation(this->shader, "lightingQuality");
    R_SetUniformf(lightingQuality_loc, quality);

    this->lastQuality = quality;
}

void NewLightingShader::SetInside(F32 inside) {
    int inside_loc = R_GetUniformLocation(this->shader, "inside");
    R_SetUniformf(inside_loc, inside);

    this->lastInside = inside;
}

void NewLightingShader::SetBounds(F32 minX, F32 minY, F32 maxX, F32 maxY) {
    int minX_loc = R_GetUniformLocation(this->shader, "minX");
    int minY_loc = R_GetUniformLocation(this->shader, "minY");
    int maxX_loc = R_GetUniformLocation(this->shader, "maxX");
    int maxY_loc = R_GetUniformLocation(this->shader, "maxY");

    R_SetUniformf(minX_loc, minX);
    R_SetUniformf(minY_loc, minY);
    R_SetUniformf(maxX_loc, maxX);
    R_SetUniformf(maxY_loc, maxY);
}

void NewLightingShader::Update(R_Image *tex, R_Image *emit, F32 x, F32 y) {
    int txrmap_loc = R_GetUniformLocation(this->shader, "txrmap");
    int emitmap_loc = R_GetUniformLocation(this->shader, "emitmap");
    int txrsize_loc = R_GetUniformLocation(this->shader, "texSize");
    int t0_loc = R_GetUniformLocation(this->shader, "t0");

    this->lastLx = x;
    this->lastLy = y;

    F32 res[2] = {x, y};
    R_SetUniformfv(t0_loc, 2, 1, res);

    F32 tres[2] = {(F32)tex->w, (F32)tex->h};
    R_SetUniformfv(txrsize_loc, 2, 1, tres);

    R_SetShaderImage(tex, txrmap_loc, 1);
    R_SetShaderImage(emit, emitmap_loc, 2);
}

void FireShader::Update(R_Image *tex) {
    int firemap_loc = R_GetUniformLocation(this->shader, "firemap");
    int txrsize_loc = R_GetUniformLocation(this->shader, "texSize");

    F32 tres[2] = {(F32)tex->w, (F32)tex->h};
    R_SetUniformfv(txrsize_loc, 2, 1, tres);

    R_SetShaderImage(tex, firemap_loc, 1);
}

void Fire2Shader::Update(R_Image *tex) {
    int firemap_loc = R_GetUniformLocation(this->shader, "firemap");
    int txrsize_loc = R_GetUniformLocation(this->shader, "texSize");

    F32 tres[2] = {(F32)tex->w, (F32)tex->h};
    R_SetUniformfv(txrsize_loc, 2, 1, tres);

    R_SetShaderImage(tex, firemap_loc, 1);
}

#pragma endregion Shaders

void LoadShaders(ShaderWorker *shaderworker) {
    EndShaders(shaderworker);

    shaderworker->waterShader = new WaterShader;
    shaderworker->waterFlowPassShader = new WaterFlowPassShader;
    shaderworker->newLightingShader = new NewLightingShader;
    shaderworker->fireShader = new FireShader;
    shaderworker->fire2Shader = new Fire2Shader;

    shaderworker->waterShader->vertex_shader_file = METADOT_RESLOC("data/shaders/common.vert");
    shaderworker->waterShader->fragment_shader_file = METADOT_RESLOC("data/shaders/water.frag");
    shaderworker->waterFlowPassShader->vertex_shader_file = METADOT_RESLOC("data/shaders/common.vert");
    shaderworker->waterFlowPassShader->fragment_shader_file = METADOT_RESLOC("data/shaders/waterFlow.frag");
    shaderworker->newLightingShader->vertex_shader_file = METADOT_RESLOC("data/shaders/common.vert");
    shaderworker->newLightingShader->fragment_shader_file = METADOT_RESLOC("data/shaders/newLighting.frag");
    shaderworker->fireShader->vertex_shader_file = METADOT_RESLOC("data/shaders/common.vert");
    shaderworker->fireShader->fragment_shader_file = METADOT_RESLOC("data/shaders/fire.frag");
    shaderworker->fire2Shader->vertex_shader_file = METADOT_RESLOC("data/shaders/common.vert");
    shaderworker->fire2Shader->fragment_shader_file = METADOT_RESLOC("data/shaders/fire2.frag");

    shaderworker->waterFlowPassShader->dirty = false;

    shaderworker->newLightingShader->lastLx = 0.0;
    shaderworker->newLightingShader->lastLy = 0.0;
    shaderworker->newLightingShader->lastQuality = 0.0;
    shaderworker->newLightingShader->lastInside = 0.0;
    shaderworker->newLightingShader->lastSimpleMode = false;
    shaderworker->newLightingShader->lastEmissionEnabled = false;
    shaderworker->newLightingShader->lastDitheringEnabled = false;
    shaderworker->newLightingShader->insideCur = 0.0f;
    shaderworker->newLightingShader->insideDes = 0.0f;

    shaderworker->waterShader->Init();
    shaderworker->waterFlowPassShader->Init();
    shaderworker->newLightingShader->Init();
    shaderworker->fireShader->Init();
    shaderworker->fire2Shader->Init();
}

void EndShaders(ShaderWorker *shaderworker) {
    if (shaderworker->waterShader) {
        shaderworker->waterShader->Unload();
        delete shaderworker->waterShader;
    }
    if (shaderworker->waterFlowPassShader) {
        shaderworker->waterFlowPassShader->Unload();
        delete shaderworker->waterFlowPassShader;
    }
    if (shaderworker->newLightingShader) {
        shaderworker->newLightingShader->Unload();
        delete shaderworker->newLightingShader;
    }
    if (shaderworker->fireShader) {
        shaderworker->fireShader->Unload();
        delete shaderworker->fireShader;
    }
    if (shaderworker->fire2Shader) {
        shaderworker->fire2Shader->Unload();
        delete shaderworker->fire2Shader;
    }
}
