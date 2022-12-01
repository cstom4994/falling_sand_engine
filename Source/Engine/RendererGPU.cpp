
#include "RendererGPU.h"
#include "Core/Core.hpp"
#include "Core/DebugImpl.hpp"
#include "Core/Macros.hpp"
#include "RendererGPU.h"
#include "RendererImpl.h"

#include "external/stb_image.h"
#include "external/stb_image_write.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#ifndef PI
#define PI 3.1415926f
#endif

#define RAD_PER_DEG 0.017453293f
#define DEG_PER_RAD 57.2957795f

// Visual C does not support static inline
#ifndef static_inline
#ifdef _MSC_VER
#define static_inline static
#else
#define static_inline static inline
#endif
#endif

#define METAENGINE_Render_MAX_ACTIVE_RENDERERS 20
#define METAENGINE_Render_MAX_REGISTERED_RENDERERS 10

void gpu_init_renderer_register(void);
void gpu_free_renderer_register(void);

typedef struct METAENGINE_Render_RendererRegistration
{
    METAENGINE_Render_RendererID id;
    METAENGINE_Render_Renderer *(*createFn)(METAENGINE_Render_RendererID request);
    void (*freeFn)(METAENGINE_Render_Renderer *);
} METAENGINE_Render_RendererRegistration;

static METAENGINE_Render_bool _gpu_renderer_register_is_initialized = METAENGINE_Render_FALSE;

static METAENGINE_Render_Renderer *_gpu_renderer_map[METAENGINE_Render_MAX_ACTIVE_RENDERERS];
static METAENGINE_Render_RendererRegistration
        _gpu_renderer_register[METAENGINE_Render_MAX_REGISTERED_RENDERERS];

static int _gpu_renderer_order_size = 0;
static METAENGINE_Render_RendererID _gpu_renderer_order[METAENGINE_Render_RENDERER_ORDER_MAX];

METAENGINE_Render_RendererEnum METAENGINE_Render_ReserveNextRendererEnum(void) {
    static METAENGINE_Render_RendererEnum last_enum = METAENGINE_Render_RENDERER_CUSTOM_0;
    return last_enum++;
}

int METAENGINE_Render_GetNumActiveRenderers(void) {
    int count;
    int i;

    gpu_init_renderer_register();

    count = 0;
    for (i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++) {
        if (_gpu_renderer_map[i] != NULL) count++;
    }
    return count;
}

void METAENGINE_Render_GetActiveRendererList(METAENGINE_Render_RendererID *renderers_array) {
    int count;
    int i;

    gpu_init_renderer_register();

    count = 0;
    for (i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++) {
        if (_gpu_renderer_map[i] != NULL) {
            renderers_array[count] = _gpu_renderer_map[i]->id;
            count++;
        }
    }
}

int METAENGINE_Render_GetNumRegisteredRenderers(void) {
    int count;
    int i;

    gpu_init_renderer_register();

    count = 0;
    for (i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++) {
        if (_gpu_renderer_register[i].id.renderer != METAENGINE_Render_RENDERER_UNKNOWN) count++;
    }
    return count;
}

void METAENGINE_Render_GetRegisteredRendererList(METAENGINE_Render_RendererID *renderers_array) {
    int count;
    int i;

    gpu_init_renderer_register();

    count = 0;
    for (i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++) {
        if (_gpu_renderer_register[i].id.renderer != METAENGINE_Render_RENDERER_UNKNOWN) {
            renderers_array[count] = _gpu_renderer_register[i].id;
            count++;
        }
    }
}

METAENGINE_Render_RendererID METAENGINE_Render_GetRendererID(
        METAENGINE_Render_RendererEnum renderer) {
    int i;

    gpu_init_renderer_register();

    for (i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++) {
        if (_gpu_renderer_register[i].id.renderer == renderer) return _gpu_renderer_register[i].id;
    }

    return METAENGINE_Render_MakeRendererID("Unknown", METAENGINE_Render_RENDERER_UNKNOWN, 0, 0);
}

METAENGINE_Render_Renderer *METAENGINE_Render_CreateRenderer_OpenGL_3(
        METAENGINE_Render_RendererID request);
void METAENGINE_Render_FreeRenderer_OpenGL_3(METAENGINE_Render_Renderer *renderer);

void METAENGINE_Render_RegisterRenderer(
        METAENGINE_Render_RendererID id,
        METAENGINE_Render_Renderer *(*create_renderer)(METAENGINE_Render_RendererID request),
        void (*free_renderer)(METAENGINE_Render_Renderer *renderer)) {
    int i = METAENGINE_Render_GetNumRegisteredRenderers();

    if (i >= METAENGINE_Render_MAX_REGISTERED_RENDERERS) return;

    if (id.renderer == METAENGINE_Render_RENDERER_UNKNOWN) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR,
                                        "Invalid renderer ID");
        return;
    }
    if (create_renderer == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR,
                                        "NULL renderer create callback");
        return;
    }
    if (free_renderer == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR,
                                        "NULL renderer free callback");
        return;
    }

    _gpu_renderer_register[i].id = id;
    _gpu_renderer_register[i].createFn = create_renderer;
    _gpu_renderer_register[i].freeFn = free_renderer;
}

void gpu_register_built_in_renderers(void) {
#ifdef __MACOSX__
    // Depending on OS X version, it might only support core GL 3.3 or 3.2
    METAENGINE_Render_RegisterRenderer(
            METAENGINE_Render_MakeRendererID("OpenGL 3", METAENGINE_Render_RENDERER_OPENGL_3, 3, 2),
            &METAENGINE_Render_CreateRenderer_OpenGL_3, &METAENGINE_Render_FreeRenderer_OpenGL_3);
#else
    METAENGINE_Render_RegisterRenderer(
            METAENGINE_Render_MakeRendererID("OpenGL 3", METAENGINE_Render_RENDERER_OPENGL_3, 3, 0),
            &METAENGINE_Render_CreateRenderer_OpenGL_3, &METAENGINE_Render_FreeRenderer_OpenGL_3);
#endif
}

void gpu_init_renderer_register(void) {
    int i;

    if (_gpu_renderer_register_is_initialized) return;

    for (i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++) {
        _gpu_renderer_register[i].id.name = "Unknown";
        _gpu_renderer_register[i].id.renderer = METAENGINE_Render_RENDERER_UNKNOWN;
        _gpu_renderer_register[i].createFn = NULL;
        _gpu_renderer_register[i].freeFn = NULL;
    }
    for (i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++) { _gpu_renderer_map[i] = NULL; }

    METAENGINE_Render_GetDefaultRendererOrder(&_gpu_renderer_order_size, _gpu_renderer_order);

    _gpu_renderer_register_is_initialized = 1;

    gpu_register_built_in_renderers();
}

void gpu_free_renderer_register(void) {
    int i;

    for (i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++) {
        _gpu_renderer_register[i].id.name = "Unknown";
        _gpu_renderer_register[i].id.renderer = METAENGINE_Render_RENDERER_UNKNOWN;
        _gpu_renderer_register[i].createFn = NULL;
        _gpu_renderer_register[i].freeFn = NULL;
    }
    for (i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++) { _gpu_renderer_map[i] = NULL; }

    _gpu_renderer_register_is_initialized = 0;
    _gpu_renderer_order_size = 0;
}

void METAENGINE_Render_GetRendererOrder(int *order_size, METAENGINE_Render_RendererID *order) {
    if (order_size != NULL) *order_size = _gpu_renderer_order_size;

    if (order != NULL && _gpu_renderer_order_size > 0)
        memcpy(order, _gpu_renderer_order,
               _gpu_renderer_order_size * sizeof(METAENGINE_Render_RendererID));
}

void METAENGINE_Render_SetRendererOrder(int order_size, METAENGINE_Render_RendererID *order) {
    if (order == NULL) {
        // Restore the default order
        int count = 0;
        METAENGINE_Render_RendererID default_order[METAENGINE_Render_RENDERER_ORDER_MAX];
        METAENGINE_Render_GetDefaultRendererOrder(&count, default_order);
        METAENGINE_Render_SetRendererOrder(count,
                                           default_order);// Call us again with the default order
        return;
    }

    if (order_size <= 0) return;

    if (order_size > METAENGINE_Render_RENDERER_ORDER_MAX) {
        METAENGINE_Render_PushErrorCode(
                __func__, METAENGINE_Render_ERROR_USER_ERROR,
                "Given order_size (%d) is greater than METAENGINE_Render_RENDERER_ORDER_MAX (%d)",
                order_size, METAENGINE_Render_RENDERER_ORDER_MAX);
        order_size = METAENGINE_Render_RENDERER_ORDER_MAX;
    }

    memcpy(_gpu_renderer_order, order, order_size * sizeof(METAENGINE_Render_RendererID));
    _gpu_renderer_order_size = order_size;
}

void METAENGINE_Render_GetDefaultRendererOrder(int *order_size,
                                               METAENGINE_Render_RendererID *order) {
    int count = 0;
    METAENGINE_Render_RendererID default_order[METAENGINE_Render_RENDERER_ORDER_MAX];

#ifdef __MACOSX__
    default_order[count++] =
            METAENGINE_Render_MakeRendererID("OpenGL 3", METAENGINE_Render_RENDERER_OPENGL_3, 3, 2);
#else
    default_order[count++] =
            METAENGINE_Render_MakeRendererID("OpenGL 3", METAENGINE_Render_RENDERER_OPENGL_3, 3, 0);
#endif

    if (order_size != NULL) *order_size = count;

    if (order != NULL && count > 0)
        memcpy(order, default_order, count * sizeof(METAENGINE_Render_RendererID));
}

METAENGINE_Render_Renderer *METAENGINE_Render_CreateRenderer(METAENGINE_Render_RendererID id) {
    METAENGINE_Render_Renderer *result = NULL;
    int i;
    for (i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++) {
        if (_gpu_renderer_register[i].id.renderer == METAENGINE_Render_RENDERER_UNKNOWN) continue;

        if (id.renderer == _gpu_renderer_register[i].id.renderer) {
            if (_gpu_renderer_register[i].createFn != NULL) {
                // Use the registered name
                id.name = _gpu_renderer_register[i].id.name;
                result = _gpu_renderer_register[i].createFn(id);
            }
            break;
        }
    }

    if (result == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                        "Renderer was not found in the renderer registry.");
    }
    return result;
}

// Get a renderer from the map.
METAENGINE_Render_Renderer *METAENGINE_Render_GetRenderer(METAENGINE_Render_RendererID id) {
    int i;
    gpu_init_renderer_register();

    // Invalid enum?
    if (id.renderer == METAENGINE_Render_RENDERER_UNKNOWN) return NULL;

    for (i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++) {
        if (_gpu_renderer_map[i] == NULL) continue;

        if (id.renderer == _gpu_renderer_map[i]->id.renderer) return _gpu_renderer_map[i];
    }

    return NULL;
}

// Create a new renderer based on a registered id and store it in the map.
METAENGINE_Render_Renderer *gpu_create_and_add_renderer(METAENGINE_Render_RendererID id) {
    int i;
    for (i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++) {
        if (_gpu_renderer_map[i] == NULL) {
            // Create
            METAENGINE_Render_Renderer *renderer = METAENGINE_Render_CreateRenderer(id);
            if (renderer == NULL) {
                METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_BACKEND_ERROR,
                                                "Failed to create new renderer.");
                return NULL;
            }

            // Add
            _gpu_renderer_map[i] = renderer;
            // Return
            return renderer;
        }
    }

    METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_BACKEND_ERROR,
                                    "Couldn't create a new renderer.  Too many active renderers "
                                    "for METAENGINE_Render_MAX_ACTIVE_RENDERERS (%d).",
                                    METAENGINE_Render_MAX_ACTIVE_RENDERERS);
    return NULL;
}

// Free renderer memory according to how the registry instructs
void gpu_free_renderer_memory(METAENGINE_Render_Renderer *renderer) {
    int i;
    if (renderer == NULL) return;

    for (i = 0; i < METAENGINE_Render_MAX_REGISTERED_RENDERERS; i++) {
        if (_gpu_renderer_register[i].id.renderer == METAENGINE_Render_RENDERER_UNKNOWN) continue;

        if (renderer->id.renderer == _gpu_renderer_register[i].id.renderer) {
            _gpu_renderer_register[i].freeFn(renderer);
            return;
        }
    }
}

// Remove a renderer from the active map and free it.
void METAENGINE_Render_FreeRenderer(METAENGINE_Render_Renderer *renderer) {
    int i;
    METAENGINE_Render_Renderer *current_renderer;

    if (renderer == NULL) return;

    current_renderer = METAENGINE_Render_GetCurrentRenderer();
    if (current_renderer == renderer)
        METAENGINE_Render_SetCurrentRenderer(METAENGINE_Render_MakeRendererID(
                "Unknown", METAENGINE_Render_RENDERER_UNKNOWN, 0, 0));

    for (i = 0; i < METAENGINE_Render_MAX_ACTIVE_RENDERERS; i++) {
        if (renderer == _gpu_renderer_map[i]) {
            gpu_free_renderer_memory(renderer);
            _gpu_renderer_map[i] = NULL;
            return;
        }
    }
}

#define GET_ALPHA(sdl_color) ((sdl_color).a)

#define CHECK_RENDERER (_gpu_current_renderer != NULL)
#define MAKE_CURRENT_IF_NONE(target)                                                               \
    do {                                                                                           \
        if (_gpu_current_renderer->current_context_target == NULL && target != NULL &&             \
            target->context != NULL)                                                               \
            METAENGINE_Render_MakeCurrent(target, target->context->windowID);                      \
    } while (0)
#define CHECK_CONTEXT (_gpu_current_renderer->current_context_target != NULL)
#define RETURN_ERROR(code, details)                                                                \
    do {                                                                                           \
        METAENGINE_Render_PushErrorCode(__func__, code, "%s", details);                            \
        return;                                                                                    \
    } while (0)

int gpu_strcasecmp(const char *s1, const char *s2);

void gpu_init_renderer_register(void);
void gpu_free_renderer_register(void);
METAENGINE_Render_Renderer *gpu_create_and_add_renderer(METAENGINE_Render_RendererID id);

/*! A mapping of windowID to a METAENGINE_Render_Target to facilitate METAENGINE_Render_GetWindowTarget(). */
typedef struct METAENGINE_Render_WindowMapping
{
    Uint32 windowID;
    METAENGINE_Render_Target *target;
} METAENGINE_Render_WindowMapping;

static METAENGINE_Render_Renderer *_gpu_current_renderer = NULL;

#define METAENGINE_Render_DEFAULT_MAX_NUM_ERRORS 20
#define METAENGINE_Render_ERROR_FUNCTION_STRING_MAX 128
#define METAENGINE_Render_ERROR_DETAILS_STRING_MAX 512
static METAENGINE_Render_ErrorObject *_gpu_error_code_queue = NULL;
static unsigned int _gpu_num_error_codes = 0;
static unsigned int _gpu_error_code_queue_size = METAENGINE_Render_DEFAULT_MAX_NUM_ERRORS;
static METAENGINE_Render_ErrorObject _gpu_error_code_result;

#define METAENGINE_Render_INITIAL_WINDOW_MAPPINGS_SIZE 10
static METAENGINE_Render_WindowMapping *_gpu_window_mappings = NULL;
static int _gpu_window_mappings_size = 0;
static int _gpu_num_window_mappings = 0;

static Uint32 _gpu_init_windowID = 0;

static METAENGINE_Render_InitFlagEnum _gpu_preinit_flags = METAENGINE_Render_DEFAULT_INIT_FLAGS;
static METAENGINE_Render_InitFlagEnum _gpu_required_features = 0;

static METAENGINE_Render_bool _gpu_initialized_SDL_core = METAENGINE_Render_FALSE;
static METAENGINE_Render_bool _gpu_initialized_SDL = METAENGINE_Render_FALSE;

SDL_version METAENGINE_Render_GetLinkedVersion(void) {
    return METAENGINE_Render_GetCompiledVersion();
}

void METAENGINE_Render_SetCurrentRenderer(METAENGINE_Render_RendererID id) {
    _gpu_current_renderer = METAENGINE_Render_GetRenderer(id);

    if (_gpu_current_renderer != NULL)
        _gpu_current_renderer->impl->SetAsCurrent(_gpu_current_renderer);
}

void METAENGINE_Render_ResetRendererState(void) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->ResetRendererState(_gpu_current_renderer);
}

void METAENGINE_Render_SetCoordinateMode(METAENGINE_Render_bool use_math_coords) {
    if (_gpu_current_renderer == NULL) return;

    _gpu_current_renderer->coordinate_mode = use_math_coords;
}

METAENGINE_Render_bool METAENGINE_Render_GetCoordinateMode(void) {
    if (_gpu_current_renderer == NULL) return METAENGINE_Render_FALSE;

    return _gpu_current_renderer->coordinate_mode;
}

METAENGINE_Render_Renderer *METAENGINE_Render_GetCurrentRenderer(void) {
    return _gpu_current_renderer;
}

Uint32 METAENGINE_Render_GetCurrentShaderProgram(void) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return 0;

    return _gpu_current_renderer->current_context_target->context->current_shader_program;
}

static METAENGINE_Render_bool gpu_init_SDL(void) {
    if (!_gpu_initialized_SDL) {
        if (!_gpu_initialized_SDL_core && !SDL_WasInit(SDL_INIT_EVERYTHING)) {
            // Nothing has been set up, so init SDL and the video subsystem.
            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                METAENGINE_Render_PushErrorCode("METAENGINE_Render_Init",
                                                METAENGINE_Render_ERROR_BACKEND_ERROR,
                                                "Failed to initialize SDL video subsystem");
                return METAENGINE_Render_FALSE;
            }
            _gpu_initialized_SDL_core = METAENGINE_Render_TRUE;
        }

        // SDL is definitely ready now, but we're going to init the video subsystem to be sure that SDL_gpu keeps it available until METAENGINE_Render_Quit().
        if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
            METAENGINE_Render_PushErrorCode("METAENGINE_Render_Init",
                                            METAENGINE_Render_ERROR_BACKEND_ERROR,
                                            "Failed to initialize SDL video subsystem");
            return METAENGINE_Render_FALSE;
        }
        _gpu_initialized_SDL = METAENGINE_Render_TRUE;
    }
    return METAENGINE_Render_TRUE;
}

void METAENGINE_Render_SetInitWindow(Uint32 windowID) { _gpu_init_windowID = windowID; }

Uint32 METAENGINE_Render_GetInitWindow(void) { return _gpu_init_windowID; }

void METAENGINE_Render_SetPreInitFlags(METAENGINE_Render_InitFlagEnum METAENGINE_Render_flags) {
    _gpu_preinit_flags = METAENGINE_Render_flags;
}

METAENGINE_Render_InitFlagEnum METAENGINE_Render_GetPreInitFlags(void) {
    return _gpu_preinit_flags;
}

void METAENGINE_Render_SetRequiredFeatures(METAENGINE_Render_FeatureEnum features) {
    _gpu_required_features = features;
}

METAENGINE_Render_FeatureEnum METAENGINE_Render_GetRequiredFeatures(void) {
    return _gpu_required_features;
}

static void gpu_init_error_queue(void) {
    if (_gpu_error_code_queue == NULL) {
        unsigned int i;
        _gpu_error_code_queue = (METAENGINE_Render_ErrorObject *) SDL_malloc(
                sizeof(METAENGINE_Render_ErrorObject) * _gpu_error_code_queue_size);

        for (i = 0; i < _gpu_error_code_queue_size; i++) {
            _gpu_error_code_queue[i].function =
                    (char *) SDL_malloc(METAENGINE_Render_ERROR_FUNCTION_STRING_MAX + 1);
            _gpu_error_code_queue[i].error = METAENGINE_Render_ERROR_NONE;
            _gpu_error_code_queue[i].details =
                    (char *) SDL_malloc(METAENGINE_Render_ERROR_DETAILS_STRING_MAX + 1);
        }
        _gpu_num_error_codes = 0;

        _gpu_error_code_result.function =
                (char *) SDL_malloc(METAENGINE_Render_ERROR_FUNCTION_STRING_MAX + 1);
        _gpu_error_code_result.error = METAENGINE_Render_ERROR_NONE;
        _gpu_error_code_result.details =
                (char *) SDL_malloc(METAENGINE_Render_ERROR_DETAILS_STRING_MAX + 1);
    }
}

static void gpu_init_window_mappings(void) {
    if (_gpu_window_mappings == NULL) {
        _gpu_window_mappings_size = METAENGINE_Render_INITIAL_WINDOW_MAPPINGS_SIZE;
        _gpu_window_mappings = (METAENGINE_Render_WindowMapping *) SDL_malloc(
                _gpu_window_mappings_size * sizeof(METAENGINE_Render_WindowMapping));
        _gpu_num_window_mappings = 0;
    }
}

void METAENGINE_Render_AddWindowMapping(METAENGINE_Render_Target *target) {
    Uint32 windowID;
    int i;

    if (_gpu_window_mappings == NULL) gpu_init_window_mappings();

    if (target == NULL || target->context == NULL) return;

    windowID = target->context->windowID;
    if (windowID == 0)// Invalid window ID
        return;

    // Check for duplicates
    for (i = 0; i < _gpu_num_window_mappings; i++) {
        if (_gpu_window_mappings[i].windowID == windowID) {
            if (_gpu_window_mappings[i].target != target)
                METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                                "WindowID %u already has a mapping.", windowID);
            return;
        }
        // Don't check the target because it's okay for a single target to be used with multiple windows
    }

    // Check if list is big enough to hold another
    if (_gpu_num_window_mappings >= _gpu_window_mappings_size) {
        METAENGINE_Render_WindowMapping *new_array;
        _gpu_window_mappings_size *= 2;
        new_array = (METAENGINE_Render_WindowMapping *) SDL_malloc(
                _gpu_window_mappings_size * sizeof(METAENGINE_Render_WindowMapping));
        memcpy(new_array, _gpu_window_mappings,
               _gpu_num_window_mappings * sizeof(METAENGINE_Render_WindowMapping));
        SDL_free(_gpu_window_mappings);
        _gpu_window_mappings = new_array;
    }

    // Add to end of list
    {
        METAENGINE_Render_WindowMapping m;
        m.windowID = windowID;
        m.target = target;
        _gpu_window_mappings[_gpu_num_window_mappings] = m;
    }
    _gpu_num_window_mappings++;
}

void METAENGINE_Render_RemoveWindowMapping(Uint32 windowID) {
    int i;

    if (_gpu_window_mappings == NULL) gpu_init_window_mappings();

    if (windowID == 0)// Invalid window ID
        return;

    // Find the occurrence
    for (i = 0; i < _gpu_num_window_mappings; i++) {
        if (_gpu_window_mappings[i].windowID == windowID) {
            int num_to_move;

            // Unset the target's window
            _gpu_window_mappings[i].target->context->windowID = 0;

            // Move the remaining entries to replace the removed one
            _gpu_num_window_mappings--;
            num_to_move = _gpu_num_window_mappings - i;
            if (num_to_move > 0)
                memmove(&_gpu_window_mappings[i], &_gpu_window_mappings[i + 1],
                        num_to_move * sizeof(METAENGINE_Render_WindowMapping));
            return;
        }
    }
}

void METAENGINE_Render_RemoveWindowMappingByTarget(METAENGINE_Render_Target *target) {
    Uint32 windowID;
    int i;

    if (_gpu_window_mappings == NULL) gpu_init_window_mappings();

    if (target == NULL || target->context == NULL) return;

    windowID = target->context->windowID;
    if (windowID == 0)// Invalid window ID
        return;

    // Unset the target's window
    target->context->windowID = 0;

    // Find the occurrences
    for (i = 0; i < _gpu_num_window_mappings; ++i) {
        if (_gpu_window_mappings[i].target == target) {
            // Move the remaining entries to replace the removed one
            int num_to_move;
            _gpu_num_window_mappings--;
            num_to_move = _gpu_num_window_mappings - i;
            if (num_to_move > 0)
                memmove(&_gpu_window_mappings[i], &_gpu_window_mappings[i + 1],
                        num_to_move * sizeof(METAENGINE_Render_WindowMapping));
            return;
        }
    }
}

METAENGINE_Render_Target *METAENGINE_Render_GetWindowTarget(Uint32 windowID) {
    int i;

    if (_gpu_window_mappings == NULL) gpu_init_window_mappings();

    if (windowID == 0)// Invalid window ID
        return NULL;

    // Find the occurrence
    for (i = 0; i < _gpu_num_window_mappings; ++i) {
        if (_gpu_window_mappings[i].windowID == windowID) return _gpu_window_mappings[i].target;
    }

    return NULL;
}

METAENGINE_Render_Target *METAENGINE_Render_Init(Uint16 w, Uint16 h,
                                                 METAENGINE_Render_WindowFlagEnum SDL_flags) {
    int renderer_order_size;
    int i;
    METAENGINE_Render_RendererID renderer_order[METAENGINE_Render_RENDERER_ORDER_MAX];

    gpu_init_error_queue();

    gpu_init_renderer_register();

    if (!gpu_init_SDL()) return NULL;

    renderer_order_size = 0;
    METAENGINE_Render_GetRendererOrder(&renderer_order_size, renderer_order);

    // Init the renderers in order
    for (i = 0; i < renderer_order_size; i++) {
        METAENGINE_Render_Target *screen =
                METAENGINE_Render_InitRendererByID(renderer_order[i], w, h, SDL_flags);
        if (screen != NULL) return screen;
    }

    METAENGINE_Render_PushErrorCode("METAENGINE_Render_Init", METAENGINE_Render_ERROR_BACKEND_ERROR,
                                    "No renderer out of %d was able to initialize properly",
                                    renderer_order_size);
    return NULL;
}

METAENGINE_Render_Target *METAENGINE_Render_InitRenderer(
        METAENGINE_Render_RendererEnum renderer_enum, Uint16 w, Uint16 h,
        METAENGINE_Render_WindowFlagEnum SDL_flags) {
    // Search registry for this renderer and use that id
    return METAENGINE_Render_InitRendererByID(METAENGINE_Render_GetRendererID(renderer_enum), w, h,
                                              SDL_flags);
}

METAENGINE_Render_Target *METAENGINE_Render_InitRendererByID(
        METAENGINE_Render_RendererID renderer_request, Uint16 w, Uint16 h,
        METAENGINE_Render_WindowFlagEnum SDL_flags) {
    METAENGINE_Render_Renderer *renderer;
    METAENGINE_Render_Target *screen;

    gpu_init_error_queue();
    gpu_init_renderer_register();

    if (!gpu_init_SDL()) return NULL;

    renderer = gpu_create_and_add_renderer(renderer_request);
    if (renderer == NULL) return NULL;

    METAENGINE_Render_SetCurrentRenderer(renderer->id);

    screen = renderer->impl->Init(renderer, renderer_request, w, h, SDL_flags);
    if (screen == NULL) {
        METAENGINE_Render_PushErrorCode(
                "METAENGINE_Render_InitRendererByID", METAENGINE_Render_ERROR_BACKEND_ERROR,
                "Renderer %s failed to initialize properly", renderer->id.name);
        // Init failed, destroy the renderer...
        // Erase the window mappings
        _gpu_num_window_mappings = 0;
        METAENGINE_Render_CloseCurrentRenderer();
    } else
        METAENGINE_Render_SetInitWindow(0);
    return screen;
}

METAENGINE_Render_bool METAENGINE_Render_IsFeatureEnabled(METAENGINE_Render_FeatureEnum feature) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return METAENGINE_Render_FALSE;

    return ((_gpu_current_renderer->enabled_features & feature) == feature);
}

METAENGINE_Render_Target *METAENGINE_Render_CreateTargetFromWindow(Uint32 windowID) {
    if (_gpu_current_renderer == NULL) return NULL;

    return _gpu_current_renderer->impl->CreateTargetFromWindow(_gpu_current_renderer, windowID,
                                                               NULL);
}

METAENGINE_Render_Target *METAENGINE_Render_CreateAliasTarget(METAENGINE_Render_Target *target) {
    if (!CHECK_RENDERER) return NULL;
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) return NULL;

    return _gpu_current_renderer->impl->CreateAliasTarget(_gpu_current_renderer, target);
}

void METAENGINE_Render_MakeCurrent(METAENGINE_Render_Target *target, Uint32 windowID) {
    if (_gpu_current_renderer == NULL) return;

    _gpu_current_renderer->impl->MakeCurrent(_gpu_current_renderer, target, windowID);
}

METAENGINE_Render_bool METAENGINE_Render_SetFullscreen(
        METAENGINE_Render_bool enable_fullscreen, METAENGINE_Render_bool use_desktop_resolution) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return METAENGINE_Render_FALSE;

    return _gpu_current_renderer->impl->SetFullscreen(_gpu_current_renderer, enable_fullscreen,
                                                      use_desktop_resolution);
}

METAENGINE_Render_bool METAENGINE_Render_GetFullscreen(void) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetContextTarget();
    if (target == NULL) return METAENGINE_Render_FALSE;
    return (SDL_GetWindowFlags(SDL_GetWindowFromID(target->context->windowID)) &
            (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;
}

METAENGINE_Render_Target *METAENGINE_Render_GetActiveTarget(void) {
    METAENGINE_Render_Target *context_target = METAENGINE_Render_GetContextTarget();
    if (context_target == NULL) return NULL;

    return context_target->context->active_target;
}

METAENGINE_Render_bool METAENGINE_Render_SetActiveTarget(METAENGINE_Render_Target *target) {
    if (_gpu_current_renderer == NULL) return METAENGINE_Render_FALSE;

    return _gpu_current_renderer->impl->SetActiveTarget(_gpu_current_renderer, target);
}

METAENGINE_Render_bool METAENGINE_Render_AddDepthBuffer(METAENGINE_Render_Target *target) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL ||
        target == NULL)
        return METAENGINE_Render_FALSE;

    return _gpu_current_renderer->impl->AddDepthBuffer(_gpu_current_renderer, target);
}

void METAENGINE_Render_SetDepthTest(METAENGINE_Render_Target *target,
                                    METAENGINE_Render_bool enable) {
    if (target != NULL) target->use_depth_test = enable;
}

void METAENGINE_Render_SetDepthWrite(METAENGINE_Render_Target *target,
                                     METAENGINE_Render_bool enable) {
    if (target != NULL) target->use_depth_write = enable;
}

void METAENGINE_Render_SetDepthFunction(METAENGINE_Render_Target *target,
                                        METAENGINE_Render_ComparisonEnum compare_operation) {
    if (target != NULL) target->depth_function = compare_operation;
}

METAENGINE_Render_bool METAENGINE_Render_SetWindowResolution(Uint16 w, Uint16 h) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL ||
        w == 0 || h == 0)
        return METAENGINE_Render_FALSE;

    return _gpu_current_renderer->impl->SetWindowResolution(_gpu_current_renderer, w, h);
}

void METAENGINE_Render_GetVirtualResolution(METAENGINE_Render_Target *target, Uint16 *w,
                                            Uint16 *h) {
    // No checking here for NULL w or h...  Should we?
    if (target == NULL) {
        *w = 0;
        *h = 0;
    } else {
        *w = target->w;
        *h = target->h;
    }
}

void METAENGINE_Render_SetVirtualResolution(METAENGINE_Render_Target *target, Uint16 w, Uint16 h) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");
    if (w == 0 || h == 0) return;

    _gpu_current_renderer->impl->SetVirtualResolution(_gpu_current_renderer, target, w, h);
}

void METAENGINE_Render_UnsetVirtualResolution(METAENGINE_Render_Target *target) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    _gpu_current_renderer->impl->UnsetVirtualResolution(_gpu_current_renderer, target);
}

void METAENGINE_Render_SetImageVirtualResolution(METAENGINE_Render_Image *image, Uint16 w,
                                                 Uint16 h) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL ||
        w == 0 || h == 0)
        return;

    if (image == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();// TODO: Perhaps move SetImageVirtualResolution into the renderer so we can check to see if this image is bound first.
    image->w = w;
    image->h = h;
    image->using_virtual_resolution = 1;
}

void METAENGINE_Render_UnsetImageVirtualResolution(METAENGINE_Render_Image *image) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    if (image == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();// TODO: Perhaps move SetImageVirtualResolution into the renderer so we can check to see if this image is bound first.
    image->w = image->base_w;
    image->h = image->base_h;
    image->using_virtual_resolution = 0;
}

void gpu_free_error_queue(void) {
    unsigned int i;
    // Free the error queue
    for (i = 0; i < _gpu_error_code_queue_size; i++) {
        SDL_free(_gpu_error_code_queue[i].function);
        _gpu_error_code_queue[i].function = NULL;
        SDL_free(_gpu_error_code_queue[i].details);
        _gpu_error_code_queue[i].details = NULL;
    }
    SDL_free(_gpu_error_code_queue);
    _gpu_error_code_queue = NULL;
    _gpu_num_error_codes = 0;

    SDL_free(_gpu_error_code_result.function);
    _gpu_error_code_result.function = NULL;
    SDL_free(_gpu_error_code_result.details);
    _gpu_error_code_result.details = NULL;
}

// Deletes all existing errors
void METAENGINE_Render_SetErrorQueueMax(unsigned int max) {
    gpu_free_error_queue();

    // Reallocate with new size
    _gpu_error_code_queue_size = max;
    gpu_init_error_queue();
}

void METAENGINE_Render_CloseCurrentRenderer(void) {
    if (_gpu_current_renderer == NULL) return;

    _gpu_current_renderer->impl->Quit(_gpu_current_renderer);
    METAENGINE_Render_FreeRenderer(_gpu_current_renderer);
}

void METAENGINE_Render_Quit(void) {
    if (_gpu_num_error_codes > 0)
        METADOT_ERROR("METAENGINE_Render_Quit: {} uncleared error{}.", _gpu_num_error_codes,
                      (_gpu_num_error_codes > 1 ? "s" : ""));

    gpu_free_error_queue();

    if (_gpu_current_renderer == NULL) return;

    _gpu_current_renderer->impl->Quit(_gpu_current_renderer);
    METAENGINE_Render_FreeRenderer(_gpu_current_renderer);
    // FIXME: Free all renderers
    _gpu_current_renderer = NULL;

    _gpu_init_windowID = 0;

    // Free window mappings
    SDL_free(_gpu_window_mappings);
    _gpu_window_mappings = NULL;
    _gpu_window_mappings_size = 0;
    _gpu_num_window_mappings = 0;

    gpu_free_renderer_register();

    if (_gpu_initialized_SDL) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        _gpu_initialized_SDL = 0;

        if (_gpu_initialized_SDL_core) {
            SDL_Quit();
            _gpu_initialized_SDL_core = 0;
        }
    }
}

void METAENGINE_Render_PushErrorCode(const char *function, METAENGINE_Render_ErrorEnum error,
                                     const char *details, ...) {
    gpu_init_error_queue();

    if (METADOT_DEBUG) {
        // Print the message
        if (details != NULL) {
            char buf[METAENGINE_Render_ERROR_DETAILS_STRING_MAX];
            va_list lst;
            va_start(lst, details);
            vsnprintf(buf, METAENGINE_Render_ERROR_DETAILS_STRING_MAX, details, lst);
            va_end(lst);

            METADOT_ERROR("{0}: {1} - {2}", (function == NULL ? "NULL" : function),
                          METAENGINE_Render_GetErrorString(error), buf);
        } else
            METADOT_ERROR("{0}: {1}", (function == NULL ? "NULL" : function),
                          METAENGINE_Render_GetErrorString(error));
    }

    if (_gpu_num_error_codes < _gpu_error_code_queue_size) {
        if (function == NULL) _gpu_error_code_queue[_gpu_num_error_codes].function[0] = '\0';
        else {
            strncpy(_gpu_error_code_queue[_gpu_num_error_codes].function, function,
                    METAENGINE_Render_ERROR_FUNCTION_STRING_MAX);
            _gpu_error_code_queue[_gpu_num_error_codes]
                    .function[METAENGINE_Render_ERROR_FUNCTION_STRING_MAX] = '\0';
        }
        _gpu_error_code_queue[_gpu_num_error_codes].error = error;
        if (details == NULL) _gpu_error_code_queue[_gpu_num_error_codes].details[0] = '\0';
        else {
            va_list lst;
            va_start(lst, details);
            vsnprintf(_gpu_error_code_queue[_gpu_num_error_codes].details,
                      METAENGINE_Render_ERROR_DETAILS_STRING_MAX, details, lst);
            va_end(lst);
        }
        _gpu_num_error_codes++;
    }
}

METAENGINE_Render_ErrorObject METAENGINE_Render_PopErrorCode(void) {
    unsigned int i;
    METAENGINE_Render_ErrorObject result = {NULL, NULL, METAENGINE_Render_ERROR_NONE};

    gpu_init_error_queue();

    if (_gpu_num_error_codes <= 0) return result;

    // Pop the oldest
    strcpy(_gpu_error_code_result.function, _gpu_error_code_queue[0].function);
    _gpu_error_code_result.error = _gpu_error_code_queue[0].error;
    strcpy(_gpu_error_code_result.details, _gpu_error_code_queue[0].details);

    // We'll be returning that one
    result = _gpu_error_code_result;

    // Move the rest down
    _gpu_num_error_codes--;
    for (i = 0; i < _gpu_num_error_codes; i++) {
        strcpy(_gpu_error_code_queue[i].function, _gpu_error_code_queue[i + 1].function);
        _gpu_error_code_queue[i].error = _gpu_error_code_queue[i + 1].error;
        strcpy(_gpu_error_code_queue[i].details, _gpu_error_code_queue[i + 1].details);
    }
    return result;
}

const char *METAENGINE_Render_GetErrorString(METAENGINE_Render_ErrorEnum error) {
    switch (error) {
        case METAENGINE_Render_ERROR_NONE:
            return "NO ERROR";
        case METAENGINE_Render_ERROR_BACKEND_ERROR:
            return "BACKEND ERROR";
        case METAENGINE_Render_ERROR_DATA_ERROR:
            return "DATA ERROR";
        case METAENGINE_Render_ERROR_USER_ERROR:
            return "USER ERROR";
        case METAENGINE_Render_ERROR_UNSUPPORTED_FUNCTION:
            return "UNSUPPORTED FUNCTION";
        case METAENGINE_Render_ERROR_NULL_ARGUMENT:
            return "NULL ARGUMENT";
        case METAENGINE_Render_ERROR_FILE_NOT_FOUND:
            return "FILE NOT FOUND";
    }
    return "UNKNOWN ERROR";
}

void METAENGINE_Render_GetVirtualCoords(METAENGINE_Render_Target *target, float *x, float *y,
                                        float displayX, float displayY) {
    if (target == NULL || _gpu_current_renderer == NULL) return;

    // Scale from raw window/image coords to the virtual scale
    if (target->context != NULL) {
        if (x != NULL) *x = (displayX * target->w) / target->context->window_w;
        if (y != NULL) *y = (displayY * target->h) / target->context->window_h;
    } else if (target->image != NULL) {
        if (x != NULL) *x = (displayX * target->w) / target->image->w;
        if (y != NULL) *y = (displayY * target->h) / target->image->h;
    } else {
        // What is the backing for this target?!
        if (x != NULL) *x = displayX;
        if (y != NULL) *y = displayY;
    }

    // Invert coordinates to math coords
    if (_gpu_current_renderer->coordinate_mode) *y = target->h - *y;
}

METAENGINE_Render_Rect METAENGINE_Render_MakeRect(float x, float y, float w, float h) {
    METAENGINE_Render_Rect r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;

    return r;
}

SDL_Color METAENGINE_Render_MakeColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    GET_ALPHA(c) = a;

    return c;
}

METAENGINE_Render_RendererID METAENGINE_Render_MakeRendererID(
        const char *name, METAENGINE_Render_RendererEnum renderer, int major_version,
        int minor_version) {
    METAENGINE_Render_RendererID r;
    r.name = name;
    r.renderer = renderer;
    r.major_version = major_version;
    r.minor_version = minor_version;

    return r;
}

void METAENGINE_Render_SetViewport(METAENGINE_Render_Target *target,
                                   METAENGINE_Render_Rect viewport) {
    if (target != NULL) target->viewport = viewport;
}

void METAENGINE_Render_UnsetViewport(METAENGINE_Render_Target *target) {
    if (target != NULL) target->viewport = METAENGINE_Render_MakeRect(0, 0, target->w, target->h);
}

METAENGINE_Render_Camera METAENGINE_Render_GetDefaultCamera(void) {
    METAENGINE_Render_Camera cam = {0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, -100.0f, 100.0f, true};
    return cam;
}

METAENGINE_Render_Camera METAENGINE_Render_GetCamera(METAENGINE_Render_Target *target) {
    if (target == NULL) return METAENGINE_Render_GetDefaultCamera();
    return target->camera;
}

METAENGINE_Render_Camera METAENGINE_Render_SetCamera(METAENGINE_Render_Target *target,
                                                     METAENGINE_Render_Camera *cam) {
    if (_gpu_current_renderer == NULL) return METAENGINE_Render_GetDefaultCamera();
    MAKE_CURRENT_IF_NONE(target);
    if (_gpu_current_renderer->current_context_target == NULL)
        return METAENGINE_Render_GetDefaultCamera();
    // TODO: Remove from renderer and flush here
    return _gpu_current_renderer->impl->SetCamera(_gpu_current_renderer, target, cam);
}

void METAENGINE_Render_EnableCamera(METAENGINE_Render_Target *target,
                                    METAENGINE_Render_bool use_camera) {
    if (target == NULL) return;
    // TODO: Flush here
    target->use_camera = use_camera;
}

METAENGINE_Render_bool METAENGINE_Render_IsCameraEnabled(METAENGINE_Render_Target *target) {
    if (target == NULL) return METAENGINE_Render_FALSE;
    return target->use_camera;
}

METAENGINE_Render_Image *METAENGINE_Render_CreateImage(Uint16 w, Uint16 h,
                                                       METAENGINE_Render_FormatEnum format) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return _gpu_current_renderer->impl->CreateImage(_gpu_current_renderer, w, h, format);
}

METAENGINE_Render_Image *METAENGINE_Render_CreateImageUsingTexture(
        METAENGINE_Render_TextureHandle handle, METAENGINE_Render_bool take_ownership) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return _gpu_current_renderer->impl->CreateImageUsingTexture(_gpu_current_renderer, handle,
                                                                take_ownership);
}

METAENGINE_Render_Image *METAENGINE_Render_LoadImage(const char *filename) {
    return METAENGINE_Render_LoadImage_RW(SDL_RWFromFile(filename, "r"), 1);
}

METAENGINE_Render_Image *METAENGINE_Render_LoadImage_RW(SDL_RWops *rwops,
                                                        METAENGINE_Render_bool free_rwops) {
    METAENGINE_Render_Image *result;
    SDL_Surface *surface;
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return NULL;

    surface = METAENGINE_Render_LoadSurface_RW(rwops, free_rwops);
    if (surface == NULL) {
        METAENGINE_Render_PushErrorCode("METAENGINE_Render_LoadImage_RW",
                                        METAENGINE_Render_ERROR_DATA_ERROR,
                                        "Failed to load image data.");
        return NULL;
    }

    result =
            _gpu_current_renderer->impl->CopyImageFromSurface(_gpu_current_renderer, surface, NULL);
    SDL_FreeSurface(surface);

    return result;
}

METAENGINE_Render_Image *METAENGINE_Render_CreateAliasImage(METAENGINE_Render_Image *image) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return _gpu_current_renderer->impl->CreateAliasImage(_gpu_current_renderer, image);
}

METAENGINE_Render_bool METAENGINE_Render_SaveImage(METAENGINE_Render_Image *image,
                                                   const char *filename,
                                                   METAENGINE_Render_FileFormatEnum format) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return METAENGINE_Render_FALSE;

    return _gpu_current_renderer->impl->SaveImage(_gpu_current_renderer, image, filename, format);
}

METAENGINE_Render_bool METAENGINE_Render_SaveImage_RW(METAENGINE_Render_Image *image,
                                                      SDL_RWops *rwops,
                                                      METAENGINE_Render_bool free_rwops,
                                                      METAENGINE_Render_FileFormatEnum format) {
    METAENGINE_Render_bool result;
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return METAENGINE_Render_FALSE;

    SDL_Surface *surface = METAENGINE_Render_CopySurfaceFromImage(image);
    result = METAENGINE_Render_SaveSurface_RW(surface, rwops, free_rwops, format);
    SDL_FreeSurface(surface);
    return result;
}

METAENGINE_Render_Image *METAENGINE_Render_CopyImage(METAENGINE_Render_Image *image) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return _gpu_current_renderer->impl->CopyImage(_gpu_current_renderer, image);
}

void METAENGINE_Render_UpdateImage(METAENGINE_Render_Image *image,
                                   const METAENGINE_Render_Rect *image_rect, SDL_Surface *surface,
                                   const METAENGINE_Render_Rect *surface_rect) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->UpdateImage(_gpu_current_renderer, image, image_rect, surface,
                                             surface_rect);
}

void METAENGINE_Render_UpdateImageBytes(METAENGINE_Render_Image *image,
                                        const METAENGINE_Render_Rect *image_rect,
                                        const unsigned char *bytes, int bytes_per_row) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->UpdateImageBytes(_gpu_current_renderer, image, image_rect, bytes,
                                                  bytes_per_row);
}

METAENGINE_Render_bool METAENGINE_Render_ReplaceImage(METAENGINE_Render_Image *image,
                                                      SDL_Surface *surface,
                                                      const METAENGINE_Render_Rect *surface_rect) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return METAENGINE_Render_FALSE;

    return _gpu_current_renderer->impl->ReplaceImage(_gpu_current_renderer, image, surface,
                                                     surface_rect);
}

static SDL_Surface *gpu_copy_raw_surface_data(unsigned char *data, int width, int height,
                                              int channels) {
    int i;
    Uint32 Rmask, Gmask, Bmask, Amask = 0;
    SDL_Surface *result;

    if (data == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                        "Got NULL data");
        return NULL;
    }

    switch (channels) {
        case 1:
            Rmask = Gmask = Bmask = 0;// Use default RGB masks for 8-bit
            break;
        case 2:
            Rmask = Gmask = Bmask = 0;// Use default RGB masks for 16-bit
            break;
        case 3:
            // These are reversed from what SDL_image uses...  That is bad. :(  Needs testing.
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            Rmask = 0xff0000;
            Gmask = 0x00ff00;
            Bmask = 0x0000ff;
#else
            Rmask = 0x0000ff;
            Gmask = 0x00ff00;
            Bmask = 0xff0000;
#endif
            break;
        case 4:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
            rmask = 0xff000000;
            gmask = 0x00ff0000;
            bmask = 0x0000ff00;
            amask = 0x000000ff;
#else
            Rmask = 0x000000ff;
            Gmask = 0x0000ff00;
            Bmask = 0x00ff0000;
            Amask = 0xff000000;
#endif
            break;
        default:
            Rmask = Gmask = Bmask = 0;
            METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                            "Invalid number of channels: %d", channels);
            return NULL;
            break;
    }

    result = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, channels * 8, Rmask, Gmask, Bmask,
                                  Amask);
    //result = SDL_CreateRGBSurfaceFrom(data, width, height, channels * 8, width * channels, Rmask, Gmask, Bmask, Amask);
    if (result == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                        "Failed to create new %dx%d surface", width, height);
        return NULL;
    }

    // Copy row-by-row in case the pitch doesn't match
    for (i = 0; i < height; ++i) {
        memcpy((Uint8 *) result->pixels + i * result->pitch, data + channels * width * i,
               channels * width);
    }

    if (result != NULL && result->format->palette != NULL) {
        // SDL_CreateRGBSurface has no idea what palette to use, so it uses a blank one.
        // We'll at least create a grayscale one, but it's not ideal...
        // Better would be to get the palette from stbi, but stbi doesn't do that!
        SDL_Color colors[256];

        for (i = 0; i < 256; i++) { colors[i].r = colors[i].g = colors[i].b = (Uint8) i; }

        /* Set palette */
        SDL_SetPaletteColors(result->format->palette, colors, 0, 256);
    }

    return result;
}

SDL_Surface *METAENGINE_Render_LoadSurface_RW(SDL_RWops *rwops, METAENGINE_Render_bool free_rwops) {
    int width, height, channels;
    unsigned char *data;
    SDL_Surface *result;

    int data_bytes;
    unsigned char *c_data;

    if (rwops == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_NULL_ARGUMENT, "rwops");
        return NULL;
    }

    // Get count of bytes
    SDL_RWseek(rwops, 0, SEEK_SET);
    data_bytes = (int) SDL_RWseek(rwops, 0, SEEK_END);
    SDL_RWseek(rwops, 0, SEEK_SET);

    // Read in the rwops data
    c_data = (unsigned char *) SDL_malloc(data_bytes);
    SDL_RWread(rwops, c_data, 1, data_bytes);

    // Load image
    data = stbi_load_from_memory(c_data, data_bytes, &width, &height, &channels, 0);

    // Clean up temp data
    SDL_free(c_data);
    if (free_rwops) SDL_RWclose(rwops);

    if (data == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                        "Failed to load from rwops: %s", stbi_failure_reason());
        return NULL;
    }

    // Copy into a surface
    result = gpu_copy_raw_surface_data(data, width, height, channels);

    stbi_image_free(data);

    return result;
}

SDL_Surface *METAENGINE_Render_LoadSurface(const char *filename) {
    return METAENGINE_Render_LoadSurface_RW(SDL_RWFromFile(filename, "r"), 1);
}

// From http://stackoverflow.com/questions/5309471/getting-file-extension-in-c
static const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

METAENGINE_Render_bool METAENGINE_Render_SaveSurface(SDL_Surface *surface, const char *filename,
                                                     METAENGINE_Render_FileFormatEnum format) {
    METAENGINE_Render_bool result;
    unsigned char *data;

    if (surface == NULL || filename == NULL || surface->w < 1 || surface->h < 1) {
        return METAENGINE_Render_FALSE;
    }

    data = (unsigned char *) surface->pixels;

    if (format == METAENGINE_Render_FILE_AUTO) {
        const char *extension = get_filename_ext(filename);
        if (gpu_strcasecmp(extension, "png") == 0) format = METAENGINE_Render_FILE_PNG;
        else if (gpu_strcasecmp(extension, "bmp") == 0)
            format = METAENGINE_Render_FILE_BMP;
        else if (gpu_strcasecmp(extension, "tga") == 0)
            format = METAENGINE_Render_FILE_TGA;
        else {
            METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                            "Could not detect output file format from file name");
            return METAENGINE_Render_FALSE;
        }
    }

    switch (format) {
        case METAENGINE_Render_FILE_PNG:
            result = (stbi_write_png(filename, surface->w, surface->h,
                                     surface->format->BytesPerPixel,
                                     (const unsigned char *const) data, 0) > 0);
            break;
        case METAENGINE_Render_FILE_BMP:
            result = (stbi_write_bmp(filename, surface->w, surface->h,
                                     surface->format->BytesPerPixel, (void *) data) > 0);
            break;
        case METAENGINE_Render_FILE_TGA:
            result = (stbi_write_tga(filename, surface->w, surface->h,
                                     surface->format->BytesPerPixel, (void *) data) > 0);
            break;
        default:
            METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                            "Unsupported output file format");
            result = METAENGINE_Render_FALSE;
            break;
    }

    return result;
}

static void write_func(void *context, void *data, int size) {
    SDL_RWwrite((SDL_RWops *) context, data, 1, size);
}

METAENGINE_Render_bool METAENGINE_Render_SaveSurface_RW(SDL_Surface *surface, SDL_RWops *rwops,
                                                        METAENGINE_Render_bool free_rwops,
                                                        METAENGINE_Render_FileFormatEnum format) {
    METAENGINE_Render_bool result;
    unsigned char *data;

    if (surface == NULL || rwops == NULL || surface->w < 1 || surface->h < 1) {
        return METAENGINE_Render_FALSE;
    }

    data = (unsigned char *) surface->pixels;

    if (format == METAENGINE_Render_FILE_AUTO) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                        "Invalid output file format (METAENGINE_Render_FILE_AUTO)");
        return METAENGINE_Render_FALSE;
    }

    // FIXME: The limitations here are not communicated clearly.  BMP and TGA won't support arbitrary row length/pitch.
    switch (format) {
        case METAENGINE_Render_FILE_PNG:
            result =
                    (stbi_write_png_to_func(write_func, rwops, surface->w, surface->h,
                                            surface->format->BytesPerPixel,
                                            (const unsigned char *const) data, surface->pitch) > 0);
            break;
        case METAENGINE_Render_FILE_BMP:
            result = (stbi_write_bmp_to_func(write_func, rwops, surface->w, surface->h,
                                             surface->format->BytesPerPixel,
                                             (const unsigned char *const) data) > 0);
            break;
        case METAENGINE_Render_FILE_TGA:
            result = (stbi_write_tga_to_func(write_func, rwops, surface->w, surface->h,
                                             surface->format->BytesPerPixel,
                                             (const unsigned char *const) data) > 0);
            break;
        default:
            METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_DATA_ERROR,
                                            "Unsupported output file format");
            result = METAENGINE_Render_FALSE;
            break;
    }

    if (result && free_rwops) SDL_RWclose(rwops);
    return result;
}

METAENGINE_Render_Image *METAENGINE_Render_CopyImageFromSurface(SDL_Surface *surface) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return _gpu_current_renderer->impl->CopyImageFromSurface(_gpu_current_renderer, surface, NULL);
}

METAENGINE_Render_Image *METAENGINE_Render_CopyImageFromSurfaceRect(
        SDL_Surface *surface, METAENGINE_Render_Rect *surface_rect) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return _gpu_current_renderer->impl->CopyImageFromSurface(_gpu_current_renderer, surface,
                                                             surface_rect);
}

METAENGINE_Render_Image *METAENGINE_Render_CopyImageFromTarget(METAENGINE_Render_Target *target) {
    if (_gpu_current_renderer == NULL) return NULL;
    MAKE_CURRENT_IF_NONE(target);
    if (_gpu_current_renderer->current_context_target == NULL) return NULL;

    return _gpu_current_renderer->impl->CopyImageFromTarget(_gpu_current_renderer, target);
}

SDL_Surface *METAENGINE_Render_CopySurfaceFromTarget(METAENGINE_Render_Target *target) {
    if (_gpu_current_renderer == NULL) return NULL;
    MAKE_CURRENT_IF_NONE(target);
    if (_gpu_current_renderer->current_context_target == NULL) return NULL;

    return _gpu_current_renderer->impl->CopySurfaceFromTarget(_gpu_current_renderer, target);
}

SDL_Surface *METAENGINE_Render_CopySurfaceFromImage(METAENGINE_Render_Image *image) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return _gpu_current_renderer->impl->CopySurfaceFromImage(_gpu_current_renderer, image);
}

void METAENGINE_Render_FreeImage(METAENGINE_Render_Image *image) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->FreeImage(_gpu_current_renderer, image);
}

METAENGINE_Render_Target *METAENGINE_Render_GetContextTarget(void) {
    if (_gpu_current_renderer == NULL) return NULL;

    return _gpu_current_renderer->current_context_target;
}

METAENGINE_Render_Target *METAENGINE_Render_LoadTarget(METAENGINE_Render_Image *image) {
    METAENGINE_Render_Target *result = METAENGINE_Render_GetTarget(image);

    if (result != NULL) result->refcount++;

    return result;
}

METAENGINE_Render_Target *METAENGINE_Render_GetTarget(METAENGINE_Render_Image *image) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return _gpu_current_renderer->impl->GetTarget(_gpu_current_renderer, image);
}

void METAENGINE_Render_FreeTarget(METAENGINE_Render_Target *target) {
    if (_gpu_current_renderer == NULL) return;

    _gpu_current_renderer->impl->FreeTarget(_gpu_current_renderer, target);
}

void METAENGINE_Render_Blit(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                            METAENGINE_Render_Target *target, float x, float y) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    if (image == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "image");
    if (target == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "target");

    _gpu_current_renderer->impl->Blit(_gpu_current_renderer, image, src_rect, target, x, y);
}

void METAENGINE_Render_BlitRotate(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                                  METAENGINE_Render_Target *target, float x, float y,
                                  float degrees) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    if (image == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "image");
    if (target == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "target");

    _gpu_current_renderer->impl->BlitRotate(_gpu_current_renderer, image, src_rect, target, x, y,
                                            degrees);
}

void METAENGINE_Render_BlitScale(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                                 METAENGINE_Render_Target *target, float x, float y, float scaleX,
                                 float scaleY) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    if (image == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "image");
    if (target == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "target");

    _gpu_current_renderer->impl->BlitScale(_gpu_current_renderer, image, src_rect, target, x, y,
                                           scaleX, scaleY);
}

void METAENGINE_Render_BlitTransform(METAENGINE_Render_Image *image,
                                     METAENGINE_Render_Rect *src_rect,
                                     METAENGINE_Render_Target *target, float x, float y,
                                     float degrees, float scaleX, float scaleY) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    if (image == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "image");
    if (target == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "target");

    _gpu_current_renderer->impl->BlitTransform(_gpu_current_renderer, image, src_rect, target, x, y,
                                               degrees, scaleX, scaleY);
}

void METAENGINE_Render_BlitTransformX(METAENGINE_Render_Image *image,
                                      METAENGINE_Render_Rect *src_rect,
                                      METAENGINE_Render_Target *target, float x, float y,
                                      float pivot_x, float pivot_y, float degrees, float scaleX,
                                      float scaleY) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    if (image == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "image");
    if (target == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "target");

    _gpu_current_renderer->impl->BlitTransformX(_gpu_current_renderer, image, src_rect, target, x,
                                                y, pivot_x, pivot_y, degrees, scaleX, scaleY);
}

void METAENGINE_Render_BlitRect(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                                METAENGINE_Render_Target *target,
                                METAENGINE_Render_Rect *dest_rect) {
    float w = 0.0f;
    float h = 0.0f;

    if (image == NULL) return;

    if (src_rect == NULL) {
        w = image->w;
        h = image->h;
    } else {
        w = src_rect->w;
        h = src_rect->h;
    }

    METAENGINE_Render_BlitRectX(image, src_rect, target, dest_rect, 0.0f, w * 0.5f, h * 0.5f,
                                METAENGINE_Render_FLIP_NONE);
}

void METAENGINE_Render_BlitRectX(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                                 METAENGINE_Render_Target *target,
                                 METAENGINE_Render_Rect *dest_rect, float degrees, float pivot_x,
                                 float pivot_y, METAENGINE_Render_FlipEnum flip_direction) {
    float w, h;
    float dx, dy;
    float dw, dh;
    float scale_x, scale_y;

    if (image == NULL || target == NULL) return;

    if (src_rect == NULL) {
        w = image->w;
        h = image->h;
    } else {
        w = src_rect->w;
        h = src_rect->h;
    }

    if (dest_rect == NULL) {
        dx = 0.0f;
        dy = 0.0f;
        dw = target->w;
        dh = target->h;
    } else {
        dx = dest_rect->x;
        dy = dest_rect->y;
        dw = dest_rect->w;
        dh = dest_rect->h;
    }

    scale_x = dw / w;
    scale_y = dh / h;

    if (flip_direction & METAENGINE_Render_FLIP_HORIZONTAL) {
        scale_x = -scale_x;
        dx += dw;
        pivot_x = w - pivot_x;
    }
    if (flip_direction & METAENGINE_Render_FLIP_VERTICAL) {
        scale_y = -scale_y;
        dy += dh;
        pivot_y = h - pivot_y;
    }

    METAENGINE_Render_BlitTransformX(image, src_rect, target, dx + pivot_x * scale_x,
                                     dy + pivot_y * scale_y, pivot_x, pivot_y, degrees, scale_x,
                                     scale_y);
}

void METAENGINE_Render_TriangleBatch(METAENGINE_Render_Image *image,
                                     METAENGINE_Render_Target *target, unsigned short num_vertices,
                                     float *values, unsigned int num_indices,
                                     unsigned short *indices,
                                     METAENGINE_Render_BatchFlagEnum flags) {
    METAENGINE_Render_PrimitiveBatchV(image, target, METAENGINE_Render_TRIANGLES, num_vertices,
                                      (void *) values, num_indices, indices, flags);
}

void METAENGINE_Render_TriangleBatchX(METAENGINE_Render_Image *image,
                                      METAENGINE_Render_Target *target, unsigned short num_vertices,
                                      void *values, unsigned int num_indices,
                                      unsigned short *indices,
                                      METAENGINE_Render_BatchFlagEnum flags) {
    METAENGINE_Render_PrimitiveBatchV(image, target, METAENGINE_Render_TRIANGLES, num_vertices,
                                      values, num_indices, indices, flags);
}

void METAENGINE_Render_PrimitiveBatch(METAENGINE_Render_Image *image,
                                      METAENGINE_Render_Target *target,
                                      METAENGINE_Render_PrimitiveEnum primitive_type,
                                      unsigned short num_vertices, float *values,
                                      unsigned int num_indices, unsigned short *indices,
                                      METAENGINE_Render_BatchFlagEnum flags) {
    METAENGINE_Render_PrimitiveBatchV(image, target, primitive_type, num_vertices, (void *) values,
                                      num_indices, indices, flags);
}

void METAENGINE_Render_PrimitiveBatchV(METAENGINE_Render_Image *image,
                                       METAENGINE_Render_Target *target,
                                       METAENGINE_Render_PrimitiveEnum primitive_type,
                                       unsigned short num_vertices, void *values,
                                       unsigned int num_indices, unsigned short *indices,
                                       METAENGINE_Render_BatchFlagEnum flags) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    if (target == NULL) RETURN_ERROR(METAENGINE_Render_ERROR_NULL_ARGUMENT, "target");

    if (num_vertices == 0) return;

    _gpu_current_renderer->impl->PrimitiveBatchV(_gpu_current_renderer, image, target,
                                                 primitive_type, num_vertices, values, num_indices,
                                                 indices, flags);
}

void METAENGINE_Render_GenerateMipmaps(METAENGINE_Render_Image *image) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->GenerateMipmaps(_gpu_current_renderer, image);
}

METAENGINE_Render_Rect METAENGINE_Render_SetClipRect(METAENGINE_Render_Target *target,
                                                     METAENGINE_Render_Rect rect) {
    if (target == NULL || _gpu_current_renderer == NULL ||
        _gpu_current_renderer->current_context_target == NULL) {
        METAENGINE_Render_Rect r = {0, 0, 0, 0};
        return r;
    }

    return _gpu_current_renderer->impl->SetClip(_gpu_current_renderer, target, (Sint16) rect.x,
                                                (Sint16) rect.y, (Uint16) rect.w, (Uint16) rect.h);
}

METAENGINE_Render_Rect METAENGINE_Render_SetClip(METAENGINE_Render_Target *target, Sint16 x,
                                                 Sint16 y, Uint16 w, Uint16 h) {
    if (target == NULL || _gpu_current_renderer == NULL ||
        _gpu_current_renderer->current_context_target == NULL) {
        METAENGINE_Render_Rect r = {0, 0, 0, 0};
        return r;
    }

    return _gpu_current_renderer->impl->SetClip(_gpu_current_renderer, target, x, y, w, h);
}

void METAENGINE_Render_UnsetClip(METAENGINE_Render_Target *target) {
    if (target == NULL || _gpu_current_renderer == NULL ||
        _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->UnsetClip(_gpu_current_renderer, target);
}

/* Adapted from SDL_IntersectRect() */
METAENGINE_Render_bool METAENGINE_Render_IntersectRect(METAENGINE_Render_Rect A,
                                                       METAENGINE_Render_Rect B,
                                                       METAENGINE_Render_Rect *result) {
    METAENGINE_Render_bool has_horiz_intersection = METAENGINE_Render_FALSE;
    float Amin, Amax, Bmin, Bmax;
    METAENGINE_Render_Rect intersection;

    // Special case for empty rects
    if (A.w <= 0.0f || A.h <= 0.0f || B.w <= 0.0f || B.h <= 0.0f) return METAENGINE_Render_FALSE;

    // Horizontal intersection
    Amin = A.x;
    Amax = Amin + A.w;
    Bmin = B.x;
    Bmax = Bmin + B.w;
    if (Bmin > Amin) Amin = Bmin;
    if (Bmax < Amax) Amax = Bmax;

    intersection.x = Amin;
    intersection.w = Amax - Amin;

    has_horiz_intersection = (Amax > Amin);

    // Vertical intersection
    Amin = A.y;
    Amax = Amin + A.h;
    Bmin = B.y;
    Bmax = Bmin + B.h;
    if (Bmin > Amin) Amin = Bmin;
    if (Bmax < Amax) Amax = Bmax;

    intersection.y = Amin;
    intersection.h = Amax - Amin;

    if (has_horiz_intersection && Amax > Amin) {
        if (result != NULL) *result = intersection;
        return METAENGINE_Render_TRUE;
    } else
        return METAENGINE_Render_FALSE;
}

METAENGINE_Render_bool METAENGINE_Render_IntersectClipRect(METAENGINE_Render_Target *target,
                                                           METAENGINE_Render_Rect B,
                                                           METAENGINE_Render_Rect *result) {
    if (target == NULL) return METAENGINE_Render_FALSE;

    if (!target->use_clip_rect) {
        METAENGINE_Render_Rect A = {0, 0, static_cast<float>(target->w),
                                    static_cast<float>(target->h)};
        return METAENGINE_Render_IntersectRect(A, B, result);
    }

    return METAENGINE_Render_IntersectRect(target->clip_rect, B, result);
}

void METAENGINE_Render_SetColor(METAENGINE_Render_Image *image, SDL_Color color) {
    if (image == NULL) return;

    image->color = color;
}

void METAENGINE_Render_SetRGB(METAENGINE_Render_Image *image, Uint8 r, Uint8 g, Uint8 b) {
    SDL_Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    GET_ALPHA(c) = 255;

    if (image == NULL) return;

    image->color = c;
}

void METAENGINE_Render_SetRGBA(METAENGINE_Render_Image *image, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    GET_ALPHA(c) = a;

    if (image == NULL) return;

    image->color = c;
}

void METAENGINE_Render_UnsetColor(METAENGINE_Render_Image *image) {
    SDL_Color c = {255, 255, 255, 255};
    if (image == NULL) return;

    image->color = c;
}

void METAENGINE_Render_SetTargetColor(METAENGINE_Render_Target *target, SDL_Color color) {
    if (target == NULL) return;

    target->use_color = 1;
    target->color = color;
}

void METAENGINE_Render_SetTargetRGB(METAENGINE_Render_Target *target, Uint8 r, Uint8 g, Uint8 b) {
    SDL_Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    GET_ALPHA(c) = 255;

    if (target == NULL) return;

    target->use_color = !(r == 255 && g == 255 && b == 255);
    target->color = c;
}

void METAENGINE_Render_SetTargetRGBA(METAENGINE_Render_Target *target, Uint8 r, Uint8 g, Uint8 b,
                                     Uint8 a) {
    SDL_Color c;
    c.r = r;
    c.g = g;
    c.b = b;
    GET_ALPHA(c) = a;

    if (target == NULL) return;

    target->use_color = !(r == 255 && g == 255 && b == 255 && a == 255);
    target->color = c;
}

void METAENGINE_Render_UnsetTargetColor(METAENGINE_Render_Target *target) {
    SDL_Color c = {255, 255, 255, 255};
    if (target == NULL) return;

    target->use_color = METAENGINE_Render_FALSE;
    target->color = c;
}

METAENGINE_Render_bool METAENGINE_Render_GetBlending(METAENGINE_Render_Image *image) {
    if (image == NULL) return METAENGINE_Render_FALSE;

    return image->use_blending;
}

void METAENGINE_Render_SetBlending(METAENGINE_Render_Image *image, METAENGINE_Render_bool enable) {
    if (image == NULL) return;

    image->use_blending = enable;
}

void METAENGINE_Render_SetShapeBlending(METAENGINE_Render_bool enable) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->current_context_target->context->shapes_use_blending = enable;
}

METAENGINE_Render_BlendMode METAENGINE_Render_GetBlendModeFromPreset(
        METAENGINE_Render_BlendPresetEnum preset) {
    switch (preset) {
        case METAENGINE_Render_BLEND_NORMAL: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_EQ_ADD,         METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_PREMULTIPLIED_ALPHA: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_ONE, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_FUNC_ONE, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_EQ_ADD,   METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_MULTIPLY: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_DST_COLOR, METAENGINE_Render_FUNC_ZERO,
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_EQ_ADD,         METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_ADD: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE,
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE,
                    METAENGINE_Render_EQ_ADD,         METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_SUBTRACT:
            // FIXME: Use src alpha for source components?
            {
                METAENGINE_Render_BlendMode b = {
                        METAENGINE_Render_FUNC_ONE,    METAENGINE_Render_FUNC_ONE,
                        METAENGINE_Render_FUNC_ONE,    METAENGINE_Render_FUNC_ONE,
                        METAENGINE_Render_EQ_SUBTRACT, METAENGINE_Render_EQ_SUBTRACT};
                return b;
            }
            break;
        case METAENGINE_Render_BLEND_MOD_ALPHA:
            // Don't disturb the colors, but multiply the dest alpha by the src alpha
            {
                METAENGINE_Render_BlendMode b = {
                        METAENGINE_Render_FUNC_ZERO, METAENGINE_Render_FUNC_ONE,
                        METAENGINE_Render_FUNC_ZERO, METAENGINE_Render_FUNC_SRC_ALPHA,
                        METAENGINE_Render_EQ_ADD,    METAENGINE_Render_EQ_ADD};
                return b;
            }
            break;
        case METAENGINE_Render_BLEND_SET_ALPHA:
            // Don't disturb the colors, but set the alpha to the src alpha
            {
                METAENGINE_Render_BlendMode b = {
                        METAENGINE_Render_FUNC_ZERO, METAENGINE_Render_FUNC_ONE,
                        METAENGINE_Render_FUNC_ONE,  METAENGINE_Render_FUNC_ZERO,
                        METAENGINE_Render_EQ_ADD,    METAENGINE_Render_EQ_ADD};
                return b;
            }
            break;
        case METAENGINE_Render_BLEND_SET: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_ONE, METAENGINE_Render_FUNC_ZERO,
                    METAENGINE_Render_FUNC_ONE, METAENGINE_Render_FUNC_ZERO,
                    METAENGINE_Render_EQ_ADD,   METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_NORMAL_KEEP_ALPHA: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_FUNC_ZERO,      METAENGINE_Render_FUNC_ONE,
                    METAENGINE_Render_EQ_ADD,         METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_NORMAL_ADD_ALPHA: {
            METAENGINE_Render_BlendMode b = {
                    METAENGINE_Render_FUNC_SRC_ALPHA, METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                    METAENGINE_Render_FUNC_ONE,       METAENGINE_Render_FUNC_ONE,
                    METAENGINE_Render_EQ_ADD,         METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        case METAENGINE_Render_BLEND_NORMAL_FACTOR_ALPHA: {
            METAENGINE_Render_BlendMode b = {METAENGINE_Render_FUNC_SRC_ALPHA,
                                             METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                                             METAENGINE_Render_FUNC_ONE_MINUS_DST_ALPHA,
                                             METAENGINE_Render_FUNC_ONE,
                                             METAENGINE_Render_EQ_ADD,
                                             METAENGINE_Render_EQ_ADD};
            return b;
        } break;
        default:
            METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR,
                                            "Blend preset not supported: %d", preset);
            {
                METAENGINE_Render_BlendMode b = {METAENGINE_Render_FUNC_SRC_ALPHA,
                                                 METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                                                 METAENGINE_Render_FUNC_SRC_ALPHA,
                                                 METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA,
                                                 METAENGINE_Render_EQ_ADD,
                                                 METAENGINE_Render_EQ_ADD};
                return b;
            }
            break;
    }
}

void METAENGINE_Render_SetBlendFunction(METAENGINE_Render_Image *image,
                                        METAENGINE_Render_BlendFuncEnum source_color,
                                        METAENGINE_Render_BlendFuncEnum dest_color,
                                        METAENGINE_Render_BlendFuncEnum source_alpha,
                                        METAENGINE_Render_BlendFuncEnum dest_alpha) {
    if (image == NULL) return;

    image->blend_mode.source_color = source_color;
    image->blend_mode.dest_color = dest_color;
    image->blend_mode.source_alpha = source_alpha;
    image->blend_mode.dest_alpha = dest_alpha;
}

void METAENGINE_Render_SetBlendEquation(METAENGINE_Render_Image *image,
                                        METAENGINE_Render_BlendEqEnum color_equation,
                                        METAENGINE_Render_BlendEqEnum alpha_equation) {
    if (image == NULL) return;

    image->blend_mode.color_equation = color_equation;
    image->blend_mode.alpha_equation = alpha_equation;
}

void METAENGINE_Render_SetBlendMode(METAENGINE_Render_Image *image,
                                    METAENGINE_Render_BlendPresetEnum preset) {
    METAENGINE_Render_BlendMode b;
    if (image == NULL) return;

    b = METAENGINE_Render_GetBlendModeFromPreset(preset);
    METAENGINE_Render_SetBlendFunction(image, b.source_color, b.dest_color, b.source_alpha,
                                       b.dest_alpha);
    METAENGINE_Render_SetBlendEquation(image, b.color_equation, b.alpha_equation);
}

void METAENGINE_Render_SetShapeBlendFunction(METAENGINE_Render_BlendFuncEnum source_color,
                                             METAENGINE_Render_BlendFuncEnum dest_color,
                                             METAENGINE_Render_BlendFuncEnum source_alpha,
                                             METAENGINE_Render_BlendFuncEnum dest_alpha) {
    METAENGINE_Render_Context *context;
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    context = _gpu_current_renderer->current_context_target->context;

    context->shapes_blend_mode.source_color = source_color;
    context->shapes_blend_mode.dest_color = dest_color;
    context->shapes_blend_mode.source_alpha = source_alpha;
    context->shapes_blend_mode.dest_alpha = dest_alpha;
}

void METAENGINE_Render_SetShapeBlendEquation(METAENGINE_Render_BlendEqEnum color_equation,
                                             METAENGINE_Render_BlendEqEnum alpha_equation) {
    METAENGINE_Render_Context *context;
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    context = _gpu_current_renderer->current_context_target->context;

    context->shapes_blend_mode.color_equation = color_equation;
    context->shapes_blend_mode.alpha_equation = alpha_equation;
}

void METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BlendPresetEnum preset) {
    METAENGINE_Render_BlendMode b;
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    b = METAENGINE_Render_GetBlendModeFromPreset(preset);
    METAENGINE_Render_SetShapeBlendFunction(b.source_color, b.dest_color, b.source_alpha,
                                            b.dest_alpha);
    METAENGINE_Render_SetShapeBlendEquation(b.color_equation, b.alpha_equation);
}

void METAENGINE_Render_SetImageFilter(METAENGINE_Render_Image *image,
                                      METAENGINE_Render_FilterEnum filter) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;
    if (image == NULL) return;

    _gpu_current_renderer->impl->SetImageFilter(_gpu_current_renderer, image, filter);
}

void METAENGINE_Render_SetDefaultAnchor(float anchor_x, float anchor_y) {
    if (_gpu_current_renderer == NULL) return;

    _gpu_current_renderer->default_image_anchor_x = anchor_x;
    _gpu_current_renderer->default_image_anchor_y = anchor_y;
}

void METAENGINE_Render_GetDefaultAnchor(float *anchor_x, float *anchor_y) {
    if (_gpu_current_renderer == NULL) return;

    if (anchor_x != NULL) *anchor_x = _gpu_current_renderer->default_image_anchor_x;

    if (anchor_y != NULL) *anchor_y = _gpu_current_renderer->default_image_anchor_y;
}

void METAENGINE_Render_SetAnchor(METAENGINE_Render_Image *image, float anchor_x, float anchor_y) {
    if (image == NULL) return;

    image->anchor_x = anchor_x;
    image->anchor_y = anchor_y;
}

void METAENGINE_Render_GetAnchor(METAENGINE_Render_Image *image, float *anchor_x, float *anchor_y) {
    if (image == NULL) return;

    if (anchor_x != NULL) *anchor_x = image->anchor_x;

    if (anchor_y != NULL) *anchor_y = image->anchor_y;
}

METAENGINE_Render_SnapEnum METAENGINE_Render_GetSnapMode(METAENGINE_Render_Image *image) {
    if (image == NULL) return (METAENGINE_Render_SnapEnum) 0;

    return image->snap_mode;
}

void METAENGINE_Render_SetSnapMode(METAENGINE_Render_Image *image,
                                   METAENGINE_Render_SnapEnum mode) {
    if (image == NULL) return;

    image->snap_mode = mode;
}

void METAENGINE_Render_SetWrapMode(METAENGINE_Render_Image *image,
                                   METAENGINE_Render_WrapEnum wrap_mode_x,
                                   METAENGINE_Render_WrapEnum wrap_mode_y) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;
    if (image == NULL) return;

    _gpu_current_renderer->impl->SetWrapMode(_gpu_current_renderer, image, wrap_mode_x,
                                             wrap_mode_y);
}

METAENGINE_Render_TextureHandle METAENGINE_Render_GetTextureHandle(METAENGINE_Render_Image *image) {
    if (image == NULL || image->renderer == NULL) return 0;
    return image->renderer->impl->GetTextureHandle(image->renderer, image);
}

SDL_Color METAENGINE_Render_GetPixel(METAENGINE_Render_Target *target, Sint16 x, Sint16 y) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL) {
        SDL_Color c = {0, 0, 0, 0};
        return c;
    }

    return _gpu_current_renderer->impl->GetPixel(_gpu_current_renderer, target, x, y);
}

void METAENGINE_Render_Clear(METAENGINE_Render_Target *target) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    _gpu_current_renderer->impl->ClearRGBA(_gpu_current_renderer, target, 0, 0, 0, 0);
}

void METAENGINE_Render_ClearColor(METAENGINE_Render_Target *target, SDL_Color color) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    _gpu_current_renderer->impl->ClearRGBA(_gpu_current_renderer, target, color.r, color.g, color.b,
                                           GET_ALPHA(color));
}

void METAENGINE_Render_ClearRGB(METAENGINE_Render_Target *target, Uint8 r, Uint8 g, Uint8 b) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    _gpu_current_renderer->impl->ClearRGBA(_gpu_current_renderer, target, r, g, b, 255);
}

void METAENGINE_Render_ClearRGBA(METAENGINE_Render_Target *target, Uint8 r, Uint8 g, Uint8 b,
                                 Uint8 a) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");
    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    _gpu_current_renderer->impl->ClearRGBA(_gpu_current_renderer, target, r, g, b, a);
}

void METAENGINE_Render_FlushBlitBuffer(void) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->FlushBlitBuffer(_gpu_current_renderer);
}

void METAENGINE_Render_Flip(METAENGINE_Render_Target *target) {
    if (!CHECK_RENDERER) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL renderer");

    if (target != NULL && target->context == NULL) {
        _gpu_current_renderer->impl->FlushBlitBuffer(_gpu_current_renderer);
        return;
    }

    MAKE_CURRENT_IF_NONE(target);
    if (!CHECK_CONTEXT) RETURN_ERROR(METAENGINE_Render_ERROR_USER_ERROR, "NULL context");

    _gpu_current_renderer->impl->Flip(_gpu_current_renderer, target);
}

// Shader API

Uint32 METAENGINE_Render_CompileShader_RW(METAENGINE_Render_ShaderEnum shader_type,
                                          SDL_RWops *shader_source,
                                          METAENGINE_Render_bool free_rwops) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL) {
        if (free_rwops) SDL_RWclose(shader_source);
        return METAENGINE_Render_FALSE;
    }

    return _gpu_current_renderer->impl->CompileShader_RW(_gpu_current_renderer, shader_type,
                                                         shader_source, free_rwops);
}

Uint32 METAENGINE_Render_LoadShader(METAENGINE_Render_ShaderEnum shader_type,
                                    const char *filename) {
    SDL_RWops *rwops;

    if (filename == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_NULL_ARGUMENT,
                                        "filename");
        return 0;
    }

    rwops = SDL_RWFromFile(filename, "r");
    if (rwops == NULL) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_FILE_NOT_FOUND, "%s",
                                        filename);
        return 0;
    }

    return METAENGINE_Render_CompileShader_RW(shader_type, rwops, 1);
}

Uint32 METAENGINE_Render_CompileShader(METAENGINE_Render_ShaderEnum shader_type,
                                       const char *shader_source) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return 0;

    return _gpu_current_renderer->impl->CompileShader(_gpu_current_renderer, shader_type,
                                                      shader_source);
}

METAENGINE_Render_bool METAENGINE_Render_LinkShaderProgram(Uint32 program_object) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return METAENGINE_Render_FALSE;

    return _gpu_current_renderer->impl->LinkShaderProgram(_gpu_current_renderer, program_object);
}

Uint32 METAENGINE_Render_CreateShaderProgram(void) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return 0;

    return _gpu_current_renderer->impl->CreateShaderProgram(_gpu_current_renderer);
}

Uint32 METAENGINE_Render_LinkShaders(Uint32 shader_object1, Uint32 shader_object2) {
    Uint32 shaders[2];
    shaders[0] = shader_object1;
    shaders[1] = shader_object2;
    return METAENGINE_Render_LinkManyShaders(shaders, 2);
}

Uint32 METAENGINE_Render_LinkManyShaders(Uint32 *shader_objects, int count) {
    Uint32 p;
    int i;

    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return 0;

    if ((_gpu_current_renderer->enabled_features & METAENGINE_Render_FEATURE_BASIC_SHADERS) !=
        METAENGINE_Render_FEATURE_BASIC_SHADERS)
        return 0;

    p = _gpu_current_renderer->impl->CreateShaderProgram(_gpu_current_renderer);

    for (i = 0; i < count; i++)
        _gpu_current_renderer->impl->AttachShader(_gpu_current_renderer, p, shader_objects[i]);

    if (_gpu_current_renderer->impl->LinkShaderProgram(_gpu_current_renderer, p)) return p;

    _gpu_current_renderer->impl->FreeShaderProgram(_gpu_current_renderer, p);
    return 0;
}

void METAENGINE_Render_FreeShader(Uint32 shader_object) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->FreeShader(_gpu_current_renderer, shader_object);
}

void METAENGINE_Render_FreeShaderProgram(Uint32 program_object) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->FreeShaderProgram(_gpu_current_renderer, program_object);
}

void METAENGINE_Render_AttachShader(Uint32 program_object, Uint32 shader_object) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->AttachShader(_gpu_current_renderer, program_object, shader_object);
}

void METAENGINE_Render_DetachShader(Uint32 program_object, Uint32 shader_object) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->DetachShader(_gpu_current_renderer, program_object, shader_object);
}

METAENGINE_Render_bool METAENGINE_Render_IsDefaultShaderProgram(Uint32 program_object) {
    METAENGINE_Render_Context *context;

    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return METAENGINE_Render_FALSE;

    context = _gpu_current_renderer->current_context_target->context;
    return (program_object == context->default_textured_shader_program ||
            program_object == context->default_untextured_shader_program);
}

void METAENGINE_Render_ActivateShaderProgram(Uint32 program_object,
                                             METAENGINE_Render_ShaderBlock *block) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->ActivateShaderProgram(_gpu_current_renderer, program_object,
                                                       block);
}

void METAENGINE_Render_DeactivateShaderProgram(void) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->DeactivateShaderProgram(_gpu_current_renderer);
}

const char *METAENGINE_Render_GetShaderMessage(void) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return NULL;

    return _gpu_current_renderer->impl->GetShaderMessage(_gpu_current_renderer);
}

int METAENGINE_Render_GetAttributeLocation(Uint32 program_object, const char *attrib_name) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return 0;

    return _gpu_current_renderer->impl->GetAttributeLocation(_gpu_current_renderer, program_object,
                                                             attrib_name);
}

METAENGINE_Render_AttributeFormat METAENGINE_Render_MakeAttributeFormat(
        int num_elems_per_vertex, METAENGINE_Render_TypeEnum type, METAENGINE_Render_bool normalize,
        int stride_bytes, int offset_bytes) {
    METAENGINE_Render_AttributeFormat f;
    f.is_per_sprite = METAENGINE_Render_FALSE;
    f.num_elems_per_value = num_elems_per_vertex;
    f.type = type;
    f.normalize = normalize;
    f.stride_bytes = stride_bytes;
    f.offset_bytes = offset_bytes;
    return f;
}

METAENGINE_Render_Attribute METAENGINE_Render_MakeAttribute(
        int location, void *values, METAENGINE_Render_AttributeFormat format) {
    METAENGINE_Render_Attribute a;
    a.location = location;
    a.values = values;
    a.format = format;
    return a;
}

int METAENGINE_Render_GetUniformLocation(Uint32 program_object, const char *uniform_name) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return 0;

    return _gpu_current_renderer->impl->GetUniformLocation(_gpu_current_renderer, program_object,
                                                           uniform_name);
}

METAENGINE_Render_ShaderBlock METAENGINE_Render_LoadShaderBlock(Uint32 program_object,
                                                                const char *position_name,
                                                                const char *texcoord_name,
                                                                const char *color_name,
                                                                const char *modelViewMatrix_name) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL) {
        METAENGINE_Render_ShaderBlock b;
        b.position_loc = -1;
        b.texcoord_loc = -1;
        b.color_loc = -1;
        b.modelViewProjection_loc = -1;
        return b;
    }

    return _gpu_current_renderer->impl->LoadShaderBlock(_gpu_current_renderer, program_object,
                                                        position_name, texcoord_name, color_name,
                                                        modelViewMatrix_name);
}

void METAENGINE_Render_SetShaderBlock(METAENGINE_Render_ShaderBlock block) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->current_context_target->context->current_shader_block = block;
}

METAENGINE_Render_ShaderBlock METAENGINE_Render_GetShaderBlock(void) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL) {
        METAENGINE_Render_ShaderBlock b;
        b.position_loc = -1;
        b.texcoord_loc = -1;
        b.color_loc = -1;
        b.modelViewProjection_loc = -1;
        return b;
    }

    return _gpu_current_renderer->current_context_target->context->current_shader_block;
}

void METAENGINE_Render_SetShaderImage(METAENGINE_Render_Image *image, int location,
                                      int image_unit) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetShaderImage(_gpu_current_renderer, image, location, image_unit);
}

void METAENGINE_Render_GetUniformiv(Uint32 program_object, int location, int *values) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->GetUniformiv(_gpu_current_renderer, program_object, location,
                                              values);
}

void METAENGINE_Render_SetUniformi(int location, int value) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetUniformi(_gpu_current_renderer, location, value);
}

void METAENGINE_Render_SetUniformiv(int location, int num_elements_per_value, int num_values,
                                    int *values) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetUniformiv(_gpu_current_renderer, location,
                                              num_elements_per_value, num_values, values);
}

void METAENGINE_Render_GetUniformuiv(Uint32 program_object, int location, unsigned int *values) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->GetUniformuiv(_gpu_current_renderer, program_object, location,
                                               values);
}

void METAENGINE_Render_SetUniformui(int location, unsigned int value) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetUniformui(_gpu_current_renderer, location, value);
}

void METAENGINE_Render_SetUniformuiv(int location, int num_elements_per_value, int num_values,
                                     unsigned int *values) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetUniformuiv(_gpu_current_renderer, location,
                                               num_elements_per_value, num_values, values);
}

void METAENGINE_Render_GetUniformfv(Uint32 program_object, int location, float *values) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->GetUniformfv(_gpu_current_renderer, program_object, location,
                                              values);
}

void METAENGINE_Render_SetUniformf(int location, float value) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetUniformf(_gpu_current_renderer, location, value);
}

void METAENGINE_Render_SetUniformfv(int location, int num_elements_per_value, int num_values,
                                    float *values) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetUniformfv(_gpu_current_renderer, location,
                                              num_elements_per_value, num_values, values);
}

// Same as METAENGINE_Render_GetUniformfv()
void METAENGINE_Render_GetUniformMatrixfv(Uint32 program_object, int location, float *values) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->GetUniformfv(_gpu_current_renderer, program_object, location,
                                              values);
}

void METAENGINE_Render_SetUniformMatrixfv(int location, int num_matrices, int num_rows,
                                          int num_columns, METAENGINE_Render_bool transpose,
                                          float *values) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetUniformMatrixfv(_gpu_current_renderer, location, num_matrices,
                                                    num_rows, num_columns, transpose, values);
}

void METAENGINE_Render_SetAttributef(int location, float value) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetAttributef(_gpu_current_renderer, location, value);
}

void METAENGINE_Render_SetAttributei(int location, int value) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetAttributei(_gpu_current_renderer, location, value);
}

void METAENGINE_Render_SetAttributeui(int location, unsigned int value) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetAttributeui(_gpu_current_renderer, location, value);
}

void METAENGINE_Render_SetAttributefv(int location, int num_elements, float *value) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetAttributefv(_gpu_current_renderer, location, num_elements,
                                                value);
}

void METAENGINE_Render_SetAttributeiv(int location, int num_elements, int *value) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetAttributeiv(_gpu_current_renderer, location, num_elements,
                                                value);
}

void METAENGINE_Render_SetAttributeuiv(int location, int num_elements, unsigned int *value) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetAttributeuiv(_gpu_current_renderer, location, num_elements,
                                                 value);
}

void METAENGINE_Render_SetAttributeSource(int num_values, METAENGINE_Render_Attribute source) {
    if (_gpu_current_renderer == NULL || _gpu_current_renderer->current_context_target == NULL)
        return;

    _gpu_current_renderer->impl->SetAttributeSource(_gpu_current_renderer, num_values, source);
}

// gpu_strcasecmp()
// A portable strcasecmp() from UC Berkeley
/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific written prior permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static const char caseless_charmap[] = {
        '\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007', '\010', '\011', '\012',
        '\013', '\014', '\015', '\016', '\017', '\020', '\021', '\022', '\023', '\024', '\025',
        '\026', '\027', '\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037', '\040',
        '\041', '\042', '\043', '\044', '\045', '\046', '\047', '\050', '\051', '\052', '\053',
        '\054', '\055', '\056', '\057', '\060', '\061', '\062', '\063', '\064', '\065', '\066',
        '\067', '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077', '\100', '\141',
        '\142', '\143', '\144', '\145', '\146', '\147', '\150', '\151', '\152', '\153', '\154',
        '\155', '\156', '\157', '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
        '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137', '\140', '\141', '\142',
        '\143', '\144', '\145', '\146', '\147', '\150', '\151', '\152', '\153', '\154', '\155',
        '\156', '\157', '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167', '\170',
        '\171', '\172', '\173', '\174', '\175', '\176', '\177', '\200', '\201', '\202', '\203',
        '\204', '\205', '\206', '\207', '\210', '\211', '\212', '\213', '\214', '\215', '\216',
        '\217', '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227', '\230', '\231',
        '\232', '\233', '\234', '\235', '\236', '\237', '\240', '\241', '\242', '\243', '\244',
        '\245', '\246', '\247', '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
        '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267', '\270', '\271', '\272',
        '\273', '\274', '\275', '\276', '\277', '\300', '\341', '\342', '\343', '\344', '\345',
        '\346', '\347', '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357', '\360',
        '\361', '\362', '\363', '\364', '\365', '\366', '\367', '\370', '\371', '\372', '\333',
        '\334', '\335', '\336', '\337', '\340', '\341', '\342', '\343', '\344', '\345', '\346',
        '\347', '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357', '\360', '\361',
        '\362', '\363', '\364', '\365', '\366', '\367', '\370', '\371', '\372', '\373', '\374',
        '\375', '\376', '\377',
};

int gpu_strcasecmp(const char *s1, const char *s2) {
    unsigned char u1, u2;

    do {
        u1 = (unsigned char) *s1++;
        u2 = (unsigned char) *s2++;
        if (caseless_charmap[u1] != caseless_charmap[u2])
            return caseless_charmap[u1] - caseless_charmap[u2];
    } while (u1 != '\0');

    return 0;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

METAENGINE_Render_MatrixStack *METAENGINE_Render_CreateMatrixStack(void) {
    METAENGINE_Render_MatrixStack *stack =
            (METAENGINE_Render_MatrixStack *) SDL_malloc(sizeof(METAENGINE_Render_MatrixStack));
    stack->matrix = NULL;
    stack->size = 0;
    stack->storage_size = 0;
    METAENGINE_Render_InitMatrixStack(stack);
    return stack;
}

void METAENGINE_Render_FreeMatrixStack(METAENGINE_Render_MatrixStack *stack) {
    METAENGINE_Render_ClearMatrixStack(stack);
    SDL_free(stack);
}

void METAENGINE_Render_InitMatrixStack(METAENGINE_Render_MatrixStack *stack) {
    if (stack == NULL) return;

    if (stack->storage_size != 0) METAENGINE_Render_ClearMatrixStack(stack);

    stack->storage_size = 1;
    stack->size = 1;

    stack->matrix = (float **) SDL_malloc(sizeof(float *) * stack->storage_size);
    stack->matrix[0] = (float *) SDL_malloc(sizeof(float) * 16);
    METAENGINE_Render_MatrixIdentity(stack->matrix[0]);
}

void METAENGINE_Render_CopyMatrixStack(const METAENGINE_Render_MatrixStack *source,
                                       METAENGINE_Render_MatrixStack *dest) {
    unsigned int i;
    unsigned int matrix_size = sizeof(float) * 16;
    if (source == NULL || dest == NULL) return;

    METAENGINE_Render_ClearMatrixStack(dest);
    dest->matrix = (float **) SDL_malloc(sizeof(float *) * source->storage_size);
    for (i = 0; i < source->storage_size; ++i) {
        dest->matrix[i] = (float *) SDL_malloc(matrix_size);
        memcpy(dest->matrix[i], source->matrix[i], matrix_size);
    }
    dest->storage_size = source->storage_size;
}

void METAENGINE_Render_ClearMatrixStack(METAENGINE_Render_MatrixStack *stack) {
    unsigned int i;
    for (i = 0; i < stack->storage_size; ++i) { SDL_free(stack->matrix[i]); }
    SDL_free(stack->matrix);

    stack->matrix = NULL;
    stack->storage_size = 0;
}

void METAENGINE_Render_ResetProjection(METAENGINE_Render_Target *target) {
    if (target == NULL) return;

    METAENGINE_Render_bool invert = (target->image != NULL);

    // Set up default projection
    float *projection_matrix = METAENGINE_Render_GetTopMatrix(&target->projection_matrix);
    METAENGINE_Render_MatrixIdentity(projection_matrix);

    if (!invert ^ METAENGINE_Render_GetCoordinateMode())
        METAENGINE_Render_MatrixOrtho(projection_matrix, 0, target->w, target->h, 0,
                                      target->camera.z_near, target->camera.z_far);
    else
        METAENGINE_Render_MatrixOrtho(
                projection_matrix, 0, target->w, 0, target->h, target->camera.z_near,
                target->camera
                        .z_far);// Special inverted orthographic projection because tex coords are inverted already for render-to-texture
}

// Column-major
#define INDEX(row, col) ((col) *4 + (row))

float METAENGINE_Render_VectorLength(const float *vec3) {
    return sqrtf(vec3[0] * vec3[0] + vec3[1] * vec3[1] + vec3[2] * vec3[2]);
}

void METAENGINE_Render_VectorNormalize(float *vec3) {
    float mag = METAENGINE_Render_VectorLength(vec3);
    vec3[0] /= mag;
    vec3[1] /= mag;
    vec3[2] /= mag;
}

float METAENGINE_Render_VectorDot(const float *A, const float *B) {
    return A[0] * B[0] + A[1] * B[1] + A[2] * B[2];
}

void METAENGINE_Render_VectorCross(float *result, const float *A, const float *B) {
    result[0] = A[1] * B[2] - A[2] * B[1];
    result[1] = A[2] * B[0] - A[0] * B[2];
    result[2] = A[0] * B[1] - A[1] * B[0];
}

void METAENGINE_Render_VectorCopy(float *result, const float *A) {
    result[0] = A[0];
    result[1] = A[1];
    result[2] = A[2];
}

void METAENGINE_Render_VectorApplyMatrix(float *vec3, const float *matrix_4x4) {
    float x = matrix_4x4[0] * vec3[0] + matrix_4x4[4] * vec3[1] + matrix_4x4[8] * vec3[2] +
              matrix_4x4[12];
    float y = matrix_4x4[1] * vec3[0] + matrix_4x4[5] * vec3[1] + matrix_4x4[9] * vec3[2] +
              matrix_4x4[13];
    float z = matrix_4x4[2] * vec3[0] + matrix_4x4[6] * vec3[1] + matrix_4x4[10] * vec3[2] +
              matrix_4x4[14];
    float w = matrix_4x4[3] * vec3[0] + matrix_4x4[7] * vec3[1] + matrix_4x4[11] * vec3[2] +
              matrix_4x4[15];
    vec3[0] = x / w;
    vec3[1] = y / w;
    vec3[2] = z / w;
}

void METAENGINE_Render_Vector4ApplyMatrix(float *vec4, const float *matrix_4x4) {
    float x = matrix_4x4[0] * vec4[0] + matrix_4x4[4] * vec4[1] + matrix_4x4[8] * vec4[2] +
              matrix_4x4[12] * vec4[3];
    float y = matrix_4x4[1] * vec4[0] + matrix_4x4[5] * vec4[1] + matrix_4x4[9] * vec4[2] +
              matrix_4x4[13] * vec4[3];
    float z = matrix_4x4[2] * vec4[0] + matrix_4x4[6] * vec4[1] + matrix_4x4[10] * vec4[2] +
              matrix_4x4[14] * vec4[3];
    float w = matrix_4x4[3] * vec4[0] + matrix_4x4[7] * vec4[1] + matrix_4x4[11] * vec4[2] +
              matrix_4x4[15] * vec4[3];

    vec4[0] = x;
    vec4[1] = y;
    vec4[2] = z;
    vec4[3] = w;
    if (w != 0.0f) {
        vec4[0] = x / w;
        vec4[1] = y / w;
        vec4[2] = z / w;
        vec4[3] = 1;
    }
}

// Matrix math implementations based on Wayne Cochran's (wcochran) matrix.c

#define FILL_MATRIX_4x4(A, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15)   \
    A[0] = a0;                                                                                     \
    A[1] = a1;                                                                                     \
    A[2] = a2;                                                                                     \
    A[3] = a3;                                                                                     \
    A[4] = a4;                                                                                     \
    A[5] = a5;                                                                                     \
    A[6] = a6;                                                                                     \
    A[7] = a7;                                                                                     \
    A[8] = a8;                                                                                     \
    A[9] = a9;                                                                                     \
    A[10] = a10;                                                                                   \
    A[11] = a11;                                                                                   \
    A[12] = a12;                                                                                   \
    A[13] = a13;                                                                                   \
    A[14] = a14;                                                                                   \
    A[15] = a15;

void METAENGINE_Render_MatrixCopy(float *result, const float *A) {
    memcpy(result, A, 16 * sizeof(float));
}

void METAENGINE_Render_MatrixIdentity(float *result) {
    memset(result, 0, 16 * sizeof(float));
    result[0] = result[5] = result[10] = result[15] = 1;
}

void METAENGINE_Render_MatrixOrtho(float *result, float left, float right, float bottom, float top,
                                   float z_near, float z_far) {
    if (result == NULL) return;

    {
#ifdef ROW_MAJOR
        float A[16];
        FILL_MATRIX_4x4(A, 2 / (right - left), 0, 0, -(right + left) / (right - left), 0,
                        2 / (top - bottom), 0, -(top + bottom) / (top - bottom), 0, 0,
                        -2 / (z_far - z_near), -(z_far + z_near) / (z_far - z_near), 0, 0, 0, 1);
#else
        float A[16];
        FILL_MATRIX_4x4(A, 2 / (right - left), 0, 0, 0, 0, 2 / (top - bottom), 0, 0, 0, 0,
                        -2 / (z_far - z_near), 0, -(right + left) / (right - left),
                        -(top + bottom) / (top - bottom), -(z_far + z_near) / (z_far - z_near), 1);
#endif

        METAENGINE_Render_MultiplyAndAssign(result, A);
    }
}

void METAENGINE_Render_MatrixFrustum(float *result, float left, float right, float bottom,
                                     float top, float z_near, float z_far) {
    if (result == NULL) return;

    {
        float A[16];
        FILL_MATRIX_4x4(A, 2 * z_near / (right - left), 0, 0, 0, 0, 2 * z_near / (top - bottom), 0,
                        0, (right + left) / (right - left), (top + bottom) / (top - bottom),
                        -(z_far + z_near) / (z_far - z_near), -1, 0, 0,
                        -(2 * z_far * z_near) / (z_far - z_near), 0);

        METAENGINE_Render_MultiplyAndAssign(result, A);
    }
}

void METAENGINE_Render_MatrixPerspective(float *result, float fovy, float aspect, float z_near,
                                         float z_far) {
    float fW, fH;

    // Make it left-handed?
    fovy = -fovy;
    aspect = -aspect;

    fH = tanf((fovy / 360) * PI) * z_near;
    fW = fH * aspect;
    METAENGINE_Render_MatrixFrustum(result, -fW, fW, -fH, fH, z_near, z_far);
}

void METAENGINE_Render_MatrixLookAt(float *matrix, float eye_x, float eye_y, float eye_z,
                                    float target_x, float target_y, float target_z, float up_x,
                                    float up_y, float up_z) {
    float forward[3] = {target_x - eye_x, target_y - eye_y, target_z - eye_z};
    float up[3] = {up_x, up_y, up_z};
    float side[3];
    float view[16];

    METAENGINE_Render_VectorNormalize(forward);
    METAENGINE_Render_VectorNormalize(up);

    // Calculate sideways vector
    METAENGINE_Render_VectorCross(side, forward, up);

    // Calculate new up vector
    METAENGINE_Render_VectorCross(up, side, forward);

    // Set up view matrix
    view[0] = side[0];
    view[4] = side[1];
    view[8] = side[2];
    view[12] = 0.0f;

    view[1] = up[0];
    view[5] = up[1];
    view[9] = up[2];
    view[13] = 0.0f;

    view[2] = -forward[0];
    view[6] = -forward[1];
    view[10] = -forward[2];
    view[14] = 0.0f;

    view[3] = view[7] = view[11] = 0.0f;
    view[15] = 1.0f;

    METAENGINE_Render_MultiplyAndAssign(matrix, view);
    METAENGINE_Render_MatrixTranslate(matrix, -eye_x, -eye_y, -eye_z);
}

void METAENGINE_Render_MatrixTranslate(float *result, float x, float y, float z) {
    if (result == NULL) return;

    {
#ifdef ROW_MAJOR
        float A[16];
        FILL_MATRIX_4x4(A, 1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z, 0, 0, 0, 1);
#else
        float A[16];
        FILL_MATRIX_4x4(A, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1);
#endif

        METAENGINE_Render_MultiplyAndAssign(result, A);
    }
}

void METAENGINE_Render_MatrixScale(float *result, float sx, float sy, float sz) {
    if (result == NULL) return;

    {
        float A[16];
        FILL_MATRIX_4x4(A, sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, sz, 0, 0, 0, 0, 1);

        METAENGINE_Render_MultiplyAndAssign(result, A);
    }
}

void METAENGINE_Render_MatrixRotate(float *result, float degrees, float x, float y, float z) {
    float p, radians, c, s, c_, zc_, yc_, xzc_, xyc_, yzc_, xs, ys, zs;

    if (result == NULL) return;

    p = 1 / sqrtf(x * x + y * y + z * z);
    x *= p;
    y *= p;
    z *= p;
    radians = degrees * RAD_PER_DEG;
    c = cosf(radians);
    s = sinf(radians);
    c_ = 1 - c;
    zc_ = z * c_;
    yc_ = y * c_;
    xzc_ = x * zc_;
    xyc_ = x * y * c_;
    yzc_ = y * zc_;
    xs = x * s;
    ys = y * s;
    zs = z * s;

    {
#ifdef ROW_MAJOR
        float A[16];
        FILL_MATRIX_4x4(A, x * x * c_ + c, xyc_ - zs, xzc_ + ys, 0, xyc_ + zs, y * yc_ + c,
                        yzc_ - xs, 0, xzc_ - ys, yzc_ + xs, z * zc_ + c, 0, 0, 0, 0, 1);
#else
        float A[16];
        FILL_MATRIX_4x4(A, x * x * c_ + c, xyc_ + zs, xzc_ - ys, 0, xyc_ - zs, y * yc_ + c,
                        yzc_ + xs, 0, xzc_ + ys, yzc_ - xs, z * zc_ + c, 0, 0, 0, 0, 1);
#endif

        METAENGINE_Render_MultiplyAndAssign(result, A);
    }
}

// Matrix multiply: result = A * B
void METAENGINE_Render_MatrixMultiply(float *result, const float *A, const float *B) {
    float(*matR)[4] = (float(*)[4]) result;
    float(*matA)[4] = (float(*)[4]) A;
    float(*matB)[4] = (float(*)[4]) B;
    matR[0][0] = matB[0][0] * matA[0][0] + matB[0][1] * matA[1][0] + matB[0][2] * matA[2][0] +
                 matB[0][3] * matA[3][0];
    matR[0][1] = matB[0][0] * matA[0][1] + matB[0][1] * matA[1][1] + matB[0][2] * matA[2][1] +
                 matB[0][3] * matA[3][1];
    matR[0][2] = matB[0][0] * matA[0][2] + matB[0][1] * matA[1][2] + matB[0][2] * matA[2][2] +
                 matB[0][3] * matA[3][2];
    matR[0][3] = matB[0][0] * matA[0][3] + matB[0][1] * matA[1][3] + matB[0][2] * matA[2][3] +
                 matB[0][3] * matA[3][3];
    matR[1][0] = matB[1][0] * matA[0][0] + matB[1][1] * matA[1][0] + matB[1][2] * matA[2][0] +
                 matB[1][3] * matA[3][0];
    matR[1][1] = matB[1][0] * matA[0][1] + matB[1][1] * matA[1][1] + matB[1][2] * matA[2][1] +
                 matB[1][3] * matA[3][1];
    matR[1][2] = matB[1][0] * matA[0][2] + matB[1][1] * matA[1][2] + matB[1][2] * matA[2][2] +
                 matB[1][3] * matA[3][2];
    matR[1][3] = matB[1][0] * matA[0][3] + matB[1][1] * matA[1][3] + matB[1][2] * matA[2][3] +
                 matB[1][3] * matA[3][3];
    matR[2][0] = matB[2][0] * matA[0][0] + matB[2][1] * matA[1][0] + matB[2][2] * matA[2][0] +
                 matB[2][3] * matA[3][0];
    matR[2][1] = matB[2][0] * matA[0][1] + matB[2][1] * matA[1][1] + matB[2][2] * matA[2][1] +
                 matB[2][3] * matA[3][1];
    matR[2][2] = matB[2][0] * matA[0][2] + matB[2][1] * matA[1][2] + matB[2][2] * matA[2][2] +
                 matB[2][3] * matA[3][2];
    matR[2][3] = matB[2][0] * matA[0][3] + matB[2][1] * matA[1][3] + matB[2][2] * matA[2][3] +
                 matB[2][3] * matA[3][3];
    matR[3][0] = matB[3][0] * matA[0][0] + matB[3][1] * matA[1][0] + matB[3][2] * matA[2][0] +
                 matB[3][3] * matA[3][0];
    matR[3][1] = matB[3][0] * matA[0][1] + matB[3][1] * matA[1][1] + matB[3][2] * matA[2][1] +
                 matB[3][3] * matA[3][1];
    matR[3][2] = matB[3][0] * matA[0][2] + matB[3][1] * matA[1][2] + matB[3][2] * matA[2][2] +
                 matB[3][3] * matA[3][2];
    matR[3][3] = matB[3][0] * matA[0][3] + matB[3][1] * matA[1][3] + matB[3][2] * matA[2][3] +
                 matB[3][3] * matA[3][3];
}

void METAENGINE_Render_MultiplyAndAssign(float *result, const float *B) {
    float temp[16];
    METAENGINE_Render_MatrixMultiply(temp, result, B);
    METAENGINE_Render_MatrixCopy(result, temp);
}

// Can be used up to two times per line evaluation...
const char *METAENGINE_Render_GetMatrixString(const float *A) {
    static char buffer[512];
    static char buffer2[512];
    static char flip = 0;

    char *b = (flip ? buffer : buffer2);
    flip = !flip;

    snprintf(b, 512,
             "%.1f %.1f %.1f %.1f\n"
             "%.1f %.1f %.1f %.1f\n"
             "%.1f %.1f %.1f %.1f\n"
             "%.1f %.1f %.1f %.1f",
             A[0], A[1], A[2], A[3], A[4], A[5], A[6], A[7], A[8], A[9], A[10], A[11], A[12], A[13],
             A[14], A[15]);
    return b;
}

void METAENGINE_Render_MatrixMode(METAENGINE_Render_Target *target, int matrix_mode) {
    METAENGINE_Render_Target *context_target;
    if (target == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();
    target->matrix_mode = matrix_mode;

    context_target = METAENGINE_Render_GetContextTarget();
    if (context_target != NULL && context_target == target->context_target)
        context_target->context->active_target = target;
}

float *METAENGINE_Render_GetModel(void) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL) return NULL;
    return METAENGINE_Render_GetTopMatrix(&target->model_matrix);
}

float *METAENGINE_Render_GetView(void) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL) return NULL;
    return METAENGINE_Render_GetTopMatrix(&target->view_matrix);
}

float *METAENGINE_Render_GetProjection(void) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL) return NULL;
    return METAENGINE_Render_GetTopMatrix(&target->projection_matrix);
}

float *METAENGINE_Render_GetCurrentMatrix(void) {
    METAENGINE_Render_MatrixStack *stack;
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL) return NULL;
    if (target->matrix_mode == METAENGINE_Render_MODEL) stack = &target->model_matrix;
    else if (target->matrix_mode == METAENGINE_Render_VIEW)
        stack = &target->view_matrix;
    else// if(target->matrix_mode == METAENGINE_Render_PROJECTION)
        stack = &target->projection_matrix;

    return METAENGINE_Render_GetTopMatrix(stack);
}

void METAENGINE_Render_PushMatrix(void) {
    METAENGINE_Render_MatrixStack *stack;
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL) return;

    if (target->matrix_mode == METAENGINE_Render_MODEL) stack = &target->model_matrix;
    else if (target->matrix_mode == METAENGINE_Render_VIEW)
        stack = &target->view_matrix;
    else// if(target->matrix_mode == METAENGINE_Render_PROJECTION)
        stack = &target->projection_matrix;

    if (stack->size + 1 >= stack->storage_size) {
        // Grow matrix stack (1, 6, 16, 36, ...)

        // Alloc new one
        unsigned int new_storage_size = stack->storage_size * 2 + 4;
        float **new_stack = (float **) SDL_malloc(sizeof(float *) * new_storage_size);
        unsigned int i;
        for (i = 0; i < new_storage_size; ++i) {
            new_stack[i] = (float *) SDL_malloc(sizeof(float) * 16);
        }
        // Copy old one
        for (i = 0; i < stack->size; ++i) {
            METAENGINE_Render_MatrixCopy(new_stack[i], stack->matrix[i]);
        }
        // Free old one
        for (i = 0; i < stack->storage_size; ++i) { SDL_free(stack->matrix[i]); }
        SDL_free(stack->matrix);

        // Switch to new one
        stack->storage_size = new_storage_size;
        stack->matrix = new_stack;
    }
    METAENGINE_Render_MatrixCopy(stack->matrix[stack->size], stack->matrix[stack->size - 1]);
    stack->size++;
}

void METAENGINE_Render_PopMatrix(void) {
    METAENGINE_Render_MatrixStack *stack;

    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL) return;

    // FIXME: Flushing here is not always necessary if this isn't the last target
    METAENGINE_Render_FlushBlitBuffer();

    if (target->matrix_mode == METAENGINE_Render_MODEL) stack = &target->model_matrix;
    else if (target->matrix_mode == METAENGINE_Render_VIEW)
        stack = &target->view_matrix;
    else//if(target->matrix_mode == METAENGINE_Render_PROJECTION)
        stack = &target->projection_matrix;

    if (stack->size == 0) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR,
                                        "Matrix stack is empty.");
    } else if (stack->size == 1) {
        METAENGINE_Render_PushErrorCode(__func__, METAENGINE_Render_ERROR_USER_ERROR,
                                        "Matrix stack would become empty!");
    } else
        stack->size--;
}

void METAENGINE_Render_SetProjection(const float *A) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL || A == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixCopy(METAENGINE_Render_GetProjection(), A);
}

void METAENGINE_Render_SetModel(const float *A) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL || A == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixCopy(METAENGINE_Render_GetModel(), A);
}

void METAENGINE_Render_SetView(const float *A) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL || A == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixCopy(METAENGINE_Render_GetView(), A);
}

void METAENGINE_Render_SetProjectionFromStack(METAENGINE_Render_MatrixStack *stack) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL || stack == NULL) return;

    METAENGINE_Render_SetProjection(METAENGINE_Render_GetTopMatrix(stack));
}

void METAENGINE_Render_SetModelFromStack(METAENGINE_Render_MatrixStack *stack) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL || stack == NULL) return;

    METAENGINE_Render_SetModel(METAENGINE_Render_GetTopMatrix(stack));
}

void METAENGINE_Render_SetViewFromStack(METAENGINE_Render_MatrixStack *stack) {
    METAENGINE_Render_Target *target = METAENGINE_Render_GetActiveTarget();
    if (target == NULL || stack == NULL) return;

    METAENGINE_Render_SetView(METAENGINE_Render_GetTopMatrix(stack));
}

float *METAENGINE_Render_GetTopMatrix(METAENGINE_Render_MatrixStack *stack) {
    if (stack == NULL || stack->size == 0) return NULL;
    return stack->matrix[stack->size - 1];
}

void METAENGINE_Render_LoadIdentity(void) {
    float *result = METAENGINE_Render_GetCurrentMatrix();
    if (result == NULL) return;

    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixIdentity(result);
}

void METAENGINE_Render_LoadMatrix(const float *A) {
    float *result = METAENGINE_Render_GetCurrentMatrix();
    if (result == NULL) return;
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixCopy(result, A);
}

void METAENGINE_Render_Ortho(float left, float right, float bottom, float top, float z_near,
                             float z_far) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixOrtho(METAENGINE_Render_GetCurrentMatrix(), left, right, bottom, top,
                                  z_near, z_far);
}

void METAENGINE_Render_Frustum(float left, float right, float bottom, float top, float z_near,
                               float z_far) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixFrustum(METAENGINE_Render_GetCurrentMatrix(), left, right, bottom, top,
                                    z_near, z_far);
}

void METAENGINE_Render_Perspective(float fovy, float aspect, float z_near, float z_far) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixPerspective(METAENGINE_Render_GetCurrentMatrix(), fovy, aspect, z_near,
                                        z_far);
}

void METAENGINE_Render_LookAt(float eye_x, float eye_y, float eye_z, float target_x, float target_y,
                              float target_z, float up_x, float up_y, float up_z) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixLookAt(METAENGINE_Render_GetCurrentMatrix(), eye_x, eye_y, eye_z,
                                   target_x, target_y, target_z, up_x, up_y, up_z);
}

void METAENGINE_Render_Translate(float x, float y, float z) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixTranslate(METAENGINE_Render_GetCurrentMatrix(), x, y, z);
}

void METAENGINE_Render_Scale(float sx, float sy, float sz) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixScale(METAENGINE_Render_GetCurrentMatrix(), sx, sy, sz);
}

void METAENGINE_Render_Rotate(float degrees, float x, float y, float z) {
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MatrixRotate(METAENGINE_Render_GetCurrentMatrix(), degrees, x, y, z);
}

void METAENGINE_Render_MultMatrix(const float *A) {
    float *result = METAENGINE_Render_GetCurrentMatrix();
    if (result == NULL) return;
    METAENGINE_Render_FlushBlitBuffer();
    METAENGINE_Render_MultiplyAndAssign(result, A);
}

void METAENGINE_Render_GetModelViewProjection(float *result) {
    // MVP = P * V * M
    METAENGINE_Render_MatrixMultiply(result, METAENGINE_Render_GetProjection(),
                                     METAENGINE_Render_GetView());
    METAENGINE_Render_MultiplyAndAssign(result, METAENGINE_Render_GetModel());
}

#define CHECK_RENDERER()                                                                           \
    METAENGINE_Render_Renderer *renderer = METAENGINE_Render_GetCurrentRenderer();                 \
    if (renderer == NULL) return;

#define CHECK_RENDERER_1(ret)                                                                      \
    METAENGINE_Render_Renderer *renderer = METAENGINE_Render_GetCurrentRenderer();                 \
    if (renderer == NULL) return ret;

float METAENGINE_Render_SetLineThickness(float thickness) {
    CHECK_RENDERER_1(1.0f);
    return renderer->impl->SetLineThickness(renderer, thickness);
}

float METAENGINE_Render_GetLineThickness(void) {
    CHECK_RENDERER_1(1.0f);
    return renderer->impl->GetLineThickness(renderer);
}

void METAENGINE_Render_Pixel(METAENGINE_Render_Target *target, float x, float y, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->Pixel(renderer, target, x, y, color);
}

void METAENGINE_Render_Line(METAENGINE_Render_Target *target, float x1, float y1, float x2,
                            float y2, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->Line(renderer, target, x1, y1, x2, y2, color);
}

void METAENGINE_Render_Arc(METAENGINE_Render_Target *target, float x, float y, float radius,
                           float start_angle, float end_angle, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->Arc(renderer, target, x, y, radius, start_angle, end_angle, color);
}

void METAENGINE_Render_ArcFilled(METAENGINE_Render_Target *target, float x, float y, float radius,
                                 float start_angle, float end_angle, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->ArcFilled(renderer, target, x, y, radius, start_angle, end_angle, color);
}

void METAENGINE_Render_Circle(METAENGINE_Render_Target *target, float x, float y, float radius,
                              SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->Circle(renderer, target, x, y, radius, color);
}

void METAENGINE_Render_CircleFilled(METAENGINE_Render_Target *target, float x, float y,
                                    float radius, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->CircleFilled(renderer, target, x, y, radius, color);
}

void METAENGINE_Render_Ellipse(METAENGINE_Render_Target *target, float x, float y, float rx,
                               float ry, float degrees, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->Ellipse(renderer, target, x, y, rx, ry, degrees, color);
}

void METAENGINE_Render_EllipseFilled(METAENGINE_Render_Target *target, float x, float y, float rx,
                                     float ry, float degrees, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->EllipseFilled(renderer, target, x, y, rx, ry, degrees, color);
}

void METAENGINE_Render_Sector(METAENGINE_Render_Target *target, float x, float y,
                              float inner_radius, float outer_radius, float start_angle,
                              float end_angle, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->Sector(renderer, target, x, y, inner_radius, outer_radius, start_angle,
                           end_angle, color);
}

void METAENGINE_Render_SectorFilled(METAENGINE_Render_Target *target, float x, float y,
                                    float inner_radius, float outer_radius, float start_angle,
                                    float end_angle, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->SectorFilled(renderer, target, x, y, inner_radius, outer_radius, start_angle,
                                 end_angle, color);
}

void METAENGINE_Render_Tri(METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2,
                           float x3, float y3, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->Tri(renderer, target, x1, y1, x2, y2, x3, y3, color);
}

void METAENGINE_Render_TriFilled(METAENGINE_Render_Target *target, float x1, float y1, float x2,
                                 float y2, float x3, float y3, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->TriFilled(renderer, target, x1, y1, x2, y2, x3, y3, color);
}

void METAENGINE_Render_Rectangle(METAENGINE_Render_Target *target, float x1, float y1, float x2,
                                 float y2, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->Rectangle(renderer, target, x1, y1, x2, y2, color);
}

void METAENGINE_Render_Rectangle2(METAENGINE_Render_Target *target, METAENGINE_Render_Rect rect,
                                  SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->Rectangle(renderer, target, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h,
                              color);
}

void METAENGINE_Render_RectangleFilled(METAENGINE_Render_Target *target, float x1, float y1,
                                       float x2, float y2, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->RectangleFilled(renderer, target, x1, y1, x2, y2, color);
}

void METAENGINE_Render_RectangleFilled2(METAENGINE_Render_Target *target,
                                        METAENGINE_Render_Rect rect, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->RectangleFilled(renderer, target, rect.x, rect.y, rect.x + rect.w,
                                    rect.y + rect.h, color);
}

void METAENGINE_Render_RectangleRound(METAENGINE_Render_Target *target, float x1, float y1,
                                      float x2, float y2, float radius, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->RectangleRound(renderer, target, x1, y1, x2, y2, radius, color);
}

void METAENGINE_Render_RectangleRound2(METAENGINE_Render_Target *target,
                                       METAENGINE_Render_Rect rect, float radius, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->RectangleRound(renderer, target, rect.x, rect.y, rect.x + rect.w,
                                   rect.y + rect.h, radius, color);
}

void METAENGINE_Render_RectangleRoundFilled(METAENGINE_Render_Target *target, float x1, float y1,
                                            float x2, float y2, float radius, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->RectangleRoundFilled(renderer, target, x1, y1, x2, y2, radius, color);
}

void METAENGINE_Render_RectangleRoundFilled2(METAENGINE_Render_Target *target,
                                             METAENGINE_Render_Rect rect, float radius,
                                             SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->RectangleRoundFilled(renderer, target, rect.x, rect.y, rect.x + rect.w,
                                         rect.y + rect.h, radius, color);
}

void METAENGINE_Render_Polygon(METAENGINE_Render_Target *target, unsigned int num_vertices,
                               float *vertices, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->Polygon(renderer, target, num_vertices, vertices, color);
}

void METAENGINE_Render_Polyline(METAENGINE_Render_Target *target, unsigned int num_vertices,
                                float *vertices, SDL_Color color,
                                METAENGINE_Render_bool close_loop) {
    CHECK_RENDERER();
    renderer->impl->Polyline(renderer, target, num_vertices, vertices, color, close_loop);
}

void METAENGINE_Render_PolygonFilled(METAENGINE_Render_Target *target, unsigned int num_vertices,
                                     float *vertices, SDL_Color color) {
    CHECK_RENDERER();
    renderer->impl->PolygonFilled(renderer, target, num_vertices, vertices, color);
}
