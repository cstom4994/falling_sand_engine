#include "renderer_gpu_GLES_2.h"
#include "renderer_gpu_RendererImpl.h"


#if defined(SDL_METAENGINE_Render_DISABLE_GLES) || defined(SDL_METAENGINE_Render_DISABLE_GLES_2)

// Dummy implementations
METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_GLES_2(METAENGINE_Render_RendererID request) {return NULL;}
void METAENGINE_Render_FreeRenderer_GLES_2(METAENGINE_Render_Renderer* renderer) {}

#else


// Most of the code pulled in from here...
#define SDL_METAENGINE_Render_USE_GLES
#define SDL_METAENGINE_Render_GLES_MAJOR_VERSION 2
#define SDL_METAENGINE_Render_GLSL_VERSION 100

#define SDL_METAENGINE_Render_USE_BUFFER_PIPELINE
#define SDL_METAENGINE_Render_SKIP_ENABLE_TEXTURE_2D
#define SDL_METAENGINE_Render_ASSUME_CORE_FBO
#define SDL_METAENGINE_Render_ASSUME_SHADERS
#define SDL_METAENGINE_Render_DISABLE_TEXTURE_GETS
#define SDL_METAENGINE_Render_NO_VAO

#include "renderer_GL_common.inl"
#include "renderer_shapes_GL_common.inl"


METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_GLES_2(METAENGINE_Render_RendererID request)
{
    METAENGINE_Render_Renderer* renderer = (METAENGINE_Render_Renderer*)SDL_malloc(sizeof(METAENGINE_Render_Renderer));
    if(renderer == NULL)
        return NULL;

    memset(renderer, 0, sizeof(METAENGINE_Render_Renderer));

    renderer->id = request;
    renderer->id.renderer = METAENGINE_Render_RENDERER_GLES_2;
    renderer->shader_language = METAENGINE_Render_LANGUAGE_GLSLES;
    renderer->min_shader_version = 100;
    renderer->max_shader_version = SDL_METAENGINE_Render_GLSL_VERSION;
    
    renderer->default_image_anchor_x = 0.5f;
    renderer->default_image_anchor_y = 0.5f;
    
    renderer->current_context_target = NULL;
    
    renderer->impl = (METAENGINE_Render_RendererImpl*)SDL_malloc(sizeof(METAENGINE_Render_RendererImpl));
    memset(renderer->impl, 0, sizeof(METAENGINE_Render_RendererImpl));
    SET_COMMON_FUNCTIONS(renderer->impl);

    return renderer;
}

void METAENGINE_Render_FreeRenderer_GLES_2(METAENGINE_Render_Renderer* renderer)
{
    if(renderer == NULL)
        return;

    SDL_free(renderer->impl);
    SDL_free(renderer);
}


#endif
