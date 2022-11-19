// Copyright(c) 2022, KaoruXun All rights reserved.


#include "Engine/Render/SDLWrapper.hpp"

#include "Engine/Render/renderer_gpu.h"
#include "Game/FileSystem.hpp"

#include <cstdlib>
#include <string>
#include <vector>

// Based on https://github.com/grimfang4/sdl-gpu/blob/master/demos/simple-shader/main.c (MIT License)

class Shaders {
public:
    // Loads a shader and prepends version/compatibility info before compiling it.
    static UInt32 load_shader(METAENGINE_Render_ShaderEnum shader_type, const char *filename) {

        UInt32 shader;
        METAENGINE_Render_Renderer *renderer = METAENGINE_Render_GetCurrentRenderer();

        std::string source = MetaEngine::FUtil::readFileString(filename);

        if (source.empty()) {
            METAENGINE_Render_PushErrorCode("load_shader", METAENGINE_Render_ERROR_FILE_NOT_FOUND, "Shader file \"%s\" not found", filename);
            return 0;
        }

        // Compile the shader
        shader = METAENGINE_Render_CompileShader(shader_type, source.c_str());

        return shader;
    }

    static METAENGINE_Render_ShaderBlock load_shader_program(UInt32 *p, const char *vertex_shader_file, const char *fragment_shader_file) {
        UInt32 v, f;
        v = load_shader(METAENGINE_Render_VERTEX_SHADER, vertex_shader_file);

        if (!v)
            METAENGINE_Render_LogError("Failed to load vertex shader (%s): %s\n", vertex_shader_file, METAENGINE_Render_GetShaderMessage());

        f = load_shader(METAENGINE_Render_FRAGMENT_SHADER, fragment_shader_file);

        if (!f)
            METAENGINE_Render_LogError("Failed to load fragment shader (%s): %s\n", fragment_shader_file, METAENGINE_Render_GetShaderMessage());

        *p = METAENGINE_Render_LinkShaders(v, f);

        if (!*p) {
            METAENGINE_Render_ShaderBlock b = {-1, -1, -1, -1};
            METAENGINE_Render_LogError("Failed to link shader program (%s + %s): %s\n", vertex_shader_file, fragment_shader_file, METAENGINE_Render_GetShaderMessage());
            return b;
        }

        {
            METAENGINE_Render_ShaderBlock block = METAENGINE_Render_LoadShaderBlock(*p, "gpu_Vertex", "gpu_TexCoord", "gpu_Color", "gpu_ModelViewProjectionMatrix");
            METAENGINE_Render_ActivateShaderProgram(*p, &block);

            return block;
        }
    }

    static void free_shader(UInt32 p) {
        METAENGINE_Render_FreeShaderProgram(p);
    }
};

class Shader {
public:
    UInt32 shader;
    METAENGINE_Render_ShaderBlock block;

    Shader(const char *vertex_shader_file, const char *fragment_shader_file) {
        shader = 0;
        block = Shaders::load_shader_program(&shader, vertex_shader_file, fragment_shader_file);
    }

    ~Shader() {
        Shaders::free_shader(shader);
    }

    virtual void prepare() = 0;

    void activate() {
        METAENGINE_Render_ActivateShaderProgram(shader, &block);
    }
};

class WaterFlowPassShader : public Shader {
public:
    bool dirty = false;

    WaterFlowPassShader() : Shader(METADOT_RESLOC_STR("data/shaders/common.vert"), METADOT_RESLOC_STR("data/shaders/waterFlow.frag")){};

    void prepare() {}

    void update(int w, int h) {
        int res_loc = METAENGINE_Render_GetUniformLocation(shader, "resolution");

        float res[2] = {(float) w, (float) h};
        METAENGINE_Render_SetUniformfv(res_loc, 2, 1, res);
    }
};

class WaterShader : public Shader {
public:
    WaterShader() : Shader(METADOT_RESLOC_STR("data/shaders/common.vert"), METADOT_RESLOC_STR("data/shaders/water.frag")){};

    void prepare() {}

    void update(float t, int w, int h, METAENGINE_Render_Image *maskImg, int mask_x, int mask_y, int mask_w, int mask_h, int scale, METAENGINE_Render_Image *flowImg, int overlay, bool showFlow, bool pixelated) {
        int time_loc = METAENGINE_Render_GetUniformLocation(shader, "time");
        int res_loc = METAENGINE_Render_GetUniformLocation(shader, "resolution");
        int mask_loc = METAENGINE_Render_GetUniformLocation(shader, "mask");
        int mask_pos_loc = METAENGINE_Render_GetUniformLocation(shader, "maskPos");
        int mask_size_loc = METAENGINE_Render_GetUniformLocation(shader, "maskSize");
        int scale_loc = METAENGINE_Render_GetUniformLocation(shader, "scale");
        int flowTex_loc = METAENGINE_Render_GetUniformLocation(shader, "flowTex");
        int overlay_loc = METAENGINE_Render_GetUniformLocation(shader, "overlay");
        int showFlow_loc = METAENGINE_Render_GetUniformLocation(shader, "showFlow");
        int pixelated_loc = METAENGINE_Render_GetUniformLocation(shader, "pixelated");

        METAENGINE_Render_SetUniformf(time_loc, t);

        float res[2] = {(float) w, (float) h};
        METAENGINE_Render_SetUniformfv(res_loc, 2, 1, res);

        METAENGINE_Render_SetShaderImage(maskImg, mask_loc, 1);

        float res2[2] = {(float) mask_x, (float) mask_y};
        METAENGINE_Render_SetUniformfv(mask_pos_loc, 2, 1, res2);
        float res3[2] = {(float) mask_w, (float) mask_h};
        METAENGINE_Render_SetUniformfv(mask_size_loc, 2, 1, res3);

        METAENGINE_Render_SetUniformf(scale_loc, scale);

        METAENGINE_Render_SetShaderImage(flowImg, flowTex_loc, 2);

        METAENGINE_Render_SetUniformi(overlay_loc, overlay);
        METAENGINE_Render_SetUniformi(showFlow_loc, showFlow);
        METAENGINE_Render_SetUniformi(pixelated_loc, pixelated);
    }
};

class NewLightingShader : public Shader {
public:
    float lastLx = 0.0;
    float lastLy = 0.0;
    float lastQuality = 0.0;
    float lastInside = 0.0;
    bool lastSimpleMode = false;
    bool lastEmissionEnabled = false;
    bool lastDitheringEnabled = false;

    NewLightingShader() : Shader(METADOT_RESLOC_STR("data/shaders/common.vert"), METADOT_RESLOC_STR("data/shaders/newLighting.frag")){};

    void prepare() {}

    void setSimpleMode(bool simpleMode) {
        int simpleOnly_loc = METAENGINE_Render_GetUniformLocation(shader, "simpleOnly");
        METAENGINE_Render_SetUniformi(simpleOnly_loc, simpleMode);

        lastSimpleMode = simpleMode;
    }

    void setEmissionEnabled(bool emissionEnabled) {
        int emission_loc = METAENGINE_Render_GetUniformLocation(shader, "emission");
        METAENGINE_Render_SetUniformi(emission_loc, emissionEnabled);

        lastEmissionEnabled = emissionEnabled;
    }

    void setDitheringEnabled(bool ditheringEnabled) {
        int dithering_loc = METAENGINE_Render_GetUniformLocation(shader, "dithering");
        METAENGINE_Render_SetUniformi(dithering_loc, ditheringEnabled);

        lastDitheringEnabled = ditheringEnabled;
    }

    void setQuality(float quality) {
        int lightingQuality_loc = METAENGINE_Render_GetUniformLocation(shader, "lightingQuality");
        METAENGINE_Render_SetUniformf(lightingQuality_loc, quality);

        lastQuality = quality;
    }

    void setInside(float inside) {
        int inside_loc = METAENGINE_Render_GetUniformLocation(shader, "inside");
        METAENGINE_Render_SetUniformf(inside_loc, inside);

        lastInside = inside;
    }

    void setBounds(float minX, float minY, float maxX, float maxY) {
        int minX_loc = METAENGINE_Render_GetUniformLocation(shader, "minX");
        int minY_loc = METAENGINE_Render_GetUniformLocation(shader, "minY");
        int maxX_loc = METAENGINE_Render_GetUniformLocation(shader, "maxX");
        int maxY_loc = METAENGINE_Render_GetUniformLocation(shader, "maxY");

        METAENGINE_Render_SetUniformf(minX_loc, minX);
        METAENGINE_Render_SetUniformf(minY_loc, minY);
        METAENGINE_Render_SetUniformf(maxX_loc, maxX);
        METAENGINE_Render_SetUniformf(maxY_loc, maxY);
    }

    void update(METAENGINE_Render_Image *tex, METAENGINE_Render_Image *emit, float x, float y) {
        int txrmap_loc = METAENGINE_Render_GetUniformLocation(shader, "txrmap");
        int emitmap_loc = METAENGINE_Render_GetUniformLocation(shader, "emitmap");
        int txrsize_loc = METAENGINE_Render_GetUniformLocation(shader, "texSize");
        int t0_loc = METAENGINE_Render_GetUniformLocation(shader, "t0");

        lastLx = x;
        lastLy = y;

        float res[2] = {x, y};
        METAENGINE_Render_SetUniformfv(t0_loc, 2, 1, res);

        float tres[2] = {(float) tex->w, (float) tex->h};
        METAENGINE_Render_SetUniformfv(txrsize_loc, 2, 1, tres);

        METAENGINE_Render_SetShaderImage(tex, txrmap_loc, 1);
        METAENGINE_Render_SetShaderImage(emit, emitmap_loc, 2);
    }
};

class FireShader : public Shader {
public:
    FireShader() : Shader(METADOT_RESLOC_STR("data/shaders/common.vert"), METADOT_RESLOC_STR("data/shaders/fire.frag")){};

    void prepare() {}

    void update(METAENGINE_Render_Image *tex) {
        int firemap_loc = METAENGINE_Render_GetUniformLocation(shader, "firemap");
        int txrsize_loc = METAENGINE_Render_GetUniformLocation(shader, "texSize");

        float tres[2] = {(float) tex->w, (float) tex->h};
        METAENGINE_Render_SetUniformfv(txrsize_loc, 2, 1, tres);

        METAENGINE_Render_SetShaderImage(tex, firemap_loc, 1);
    }
};

class Fire2Shader : public Shader {
public:
    Fire2Shader() : Shader(METADOT_RESLOC_STR("data/shaders/common.vert"), METADOT_RESLOC_STR("data/shaders/fire2.frag")){};

    void prepare() {}

    void update(METAENGINE_Render_Image *tex) {
        int firemap_loc = METAENGINE_Render_GetUniformLocation(shader, "firemap");
        int txrsize_loc = METAENGINE_Render_GetUniformLocation(shader, "texSize");

        float tres[2] = {(float) tex->w, (float) tex->h};
        METAENGINE_Render_SetUniformfv(txrsize_loc, 2, 1, tres);

        METAENGINE_Render_SetShaderImage(tex, firemap_loc, 1);
    }
};
