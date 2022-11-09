#include "renderer_gpu_GLES_1.h"
#include "renderer_gpu_RendererImpl.h"

#if defined(SDL_METAENGINE_Render_DISABLE_GLES) || defined(SDL_METAENGINE_Render_DISABLE_GLES_1)

// Dummy implementations
METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_GLES_1(METAENGINE_Render_RendererID request) {return NULL;}
void METAENGINE_Render_FreeRenderer_GLES_1(METAENGINE_Render_Renderer* renderer) {}

#else


// Most of the code pulled in from here...
#define SDL_METAENGINE_Render_USE_GLES
#define SDL_METAENGINE_Render_GLES_MAJOR_VERSION 1

#define SDL_METAENGINE_Render_USE_ARRAY_PIPELINE
#define SDL_METAENGINE_Render_DISABLE_TEXTURE_GETS
#define SDL_METAENGINE_Render_DISABLE_SHADERS
#define SDL_METAENGINE_Render_DISABLE_RENDER_TO_TEXTURE
#define SDL_METAENGINE_Render_APPLY_TRANSFORMS_TO_GL_STACK
#define SDL_METAENGINE_Render_NO_VAO

#include "renderer_GL_common.inl"
#include "renderer_shapes_GL_common.inl"


METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_GLES_1(METAENGINE_Render_RendererID request)
{
    METAENGINE_Render_Renderer* renderer = (METAENGINE_Render_Renderer*)SDL_malloc(sizeof(METAENGINE_Render_Renderer));
    if(renderer == NULL)
        return NULL;

    memset(renderer, 0, sizeof(METAENGINE_Render_Renderer));

    renderer->id = request;
    renderer->id.renderer = METAENGINE_Render_RENDERER_GLES_1;
    renderer->shader_language = METAENGINE_Render_LANGUAGE_NONE;
    renderer->min_shader_version = 0;
    renderer->max_shader_version = 0;
    
    renderer->default_image_anchor_x = 0.5f;
    renderer->default_image_anchor_y = 0.5f;
    
    renderer->current_context_target = NULL;
    
    renderer->impl = (METAENGINE_Render_RendererImpl*)SDL_malloc(sizeof(METAENGINE_Render_RendererImpl));
    memset(renderer->impl, 0, sizeof(METAENGINE_Render_RendererImpl));
    SET_COMMON_FUNCTIONS(renderer->impl);

    return renderer;
}

void METAENGINE_Render_FreeRenderer_GLES_1(METAENGINE_Render_Renderer* renderer)
{
    if(renderer == NULL)
        return;

    SDL_free(renderer->impl);
    SDL_free(renderer);
}

#endif
