// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_SHADERS_HPP
#define ME_SHADERS_HPP

#include <stdlib.h>

#include "engine/core/io/filesystem.h"
#include "engine/core/sdl_wrapper.h"
#include "engine/renderer/renderer_gpu.h"

// Loads a shader and prepends version/compatibility info before compiling it.
u32 ME_Shaders_LoadShader(R_ShaderEnum shader_type, const char *filename);
R_ShaderBlock ME_Shaders_LoadShaderProgram(u32 *p, const char *vertex_shader_file, const char *fragment_shader_file);
void ME_Shaders_FreeShader(u32 p);

class ShaderBase {
public:
    u32 shader;
    R_ShaderBlock block;
    const char *vertex_shader_file;
    const char *fragment_shader_file;

    u32 Init();
    void Unload();
    void Activate();
};

#endif
