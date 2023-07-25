// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "game_shaders.hpp"

#include "engine/engine.hpp"
#include "game/player.hpp"

namespace ME {

#pragma region Shaders

void CrtShader::Update(int w, int h) {
    float res[] = {static_cast<float>(w), static_cast<float>(h)};

    int res_loc = R_GetUniformLocation(this->shader, "resolution");
    int crteffect_loc = R_GetUniformLocation(this->shader, "crteffect");

    R_SetUniformfv(res_loc, 2, 1, &res[0]);
    R_SetUniformi(crteffect_loc, this->enable);
}

void WaterFlowPassShader::Update(int w, int h) {
    int res_loc = R_GetUniformLocation(this->shader, "resolution");

    f32 res[2] = {(f32)w, (f32)h};
    R_SetUniformfv(res_loc, 2, 1, res);
}

void WaterShader::Update(f32 t, int w, int h, R_Image *maskImg, int mask_x, int mask_y, int mask_w, int mask_h, int scale, R_Image *flowImg, int overlay, bool showFlow, bool pixelated) {
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

    f32 res[2] = {(f32)w, (f32)h};
    R_SetUniformfv(res_loc, 2, 1, res);

    R_SetShaderImage(maskImg, mask_loc, 1);

    f32 res2[2] = {(f32)mask_x, (f32)mask_y};
    R_SetUniformfv(mask_pos_loc, 2, 1, res2);
    f32 res3[2] = {(f32)mask_w, (f32)mask_h};
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

void NewLightingShader::SetQuality(f32 quality) {
    int lightingQuality_loc = R_GetUniformLocation(this->shader, "lightingQuality");
    R_SetUniformf(lightingQuality_loc, quality);

    this->lastQuality = quality;
}

void NewLightingShader::SetInside(f32 inside) {
    int inside_loc = R_GetUniformLocation(this->shader, "inside");
    R_SetUniformf(inside_loc, inside);

    this->lastInside = inside;
}

void NewLightingShader::SetBounds(f32 minX, f32 minY, f32 maxX, f32 maxY) {
    int minX_loc = R_GetUniformLocation(this->shader, "minX");
    int minY_loc = R_GetUniformLocation(this->shader, "minY");
    int maxX_loc = R_GetUniformLocation(this->shader, "maxX");
    int maxY_loc = R_GetUniformLocation(this->shader, "maxY");

    R_SetUniformf(minX_loc, minX);
    R_SetUniformf(minY_loc, minY);
    R_SetUniformf(maxX_loc, maxX);
    R_SetUniformf(maxY_loc, maxY);
}

void NewLightingShader::Update(R_Image *tex, R_Image *emit, f32 x, f32 y) {
    int txrmap_loc = R_GetUniformLocation(this->shader, "txrmap");
    int emitmap_loc = R_GetUniformLocation(this->shader, "emitmap");
    int txrsize_loc = R_GetUniformLocation(this->shader, "texSize");
    int t0_loc = R_GetUniformLocation(this->shader, "t0");

    this->lastLx = x;
    this->lastLy = y;

    f32 res[2] = {x, y};
    R_SetUniformfv(t0_loc, 2, 1, res);

    f32 tres[2] = {(f32)tex->w, (f32)tex->h};
    R_SetUniformfv(txrsize_loc, 2, 1, tres);

    R_SetShaderImage(tex, txrmap_loc, 1);
    R_SetShaderImage(emit, emitmap_loc, 2);
}

void FireShader::Update(R_Image *tex) {
    int firemap_loc = R_GetUniformLocation(this->shader, "firemap");
    int txrsize_loc = R_GetUniformLocation(this->shader, "texSize");

    f32 tres[2] = {(f32)tex->w, (f32)tex->h};
    R_SetUniformfv(txrsize_loc, 2, 1, tres);

    R_SetShaderImage(tex, firemap_loc, 1);
}

void Fire2Shader::Update(R_Image *tex) {
    int firemap_loc = R_GetUniformLocation(this->shader, "firemap");
    int txrsize_loc = R_GetUniformLocation(this->shader, "texSize");

    f32 tres[2] = {(f32)tex->w, (f32)tex->h};
    R_SetUniformfv(txrsize_loc, 2, 1, tres);

    R_SetShaderImage(tex, firemap_loc, 1);
}

void BlurShader::Update(R_Image *tex) {
    int colorTexture_loc = R_GetUniformLocation(this->shader, "colorTexture");
    R_SetShaderImage(tex, colorTexture_loc, 1);
}

void UntexturedShader::Update(float mvp[], GLfloat gldata[]) {

    // glUseProgram();
    int vertex_loc = R_GetAttributeLocation(this->shader, "gpu_3d_Vertex");
    int color_loc = R_GetAttributeLocation(this->shader, "gpu_3d_Color");
    int modelViewProjection_loc = R_GetUniformLocation(this->shader, "gpu_3d_ModelViewProjectionMatrix");

    R_GetModelViewProjection(mvp);
    glUniformMatrix4fv(modelViewProjection_loc, 1, 0, mvp);

    glEnableVertexAttribArray(vertex_loc);
    glEnableVertexAttribArray(color_loc);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gldata), gldata, GL_STREAM_DRAW);

    glVertexAttribPointer(vertex_loc,
                          3,                  // size
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalize
                          sizeof(float) * 7,  // stride
                          (void *)0           // offset
    );

    glVertexAttribPointer(color_loc,
                          4,                           // size
                          GL_FLOAT,                    // type
                          GL_FALSE,                    // normalize
                          sizeof(float) * 7,           // stride
                          (void *)(sizeof(float) * 3)  // offset
    );

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(color_loc);
    glDisableVertexAttribArray(vertex_loc);
}

void RayLightingShader::Update(R_Image *img, float x, float y) {
    int t0_loc = R_GetUniformLocation(shader, "t0");
    int txrmap_loc = R_GetUniformLocation(shader, "txrmap");

    float res[2] = {x, y};
    R_SetUniformfv(t0_loc, 2, 1, res);

    R_SetShaderImage(img, txrmap_loc, 1);
}

#pragma endregion Shaders

void shader_worker::create() {

    Timer timer;
    timer.start();

    this->crtShader = new CrtShader;
    this->waterShader = new WaterShader;
    this->waterFlowPassShader = new WaterFlowPassShader;
    this->newLightingShader = new NewLightingShader;
    this->fireShader = new FireShader;
    this->fire2Shader = new Fire2Shader;
    this->blurShader = new BlurShader;
    this->untexturedShader = new UntexturedShader;
    this->raylightingShader = new RayLightingShader;

    this->crtShader->vertex_shader_file = ME_fs_get_path("data/shaders/common.vert");
    this->crtShader->fragment_shader_file = ME_fs_get_path("data/shaders/crt.frag");
    this->waterShader->vertex_shader_file = ME_fs_get_path("data/shaders/common.vert");
    this->waterShader->fragment_shader_file = ME_fs_get_path("data/shaders/water.frag");
    this->waterFlowPassShader->vertex_shader_file = ME_fs_get_path("data/shaders/common.vert");
    this->waterFlowPassShader->fragment_shader_file = ME_fs_get_path("data/shaders/waterFlow.frag");
    this->newLightingShader->vertex_shader_file = ME_fs_get_path("data/shaders/common.vert");
    this->newLightingShader->fragment_shader_file = ME_fs_get_path("data/shaders/newLighting.frag");
    this->fireShader->vertex_shader_file = ME_fs_get_path("data/shaders/common.vert");
    this->fireShader->fragment_shader_file = ME_fs_get_path("data/shaders/fire.frag");
    this->fire2Shader->vertex_shader_file = ME_fs_get_path("data/shaders/common.vert");
    this->fire2Shader->fragment_shader_file = ME_fs_get_path("data/shaders/fire2.frag");
    this->blurShader->vertex_shader_file = ME_fs_get_path("data/shaders/common.vert");
    this->blurShader->fragment_shader_file = ME_fs_get_path("data/shaders/gaussian_blur.frag");
    this->untexturedShader->vertex_shader_file = ME_fs_get_path("data/shaders/untextured.vert");
    this->untexturedShader->fragment_shader_file = ME_fs_get_path("data/shaders/untextured.frag");
    this->raylightingShader->vertex_shader_file = ME_fs_get_path("data/shaders/common.vert");
    this->raylightingShader->fragment_shader_file = ME_fs_get_path("data/shaders/raylighting.frag");

    this->waterFlowPassShader->dirty = false;

    this->newLightingShader->lastLx = 0.0;
    this->newLightingShader->lastLy = 0.0;
    this->newLightingShader->lastQuality = 0.0;
    this->newLightingShader->lastInside = 0.0;
    this->newLightingShader->lastSimpleMode = false;
    this->newLightingShader->lastEmissionEnabled = false;
    this->newLightingShader->lastDitheringEnabled = false;
    this->newLightingShader->insideCur = 0.0f;
    this->newLightingShader->insideDes = 0.0f;

    this->crtShader->enable = true;

    glGenBuffers(1, &this->untexturedShader->VBO);

    this->crtShader->init();
    this->waterShader->init();
    this->waterFlowPassShader->init();
    this->newLightingShader->init();
    this->fireShader->init();
    this->fire2Shader->init();
    this->blurShader->init();
    this->untexturedShader->init();

    timer.stop();

    METADOT_INFO(std::format("ShaderWorker loading done in {0:.4f} ms", timer.get()).c_str());
}

#define SAFEUNLOADSHADER(x) \
    if (this->x) {          \
        this->x->unload();  \
        delete this->x;     \
    }

void shader_worker::destory() {
    SAFEUNLOADSHADER(crtShader);
    SAFEUNLOADSHADER(waterShader);
    SAFEUNLOADSHADER(waterFlowPassShader);
    SAFEUNLOADSHADER(newLightingShader);
    SAFEUNLOADSHADER(fireShader);
    SAFEUNLOADSHADER(fire2Shader);
    SAFEUNLOADSHADER(blurShader);
    SAFEUNLOADSHADER(untexturedShader);

    METADOT_BUG("ShaderWorker destroyed");
}

void shader_worker::reload() {
    this->destory();
    this->create();
}

#undef SAFEUNLOADSHADER

void shader_worker::registerLua(lua_wrapper::State &s_lua) {}

}  // namespace ME