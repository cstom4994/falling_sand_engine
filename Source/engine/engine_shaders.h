// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_SHADERS_HPP_
#define _METADOT_SHADERS_HPP_

#include <stdlib.h>

#include "engine/filesystem.h"
#include "engine/renderer/renderer_gpu.h"
#include "engine/sdl_wrapper.h"

// Loads a shader and prepends version/compatibility info before compiling it.
U32 METAENGINE_Shaders_LoadShader(R_ShaderEnum shader_type, const char *filename);
R_ShaderBlock METAENGINE_Shaders_LoadShaderProgram(U32 *p, const char *vertex_shader_file, const char *fragment_shader_file);
void METAENGINE_Shaders_FreeShader(U32 p);

typedef struct metadot_shader_base {
    U32 shader;
    R_ShaderBlock block;
    const char *vertex_shader_file;
    const char *fragment_shader_file;
} metadot_shader_base;

U32 ShaderInit(metadot_shader_base *shader_t);
void ShaderUnload(metadot_shader_base *shader_t);
void ShaderActivate(metadot_shader_base *shader_t);

#endif