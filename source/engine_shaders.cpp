// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "engine_shaders.hpp"

#include "core/core.hpp"

U32 METAENGINE_Shaders_LoadShader(R_ShaderEnum thisype, const char *filename) {

    U32 shader;
    R_Renderer *renderer = R_GetCurrentRenderer();

    char *source = metadot_fs_readfilestring(filename);

    if (!source) {
        R_PushErrorCode("load_shader", R_ERROR_FILE_NOT_FOUND, "Shader file \"%s\" not found", filename);
        return METADOT_FAILED;
    }

    // Compile the shader
    shader = R_CompileShader(thisype, source);

    metadot_fs_freestring(source);

    return shader;
}

R_ShaderBlock METAENGINE_Shaders_LoadShaderProgram(U32 *p, const char *vertex_shader_file, const char *fragment_shader_file) {
    U32 v, f;
    v = METAENGINE_Shaders_LoadShader(R_VERTEX_SHADER, vertex_shader_file);

    if (!v) METADOT_ERROR("Failed to load vertex shader (%s): %s", vertex_shader_file, R_GetShaderMessage());

    f = METAENGINE_Shaders_LoadShader(R_FRAGMENT_SHADER, fragment_shader_file);

    if (!f) METADOT_ERROR("Failed to load fragment shader (%s): %s", fragment_shader_file, R_GetShaderMessage());

    *p = R_LinkShaders(v, f);

    if (!*p) {
        R_ShaderBlock b = {-1, -1, -1, -1};
        METADOT_ERROR("Failed to link shader program (%s + %s): %s", vertex_shader_file, fragment_shader_file, R_GetShaderMessage());
        return b;
    }

    {
        R_ShaderBlock block = R_LoadShaderBlock(*p, "gpu_Vertex", "gpu_TexCoord", "gpu_Color", "gpu_ModelViewProjectionMatrix");
        R_ActivateShaderProgram(*p, &block);

        return block;
    }
}

void METAENGINE_Shaders_FreeShader(U32 p) { R_FreeShaderProgram(p); }

U32 ShaderBase::Init() {
    this->shader = 0;
    this->block = METAENGINE_Shaders_LoadShaderProgram(&this->shader, this->vertex_shader_file, this->fragment_shader_file);
    return this->shader;
}

void ShaderBase::Unload() { METAENGINE_Shaders_FreeShader(this->shader); }

void ShaderBase::Activate() { R_ActivateShaderProgram(this->shader, &this->block); }
