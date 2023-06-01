// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "shaders.hpp"

#include "core/core.hpp"
#include "core/utils/utility.hpp"

u32 ME_Shaders_LoadShader(R_ShaderEnum thisype, const char *filename) {

    u32 shader;
    R_Renderer *renderer = R_GetCurrentRenderer();

    char *source = ME_fs_readfilestring(filename);

    if (!source) {
        R_PushErrorCode("load_shader", R_ERROR_FILE_NOT_FOUND, "Shader file \"%s\" not found", filename);
        return METADOT_FAILED;
    }

    // Compile the shader
    shader = R_CompileShader(thisype, source);

    ME_fs_freestring(source);

    return shader;
}

R_ShaderBlock ME_Shaders_LoadShaderProgram(u32 *p, const char *vertex_shader_file, const char *fragment_shader_file) {
    u32 v, f;
    v = ME_Shaders_LoadShader(R_VERTEX_SHADER, vertex_shader_file);

    if (!v) METADOT_ERROR("Failed to load vertex shader (%s): %s", vertex_shader_file, R_GetShaderMessage());

    f = ME_Shaders_LoadShader(R_FRAGMENT_SHADER, fragment_shader_file);

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

void ME_Shaders_FreeShader(u32 p) { R_FreeShaderProgram(p); }

u32 ShaderBase::Init() {
    this->shader = 0;
    this->block = ME_Shaders_LoadShaderProgram(&this->shader, this->vertex_shader_file, this->fragment_shader_file);
    return this->shader;
}

void ShaderBase::Unload() { ME_Shaders_FreeShader(this->shader); }

void ShaderBase::Activate() { R_ActivateShaderProgram(this->shader, &this->block); }
