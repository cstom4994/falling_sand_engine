#include "renderer_OpenGL_4.h"
#include "renderer_RendererImpl.h"


#if defined(METAENGINE_Render_DISABLE_OPENGL) || defined(METAENGINE_Render_DISABLE_OPENGL_4)

// Dummy implementations
METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_OpenGL_4(METAENGINE_Render_RendererID request) {return NULL;}
void METAENGINE_Render_FreeRenderer_OpenGL_4(METAENGINE_Render_Renderer* renderer) {}

#else


// Most of the code pulled in from here...
#define METAENGINE_Render_USE_OPENGL
#define METAENGINE_Render_USE_BUFFER_PIPELINE
#define METAENGINE_Render_ASSUME_CORE_FBO
#define METAENGINE_Render_ASSUME_SHADERS
#define METAENGINE_Render_SKIP_ENABLE_TEXTURE_2D
#define METAENGINE_Render_SKIP_LINE_WIDTH
#define METAENGINE_Render_GLSL_VERSION 150
#define METAENGINE_Render_GL_MAJOR_VERSION 4

#include "renderer_GL_common.inl"
#include "renderer_shapes_GL_common.inl"


METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_OpenGL_4(METAENGINE_Render_RendererID request)
{
    METAENGINE_Render_Renderer* renderer = (METAENGINE_Render_Renderer*)SDL_malloc(sizeof(METAENGINE_Render_Renderer));
    if(renderer == NULL)
        return NULL;

    memset(renderer, 0, sizeof(METAENGINE_Render_Renderer));

    renderer->id = request;
    renderer->id.renderer = METAENGINE_Render_RENDERER_OPENGL_4;
    renderer->shader_language = METAENGINE_Render_LANGUAGE_GLSL;
    renderer->min_shader_version = 110;
    renderer->max_shader_version = METAENGINE_Render_GLSL_VERSION;
    
    renderer->default_image_anchor_x = 0.5f;
    renderer->default_image_anchor_y = 0.5f;
    
    renderer->current_context_target = NULL;
    
    renderer->impl = (METAENGINE_Render_RendererImpl*)SDL_malloc(sizeof(METAENGINE_Render_RendererImpl));
    memset(renderer->impl, 0, sizeof(METAENGINE_Render_RendererImpl));
    SET_COMMON_FUNCTIONS(renderer->impl);

    return renderer;
}

void METAENGINE_Render_FreeRenderer_OpenGL_4(METAENGINE_Render_Renderer* renderer)
{
    if(renderer == NULL)
        return;

    SDL_free(renderer->impl);
    SDL_free(renderer);
}


#endif
