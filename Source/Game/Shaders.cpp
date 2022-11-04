#include "Shaders.hpp"
#include <cstring>

Uint32 Shaders::load_shader(METAENGINE_Render_ShaderEnum shader_type, const char *data) {
    Uint32 shader;
    METAENGINE_Render_Renderer *renderer = METAENGINE_Render_GetCurrentRenderer();

    if (strlen(data) == 0) throw std::runtime_error("Failed to load shader");
    shader = METAENGINE_Render_CompileShader(shader_type, data);
    return shader;
}

METAENGINE_Render_ShaderBlock Shaders::load_shader_program(Uint32 *p, const char *vertex_shader_file, const char *fragment_shader_file) {
    Uint32 v, f;
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

void Shaders::free_shader(Uint32 p) {
    METAENGINE_Render_FreeShaderProgram(p);
}
