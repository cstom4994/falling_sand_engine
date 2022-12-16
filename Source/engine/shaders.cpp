// Copyright(c) 2022, KaoruXun All rights reserved.

#include "shaders.hpp"

#include "core/core.hpp"
#include "engine/internal/builtin_box2d.h"
#include "engine/memory.hpp"

U32 METAENGINE_Shaders_LoadShader(R_ShaderEnum shader_type, const char *filename) {

    U32 shader;
    R_Renderer *renderer = R_GetCurrentRenderer();

    char *source = futil_readfilestring(filename);

    if (!source) {
        R_PushErrorCode("load_shader", R_ERROR_FILE_NOT_FOUND, "Shader file \"%s\" not found",
                        filename);
        return METADOT_FAILED;
    }

    // Compile the shader
    shader = R_CompileShader(shader_type, source);

    gc_free(&gc, source);

    return shader;
}

R_ShaderBlock METAENGINE_Shaders_LoadShaderProgram(U32 *p, const char *vertex_shader_file,
                                                   const char *fragment_shader_file) {
    U32 v, f;
    v = METAENGINE_Shaders_LoadShader(R_VERTEX_SHADER, vertex_shader_file);

    if (!v)
        METADOT_ERROR("Failed to load vertex shader (%s): %s", vertex_shader_file,
                      R_GetShaderMessage());

    f = METAENGINE_Shaders_LoadShader(R_FRAGMENT_SHADER, fragment_shader_file);

    if (!f)
        METADOT_ERROR("Failed to load fragment shader (%s): %s", fragment_shader_file,
                      R_GetShaderMessage());

    *p = R_LinkShaders(v, f);

    if (!*p) {
        R_ShaderBlock b = {-1, -1, -1, -1};
        METADOT_ERROR("Failed to link shader program (%s + %s): %s", vertex_shader_file,
                      fragment_shader_file, R_GetShaderMessage());
        return b;
    }

    {
        R_ShaderBlock block = R_LoadShaderBlock(*p, "gpu_Vertex", "gpu_TexCoord", "gpu_Color",
                                                "gpu_ModelViewProjectionMatrix");
        R_ActivateShaderProgram(*p, &block);

        return block;
    }
}

void METAENGINE_Shaders_FreeShader(U32 p) { R_FreeShaderProgram(p); }

void ShaderWorker::LoadShaders() {
    if (waterShader) METADOT_DELETE(C, waterShader, WaterShader);
    if (waterFlowPassShader) METADOT_DELETE(C, waterFlowPassShader, WaterFlowPassShader);
    if (newLightingShader) METADOT_DELETE(C, newLightingShader, NewLightingShader);
    if (fireShader) METADOT_DELETE(C, fireShader, FireShader);
    if (fire2Shader) METADOT_DELETE(C, fire2Shader, Fire2Shader);

    METADOT_NEW(C, waterShader, WaterShader);
    METADOT_NEW(C, waterFlowPassShader, WaterFlowPassShader);
    METADOT_NEW(C, newLightingShader, NewLightingShader);
    METADOT_NEW(C, fireShader, FireShader);
    METADOT_NEW(C, fire2Shader, Fire2Shader);
}