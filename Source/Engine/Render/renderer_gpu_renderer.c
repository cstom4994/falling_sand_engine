#include "renderer_gpu.h"
#include "SDL_platform.h"
#include <string.h>

#ifndef _MSC_VER
	#include <strings.h>
#else
	#define __func__ __FUNCTION__
	// Disable warning: selection for inlining
	#pragma warning(disable: 4514 4711)
	// Disable warning: Spectre mitigation
	#pragma warning(disable: 5045)
#endif

#define METAENGINE_Render_MAX_ACTIVE_RENDERERS 20
#define METAENGINE_Render_MAX_REGISTERED_RENDERERS 10

void gpu_init_renderer_register(void);
void gpu_free_renderer_register(void);

typedef struct METAENGINE_Render_RendererRegistration
{
	METAENGINE_Render_RendererID id;
	METAENGINE_Render_Renderer* (*createFn)(METAENGINE_Render_RendererID request);
	void (*freeFn)(METAENGINE_Render_Renderer*);
} METAENGINE_Render_RendererRegistration;

static METAENGINE_Render_bool _gpu_renderer_register_is_initialized = METAENGINE_Render_FALSE;

static METAENGINE_Render_Renderer* _gpu_renderer_map[METAENGINE_Render_MAX_ACTIVE_RENDERERS];
static METAENGINE_Render_RendererRegistration _gpu_renderer_register[METAENGINE_Render_MAX_REGISTERED_RENDERERS];

static int _gpu_renderer_order_size = 0;
static METAENGINE_Render_RendererID _gpu_renderer_order[METAENGINE_Render_RENDERER_ORDER_MAX];






METAENGINE_Render_RendererEnum METAENGINE_Render_ReserveNextRendererEnum(void)
{
    static METAENGINE_Render_RendererEnum last_enum = METAENGINE_Render_RENDERER_CUSTOM_0;
    return last_enum++;
}

int METAENGINE_Render_GetNumActiveRenderers(void)
{
	int count;
	int i;

	gpu_init_renderer_register();

	count = 0;
	for(i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++)
	{
		if(_gpu_renderer_map[i] != NULL)
			count++;
	}
	return count;
}

void METAENGINE_Render_GetActiveRendererList(METAENGINE_Render_RendererID* renderers_array)
{
	int count;
	int i;

	gpu_init_renderer_register();

	count = 0;
	for(i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++)
	{
		if(_gpu_renderer_map[i] != NULL)
		{
			renderers_array[count] = _gpu_renderer_map[i]->id;
			count++;
		}
	}
}


int METAENGINE_Render_GetNumRegisteredRenderers(void)
{
	int count;
	int i;

	gpu_init_renderer_register();

	count = 0;
	for(i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++)
	{
		if(_gpu_renderer_register[i].id.renderer != METAENGINE_Render_RENDERER_UNKNOWN)
			count++;
	}
	return count;
}

void METAENGINE_Render_GetRegisteredRendererList(METAENGINE_Render_RendererID* renderers_array)
{
	int count;
	int i;

	gpu_init_renderer_register();

	count = 0;
	for(i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++)
	{
		if(_gpu_renderer_register[i].id.renderer != METAENGINE_Render_RENDERER_UNKNOWN)
		{
			renderers_array[count] = _gpu_renderer_register[i].id;
			count++;
		}
	}
}


METAENGINE_Render_RendererID METAENGINE_Render_GetRendererID(METAENGINE_Render_RendererEnum renderer)
{
	int i;

	gpu_init_renderer_register();

	for(i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++)
	{
		if(_gpu_renderer_register[i].id.renderer == renderer)
			return _gpu_renderer_register[i].id;
	}
	
	return METAENGINE_Render_MakeRendererID("Unknown", METAENGINE_Render_RENDERER_UNKNOWN, 0, 0);
}

METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_OpenGL_1_BASE(METAENGINE_Render_RendererID request);
void METAENGINE_Render_FreeRenderer_OpenGL_1_BASE(METAENGINE_Render_Renderer* renderer);
METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_OpenGL_1(METAENGINE_Render_RendererID request);
void METAENGINE_Render_FreeRenderer_OpenGL_1(METAENGINE_Render_Renderer* renderer);
METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_OpenGL_2(METAENGINE_Render_RendererID request);
void METAENGINE_Render_FreeRenderer_OpenGL_2(METAENGINE_Render_Renderer* renderer);
METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_OpenGL_3(METAENGINE_Render_RendererID request);
void METAENGINE_Render_FreeRenderer_OpenGL_3(METAENGINE_Render_Renderer* renderer);
METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_OpenGL_4(METAENGINE_Render_RendererID request);
void METAENGINE_Render_FreeRenderer_OpenGL_4(METAENGINE_Render_Renderer* renderer);
METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_GLES_1(METAENGINE_Render_RendererID request);
void METAENGINE_Render_FreeRenderer_GLES_1(METAENGINE_Render_Renderer* renderer);
METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_GLES_2(METAENGINE_Render_RendererID request);
void METAENGINE_Render_FreeRenderer_GLES_2(METAENGINE_Render_Renderer* renderer);
METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer_GLES_3(METAENGINE_Render_RendererID request);
void METAENGINE_Render_FreeRenderer_GLES_3(METAENGINE_Render_Renderer* renderer);

void METAENGINE_Render_RegisterRenderer(METAENGINE_Render_RendererID id, METAENGINE_Render_Renderer* (*create_renderer)(METAENGINE_Render_RendererID request), void (*free_renderer)(METAENGINE_Render_Renderer* renderer))
{
    int i = METAENGINE_Render_GetNumRegisteredRenderers();
    
	if(i >= METAENGINE_Render_MAX_REGISTERED_RENDERERS)
		return;
    
    if(id.renderer == METAENGINE_Render_RENDERER_UNKNOWN)
    {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR, "Invalid renderer ID");
        return;
    }
    if(create_renderer == NULL)
    {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer create callback");
        return;
    }
    if(free_renderer == NULL)
    {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer free callback");
        return;
    }
    
    _gpu_renderer_register[i].id = id;
    _gpu_renderer_register[i].createFn = create_renderer;
    _gpu_renderer_register[i].freeFn = free_renderer;
}

void gpu_register_built_in_renderers(void)
{
	#ifndef SDL_METAENGINE_Render_DISABLE_OPENGL
        #ifndef SDL_METAENGINE_Render_DISABLE_OPENGL_1_BASE
        METAENGINE_Render_RegisterRenderer(METAENGINE_Render_MakeRendererID("OpenGL 1 BASE", METAENGINE_Render_RENDERER_OPENGL_1_BASE, 1, 1),
                             &METAENGINE_Render_CreateRenderer_OpenGL_1_BASE,
                             &METAENGINE_Render_FreeRenderer_OpenGL_1_BASE);
        #endif
        
        #ifndef SDL_METAENGINE_Render_DISABLE_OPENGL_1
        METAENGINE_Render_RegisterRenderer(METAENGINE_Render_MakeRendererID("OpenGL 1", METAENGINE_Render_RENDERER_OPENGL_1, 1, 1),
                             &METAENGINE_Render_CreateRenderer_OpenGL_1,
                             &METAENGINE_Render_FreeRenderer_OpenGL_1);
        #endif
	
        #ifndef SDL_METAENGINE_Render_DISABLE_OPENGL_2
            METAENGINE_Render_RegisterRenderer(METAENGINE_Render_MakeRendererID("OpenGL 2", METAENGINE_Render_RENDERER_OPENGL_2, 2, 0),
                                 &METAENGINE_Render_CreateRenderer_OpenGL_2,
                                 &METAENGINE_Render_FreeRenderer_OpenGL_2);
        #endif
	
        #ifndef SDL_METAENGINE_Render_DISABLE_OPENGL_3
            #ifdef __MACOSX__
            // Depending on OS X version, it might only support core GL 3.3 or 3.2
            METAENGINE_Render_RegisterRenderer(METAENGINE_Render_MakeRendererID("OpenGL 3", METAENGINE_Render_RENDERER_OPENGL_3, 3, 2),
                                 &METAENGINE_Render_CreateRenderer_OpenGL_3,
                                 &METAENGINE_Render_FreeRenderer_OpenGL_3);
            #else
            METAENGINE_Render_RegisterRenderer(METAENGINE_Render_MakeRendererID("OpenGL 3", METAENGINE_Render_RENDERER_OPENGL_3, 3, 0),
                                 &METAENGINE_Render_CreateRenderer_OpenGL_3,
                                 &METAENGINE_Render_FreeRenderer_OpenGL_3);
            #endif
        #endif
	
        #ifndef SDL_METAENGINE_Render_DISABLE_OPENGL_4
            #ifdef __MACOSX__
            METAENGINE_Render_RegisterRenderer(METAENGINE_Render_MakeRendererID("OpenGL 4", METAENGINE_Render_RENDERER_OPENGL_4, 4, 1),
                                 &METAENGINE_Render_CreateRenderer_OpenGL_4,
                                 &METAENGINE_Render_FreeRenderer_OpenGL_4);
            #else
            METAENGINE_Render_RegisterRenderer(METAENGINE_Render_MakeRendererID("OpenGL 4", METAENGINE_Render_RENDERER_OPENGL_4, 4, 0),
                                 &METAENGINE_Render_CreateRenderer_OpenGL_4,
                                 &METAENGINE_Render_FreeRenderer_OpenGL_4);
            #endif
        #endif
    #endif
	
	#ifndef SDL_METAENGINE_Render_DISABLE_GLES
        #ifndef SDL_METAENGINE_Render_DISABLE_GLES_1
        METAENGINE_Render_RegisterRenderer(METAENGINE_Render_MakeRendererID("OpenGLES 1", METAENGINE_Render_RENDERER_GLES_1, 1, 1),
                             &METAENGINE_Render_CreateRenderer_GLES_1,
                             &METAENGINE_Render_FreeRenderer_GLES_1);
        #endif
        #ifndef SDL_METAENGINE_Render_DISABLE_GLES_2
        METAENGINE_Render_RegisterRenderer(METAENGINE_Render_MakeRendererID("OpenGLES 2", METAENGINE_Render_RENDERER_GLES_2, 2, 0),
                             &METAENGINE_Render_CreateRenderer_GLES_2,
                             &METAENGINE_Render_FreeRenderer_GLES_2);
        #endif
        #ifndef SDL_METAENGINE_Render_DISABLE_GLES_3
        METAENGINE_Render_RegisterRenderer(METAENGINE_Render_MakeRendererID("OpenGLES 3", METAENGINE_Render_RENDERER_GLES_3, 3, 0),
                             &METAENGINE_Render_CreateRenderer_GLES_3,
                             &METAENGINE_Render_FreeRenderer_GLES_3);
        #endif
    #endif
	
}

void gpu_init_renderer_register(void)
{
	int i;

	if(_gpu_renderer_register_is_initialized)
		return;
	
	for(i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++)
	{
		_gpu_renderer_register[i].id.name = "Unknown";
		_gpu_renderer_register[i].id.renderer = METAENGINE_Render_RENDERER_UNKNOWN;
		_gpu_renderer_register[i].createFn = NULL;
		_gpu_renderer_register[i].freeFn = NULL;
	}
	for(i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++)
	{
		_gpu_renderer_map[i] = NULL;
	}
	
	METAENGINE_Render_GetDefaultRendererOrder(&_gpu_renderer_order_size, _gpu_renderer_order);
	
	_gpu_renderer_register_is_initialized = 1;
	
	gpu_register_built_in_renderers();
}

void gpu_free_renderer_register(void)
{
	int i;

	for(i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++)
	{
		_gpu_renderer_register[i].id.name = "Unknown";
		_gpu_renderer_register[i].id.renderer = METAENGINE_Render_RENDERER_UNKNOWN;
		_gpu_renderer_register[i].createFn = NULL;
		_gpu_renderer_register[i].freeFn = NULL;
	}
	for(i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++)
	{
		_gpu_renderer_map[i] = NULL;
	}
	
	_gpu_renderer_register_is_initialized = 0;
	_gpu_renderer_order_size = 0;
}


void METAENGINE_Render_GetRendererOrder(int* order_size, METAENGINE_Render_RendererID* order)
{
    if(order_size != NULL)
        *order_size = _gpu_renderer_order_size;
    
    if(order != NULL && _gpu_renderer_order_size > 0)
        memcpy(order, _gpu_renderer_order, _gpu_renderer_order_size*sizeof(METAENGINE_Render_RendererID));
}

void METAENGINE_Render_SetRendererOrder(int order_size, METAENGINE_Render_RendererID* order)
{
    if(order == NULL)
    {
        // Restore the default order
        int count = 0;
        METAENGINE_Render_RendererID default_order[METAENGINE_Render_RENDERER_ORDER_MAX];
        METAENGINE_Render_GetDefaultRendererOrder(&count, default_order);
        METAENGINE_Render_SetRendererOrder(count, default_order);  // Call us again with the default order
        return;
    }
    
    if(order_size <= 0)
        return;
    
    if(order_size > METAENGINE_Render_RENDERER_ORDER_MAX)
    {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR, "Given order_size (%d) is greater than METAENGINE_Render_RENDERER_ORDER_MAX (%d)", order_size, METAENGINE_Render_RENDERER_ORDER_MAX);
        order_size = METAENGINE_Render_RENDERER_ORDER_MAX;
    }
    
    memcpy(_gpu_renderer_order, order, order_size*sizeof(METAENGINE_Render_RendererID));
    _gpu_renderer_order_size = order_size;
}



void METAENGINE_Render_GetDefaultRendererOrder(int* order_size, METAENGINE_Render_RendererID* order)
{
    int count = 0;
    METAENGINE_Render_RendererID default_order[METAENGINE_Render_RENDERER_ORDER_MAX];
    
    #ifndef SDL_METAENGINE_Render_DISABLE_GLES
        #ifndef SDL_METAENGINE_Render_DISABLE_GLES_3
            default_order[count++] = METAENGINE_Render_MakeRendererID("OpenGLES 3", METAENGINE_Render_RENDERER_GLES_3, 3, 0);
        #endif
        #ifndef SDL_METAENGINE_Render_DISABLE_GLES_2
            default_order[count++] = METAENGINE_Render_MakeRendererID("OpenGLES 2", METAENGINE_Render_RENDERER_GLES_2, 2, 0);
        #endif
        #ifndef SDL_METAENGINE_Render_DISABLE_GLES_1
            default_order[count++] = METAENGINE_Render_MakeRendererID("OpenGLES 1", METAENGINE_Render_RENDERER_GLES_1, 1, 1);
        #endif
    #endif
    
    #ifndef SDL_METAENGINE_Render_DISABLE_OPENGL
        #ifdef __MACOSX__
        
            // My understanding of OS X OpenGL support:
            // OS X 10.9: GL 2.1, 3.3, 4.1
            // OS X 10.7: GL 2.1, 3.2
            // OS X 10.6: GL 1.4, 2.1
            #ifndef SDL_METAENGINE_Render_DISABLE_OPENGL_4
            default_order[count++] = METAENGINE_Render_MakeRendererID("OpenGL 4", METAENGINE_Render_RENDERER_OPENGL_4, 4, 1);
            #endif
            #ifndef SDL_METAENGINE_Render_DISABLE_OPENGL_3
            default_order[count++] = METAENGINE_Render_MakeRendererID("OpenGL 3", METAENGINE_Render_RENDERER_OPENGL_3, 3, 2);
            #endif
        
        #else
        
            #ifndef SDL_METAENGINE_Render_DISABLE_OPENGL_4
            default_order[count++] = METAENGINE_Render_MakeRendererID("OpenGL 4", METAENGINE_Render_RENDERER_OPENGL_4, 4, 0);
            #endif
            #ifndef SDL_METAENGINE_Render_DISABLE_OPENGL_3
            default_order[count++] = METAENGINE_Render_MakeRendererID("OpenGL 3", METAENGINE_Render_RENDERER_OPENGL_3, 3, 0);
            #endif
            
        #endif
        
        #ifndef SDL_METAENGINE_Render_DISABLE_OPENGL_2
        default_order[count++] = METAENGINE_Render_MakeRendererID("OpenGL 2", METAENGINE_Render_RENDERER_OPENGL_2, 2, 0);
        #endif
        #ifndef SDL_METAENGINE_Render_DISABLE_OPENGL_1
        default_order[count++] = METAENGINE_Render_MakeRendererID("OpenGL 1", METAENGINE_Render_RENDERER_OPENGL_1, 1, 1);
        #endif
        
    #endif
    
    if(order_size != NULL)
        *order_size = count;
    
    if(order != NULL && count > 0)
        memcpy(order, default_order, count*sizeof(METAENGINE_Render_RendererID));
}


METAENGINE_Render_Renderer* METAENGINE_Render_CreateRenderer(METAENGINE_Render_RendererID id)
{
	METAENGINE_Render_Renderer* result = NULL;
	int i;
	for(i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++)
	{
		if(_gpu_renderer_register[i].id.renderer == METAENGINE_Render_RENDERER_UNKNOWN)
			continue;
		
		if(id.renderer == _gpu_renderer_register[i].id.renderer)
		{
			if(_gpu_renderer_register[i].createFn != NULL)
            {
                // Use the registered name
                id.name = _gpu_renderer_register[i].id.name;
				result = _gpu_renderer_register[i].createFn(id);
            }
			break;
		}
	}
	
	if(result == NULL)
    {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR, "Renderer was not found in the renderer registry.");
    }
	return result;
}

// Get a renderer from the map.
METAENGINE_Render_Renderer* METAENGINE_Render_GetRenderer(METAENGINE_Render_RendererID id)
{
	int i;
	gpu_init_renderer_register();
	
	// Invalid enum?
	if(id.renderer == METAENGINE_Render_RENDERER_UNKNOWN)
        return NULL;
	
	for(i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++)
	{
		if(_gpu_renderer_map[i] == NULL)
			continue;
		
		if(id.renderer == _gpu_renderer_map[i]->id.renderer)
			return _gpu_renderer_map[i];
	}
    
    return NULL;
}

// Create a new renderer based on a registered id and store it in the map.
METAENGINE_Render_Renderer* gpu_create_and_add_renderer(METAENGINE_Render_RendererID id)
{
	int i;
	for(i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++)
	{
		if(_gpu_renderer_map[i] == NULL)
		{
			// Create
			METAENGINE_Render_Renderer* renderer = METAENGINE_Render_CreateRenderer(id);
			if(renderer == NULL)
            {
                METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_BACKEND_ERROR, "Failed to create new renderer.");
                return NULL;
            }
            
			// Add
			_gpu_renderer_map[i] = renderer;
			// Return
			return renderer;
		}
	}
	
    METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_BACKEND_ERROR, "Couldn't create a new renderer.  Too many active renderers for METAENGINE_Render_MAX_ACTIVE_RENDERERS (%d).", METAENGINE_Render_MAX_ACTIVE_RENDERERS);
	return NULL;
}

// Free renderer memory according to how the registry instructs
void gpu_free_renderer_memory(METAENGINE_Render_Renderer* renderer)
{
	int i;
	if(renderer == NULL)
        return;
	
	for(i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++)
	{
		if(_gpu_renderer_register[i].id.renderer == METAENGINE_Render_RENDERER_UNKNOWN)
			continue;
		
		if(renderer->id.renderer == _gpu_renderer_register[i].id.renderer)
		{
			_gpu_renderer_register[i].freeFn(renderer);
			return;
		}
	}
}

// Remove a renderer from the active map and free it.
void METAENGINE_Render_FreeRenderer(METAENGINE_Render_Renderer* renderer)
{
	int i;
	METAENGINE_Render_Renderer* current_renderer;
	
	if(renderer == NULL)
        return;
	
    current_renderer = METAENGINE_Render_GetCurrentRenderer();
    if(current_renderer == renderer)
        METAENGINE_Render_SetCurrentRenderer(METAENGINE_Render_MakeRendererID("Unknown", METAENGINE_Render_RENDERER_UNKNOWN, 0, 0));
        
	for(i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++)
	{
		if(renderer == _gpu_renderer_map[i])
		{
			gpu_free_renderer_memory(renderer);
			_gpu_renderer_map[i] = NULL;
			return;
		}
	}
}
