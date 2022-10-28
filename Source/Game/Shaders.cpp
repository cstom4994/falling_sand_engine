#include "Shaders.hpp"

Uint32 Shaders::load_shader(METAENGINE_Render_ShaderEnum shader_type, const char *filename) {
    Uint32 shader;
    int file_size;
    METAENGINE_Render_Renderer *renderer = METAENGINE_Render_GetCurrentRenderer();

    // Open file
    // auto file = ReadFile(filename);
    // std::string data(file.begin(), file.end());
    // file_size = file.size();

    auto data_vec = rh::embed.FindByFilename(filename);
    auto data = data_vec[0].GetArray();
    std::string source(data.begin(), data.end());
    file_size = data.size();

    // Get size from header
    // if (renderer->shader_language == METAENGINE_Render_LANGUAGE_GLSL) {
    //     if (renderer->max_shader_version >= 120)
    //         header = "#version 120\n";
    //     else
    //         header = "#version 110\n";// Maybe this is good enough?
    // } else if (renderer->shader_language == METAENGINE_Render_LANGUAGE_GLSLES)
    //     header = "#version 100\nprecision mediump int;\nprecision mediump float;\n";

    if (data.empty() || file_size == 0) throw std::runtime_error("Failed to load shader");
    shader = METAENGINE_Render_CompileShader(shader_type, source.c_str());
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
