// Copyright(c) 2022, KaoruXun All rights reserved.

#include "Shaders.hpp"

#include "Engine/Memory/Memory.hpp"

UInt32 METAENGINE_Shaders_LoadShader(METAENGINE_Render_ShaderEnum shader_type, const char *filename) {

    UInt32 shader;
    METAENGINE_Render_Renderer *renderer = METAENGINE_Render_GetCurrentRenderer();

    std::string source = FUtil::readFileString(filename);

    if (source.empty()) {
        METAENGINE_Render_PushErrorCode("load_shader", METAENGINE_Render_ERROR_FILE_NOT_FOUND, "Shader file \"%s\" not found", filename);
        return 0;
    }

    // Compile the shader
    shader = METAENGINE_Render_CompileShader(shader_type, source.c_str());

    return shader;
}

METAENGINE_Render_ShaderBlock METAENGINE_Shaders_LoadShaderProgram(UInt32 *p, const char *vertex_shader_file, const char *fragment_shader_file) {
    UInt32 v, f;
    v = METAENGINE_Shaders_LoadShader(METAENGINE_Render_VERTEX_SHADER, vertex_shader_file);

    if (!v)
        METADOT_ERROR("Failed to load vertex shader ({}): {}", vertex_shader_file, METAENGINE_Render_GetShaderMessage());

    f = METAENGINE_Shaders_LoadShader(METAENGINE_Render_FRAGMENT_SHADER, fragment_shader_file);

    if (!f)
        METADOT_ERROR("Failed to load fragment shader ({}): {}", fragment_shader_file, METAENGINE_Render_GetShaderMessage());

    *p = METAENGINE_Render_LinkShaders(v, f);

    if (!*p) {
        METAENGINE_Render_ShaderBlock b = {-1, -1, -1, -1};
        METADOT_ERROR("Failed to link shader program ({} + {}): {}", vertex_shader_file, fragment_shader_file, METAENGINE_Render_GetShaderMessage());
        return b;
    }

    {
        METAENGINE_Render_ShaderBlock block = METAENGINE_Render_LoadShaderBlock(*p, "gpu_Vertex", "gpu_TexCoord", "gpu_Color", "gpu_ModelViewProjectionMatrix");
        METAENGINE_Render_ActivateShaderProgram(*p, &block);

        return block;
    }
}

void METAENGINE_Shaders_FreeShader(UInt32 p) {
    METAENGINE_Render_FreeShaderProgram(p);
}

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
