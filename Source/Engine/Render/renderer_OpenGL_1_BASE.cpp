#include "renderer_OpenGL_1_BASE.h"
#include "renderer_RendererImpl.h"


#if defined(METAENGINE_Render_DISABLE_OPENGL) || defined(METAENGINE_Render_DISABLE_OPENGL_1_BASE)

// Dummy implementations
METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_OpenGL_1_BASE(METAENGINE_Render_RendererID request) {return NULL;}
void METAENGINE_Render_FreeRenderer_OpenGL_1_BASE(METAENGINE_Render_Renderer* renderer) {}

#else

// Most of the code pulled in from here...
#define METAENGINE_Render_USE_OPENGL
#define METAENGINE_Render_DISABLE_SHADERS
#define METAENGINE_Render_DISABLE_RENDER_TO_TEXTURE
#define METAENGINE_Render_USE_FIXED_FUNCTION_PIPELINE
#define METAENGINE_Render_GL_MAJOR_VERSION 1
#define METAENGINE_Render_APPLY_TRANSFORMS_TO_GL_STACK
#define METAENGINE_Render_NO_VAO

#include "renderer_GL_common.inl"
#include "renderer_shapes_GL_common.inl"


METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_OpenGL_1_BASE(METAENGINE_Render_RendererID request)
{
    METAENGINE_Render_Renderer* renderer = (METAENGINE_Render_Renderer*)SDL_malloc(sizeof(METAENGINE_Render_Renderer));
    if(renderer == NULL)
        return NULL;

    memset(renderer, 0, sizeof(METAENGINE_Render_Renderer));

    renderer->id = request;
    renderer->id.renderer = METAENGINE_Render_RENDERER_OPENGL_1_BASE;
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

void METAENGINE_Render_FreeRenderer_OpenGL_1_BASE(METAENGINE_Render_Renderer* renderer)
{
    if(renderer == NULL)
        return;

    SDL_free(renderer->impl);
    SDL_free(renderer);
}

#endif
