
#include "renderer_opengl.h"

#include "engine/core/base_memory.h"
#include "engine/core/const.h"
#include "engine/core/macros.hpp"
#include "engine/core/sdl_wrapper.h"
#include "libs/external/stb_image.h"
#include "libs/glad/glad.h"
#include "renderer_gpu.h"  // For poor, dumb Intellisense

// Most of the code pulled in from here...
#define R_USE_BUFFER_PIPELINE
#define R_ASSUME_CORE_FBO
#define R_ASSUME_SHADERS
#define R_SKIP_ENABLE_TEXTURE_2D
#define R_SKIP_LINE_WIDTH
#define R_GLSL_VERSION 130
#define R_GLSL_VERSION_CORE 150
#define R_GL_MAJOR_VERSION 3
#define R_ENABLE_CORE_SHADERS

namespace ME {

int gpu_strcasecmp(const char *s1, const char *s2);

// Default to buffer reset VBO upload method
#if defined(R_USE_BUFFER_PIPELINE) && !defined(R_USE_BUFFER_RESET) && !defined(R_USE_BUFFER_MAPPING) && !defined(R_USE_BUFFER_UPDATE)
#define R_USE_BUFFER_RESET
#endif

// Forces a flush when vertex limit is reached (roughly 1000 sprites)
#define R_BLIT_BUFFER_VERTICES_PER_SPRITE 4
#define R_BLIT_BUFFER_INIT_MAX_NUM_VERTICES (R_BLIT_BUFFER_VERTICES_PER_SPRITE * 1000)

// Near the unsigned short limit (65535)
#define R_BLIT_BUFFER_ABSOLUTE_MAX_VERTICES 60000
// Near the unsigned int limit (4294967295)
#define R_INDEX_BUFFER_ABSOLUTE_MAX_VERTICES 4000000000u

// x, y, s, t, r, g, b, a
#define R_BLIT_BUFFER_FLOATS_PER_VERTEX 8

// bytes per vertex
#define R_BLIT_BUFFER_STRIDE (sizeof(float) * R_BLIT_BUFFER_FLOATS_PER_VERTEX)
#define R_BLIT_BUFFER_VERTEX_OFFSET 0
#define R_BLIT_BUFFER_TEX_COORD_OFFSET 2
#define R_BLIT_BUFFER_COLOR_OFFSET 4

// SDL 2.0 translation layer

#define GET_ALPHA(sdl_color) ((sdl_color).a)

static_inline SDL_Window *get_window(u32 windowID) { return SDL_GetWindowFromID(windowID); }

static_inline u32 get_window_id(SDL_Window *window) { return SDL_GetWindowID(window); }

static_inline void get_window_dimensions(SDL_Window *window, int *w, int *h) { SDL_GetWindowSize(window, w, h); }

static_inline void get_drawable_dimensions(SDL_Window *window, int *w, int *h) { SDL_GL_GetDrawableSize(window, w, h); }

static_inline void resize_window(R_Target *target, int w, int h) { SDL_SetWindowSize(SDL_GetWindowFromID(target->context->windowID), w, h); }

static_inline bool get_fullscreen_state(SDL_Window *window) { return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN); }

static_inline bool has_colorkey(SDL_Surface *surface) { return (SDL_GetColorKey(surface, NULL) == 0); }

static_inline bool is_alpha_format(SDL_PixelFormat *format) { return SDL_ISPIXELFORMAT_ALPHA(format->format); }

static_inline void get_target_window_dimensions(R_Target *target, int *w, int *h) {
    SDL_Window *window;
    if (target == NULL || target->context == NULL) return;
    window = get_window(target->context->windowID);
    get_window_dimensions(window, w, h);
}

static_inline void get_target_drawable_dimensions(R_Target *target, int *w, int *h) {
    SDL_Window *window;
    if (target == NULL || target->context == NULL) return;
    window = get_window(target->context->windowID);
    get_drawable_dimensions(window, w, h);
}

// Workaround for Intel HD glVertexAttrib() bug.

// FIXME: This should probably exist in context storage, as I expect it to be a problem across contexts.
static bool apply_Intel_attrib_workaround = false;
static bool vendor_is_Intel = false;

static SDL_PixelFormat *AllocFormat(GLenum glFormat);
static void FreeFormat(SDL_PixelFormat *format);

static char shader_message[256];

static_inline void fast_upload_texture(const void *pixels, MErect update_rect, u32 format, int alignment, int row_length) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length);

    glTexSubImage2D(GL_TEXTURE_2D, 0, (GLint)update_rect.x, (GLint)update_rect.y, (GLsizei)update_rect.w, (GLsizei)update_rect.h, format, GL_UNSIGNED_BYTE, pixels);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void row_upload_texture(const unsigned char *pixels, MErect update_rect, u32 format, int alignment, unsigned int pitch, int bytes_per_pixel) {
    unsigned int i;
    unsigned int h = (unsigned int)update_rect.h;
    (void)bytes_per_pixel;
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    if (h > 0 && update_rect.w > 0.0f) {
        // Must upload row by row to account for row length
        for (i = 0; i < h; ++i) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, (GLint)update_rect.x, (GLint)(update_rect.y + i), (GLsizei)update_rect.w, 1, format, GL_UNSIGNED_BYTE, pixels);
            pixels += pitch;
        }
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void copy_upload_texture(const unsigned char *pixels, MErect update_rect, u32 format, int alignment, unsigned int pitch, int bytes_per_pixel) {
    unsigned int i;
    unsigned int h = (unsigned int)update_rect.h;
    unsigned int w = ((unsigned int)update_rect.w) * bytes_per_pixel;

    if (h > 0 && w > 0) {
        unsigned int rem = w % alignment;
        // If not already aligned, account for padding on each row
        if (rem > 0) w += alignment - rem;

        unsigned char *copy = (unsigned char *)ME_MALLOC(w * h);
        unsigned char *dst = copy;

        for (i = 0; i < h; ++i) {
            memcpy(dst, pixels, w);
            pixels += pitch;
            dst += w;
        }
        fast_upload_texture(copy, update_rect, format, alignment, (int)update_rect.w);
        ME_FREE(copy);
    }
}

void (*slow_upload_texture)(const unsigned char *pixels, MErect update_rect, u32 format, int alignment, unsigned int pitch, int bytes_per_pixel) = NULL;

static_inline void upload_texture(const void *pixels, MErect update_rect, u32 format, int alignment, int row_length, unsigned int pitch, int bytes_per_pixel) {
    (void)pitch;
    (void)bytes_per_pixel;
    fast_upload_texture(pixels, update_rect, format, alignment, row_length);
}

static_inline void upload_new_texture(void *pixels, MErect update_rect, u32 format, int alignment, int row_length, int bytes_per_pixel) {
    (void)bytes_per_pixel;
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length);
    glTexImage2D(GL_TEXTURE_2D, 0, format, (GLsizei)update_rect.w, (GLsizei)update_rect.h, 0, format, GL_UNSIGNED_BYTE, pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

// Define intermediates for FBO functions in case we only have EXT or OES FBO support.
#if defined(R_ASSUME_CORE_FBO)
#define glBindFramebufferPROC glBindFramebuffer
#define glCheckFramebufferStatusPROC glCheckFramebufferStatus
#define glDeleteFramebuffersPROC glDeleteFramebuffers
#define glFramebufferTexture2DPROC glFramebufferTexture2D
#define glGenFramebuffersPROC glGenFramebuffers
#define glGenerateMipmapPROC glGenerateMipmap
#else
void GLAPIENTRY glBindFramebufferNOOP(GLenum target, GLuint framebuffer) {
    (void)target;
    (void)framebuffer;
    METADOT_ERROR("{}: Unsupported operation", __func__);
}
GLenum GLAPIENTRY glCheckFramebufferStatusNOOP(GLenum target) {
    (void)target;
    METADOT_ERROR("{}: Unsupported operation", __func__);
    return 0;
}
void GLAPIENTRY glDeleteFramebuffersNOOP(GLsizei n, const GLuint *framebuffers) {
    (void)n;
    (void)framebuffers;
    METADOT_ERROR("{}: Unsupported operation", __func__);
}
void GLAPIENTRY glFramebufferTexture2DNOOP(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    (void)target;
    (void)attachment;
    (void)textarget;
    (void)texture;
    (void)level;
    METADOT_ERROR("{}: Unsupported operation", __func__);
}
void GLAPIENTRY glGenFramebuffersNOOP(GLsizei n, GLuint *ids) {
    (void)n;
    (void)ids;
    METADOT_ERROR("{}: Unsupported operation", __func__);
}
void GLAPIENTRY glGenerateMipmapNOOP(GLenum target) {
    (void)target;
    METADOT_ERROR("{}: Unsupported operation", __func__);
}

void(GLAPIENTRY *glBindFramebufferPROC)(GLenum target, GLuint framebuffer) = glBindFramebufferNOOP;
GLenum(GLAPIENTRY *glCheckFramebufferStatusPROC)(GLenum target) = glCheckFramebufferStatusNOOP;
void(GLAPIENTRY *glDeleteFramebuffersPROC)(GLsizei n, const GLuint *framebuffers) = glDeleteFramebuffersNOOP;
void(GLAPIENTRY *glFramebufferTexture2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) = glFramebufferTexture2DNOOP;
void(GLAPIENTRY *glGenFramebuffersPROC)(GLsizei n, GLuint *ids) = glGenFramebuffersNOOP;
void(GLAPIENTRY *glGenerateMipmapPROC)(GLenum target) = glGenerateMipmapNOOP;
#endif

void init_features(R_Renderer *renderer) {
    // Reset supported features
    renderer->enabled_features = 0;

    // NPOT textures
#if R_GL_MAJOR_VERSION >= 2
    // Core in GL 2+
    renderer->enabled_features |= R_FEATURE_NON_POWER_OF_TWO;
#else
    if (isExtensionSupported("GL_ARB_texture_non_power_of_two"))
        renderer->enabled_features |= R_FEATURE_NON_POWER_OF_TWO;
    else
        renderer->enabled_features &= ~R_FEATURE_NON_POWER_OF_TWO;
#endif

    // FBO support
    renderer->enabled_features |= R_FEATURE_RENDER_TARGETS;
    renderer->enabled_features |= R_FEATURE_CORE_FRAMEBUFFER_OBJECTS;

    // Blending
    renderer->enabled_features |= R_FEATURE_BLEND_EQUATIONS;
    renderer->enabled_features |= R_FEATURE_BLEND_FUNC_SEPARATE;

#if R_GL_MAJOR_VERSION >= 2
    // Core in GL 2+
    renderer->enabled_features |= R_FEATURE_BLEND_EQUATIONS_SEPARATE;
#else
    if (isExtensionSupported("GL_EXT_blend_equation_separate"))
        renderer->enabled_features |= R_FEATURE_BLEND_EQUATIONS_SEPARATE;
    else
        renderer->enabled_features &= ~R_FEATURE_BLEND_EQUATIONS_SEPARATE;
#endif

    // Wrap modes
#if R_GL_MAJOR_VERSION >= 2
    renderer->enabled_features |= R_FEATURE_WRAP_REPEAT_MIRRORED;
#else
    if (isExtensionSupported("GL_ARB_texture_mirrored_repeat"))
        renderer->enabled_features |= R_FEATURE_WRAP_REPEAT_MIRRORED;
    else
        renderer->enabled_features &= ~R_FEATURE_WRAP_REPEAT_MIRRORED;
#endif

    // GL texture formats
    // if (isExtensionSupported("GL_EXT_bgr"))
    //     renderer->enabled_features |= R_FEATURE_GL_BGR;
    // if (isExtensionSupported("GL_EXT_bgra"))
    //     renderer->enabled_features |= R_FEATURE_GL_BGRA;
    // if (isExtensionSupported("GL_EXT_abgr"))
    //     renderer->enabled_features |= R_FEATURE_GL_ABGR;

    // Shader support
    renderer->enabled_features |= R_FEATURE_FRAGMENT_SHADER;
    renderer->enabled_features |= R_FEATURE_VERTEX_SHADER;
    renderer->enabled_features |= R_FEATURE_GEOMETRY_SHADER;

#ifdef R_ASSUME_SHADERS
    renderer->enabled_features |= R_FEATURE_BASIC_SHADERS;
#endif
}

void extBindFramebuffer(R_Renderer *renderer, GLuint handle) {
    if (renderer->enabled_features & R_FEATURE_RENDER_TARGETS) glBindFramebufferPROC(GL_FRAMEBUFFER, handle);
}

static_inline bool isPowerOfTwo(unsigned int x) { return ((x != 0) && !(x & (x - 1))); }

static_inline unsigned int getNearestPowerOf2(unsigned int n) {
    unsigned int x = 1;
    while (x < n) {
        x <<= 1;
    }
    return x;
}

void bindTexture(R_Renderer *renderer, R_Image *image) {
    // Bind the texture to which subsequent calls refer
    if (image != ((R_CONTEXT_DATA *)renderer->current_context_target->context->data)->last_image) {
        GLuint handle = ((R_IMAGE_DATA *)image->data)->handle;
        FlushBlitBuffer(renderer);

        glBindTexture(GL_TEXTURE_2D, handle);
        ((R_CONTEXT_DATA *)renderer->current_context_target->context->data)->last_image = image;
    }
}

static_inline void flushAndBindTexture(R_Renderer *renderer, GLuint handle) {
    // Bind the texture to which subsequent calls refer
    FlushBlitBuffer(renderer);

    glBindTexture(GL_TEXTURE_2D, handle);
    ((R_CONTEXT_DATA *)renderer->current_context_target->context->data)->last_image = NULL;
}

// Only for window targets, which have their own contexts.
void makeContextCurrent(R_Renderer *renderer, R_Target *target) {
    if (target == NULL || target->context == NULL || renderer->current_context_target == target) return;

    FlushBlitBuffer(renderer);

    SDL_GL_MakeCurrent(SDL_GetWindowFromID(target->context->windowID), target->context->context);

    renderer->current_context_target = target;
}

// Binds the target's framebuffer.  Returns false if it can't be bound, true if it is bound or already bound.
bool SetActiveTarget(R_Renderer *renderer, R_Target *target) {
    makeContextCurrent(renderer, target);

    if (target == NULL || renderer->current_context_target == NULL) return false;

    if (renderer->enabled_features & R_FEATURE_RENDER_TARGETS) {
        // Bind the FBO
        if (target != renderer->current_context_target->context->active_target) {
            GLuint handle = ((R_TARGET_DATA *)target->data)->handle;
            FlushBlitBuffer(renderer);

            extBindFramebuffer(renderer, handle);
            renderer->current_context_target->context->active_target = target;
        }
    } else {
        // There's only one possible render target, the default framebuffer.
        // Note: Could check against the default framebuffer value (((R_TARGET_DATA*)target->data)->handle versus result of GL_FRAMEBUFFER_BINDING)...
        renderer->current_context_target->context->active_target = target;
    }
    return true;
}

static_inline void flushAndBindFramebuffer(R_Renderer *renderer, GLuint handle) {
    // Bind the FBO
    FlushBlitBuffer(renderer);

    extBindFramebuffer(renderer, handle);
    renderer->current_context_target->context->active_target = NULL;
}

static_inline void flushBlitBufferIfCurrentTexture(R_Renderer *renderer, R_Image *image) {
    if (image == ((R_CONTEXT_DATA *)renderer->current_context_target->context->data)->last_image) {
        FlushBlitBuffer(renderer);
    }
}

static_inline void flushAndClearBlitBufferIfCurrentTexture(R_Renderer *renderer, R_Image *image) {
    if (image == ((R_CONTEXT_DATA *)renderer->current_context_target->context->data)->last_image) {
        FlushBlitBuffer(renderer);
        ((R_CONTEXT_DATA *)renderer->current_context_target->context->data)->last_image = NULL;
    }
}

static_inline bool isCurrentTarget(R_Renderer *renderer, R_Target *target) {
    return (target == renderer->current_context_target->context->active_target || renderer->current_context_target->context->active_target == NULL);
}

static_inline void flushAndClearBlitBufferIfCurrentFramebuffer(R_Renderer *renderer, R_Target *target) {
    if (target == renderer->current_context_target->context->active_target || renderer->current_context_target->context->active_target == NULL) {
        FlushBlitBuffer(renderer);
        renderer->current_context_target->context->active_target = NULL;
    }
}

bool growBlitBuffer(R_CONTEXT_DATA *cdata, unsigned int minimum_vertices_needed) {
    unsigned int new_max_num_vertices;
    float *new_buffer;

    if (minimum_vertices_needed <= cdata->blit_buffer_max_num_vertices) return true;
    if (cdata->blit_buffer_max_num_vertices == R_BLIT_BUFFER_ABSOLUTE_MAX_VERTICES) return false;

    // Calculate new size (in vertices)
    new_max_num_vertices = ((unsigned int)cdata->blit_buffer_max_num_vertices) * 2;
    while (new_max_num_vertices <= minimum_vertices_needed) new_max_num_vertices *= 2;

    if (new_max_num_vertices > R_BLIT_BUFFER_ABSOLUTE_MAX_VERTICES) new_max_num_vertices = R_BLIT_BUFFER_ABSOLUTE_MAX_VERTICES;

    // R_LogError("Growing to %d vertices\n", new_max_num_vertices);
    //  Resize the blit buffer
    new_buffer = (float *)ME_MALLOC(new_max_num_vertices * R_BLIT_BUFFER_STRIDE);
    memcpy(new_buffer, cdata->blit_buffer, cdata->blit_buffer_num_vertices * R_BLIT_BUFFER_STRIDE);
    ME_FREE(cdata->blit_buffer);
    cdata->blit_buffer = new_buffer;
    cdata->blit_buffer_max_num_vertices = (unsigned short)new_max_num_vertices;

#ifdef R_USE_BUFFER_PIPELINE
// Resize the VBOs
#if !defined(R_NO_VAO)
    glBindVertexArray(cdata->blit_VAO);
#endif

    glBindBuffer(GL_ARRAY_BUFFER, cdata->blit_VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, R_BLIT_BUFFER_STRIDE * cdata->blit_buffer_max_num_vertices, NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, cdata->blit_VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, R_BLIT_BUFFER_STRIDE * cdata->blit_buffer_max_num_vertices, NULL, GL_STREAM_DRAW);

#if !defined(R_NO_VAO)
    glBindVertexArray(0);
#endif
#endif

    return true;
}

bool growIndexBuffer(R_CONTEXT_DATA *cdata, unsigned int minimum_vertices_needed) {
    unsigned int new_max_num_vertices;
    unsigned short *new_indices;

    if (minimum_vertices_needed <= cdata->index_buffer_max_num_vertices) return true;
    if (cdata->index_buffer_max_num_vertices == R_INDEX_BUFFER_ABSOLUTE_MAX_VERTICES) return false;

    // Calculate new size (in vertices)
    new_max_num_vertices = cdata->index_buffer_max_num_vertices * 2;
    while (new_max_num_vertices <= minimum_vertices_needed) new_max_num_vertices *= 2;

    if (new_max_num_vertices > R_INDEX_BUFFER_ABSOLUTE_MAX_VERTICES) new_max_num_vertices = R_INDEX_BUFFER_ABSOLUTE_MAX_VERTICES;

    // R_LogError("Growing to %d indices\n", new_max_num_vertices);
    //  Resize the index buffer
    new_indices = (unsigned short *)ME_MALLOC(new_max_num_vertices * sizeof(unsigned short));
    memcpy(new_indices, cdata->index_buffer, cdata->index_buffer_num_vertices * sizeof(unsigned short));
    ME_FREE(cdata->index_buffer);
    cdata->index_buffer = new_indices;
    cdata->index_buffer_max_num_vertices = new_max_num_vertices;

#ifdef R_USE_BUFFER_PIPELINE
// Resize the IBO
#if !defined(R_NO_VAO)
    glBindVertexArray(cdata->blit_VAO);
#endif

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cdata->blit_IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * cdata->index_buffer_max_num_vertices, NULL, GL_DYNAMIC_DRAW);

#if !defined(R_NO_VAO)
    glBindVertexArray(0);
#endif
#endif

    return true;
}

void setClipRect(R_Renderer *renderer, R_Target *target) {
    if (target->use_clip_rect) {
        R_Target *context_target = renderer->current_context_target;
        glEnable(GL_SCISSOR_TEST);
        if (target->context != NULL) {
            float y;
            if (renderer->coordinate_mode == 0)
                y = context_target->h - (target->clip_rect.y + target->clip_rect.h);
            else
                y = target->clip_rect.y;
            float xFactor = ((float)context_target->context->drawable_w) / context_target->w;
            float yFactor = ((float)context_target->context->drawable_h) / context_target->h;
            glScissor((GLint)(target->clip_rect.x * xFactor), (GLint)(y * yFactor), (GLsizei)(target->clip_rect.w * xFactor), (GLsizei)(target->clip_rect.h * yFactor));
        } else
            glScissor((GLint)target->clip_rect.x, (GLint)target->clip_rect.y, (GLsizei)target->clip_rect.w, (GLsizei)target->clip_rect.h);
    }
}

void unsetClipRect(R_Renderer *renderer, R_Target *target) {
    (void)renderer;
    if (target->use_clip_rect) glDisable(GL_SCISSOR_TEST);
}

void changeDepthTest(R_Renderer *renderer, bool enable) {
    R_CONTEXT_DATA *cdata = (R_CONTEXT_DATA *)renderer->current_context_target->context->data;
    if (cdata->last_depth_test == enable) return;

    cdata->last_depth_test = enable;
    if (enable)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    // glEnable(GL_ALPHA_TEST);
}

void changeDepthWrite(R_Renderer *renderer, bool enable) {
    R_CONTEXT_DATA *cdata = (R_CONTEXT_DATA *)renderer->current_context_target->context->data;
    if (cdata->last_depth_write == enable) return;

    cdata->last_depth_write = enable;
    glDepthMask(enable);
}

void changeDepthFunction(R_Renderer *renderer, R_ComparisonEnum compare_operation) {
    R_CONTEXT_DATA *cdata = (R_CONTEXT_DATA *)renderer->current_context_target->context->data;
    if (cdata->last_depth_function == compare_operation) return;

    cdata->last_depth_function = compare_operation;
    glDepthFunc(compare_operation);
}

void prepareToRenderToTarget(R_Renderer *renderer, R_Target *target) {
    // Set up the camera
    SetCamera(renderer, target, &target->camera);
    changeDepthTest(renderer, target->use_depth_test);
    changeDepthWrite(renderer, target->use_depth_write);
    changeDepthFunction(renderer, target->depth_function);
}

void changeColor(R_Renderer *renderer, SDL_Color color) {
#ifdef R_USE_BUFFER_PIPELINE
    (void)renderer;
    (void)color;
    return;
#else
    R_CONTEXT_DATA *cdata = (R_CONTEXT_DATA *)renderer->current_context_target->context->data;
    if (cdata->last_color.r != color.r || cdata->last_color.g != color.g || cdata->last_color.b != color.b || GET_ALPHA(cdata->last_color) != GET_ALPHA(color)) {
        FlushBlitBuffer(renderer);
        cdata->last_color = color;
        glColor4f(color.r / 255.01f, color.g / 255.01f, color.b / 255.01f, GET_ALPHA(color) / 255.01f);
    }
#endif
}

void changeBlending(R_Renderer *renderer, bool enable) {
    R_CONTEXT_DATA *cdata = (R_CONTEXT_DATA *)renderer->current_context_target->context->data;
    if (cdata->last_use_blending == enable) return;

    FlushBlitBuffer(renderer);

    if (enable)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    cdata->last_use_blending = enable;
}

void forceChangeBlendMode(R_Renderer *renderer, R_BlendMode mode) {
    R_CONTEXT_DATA *cdata = (R_CONTEXT_DATA *)renderer->current_context_target->context->data;

    FlushBlitBuffer(renderer);

    cdata->last_blend_mode = mode;

    if (mode.source_color == mode.source_alpha && mode.dest_color == mode.dest_alpha) {
        glBlendFunc(mode.source_color, mode.dest_color);
    } else if (renderer->enabled_features & R_FEATURE_BLEND_FUNC_SEPARATE) {
        glBlendFuncSeparate(mode.source_color, mode.dest_color, mode.source_alpha, mode.dest_alpha);
    } else {
        R_PushErrorCode("(SDL_gpu internal)", R_ERROR_BACKEND_ERROR,
                        "Could not set blend function because "
                        "R_FEATURE_BLEND_FUNC_SEPARATE is not supported.");
    }

    if (renderer->enabled_features & R_FEATURE_BLEND_EQUATIONS) {
        if (mode.color_equation == mode.alpha_equation)
            glBlendEquation(mode.color_equation);
        else if (renderer->enabled_features & R_FEATURE_BLEND_EQUATIONS_SEPARATE)
            glBlendEquationSeparate(mode.color_equation, mode.alpha_equation);
        else {
            R_PushErrorCode("(SDL_gpu internal)", R_ERROR_BACKEND_ERROR,
                            "Could not set blend equation because "
                            "R_FEATURE_BLEND_EQUATIONS_SEPARATE is not supported.");
        }
    } else {
        R_PushErrorCode("(SDL_gpu internal)", R_ERROR_BACKEND_ERROR,
                        "Could not set blend equation because R_FEATURE_BLEND_EQUATIONS is "
                        "not supported.");
    }
}

void changeBlendMode(R_Renderer *renderer, R_BlendMode mode) {
    R_CONTEXT_DATA *cdata = (R_CONTEXT_DATA *)renderer->current_context_target->context->data;
    if (cdata->last_blend_mode.source_color == mode.source_color && cdata->last_blend_mode.dest_color == mode.dest_color && cdata->last_blend_mode.source_alpha == mode.source_alpha &&
        cdata->last_blend_mode.dest_alpha == mode.dest_alpha && cdata->last_blend_mode.color_equation == mode.color_equation && cdata->last_blend_mode.alpha_equation == mode.alpha_equation)
        return;

    forceChangeBlendMode(renderer, mode);
}

// If 0 is returned, there is no valid shader.
u32 get_proper_program_id(R_Renderer *renderer, u32 program_object) {
    R_Context *context = renderer->current_context_target->context;
    if (context->default_textured_shader_program == 0)  // No shaders loaded!
        return 0;

    if (program_object == 0) return context->default_textured_shader_program;

    return program_object;
}

void applyTexturing(R_Renderer *renderer) {
    R_Context *context = renderer->current_context_target->context;
    if (context->use_texturing != ((R_CONTEXT_DATA *)context->data)->last_use_texturing) {
        ((R_CONTEXT_DATA *)context->data)->last_use_texturing = context->use_texturing;
#ifndef R_SKIP_ENABLE_TEXTURE_2D
        if (context->use_texturing)
            glEnable(GL_TEXTURE_2D);
        else
            glDisable(GL_TEXTURE_2D);
#endif
    }
}

void changeTexturing(R_Renderer *renderer, bool enable) {
    R_Context *context = renderer->current_context_target->context;
    if (enable != ((R_CONTEXT_DATA *)context->data)->last_use_texturing) {
        FlushBlitBuffer(renderer);

        ((R_CONTEXT_DATA *)context->data)->last_use_texturing = enable;
#ifndef R_SKIP_ENABLE_TEXTURE_2D
        if (enable)
            glEnable(GL_TEXTURE_2D);
        else
            glDisable(GL_TEXTURE_2D);
#endif
    }
}

void enableTexturing(R_Renderer *renderer) {
    if (!renderer->current_context_target->context->use_texturing) {
        FlushBlitBuffer(renderer);
        renderer->current_context_target->context->use_texturing = 1;
    }
}

void disableTexturing(R_Renderer *renderer) {
    if (renderer->current_context_target->context->use_texturing) {
        FlushBlitBuffer(renderer);
        renderer->current_context_target->context->use_texturing = 0;
    }
}

#define MIX_COLOR_COMPONENT_NORMALIZED_RESULT(a, b) ((a) / 255.0f * (b) / 255.0f)
#define MIX_COLOR_COMPONENT(a, b) ((u8)(((a) / 255.0f * (b) / 255.0f) * 255))

SDL_Color get_complete_mod_color(R_Renderer *renderer, R_Target *target, R_Image *image) {
    (void)renderer;
    SDL_Color color = {255, 255, 255, 255};
    if (target->use_color) {
        if (image != NULL) {
            color.r = MIX_COLOR_COMPONENT(target->color.r, image->color.r);
            color.g = MIX_COLOR_COMPONENT(target->color.g, image->color.g);
            color.b = MIX_COLOR_COMPONENT(target->color.b, image->color.b);
            GET_ALPHA(color) = MIX_COLOR_COMPONENT(GET_ALPHA(target->color), GET_ALPHA(image->color));
        } else {
            color = ToSDLColor(target->color);
        }

        return color;
    } else if (image != NULL)
        return ToSDLColor(image->color);
    else
        return color;
}

void prepareToRenderImage(R_Renderer *renderer, R_Target *target, R_Image *image) {
    R_Context *context = renderer->current_context_target->context;

    enableTexturing(renderer);
    if (GL_TRIANGLES != ((R_CONTEXT_DATA *)context->data)->last_shape) {
        FlushBlitBuffer(renderer);
        ((R_CONTEXT_DATA *)context->data)->last_shape = GL_TRIANGLES;
    }

    // Blitting
    changeColor(renderer, get_complete_mod_color(renderer, target, image));
    changeBlending(renderer, image->use_blending);
    changeBlendMode(renderer, image->blend_mode);

    // If we're using the untextured shader, switch it.
    if (context->current_shader_program == context->default_untextured_shader_program) ActivateShaderProgram(renderer, context->default_textured_shader_program, NULL);
}

void prepareToRenderShapes(R_Renderer *renderer, unsigned int shape) {
    R_Context *context = renderer->current_context_target->context;

    disableTexturing(renderer);
    if (shape != ((R_CONTEXT_DATA *)context->data)->last_shape) {
        FlushBlitBuffer(renderer);
        ((R_CONTEXT_DATA *)context->data)->last_shape = shape;
    }

    // Shape rendering
    // Color is set elsewhere for shapes
    changeBlending(renderer, context->shapes_use_blending);
    changeBlendMode(renderer, context->shapes_blend_mode);

    // If we're using the textured shader, switch it.
    if (context->current_shader_program == context->default_textured_shader_program) ActivateShaderProgram(renderer, context->default_untextured_shader_program, NULL);
}

void forceChangeViewport(R_Target *target, MErect viewport) {
    float y;
    R_CONTEXT_DATA *cdata = (R_CONTEXT_DATA *)(R_GetContextTarget()->context->data);

    cdata->last_viewport = viewport;

    y = viewport.y;
    if (R_GetCoordinateMode() == 0) {
        // Need the real height to flip the y-coord (from OpenGL coord system)
        if (target->image != NULL)
            y = target->image->texture_h - viewport.h - viewport.y;
        else if (target->context != NULL)
            y = target->context->drawable_h - viewport.h - viewport.y;
    }

    glViewport((GLint)viewport.x, (GLint)y, (GLsizei)viewport.w, (GLsizei)viewport.h);
}

void changeViewport(R_Target *target) {
    R_CONTEXT_DATA *cdata = (R_CONTEXT_DATA *)(R_GetContextTarget()->context->data);

    if (cdata->last_viewport.x == target->viewport.x && cdata->last_viewport.y == target->viewport.y && cdata->last_viewport.w == target->viewport.w && cdata->last_viewport.h == target->viewport.h)
        return;

    forceChangeViewport(target, target->viewport);
}

void applyTargetCamera(R_Target *target) {
    R_CONTEXT_DATA *cdata = (R_CONTEXT_DATA *)R_GetContextTarget()->context->data;

    cdata->last_camera = target->camera;
    cdata->last_camera_inverted = (target->image != NULL);
}

bool equal_cameras(R_Camera a, R_Camera b) {
    return (a.x == b.x && a.y == b.y && a.z == b.z && a.angle == b.angle && a.zoom_x == b.zoom_x && a.zoom_y == b.zoom_y && a.use_centered_origin == b.use_centered_origin);
}

void changeCamera(R_Target *target) {
    // R_CONTEXT_DATA* cdata = (R_CONTEXT_DATA*)R_GetContextTarget()->context->data;

    // if(cdata->last_camera_target != target || !equal_cameras(cdata->last_camera, target->camera))
    { applyTargetCamera(target); }
}

void get_camera_matrix(R_Target *target, float *result) {
    float offsetX, offsetY;

    R_MatrixIdentity(result);

    R_MatrixTranslate(result, -target->camera.x, -target->camera.y, -target->camera.z);

    if (target->camera.use_centered_origin) {
        offsetX = target->w / 2.0f;
        offsetY = target->h / 2.0f;
        R_MatrixTranslate(result, offsetX, offsetY, 0);
    }

    R_MatrixRotate(result, target->camera.angle, 0, 0, 1);
    R_MatrixScale(result, target->camera.zoom_x, target->camera.zoom_y, 1.0f);

    if (target->camera.use_centered_origin) R_MatrixTranslate(result, -offsetX, -offsetY, 0);
}

R_Target *Init(R_Renderer *renderer, R_RendererID renderer_request, u16 w, u16 h, R_WindowFlagEnum SDL_flags) {
    R_InitFlagEnum R_flags;
    SDL_Window *window;

    const char *vendor_string;

    if (renderer_request.major_version < 1) {
        renderer_request.major_version = 1;
        renderer_request.minor_version = 1;
    }

    // Tell SDL what we require for the GL context.
    R_flags = R_GetPreInitFlags();

    renderer->R_init_flags = R_flags;
    if (R_flags & R_INIT_DISABLE_DOUBLE_BUFFER)
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
    else
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // GL profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        0);  // Disable in case this is a fallback renderer

// GL 3.2 and 3.3 have two profile modes
// ARB_compatibility brings support for this to GL 3.1, but glGetStringi() via GLEW has chicken and egg problems.
#if R_GL_MAJOR_VERSION == 3
    if (renderer_request.minor_version >= 2) {
        if (R_flags & R_INIT_REQUEST_COMPATIBILITY_PROFILE)
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
        else {
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            // Force newer default shader version for core contexts because they don't support lower versions
            renderer->min_shader_version = R_GLSL_VERSION_CORE;
            if (renderer->min_shader_version > renderer->max_shader_version) renderer->max_shader_version = R_GLSL_VERSION_CORE;
        }
    }
#endif

    // GL version
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, renderer_request.major_version);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, renderer_request.minor_version);

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 1);  // SurfaceUI _REQUIRES_ a stencil buffer

    renderer->requested_id = renderer_request;

    window = NULL;
    // Is there a window already set up that we are supposed to use?
    if (renderer->current_context_target != NULL)
        window = SDL_GetWindowFromID(renderer->current_context_target->context->windowID);
    else
        window = SDL_GetWindowFromID(R_GetInitWindow());

    if (window == NULL) {
        int win_w, win_h;
        win_w = w;
        win_h = h;

        // Set up window flags
        SDL_flags |= SDL_WINDOW_OPENGL;
        if (!(SDL_flags & SDL_WINDOW_HIDDEN)) SDL_flags |= SDL_WINDOW_SHOWN;

        renderer->SDL_init_flags = SDL_flags;
        window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w, win_h, SDL_flags);

        if (window == NULL) {
            R_PushErrorCode("R_Init", R_ERROR_BACKEND_ERROR, "Window creation failed.");
            return NULL;
        }

        R_SetInitWindow(get_window_id(window));
    } else
        renderer->SDL_init_flags = SDL_flags;

    renderer->enabled_features = 0xFFFFFFFF;  // Pretend to support them all if using incompatible headers

    // Create or re-init the current target.  This also creates the GL context and initializes enabled_features.
    if (CreateTargetFromWindow(renderer, get_window_id(window), renderer->current_context_target) == NULL) return NULL;

    // If the dimensions of the window don't match what we asked for, then set up a virtual resolution to pretend like they are.
    if (!(R_flags & R_INIT_DISABLE_AUTO_VIRTUAL_RESOLUTION) && w != 0 && h != 0 && (w != renderer->current_context_target->w || h != renderer->current_context_target->h)) {
        SetVirtualResolution(renderer, renderer->current_context_target, w, h);
    }

    // Init glVertexAttrib workaround
    vendor_string = (const char *)glGetString(GL_VENDOR);
    if (strstr(vendor_string, "Intel") != NULL) {
        vendor_is_Intel = 1;
        apply_Intel_attrib_workaround = 1;
    }

    return renderer->current_context_target;
}

bool IsFeatureEnabled(R_Renderer *renderer, R_FeatureEnum feature) { return ((renderer->enabled_features & feature) == feature); }

bool get_GL_version(int *major, int *minor) {
    const char *version_string;

    // OpenGL < 3.0 doesn't have GL_MAJOR_VERSION.  Check via version string instead.
    version_string = (const char *)glGetString(GL_VERSION);
    if (version_string == NULL || sscanf(version_string, "%d.%d", major, minor) <= 0) {
        // Failure
        *major = R_GL_MAJOR_VERSION;
#if R_GL_MAJOR_VERSION != 3
        *minor = 1;
#else
        *minor = 0;
#endif

        R_PushErrorCode(__func__, R_ERROR_BACKEND_ERROR, "Failed to parse OpenGL version string: \"%s\"", version_string);
        return false;
    }
    return true;
}

bool get_GLSL_version(int *version) {
    const char *version_string;
    int major, minor;
    {
        version_string = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
        if (version_string == NULL || sscanf(version_string, "%d.%d", &major, &minor) <= 0) {
            R_PushErrorCode(__func__, R_ERROR_BACKEND_ERROR, "Failed to parse GLSL version string: \"%s\"", version_string);
            *version = R_GLSL_VERSION;
            return false;
        } else
            *version = major * 100 + minor;
    }
    return true;
}

bool get_API_versions(R_Renderer *renderer) { return (get_GL_version(&renderer->id.major_version, &renderer->id.minor_version) && get_GLSL_version(&renderer->max_shader_version)); }

void update_stored_dimensions(R_Target *target) {
    bool is_fullscreen;
    SDL_Window *window;

    if (target->context == NULL) return;

    window = get_window(target->context->windowID);
    get_window_dimensions(window, &target->context->window_w, &target->context->window_h);
    is_fullscreen = get_fullscreen_state(window);

    if (!is_fullscreen) {
        target->context->stored_window_w = target->context->window_w;
        target->context->stored_window_h = target->context->window_h;
    }
}

R_Target *CreateTargetFromWindow(R_Renderer *renderer, u32 windowID, R_Target *target) {
    bool created = false;  // Make a new one or repurpose an existing target?
    R_CONTEXT_DATA *cdata;
    SDL_Window *window;

    int framebuffer_handle;
    SDL_Color white = {255, 255, 255, 255};

    R_FeatureEnum required_features = R_GetRequiredFeatures();

    if (target == NULL) {
        int blit_buffer_storage_size;
        int index_buffer_storage_size;

        created = true;
        target = (R_Target *)ME_MALLOC(sizeof(R_Target));
        memset(target, 0, sizeof(R_Target));
        target->refcount = 1;
        target->is_alias = false;
        target->data = (R_TARGET_DATA *)ME_MALLOC(sizeof(R_TARGET_DATA));
        memset(target->data, 0, sizeof(R_TARGET_DATA));
        ((R_TARGET_DATA *)target->data)->refcount = 1;
        target->image = NULL;
        target->context = (R_Context *)ME_MALLOC(sizeof(R_Context));
        memset(target->context, 0, sizeof(R_Context));
        cdata = (R_CONTEXT_DATA *)ME_MALLOC(sizeof(R_CONTEXT_DATA));
        memset(cdata, 0, sizeof(R_CONTEXT_DATA));

        target->context->refcount = 1;
        target->context->data = cdata;
        target->context->context = NULL;

        cdata->last_image = NULL;
        // Initialize the blit buffer
        cdata->blit_buffer_max_num_vertices = R_BLIT_BUFFER_INIT_MAX_NUM_VERTICES;
        cdata->blit_buffer_num_vertices = 0;
        blit_buffer_storage_size = R_BLIT_BUFFER_INIT_MAX_NUM_VERTICES * R_BLIT_BUFFER_STRIDE;
        cdata->blit_buffer = (float *)ME_MALLOC(blit_buffer_storage_size);
        cdata->index_buffer_max_num_vertices = R_BLIT_BUFFER_INIT_MAX_NUM_VERTICES;
        cdata->index_buffer_num_vertices = 0;
        index_buffer_storage_size = R_BLIT_BUFFER_INIT_MAX_NUM_VERTICES * sizeof(unsigned short);
        cdata->index_buffer = (unsigned short *)ME_MALLOC(index_buffer_storage_size);
    } else {
        R_RemoveWindowMapping(target->context->windowID);
        cdata = (R_CONTEXT_DATA *)target->context->data;
    }

    window = get_window(windowID);
    if (window == NULL) {
        R_PushErrorCode("R_CreateTargetFromWindow", R_ERROR_BACKEND_ERROR, "Failed to acquire the window from the given ID.");
        if (created) {
            ME_FREE(cdata->blit_buffer);
            ME_FREE(cdata->index_buffer);
            ME_FREE(target->context->data);
            ME_FREE(target->context);
            ME_FREE(target->data);
            ME_FREE(target);
        }
        return NULL;
    }

    // Store the window info
    target->context->windowID = get_window_id(window);

    // Make a new context if needed and make it current
    if (created || target->context->context == NULL) {
        target->context->context = SDL_GL_CreateContext(window);
        if (target->context->context == NULL) {
            R_PushErrorCode("R_CreateTargetFromWindow", R_ERROR_BACKEND_ERROR, "Failed to create GL context.");
            ME_FREE(cdata->blit_buffer);
            ME_FREE(cdata->index_buffer);
            ME_FREE(target->context->data);
            ME_FREE(target->context);
            ME_FREE(target->data);
            ME_FREE(target);
            return NULL;
        }
        R_AddWindowMapping(target);
    }

    // We need a GL context before we can get the drawable size.
    SDL_GL_GetDrawableSize(window, &target->context->drawable_w, &target->context->drawable_h);

    update_stored_dimensions(target);

    ((R_TARGET_DATA *)target->data)->handle = 0;
    ((R_TARGET_DATA *)target->data)->format = GL_RGBA;

    target->renderer = renderer;
    target->context_target = target;  // This target is a context target
    target->w = (u16)target->context->drawable_w;
    target->h = (u16)target->context->drawable_h;
    target->base_w = (u16)target->context->drawable_w;
    target->base_h = (u16)target->context->drawable_h;

    target->use_clip_rect = false;
    target->clip_rect.x = 0;
    target->clip_rect.y = 0;
    target->clip_rect.w = target->w;
    target->clip_rect.h = target->h;
    target->use_color = false;

    target->viewport = R_MakeRect(0, 0, (float)target->context->drawable_w, (float)target->context->drawable_h);

    target->matrix_mode = R_MODEL;
    R_InitMatrixStack(&target->projection_matrix);
    R_InitMatrixStack(&target->view_matrix);
    R_InitMatrixStack(&target->model_matrix);

    target->camera = R_GetDefaultCamera();
    target->use_camera = true;

    target->use_depth_test = false;
    target->use_depth_write = true;

    target->context->line_thickness = 1.0f;
    target->context->use_texturing = true;
    target->context->shapes_use_blending = true;
    target->context->shapes_blend_mode = R_GetBlendModeFromPreset(R_BLEND_NORMAL);

    cdata->last_color = ToEngineColor(white);

    cdata->last_use_texturing = true;
    cdata->last_shape = GL_TRIANGLES;

    cdata->last_use_blending = false;
    cdata->last_blend_mode = R_GetBlendModeFromPreset(R_BLEND_NORMAL);

    cdata->last_viewport = target->viewport;
    cdata->last_camera = target->camera;  // Redundant due to applyTargetCamera(), below
    cdata->last_camera_inverted = false;

    cdata->last_depth_test = false;
    cdata->last_depth_write = true;

    if (!gladLoadGL()) {
        // Probably don't have the right GL version for this renderer
        R_PushErrorCode("R_CreateTargetFromWindow", R_ERROR_BACKEND_ERROR, "Failed to initialize extensions for renderer %s.", renderer->id.name);
        target->context->failed = true;
        return NULL;
    }

    MakeCurrent(renderer, target, target->context->windowID);

    framebuffer_handle = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer_handle);
    ((R_TARGET_DATA *)target->data)->handle = framebuffer_handle;

    // Update our renderer info from the current GL context.
    if (!get_API_versions(renderer)) R_PushErrorCode("R_CreateTargetFromWindow", R_ERROR_BACKEND_ERROR, "Failed to get backend API versions.");

    // Did the wrong runtime library try to use a later versioned renderer?
    if (renderer->id.major_version < renderer->requested_id.major_version) {
        R_PushErrorCode("R_CreateTargetFromWindow", R_ERROR_BACKEND_ERROR,
                        "Renderer major version (%d) is incompatible with the available OpenGL runtime "
                        "library version (%d).",
                        renderer->requested_id.major_version, renderer->id.major_version);
        target->context->failed = true;
        return NULL;
    }

    init_features(renderer);

    if (!IsFeatureEnabled(renderer, required_features)) {
        R_PushErrorCode("R_CreateTargetFromWindow", R_ERROR_BACKEND_ERROR, "Renderer does not support required features.");
        target->context->failed = true;
        return NULL;
    }

    // No preference for vsync?
    if (!(renderer->R_init_flags & (R_INIT_DISABLE_VSYNC | R_INIT_ENABLE_VSYNC))) {
        // Default to late swap vsync if available
        if (SDL_GL_SetSwapInterval(-1) < 0) SDL_GL_SetSwapInterval(1);  // Or go for vsync
    } else if (renderer->R_init_flags & R_INIT_ENABLE_VSYNC)
        SDL_GL_SetSwapInterval(1);
    else if (renderer->R_init_flags & R_INIT_DISABLE_VSYNC)
        SDL_GL_SetSwapInterval(0);

    // Set fallback texture upload method
    if (renderer->R_init_flags & R_INIT_USE_COPY_TEXTURE_UPLOAD_FALLBACK)
        slow_upload_texture = copy_upload_texture;
    else
        slow_upload_texture = row_upload_texture;

// Set up GL state

// Modes
#ifndef R_SKIP_ENABLE_TEXTURE_2D
    glEnable(GL_TEXTURE_2D);
#endif
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Viewport and Framebuffer
    glViewport(0, 0, (GLsizei)target->viewport.w, (GLsizei)target->viewport.h);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up camera
    applyTargetCamera(target);

    // Set up default projection matrix
    R_ResetProjection(target);

    SetLineThickness(renderer, 1.0f);

#ifdef R_USE_BUFFER_PIPELINE
// Create vertex array container and buffer
#if !defined(R_NO_VAO)
    glGenVertexArrays(1, &cdata->blit_VAO);
    glBindVertexArray(cdata->blit_VAO);
#endif
#endif

    target->context->default_textured_shader_program = 0;
    target->context->default_untextured_shader_program = 0;
    target->context->current_shader_program = 0;

    // Load default shaders

    if (IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) {
        u32 v, f, p;
        const char *textured_vertex_shader_source = R_DEFAULT_TEXTURED_VERTEX_SHADER_SOURCE;
        const char *textured_fragment_shader_source = R_DEFAULT_TEXTURED_FRAGMENT_SHADER_SOURCE;
        const char *untextured_vertex_shader_source = R_DEFAULT_UNTEXTURED_VERTEX_SHADER_SOURCE;
        const char *untextured_fragment_shader_source = R_DEFAULT_UNTEXTURED_FRAGMENT_SHADER_SOURCE;

        // Textured shader
        v = CompileShader(renderer, R_VERTEX_SHADER, textured_vertex_shader_source);

        if (!v) {
            R_PushErrorCode("R_CreateTargetFromWindow", R_ERROR_BACKEND_ERROR, "Failed to load default textured vertex shader: %s.", R_GetShaderMessage());
            target->context->failed = true;
            return NULL;
        }

        f = CompileShader(renderer, R_FRAGMENT_SHADER, textured_fragment_shader_source);

        if (!f) {
            R_PushErrorCode("R_CreateTargetFromWindow", R_ERROR_BACKEND_ERROR, "Failed to load default textured fragment shader: %s.", R_GetShaderMessage());
            target->context->failed = true;
            return NULL;
        }

        p = CreateShaderProgram(renderer);
        AttachShader(renderer, p, v);
        AttachShader(renderer, p, f);
        LinkShaderProgram(renderer, p);

        if (!p) {
            R_PushErrorCode("R_CreateTargetFromWindow", R_ERROR_BACKEND_ERROR, "Failed to link default textured shader program: %s.", R_GetShaderMessage());
            target->context->failed = true;
            return NULL;
        }

        target->context->default_textured_vertex_shader_id = v;
        target->context->default_textured_fragment_shader_id = f;
        target->context->default_textured_shader_program = p;

        // Get locations of the attributes in the shader
        target->context->default_textured_shader_block = R_LoadShaderBlock(p, "gpu_Vertex", "gpu_TexCoord", "gpu_Color", "gpu_ModelViewProjectionMatrix");

        // Untextured shader
        v = CompileShader(renderer, R_VERTEX_SHADER, untextured_vertex_shader_source);

        if (!v) {
            R_PushErrorCode("R_CreateTargetFromWindow", R_ERROR_BACKEND_ERROR, "Failed to load default untextured vertex shader: %s.", R_GetShaderMessage());
            target->context->failed = true;
            return NULL;
        }

        f = CompileShader(renderer, R_FRAGMENT_SHADER, untextured_fragment_shader_source);

        if (!f) {
            R_PushErrorCode("R_CreateTargetFromWindow", R_ERROR_BACKEND_ERROR, "Failed to load default untextured fragment shader: %s.", R_GetShaderMessage());
            target->context->failed = true;
            return NULL;
        }

        p = CreateShaderProgram(renderer);
        AttachShader(renderer, p, v);
        AttachShader(renderer, p, f);
        LinkShaderProgram(renderer, p);

        if (!p) {
            R_PushErrorCode("R_CreateTargetFromWindow", R_ERROR_BACKEND_ERROR, "Failed to link default untextured shader program: %s.", R_GetShaderMessage());
            target->context->failed = true;
            return NULL;
        }

        glUseProgram(p);

        target->context->default_untextured_vertex_shader_id = v;
        target->context->default_untextured_fragment_shader_id = f;
        target->context->default_untextured_shader_program = target->context->current_shader_program = p;

        // Get locations of the attributes in the shader
        target->context->default_untextured_shader_block = R_LoadShaderBlock(p, "gpu_Vertex", NULL, "gpu_Color", "gpu_ModelViewProjectionMatrix");
        R_SetShaderBlock(target->context->default_untextured_shader_block);

    } else {
        snprintf(shader_message, 256, "Shaders not supported by this hardware.  Default shaders are disabled.\n");
        target->context->default_untextured_shader_program = target->context->current_shader_program = 0;
    }

#ifdef R_USE_BUFFER_PIPELINE
    // Create vertex array container and buffer

    glGenBuffers(2, cdata->blit_VBO);
    // Create space on the GPU
    glBindBuffer(GL_ARRAY_BUFFER, cdata->blit_VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, R_BLIT_BUFFER_STRIDE * cdata->blit_buffer_max_num_vertices, NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, cdata->blit_VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, R_BLIT_BUFFER_STRIDE * cdata->blit_buffer_max_num_vertices, NULL, GL_STREAM_DRAW);
    cdata->blit_VBO_flop = false;

    glGenBuffers(1, &cdata->blit_IBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cdata->blit_IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * cdata->blit_buffer_max_num_vertices, NULL, GL_DYNAMIC_DRAW);

    glGenBuffers(16, cdata->attribute_VBO);

    // Init 16 attributes to 0 / NULL.
    memset(cdata->shader_attributes, 0, 16 * sizeof(R_AttributeSource));
#endif

    return target;
}

R_Target *CreateAliasTarget(R_Renderer *renderer, R_Target *target) {
    R_Target *result;
    (void)renderer;

    if (target == NULL) return NULL;

    result = (R_Target *)ME_MALLOC(sizeof(R_Target));

    // Copy the members
    *result = *target;

    // Deep copies
    result->projection_matrix.matrix = NULL;
    result->view_matrix.matrix = NULL;
    result->model_matrix.matrix = NULL;
    result->projection_matrix.size = result->projection_matrix.storage_size = 0;
    result->view_matrix.size = result->view_matrix.storage_size = 0;
    result->model_matrix.size = result->model_matrix.storage_size = 0;
    R_CopyMatrixStack(&target->projection_matrix, &result->projection_matrix);
    R_CopyMatrixStack(&target->view_matrix, &result->view_matrix);
    R_CopyMatrixStack(&target->model_matrix, &result->model_matrix);

    // Alias info
    if (target->image != NULL) target->image->refcount++;
    if (target->context != NULL) target->context->refcount++;
    ((R_TARGET_DATA *)target->data)->refcount++;
    result->refcount = 1;
    result->is_alias = true;

    return result;
}

void MakeCurrent(R_Renderer *renderer, R_Target *target, u32 windowID) {
    SDL_Window *window;

    if (target == NULL || target->context == NULL) return;

    if (target->image != NULL) return;

    if (target->context->context != NULL) {
        renderer->current_context_target = target;

        SDL_GL_MakeCurrent(SDL_GetWindowFromID(windowID), target->context->context);

        // Reset window mapping, base size, and camera if the target's window was changed
        if (target->context->windowID != windowID) {
            FlushBlitBuffer(renderer);

            // Update the window mappings
            R_RemoveWindowMapping(windowID);
            // Don't remove the target's current mapping.  That lets other windows refer to it.
            target->context->windowID = windowID;
            R_AddWindowMapping(target);

            // Update target's window size
            window = get_window(windowID);
            if (window != NULL) {
                get_window_dimensions(window, &target->context->window_w, &target->context->window_h);
                get_drawable_dimensions(window, &target->context->drawable_w, &target->context->drawable_h);
                target->base_w = (u16)target->context->drawable_w;
                target->base_h = (u16)target->context->drawable_h;
            }

            // Reset the camera for this window
            applyTargetCamera(renderer->current_context_target->context->active_target);
        }
    }
}

void SetAsCurrent(R_Renderer *renderer) {
    if (renderer->current_context_target == NULL) return;

    MakeCurrent(renderer, renderer->current_context_target, renderer->current_context_target->context->windowID);
}

void ResetRendererState(R_Renderer *renderer) {
    R_Target *target;
    R_CONTEXT_DATA *cdata;

    if (renderer->current_context_target == NULL) return;

    target = renderer->current_context_target;
    cdata = (R_CONTEXT_DATA *)target->context->data;

    if (IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) glUseProgram(target->context->current_shader_program);

    SDL_GL_MakeCurrent(SDL_GetWindowFromID(target->context->windowID), target->context->context);

#ifndef R_USE_BUFFER_PIPELINE
    glColor4f(cdata->last_color.r / 255.01f, cdata->last_color.g / 255.01f, cdata->last_color.b / 255.01f, GET_ALPHA(cdata->last_color) / 255.01f);
#endif
#ifndef R_SKIP_ENABLE_TEXTURE_2D
    if (cdata->last_use_texturing)
        glEnable(GL_TEXTURE_2D);
    else
        glDisable(GL_TEXTURE_2D);
#endif

    if (cdata->last_use_blending)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);

    forceChangeBlendMode(renderer, cdata->last_blend_mode);

    if (cdata->last_depth_test)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    glDepthMask(cdata->last_depth_write);

    forceChangeViewport(target, target->viewport);

    if (cdata->last_image != NULL) glBindTexture(GL_TEXTURE_2D, ((R_IMAGE_DATA *)(cdata->last_image)->data)->handle);

    if (target->context->active_target != NULL)
        extBindFramebuffer(renderer, ((R_TARGET_DATA *)target->context->active_target->data)->handle);
    else
        extBindFramebuffer(renderer, ((R_TARGET_DATA *)target->data)->handle);
}

bool AddDepthBuffer(R_Renderer *renderer, R_Target *target) {

    GLuint depth_buffer;
    GLenum status;
    R_CONTEXT_DATA *cdata;

    if (renderer->current_context_target == NULL) {
        R_PushErrorCode("R_AddDepthBuffer", R_ERROR_BACKEND_ERROR, "NULL context.");
        return false;
    }

    if (isCurrentTarget(renderer, target)) FlushBlitBuffer(renderer);

    if (!SetActiveTarget(renderer, target)) {
        R_PushErrorCode("R_AddDepthBuffer", R_ERROR_BACKEND_ERROR, "Failed to bind target framebuffer.");
        return false;
    }

    glGenRenderbuffers(1, &depth_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, target->base_w, target->base_h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

    status = glCheckFramebufferStatusPROC(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        R_PushErrorCode("R_AddDepthBuffer", R_ERROR_BACKEND_ERROR, "Failed to attach depth buffer to target.");
        return false;
    }

    cdata = (R_CONTEXT_DATA *)renderer->current_context_target->context->data;
    cdata->last_depth_write = target->use_depth_write;
    glDepthMask(target->use_depth_write);

    R_SetDepthTest(target, 1);

    return true;
}

bool SetWindowResolution(R_Renderer *renderer, u16 w, u16 h) {
    R_Target *target = renderer->current_context_target;

    bool isCurrent = isCurrentTarget(renderer, target);
    if (isCurrent) FlushBlitBuffer(renderer);

    // Don't need to resize (only update internals) when resolution isn't changing.
    get_target_window_dimensions(target, &target->context->window_w, &target->context->window_h);
    get_target_drawable_dimensions(target, &target->context->drawable_w, &target->context->drawable_h);
    if (target->context->window_w != w || target->context->window_h != h) {
        resize_window(target, w, h);
        get_target_window_dimensions(target, &target->context->window_w, &target->context->window_h);
        get_target_drawable_dimensions(target, &target->context->drawable_w, &target->context->drawable_h);
    }

#ifdef R_USE_SDL1

    // FIXME: Does the entire GL state need to be reset because the screen was recreated?
    {
        R_Context *context;

        // Reset texturing state
        context = renderer->current_context_target->context;
        context->use_texturing = true;
        ((R_CONTEXT_DATA *)context->data)->last_use_texturing = false;
    }

    // Clear target (no state change)
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

    // Store the resolution for fullscreen_desktop changes
    update_stored_dimensions(target);

    // Update base dimensions
    target->base_w = (u16)target->context->drawable_w;
    target->base_h = (u16)target->context->drawable_h;

    // Resets virtual resolution
    target->w = target->base_w;
    target->h = target->base_h;
    target->using_virtual_resolution = false;

    // Resets viewport
    target->viewport = R_MakeRect(0, 0, target->w, target->h);
    changeViewport(target);

    R_UnsetClip(target);

    if (isCurrent) applyTargetCamera(target);

    R_ResetProjection(target);

    return 1;
}

void SetVirtualResolution(R_Renderer *renderer, R_Target *target, u16 w, u16 h) {
    bool isCurrent;

    if (target == NULL) return;

    isCurrent = isCurrentTarget(renderer, target);
    if (isCurrent) FlushBlitBuffer(renderer);

    target->w = w;
    target->h = h;
    target->using_virtual_resolution = true;

    if (isCurrent) applyTargetCamera(target);

    R_ResetProjection(target);
}

void UnsetVirtualResolution(R_Renderer *renderer, R_Target *target) {
    bool isCurrent;

    if (target == NULL) return;

    isCurrent = isCurrentTarget(renderer, target);
    if (isCurrent) FlushBlitBuffer(renderer);

    target->w = target->base_w;
    target->h = target->base_h;

    target->using_virtual_resolution = false;

    if (isCurrent) applyTargetCamera(target);

    R_ResetProjection(target);
}

void Quit(R_Renderer *renderer) {
    FreeTarget(renderer, renderer->current_context_target);
    renderer->current_context_target = NULL;
}

bool SetFullscreen(R_Renderer *renderer, bool enable_fullscreen, bool use_desktop_resolution) {
    R_Target *target = renderer->current_context_target;

    SDL_Window *window = SDL_GetWindowFromID(target->context->windowID);
    u32 old_flags = SDL_GetWindowFlags(window);
    bool was_fullscreen = (old_flags & SDL_WINDOW_FULLSCREEN);
    bool is_fullscreen = was_fullscreen;

    u32 flags = 0;

    if (enable_fullscreen) {
        if (use_desktop_resolution)
            flags = SDL_WINDOW_FULLSCREEN_DESKTOP;
        else
            flags = SDL_WINDOW_FULLSCREEN;
    }

    if (SDL_SetWindowFullscreen(window, flags) >= 0) {
        flags = SDL_GetWindowFlags(window);
        is_fullscreen = (flags & SDL_WINDOW_FULLSCREEN);

        // If we just went fullscreen, save the original resolution
        // We do this because you can't depend on the resolution to be preserved by SDL
        // SDL_WINDOW_FULLSCREEN_DESKTOP changes the resolution and SDL_WINDOW_FULLSCREEN can change it when a given mode is not available
        if (!was_fullscreen && is_fullscreen) {
            target->context->stored_window_w = target->context->window_w;
            target->context->stored_window_h = target->context->window_h;
        }

        // If we're in windowed mode now and a resolution was stored, restore the original window resolution
        if (was_fullscreen && !is_fullscreen && (target->context->stored_window_w != 0 && target->context->stored_window_h != 0))
            SDL_SetWindowSize(window, target->context->stored_window_w, target->context->stored_window_h);
    }

    if (is_fullscreen != was_fullscreen) {
        // Update window dims
        get_target_window_dimensions(target, &target->context->window_w, &target->context->window_h);
        get_target_drawable_dimensions(target, &target->context->drawable_w, &target->context->drawable_h);

        // If virtual res is not set, we need to update the target dims and reset stuff that no longer is right
        if (!target->using_virtual_resolution) {
            // Update dims
            target->w = (u16)target->context->drawable_w;
            target->h = (u16)target->context->drawable_h;
        }

        // Reset viewport
        target->viewport = R_MakeRect(0, 0, (float)target->context->drawable_w, (float)target->context->drawable_h);
        changeViewport(target);

        // Reset clip
        R_UnsetClip(target);

        // Update camera
        if (isCurrentTarget(renderer, target)) applyTargetCamera(target);
    }

    target->base_w = (u16)target->context->drawable_w;
    target->base_h = (u16)target->context->drawable_h;

    return is_fullscreen;
}

R_Camera SetCamera(R_Renderer *renderer, R_Target *target, R_Camera *cam) {
    R_Camera new_camera;
    R_Camera old_camera;

    if (target == NULL) {
        R_PushErrorCode("R_SetCamera", R_ERROR_NULL_ARGUMENT, "target");
        return R_GetDefaultCamera();
    }

    if (cam == NULL)
        new_camera = R_GetDefaultCamera();
    else
        new_camera = *cam;

    old_camera = target->camera;

    if (!equal_cameras(new_camera, old_camera)) {
        if (isCurrentTarget(renderer, target)) FlushBlitBuffer(renderer);

        target->camera = new_camera;
    }

    return old_camera;
}

GLuint CreateUninitializedTexture(R_Renderer *renderer) {
    GLuint handle;

    glGenTextures(1, &handle);
    if (handle == 0) return 0;

    flushAndBindTexture(renderer, handle);

    // Set the texture's stretching properties
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return handle;
}

R_Image *CreateUninitializedImage(R_Renderer *renderer, u16 w, u16 h, R_FormatEnum format) {
    GLuint handle, num_layers, bytes_per_pixel;
    GLenum gl_format;
    R_Image *result;
    R_IMAGE_DATA *data;
    SDL_Color white = {255, 255, 255, 255};

    switch (format) {
        case R_FORMAT_LUMINANCE:
            gl_format = GL_LUMINANCE;
            num_layers = 1;
            bytes_per_pixel = 1;
            break;
        case R_FORMAT_LUMINANCE_ALPHA:
            gl_format = GL_LUMINANCE_ALPHA;
            num_layers = 1;
            bytes_per_pixel = 2;
            break;
        case R_FORMAT_RGB:
            gl_format = GL_RGB;
            num_layers = 1;
            bytes_per_pixel = 3;
            break;
        case R_FORMAT_RGBA:
            gl_format = GL_RGBA;
            num_layers = 1;
            bytes_per_pixel = 4;
            break;
#ifdef GL_BGR
        case R_FORMAT_BGR:
            gl_format = GL_BGR;
            num_layers = 1;
            bytes_per_pixel = 3;
            break;
#endif
#ifdef GL_BGRA
        case R_FORMAT_BGRA:
            gl_format = GL_BGRA;
            num_layers = 1;
            bytes_per_pixel = 4;
            break;
#endif
#ifdef GL_ABGR
        case R_FORMAT_ABGR:
            gl_format = GL_ABGR;
            num_layers = 1;
            bytes_per_pixel = 4;
            break;
#endif
        case R_FORMAT_ALPHA:
            gl_format = GL_ALPHA;
            num_layers = 1;
            bytes_per_pixel = 1;
            break;
        case R_FORMAT_RG:
            gl_format = GL_RG;
            num_layers = 1;
            bytes_per_pixel = 2;
            break;
        case R_FORMAT_YCbCr420P:
            gl_format = GL_LUMINANCE;
            num_layers = 3;
            bytes_per_pixel = 1;
            break;
        case R_FORMAT_YCbCr422:
            gl_format = GL_LUMINANCE;
            num_layers = 3;
            bytes_per_pixel = 1;
            break;
        default:
            R_PushErrorCode("R_CreateUninitializedImage", R_ERROR_DATA_ERROR, "Unsupported image format (0x%x)", format);
            return NULL;
    }

    if (bytes_per_pixel < 1 || bytes_per_pixel > 4) {
        R_PushErrorCode("R_CreateUninitializedImage", R_ERROR_DATA_ERROR, "Unsupported number of bytes per pixel (%d)", bytes_per_pixel);
        return NULL;
    }

    // Create the underlying texture
    handle = CreateUninitializedTexture(renderer);
    if (handle == 0) {
        R_PushErrorCode("R_CreateUninitializedImage", R_ERROR_BACKEND_ERROR, "Failed to generate a texture handle.");
        return NULL;
    }

    // Create the R_Image
    result = (R_Image *)ME_MALLOC(sizeof(R_Image));
    result->refcount = 1;
    data = (R_IMAGE_DATA *)ME_MALLOC(sizeof(R_IMAGE_DATA));
    data->refcount = 1;
    result->target = NULL;
    result->renderer = renderer;
    result->context_target = renderer->current_context_target;
    result->format = format;
    result->num_layers = num_layers;
    result->bytes_per_pixel = bytes_per_pixel;
    result->has_mipmaps = false;

    result->anchor_x = renderer->default_image_anchor_x;
    result->anchor_y = renderer->default_image_anchor_y;

    result->color = ToEngineColor(white);
    result->use_blending = true;
    result->blend_mode = R_GetBlendModeFromPreset(R_BLEND_NORMAL);
    result->filter_mode = R_FILTER_LINEAR;
    result->snap_mode = R_SNAP_POSITION_AND_DIMENSIONS;
    result->wrap_mode_x = R_WRAP_NONE;
    result->wrap_mode_y = R_WRAP_NONE;

    result->data = data;
    result->is_alias = false;
    data->handle = handle;
    data->owns_handle = true;
    data->format = gl_format;

    result->using_virtual_resolution = false;
    result->w = w;
    result->h = h;
    result->base_w = w;
    result->base_h = h;
    // POT textures will change this later
    result->texture_w = w;
    result->texture_h = h;

    return result;
}

R_Image *CreateImage(R_Renderer *renderer, u16 w, u16 h, R_FormatEnum format) {
    R_Image *result;
    GLenum internal_format;
    static unsigned char *zero_buffer = NULL;
    static unsigned int zero_buffer_size = 0;

    if (format < 1) {
        R_PushErrorCode("R_CreateImage", R_ERROR_DATA_ERROR, "Unsupported image format (0x%x)", format);
        return NULL;
    }

    result = CreateUninitializedImage(renderer, w, h, format);

    if (result == NULL) {
        R_PushErrorCode("R_CreateImage", R_ERROR_BACKEND_ERROR, "Could not create image as requested.");
        return NULL;
    }

    changeTexturing(renderer, true);
    bindTexture(renderer, result);

    internal_format = ((R_IMAGE_DATA *)(result->data))->format;
    w = result->w;
    h = result->h;
    if (!(renderer->enabled_features & R_FEATURE_NON_POWER_OF_TWO)) {
        if (!isPowerOfTwo(w)) w = (u16)getNearestPowerOf2(w);
        if (!isPowerOfTwo(h)) h = (u16)getNearestPowerOf2(h);
    }

    // Initialize texture using a blank buffer
    if (zero_buffer_size < (unsigned int)(w * h * result->bytes_per_pixel)) {
        ME_FREE(zero_buffer);
        zero_buffer_size = w * h * result->bytes_per_pixel;
        zero_buffer = (unsigned char *)ME_MALLOC(zero_buffer_size);
        memset(zero_buffer, 0, zero_buffer_size);
    }

    upload_new_texture(zero_buffer, R_MakeRect(0, 0, w, h), internal_format, 1, w, result->bytes_per_pixel);

    // Tell SDL_gpu what we got (power-of-two requirements have made this change)
    result->texture_w = w;
    result->texture_h = h;

    return result;
}

R_Image *CreateImageUsingTexture(R_Renderer *renderer, R_TextureHandle handle, bool take_ownership) {
#ifdef R_DISABLE_TEXTURE_GETS
    R_PushErrorCode("R_CreateImageUsingTexture", R_ERROR_UNSUPPORTED_FUNCTION, "Renderer %s does not support this function", renderer->id.name);
    return NULL;
#else

    GLint w, h;
    GLuint num_layers, bytes_per_pixel;
    GLint gl_format;
    GLint wrap_s, wrap_t;
    GLint min_filter;

    R_FormatEnum format;
    R_WrapEnum wrap_x, wrap_y;
    R_FilterEnum filter_mode;
    SDL_Color white = {255, 255, 255, 255};

    R_Image *result;
    R_IMAGE_DATA *data;

    flushAndBindTexture(renderer, handle);

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &gl_format);

    switch (gl_format) {
        case GL_LUMINANCE:
            format = R_FORMAT_LUMINANCE;
            num_layers = 1;
            bytes_per_pixel = 1;
            break;
        case GL_LUMINANCE_ALPHA:
            format = R_FORMAT_LUMINANCE_ALPHA;
            num_layers = 1;
            bytes_per_pixel = 2;
            break;
        case GL_RGB:
            format = R_FORMAT_RGB;
            num_layers = 1;
            bytes_per_pixel = 3;
            break;
        case GL_RGBA:
            format = R_FORMAT_RGBA;
            num_layers = 1;
            bytes_per_pixel = 4;
            break;
#ifdef GL_BGR
        case GL_BGR:
            format = R_FORMAT_BGR;
            num_layers = 1;
            bytes_per_pixel = 3;
            break;
#endif
#ifdef GL_BGRA
        case GL_BGRA:
            format = R_FORMAT_BGRA;
            num_layers = 1;
            bytes_per_pixel = 4;
            break;
#endif
#ifdef GL_ABGR
        case GL_ABGR:
            format = R_FORMAT_ABGR;
            num_layers = 1;
            bytes_per_pixel = 4;
            break;
#endif
        case GL_ALPHA:
            format = R_FORMAT_ALPHA;
            num_layers = 1;
            bytes_per_pixel = 1;
            break;
        case GL_RG:
            format = R_FORMAT_RG;
            num_layers = 1;
            bytes_per_pixel = 2;
            break;
        default:
            R_PushErrorCode("R_CreateImageUsingTexture", R_ERROR_DATA_ERROR, "Unsupported GL image format (0x%x)", gl_format);
            return NULL;
    }

    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);

    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_filter);
    // Ignore mag filter...  Maybe the wrong thing to do?

    // Let the user use one that we don't support and pretend that we're okay with that.
    switch (min_filter) {
        case GL_NEAREST:
            filter_mode = R_FILTER_NEAREST;
            break;
        case GL_LINEAR:
        case GL_LINEAR_MIPMAP_NEAREST:
            filter_mode = R_FILTER_LINEAR;
            break;
        case GL_LINEAR_MIPMAP_LINEAR:
            filter_mode = R_FILTER_LINEAR_MIPMAP;
            break;
        default:
            R_PushErrorCode("R_CreateImageUsingTexture", R_ERROR_USER_ERROR, "Unsupported value for GL_TEXTURE_MIN_FILTER (0x%x)", min_filter);
            filter_mode = R_FILTER_LINEAR;
            break;
    }

    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrap_s);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &wrap_t);

    // Let the user use one that we don't support and pretend that we're okay with that.
    switch (wrap_s) {
        case GL_CLAMP_TO_EDGE:
            wrap_x = R_WRAP_NONE;
            break;
        case GL_REPEAT:
            wrap_x = R_WRAP_REPEAT;
            break;
        case GL_MIRRORED_REPEAT:
            wrap_x = R_WRAP_MIRRORED;
            break;
        default:
            R_PushErrorCode("R_CreateImageUsingTexture", R_ERROR_USER_ERROR, "Unsupported value for GL_TEXTURE_WRAP_S (0x%x)", wrap_s);
            wrap_x = R_WRAP_NONE;
            break;
    }

    switch (wrap_t) {
        case GL_CLAMP_TO_EDGE:
            wrap_y = R_WRAP_NONE;
            break;
        case GL_REPEAT:
            wrap_y = R_WRAP_REPEAT;
            break;
        case GL_MIRRORED_REPEAT:
            wrap_y = R_WRAP_MIRRORED;
            break;
        default:
            R_PushErrorCode("R_CreateImageUsingTexture", R_ERROR_USER_ERROR, "Unsupported value for GL_TEXTURE_WRAP_T (0x%x)", wrap_t);
            wrap_y = R_WRAP_NONE;
            break;
    }

    // Finally create the image

    data = (R_IMAGE_DATA *)ME_MALLOC(sizeof(R_IMAGE_DATA));
    data->refcount = 1;
    data->handle = (GLuint)handle;
    data->owns_handle = take_ownership;
    data->format = gl_format;

    result = (R_Image *)ME_MALLOC(sizeof(R_Image));
    result->refcount = 1;
    result->target = NULL;
    result->renderer = renderer;
    result->context_target = renderer->current_context_target;
    result->format = format;
    result->num_layers = num_layers;
    result->bytes_per_pixel = bytes_per_pixel;
    result->has_mipmaps = false;

    result->anchor_x = renderer->default_image_anchor_x;
    result->anchor_y = renderer->default_image_anchor_y;

    result->color = ToEngineColor(white);
    result->use_blending = true;
    result->blend_mode = R_GetBlendModeFromPreset(R_BLEND_NORMAL);
    result->snap_mode = R_SNAP_POSITION_AND_DIMENSIONS;
    result->filter_mode = filter_mode;
    result->wrap_mode_x = wrap_x;
    result->wrap_mode_y = wrap_y;

    result->data = data;
    result->is_alias = false;

    result->using_virtual_resolution = false;
    result->w = (u16)w;
    result->h = (u16)h;

    result->base_w = (u16)w;
    result->base_h = (u16)h;
    result->texture_w = (u16)w;
    result->texture_h = (u16)h;

    return result;
#endif
}

R_Image *CreateAliasImage(R_Renderer *renderer, R_Image *image) {
    R_Image *result;
    (void)renderer;

    if (image == NULL) return NULL;

    result = (R_Image *)ME_MALLOC(sizeof(R_Image));
    // Copy the members
    *result = *image;

    // Alias info
    ((R_IMAGE_DATA *)image->data)->refcount++;
    result->refcount = 1;
    result->is_alias = true;

    return result;
}

bool readTargetPixels(R_Renderer *renderer, R_Target *source, GLint format, GLubyte *pixels) {
    if (source == NULL) return false;

    if (isCurrentTarget(renderer, source)) FlushBlitBuffer(renderer);

    if (SetActiveTarget(renderer, source)) {
        glReadPixels(0, 0, source->base_w, source->base_h, format, GL_UNSIGNED_BYTE, pixels);
        return true;
    }
    return false;
}

bool readImagePixels(R_Renderer *renderer, R_Image *source, GLint format, GLubyte *pixels) {

    if (source == NULL) return false;

    // Bind the texture temporarily
    glBindTexture(GL_TEXTURE_2D, ((R_IMAGE_DATA *)source->data)->handle);
    // Get the data
    glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, pixels);
    // Rebind the last texture
    if (((R_CONTEXT_DATA *)renderer->current_context_target->context->data)->last_image != NULL)
        glBindTexture(GL_TEXTURE_2D, ((R_IMAGE_DATA *)(((R_CONTEXT_DATA *)renderer->current_context_target->context->data)->last_image)->data)->handle);
    return true;
}

unsigned char *getRawTargetData(R_Renderer *renderer, R_Target *target) {
    int bytes_per_pixel;
    unsigned char *data;
    int pitch;
    unsigned char *copy;
    int y;

    if (isCurrentTarget(renderer, target)) FlushBlitBuffer(renderer);

    bytes_per_pixel = 4;
    if (target->image != NULL) bytes_per_pixel = target->image->bytes_per_pixel;
    data = (unsigned char *)ME_MALLOC(target->base_w * target->base_h * bytes_per_pixel);

    // This can take regions of pixels, so using base_w and base_h with an image target should be fine.
    if (!readTargetPixels(renderer, target, ((R_TARGET_DATA *)target->data)->format, data)) {
        ME_FREE(data);
        return NULL;
    }

    // Flip the data vertically (OpenGL framebuffer is read upside down)
    pitch = target->base_w * bytes_per_pixel;
    copy = (unsigned char *)ME_MALLOC(pitch);

    for (y = 0; y < target->base_h / 2; y++) {
        unsigned char *top = &data[target->base_w * y * bytes_per_pixel];
        unsigned char *bottom = &data[target->base_w * (target->base_h - y - 1) * bytes_per_pixel];
        memcpy(copy, top, pitch);
        memcpy(top, bottom, pitch);
        memcpy(bottom, copy, pitch);
    }
    ME_FREE(copy);

    return data;
}

unsigned char *getRawImageData(R_Renderer *renderer, R_Image *image) {
    unsigned char *data;

    if (image->target != NULL && isCurrentTarget(renderer, image->target)) FlushBlitBuffer(renderer);

    data = (unsigned char *)ME_MALLOC(image->texture_w * image->texture_h * image->bytes_per_pixel);

    // FIXME: Sometimes the texture is stored and read in RGBA even when I specify RGB.  getRawImageData() might need to return the stored format or Bpp.
    if (!readImagePixels(renderer, image, ((R_IMAGE_DATA *)image->data)->format, data)) {
        ME_FREE(data);
        return NULL;
    }

    return data;
}

void *CopySurfaceFromTarget(R_Renderer *renderer, R_Target *target) {
    unsigned char *data;
    SDL_Surface *result;
    SDL_PixelFormat *format;

    if (target == NULL) {
        R_PushErrorCode("R_CopySurfaceFromTarget", R_ERROR_NULL_ARGUMENT, "target");
        return NULL;
    }
    if (target->base_w < 1 || target->base_h < 1) {
        R_PushErrorCode("R_CopySurfaceFromTarget", R_ERROR_DATA_ERROR, "Invalid target dimensions (%dx%d)", target->base_w, target->base_h);
        return NULL;
    }

    data = getRawTargetData(renderer, target);

    if (data == NULL) {
        R_PushErrorCode("R_CopySurfaceFromTarget", R_ERROR_BACKEND_ERROR, "Could not retrieve target data.");
        return NULL;
    }

    format = AllocFormat(((R_TARGET_DATA *)target->data)->format);

    result = SDL_CreateRGBSurface(SDL_SWSURFACE, target->base_w, target->base_h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);

    if (result == NULL) {
        R_PushErrorCode("R_CopySurfaceFromTarget", R_ERROR_DATA_ERROR, "Failed to create new %dx%d surface", target->base_w, target->base_h);
        ME_FREE(data);
        return NULL;
    }

    // Copy row-by-row in case the pitch doesn't match
    {
        int i;
        int source_pitch = target->base_w * format->BytesPerPixel;
        for (i = 0; i < target->base_h; ++i) {
            memcpy((u8 *)result->pixels + i * result->pitch, data + source_pitch * i, source_pitch);
        }
    }

    ME_FREE(data);

    FreeFormat(format);
    return result;
}

void *CopySurfaceFromImage(R_Renderer *renderer, R_Image *image) {
    unsigned char *data;
    SDL_Surface *result;
    SDL_PixelFormat *format;
    int w, h;

    if (image == NULL) {
        R_PushErrorCode("R_CopySurfaceFromImage", R_ERROR_NULL_ARGUMENT, "image");
        return NULL;
    }
    if (image->w < 1 || image->h < 1) {
        R_PushErrorCode("R_CopySurfaceFromImage", R_ERROR_DATA_ERROR, "Invalid image dimensions (%dx%d)", image->base_w, image->base_h);
        return NULL;
    }

    // FIXME: Virtual resolutions overwrite the NPOT dimensions when NPOT textures are not supported!
    if (image->using_virtual_resolution) {
        w = image->texture_w;
        h = image->texture_h;
    } else {
        w = image->w;
        h = image->h;
    }
    data = getRawImageData(renderer, image);

    if (data == NULL) {
        R_PushErrorCode("R_CopySurfaceFromImage", R_ERROR_BACKEND_ERROR, "Could not retrieve target data.");
        return NULL;
    }

    format = AllocFormat(((R_IMAGE_DATA *)image->data)->format);

    result = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, format->BitsPerPixel, format->Rmask, format->Gmask, format->Bmask, format->Amask);

    if (result == NULL) {
        R_PushErrorCode("R_CopySurfaceFromImage", R_ERROR_DATA_ERROR, "Failed to create new %dx%d surface", w, h);
        ME_FREE(data);
        return NULL;
    }

    // Copy row-by-row in case the pitch doesn't match
    {
        int i;
        int source_pitch = image->texture_w * format->BytesPerPixel;  // Use the actual texture width to pull from the data
        for (i = 0; i < h; ++i) {
            memcpy((u8 *)result->pixels + i * result->pitch, data + source_pitch * i, result->pitch);
        }
    }

    ME_FREE(data);

    FreeFormat(format);
    return result;
}

// GL_RGB/GL_RGBA and Surface format
int compareFormats(R_Renderer *renderer, GLenum glFormat, SDL_Surface *surface, GLenum *surfaceFormatResult) {
    SDL_PixelFormat *format = surface->format;
    switch (glFormat) {
            // 3-channel formats
        case GL_RGB:
            if (format->BytesPerPixel != 3) return 1;

            // Looks like RGB?  Easy!
            if (format->Rmask == 0x0000FF && format->Gmask == 0x00FF00 && format->Bmask == 0xFF0000) {
                if (surfaceFormatResult != NULL) *surfaceFormatResult = GL_RGB;
                return 0;
            }
            // Looks like BGR?
            if (format->Rmask == 0xFF0000 && format->Gmask == 0x00FF00 && format->Bmask == 0x0000FF) {
#ifdef GL_BGR
                if (renderer->enabled_features & R_FEATURE_GL_BGR) {
                    if (surfaceFormatResult != NULL) *surfaceFormatResult = GL_BGR;
                    return 0;
                }
#endif
            }
            return 1;

            // 4-channel formats
        case GL_RGBA:

            if (format->BytesPerPixel != 4) return 1;

            // Looks like RGBA?  Easy!
            if (format->Rmask == 0x000000FF && format->Gmask == 0x0000FF00 && format->Bmask == 0x00FF0000) {
                if (surfaceFormatResult != NULL) *surfaceFormatResult = GL_RGBA;
                return 0;
            }
            // Looks like ABGR?
            if (format->Rmask == 0xFF000000 && format->Gmask == 0x00FF0000 && format->Bmask == 0x0000FF00) {
#ifdef GL_ABGR
                if (renderer->enabled_features & R_FEATURE_GL_ABGR) {
                    if (surfaceFormatResult != NULL) *surfaceFormatResult = GL_ABGR;
                    return 0;
                }
#endif
            }
            // Looks like BGRA?
            else if (format->Rmask == 0x00FF0000 && format->Gmask == 0x0000FF00 && format->Bmask == 0x000000FF) {
#ifdef GL_BGRA
                if (renderer->enabled_features & R_FEATURE_GL_BGRA) {
                    // ARGB, for OpenGL BGRA
                    if (surfaceFormatResult != NULL) *surfaceFormatResult = GL_BGRA;
                    return 0;
                }
#endif
            }
            return 1;
        default:
            R_PushErrorCode("R_CompareFormats", R_ERROR_DATA_ERROR, "Invalid texture format (0x%x)", glFormat);
            return -1;
    }
}

// Adapted from SDL_AllocFormat()
SDL_PixelFormat *AllocFormat(GLenum glFormat) {
    // Yes, I need to do the whole thing myself... :(
    u8 channels;
    u32 Rmask, Gmask, Bmask, Amask = 0, mask;
    SDL_PixelFormat *result;

    switch (glFormat) {
        case GL_RGB:
            channels = 3;
            Rmask = 0x0000FF;
            Gmask = 0x00FF00;
            Bmask = 0xFF0000;
            break;
#ifdef GL_BGR
        case GL_BGR:
            channels = 3;
            Rmask = 0xFF0000;
            Gmask = 0x00FF00;
            Bmask = 0x0000FF;
            break;
#endif
        case GL_RGBA:
            channels = 4;
            Rmask = 0x000000FF;
            Gmask = 0x0000FF00;
            Bmask = 0x00FF0000;
            Amask = 0xFF000000;
            break;
#ifdef GL_BGRA
        case GL_BGRA:
            channels = 4;
            Rmask = 0x00FF0000;
            Gmask = 0x0000FF00;
            Bmask = 0x000000FF;
            Amask = 0xFF000000;
            break;
#endif
#ifdef GL_ABGR
        case GL_ABGR:
            channels = 4;
            Rmask = 0xFF000000;
            Gmask = 0x00FF0000;
            Bmask = 0x0000FF00;
            Amask = 0x000000FF;
            break;
#endif
        default:
            return NULL;
    }

    // R_LogError("AllocFormat(): %d, Masks: %X %X %X %X\n", glFormat, Rmask, Gmask, Bmask, Amask);

    result = (SDL_PixelFormat *)ME_MALLOC(sizeof(SDL_PixelFormat));
    memset(result, 0, sizeof(SDL_PixelFormat));

    result->BitsPerPixel = 8 * channels;
    result->BytesPerPixel = channels;

    result->Rmask = Rmask;
    result->Rshift = 0;
    result->Rloss = 8;
    if (Rmask) {
        for (mask = Rmask; !(mask & 0x01); mask >>= 1) ++result->Rshift;
        for (; (mask & 0x01); mask >>= 1) --result->Rloss;
    }

    result->Gmask = Gmask;
    result->Gshift = 0;
    result->Gloss = 8;
    if (Gmask) {
        for (mask = Gmask; !(mask & 0x01); mask >>= 1) ++result->Gshift;
        for (; (mask & 0x01); mask >>= 1) --result->Gloss;
    }

    result->Bmask = Bmask;
    result->Bshift = 0;
    result->Bloss = 8;
    if (Bmask) {
        for (mask = Bmask; !(mask & 0x01); mask >>= 1) ++result->Bshift;
        for (; (mask & 0x01); mask >>= 1) --result->Bloss;
    }

    result->Amask = Amask;
    result->Ashift = 0;
    result->Aloss = 8;
    if (Amask) {
        for (mask = Amask; !(mask & 0x01); mask >>= 1) ++result->Ashift;
        for (; (mask & 0x01); mask >>= 1) --result->Aloss;
    }

    return result;
}

void FreeFormat(SDL_PixelFormat *format) { ME_FREE(format); }

// Returns NULL on failure.  Returns the original surface if no copy is needed.  Returns a new surface converted to the right format otherwise.
SDL_Surface *copySurfaceIfNeeded(R_Renderer *renderer, GLenum glFormat, SDL_Surface *surface, GLenum *surfaceFormatResult) {
    // If format doesn't match, we need to do a copy
    int format_compare = compareFormats(renderer, glFormat, surface, surfaceFormatResult);

    // There's a problem, logged in compareFormats()
    if (format_compare < 0) return NULL;

    // Copy it to a different format
    if (format_compare > 0) {
        // Convert to the right format
        SDL_PixelFormat *dst_fmt = AllocFormat(glFormat);
        surface = SDL_ConvertSurface(surface, dst_fmt, 0);
        FreeFormat(dst_fmt);
        if (surfaceFormatResult != NULL && surface != NULL) *surfaceFormatResult = glFormat;
    }

    // No copy needed
    return surface;
}

R_Image *gpu_copy_image_pixels_only(R_Renderer *renderer, R_Image *image) {
    R_Image *result = NULL;

    if (image == NULL) return NULL;

    switch (image->format) {
        case R_FORMAT_RGB:
        case R_FORMAT_RGBA:
        case R_FORMAT_BGR:
        case R_FORMAT_BGRA:
        case R_FORMAT_ABGR:
            // Copy via framebuffer blitting (fast)
            {
                R_Target *target;

                result = CreateImage(renderer, image->texture_w, image->texture_h, image->format);
                if (result == NULL) {
                    R_PushErrorCode("R_CopyImage", R_ERROR_BACKEND_ERROR, "Failed to create new image.");
                    return NULL;
                }

                // Don't free the target yet (a waste of perf), but let it be freed when the image is freed...
                target = R_GetTarget(result);
                if (target == NULL) {
                    R_FreeImage(result);
                    R_PushErrorCode("R_CopyImage", R_ERROR_BACKEND_ERROR, "Failed to load target.");
                    return NULL;
                }

                // For some reason, I wasn't able to get glCopyTexImage2D() or glCopyTexSubImage2D() working without getting GL_INVALID_ENUM (0x500).
                // It seemed to only work for the default framebuffer...

                {
                    // Clear the color, blending, and filter mode
                    SDL_Color color = ToSDLColor(image->color);
                    bool use_blending = image->use_blending;
                    R_FilterEnum filter_mode = image->filter_mode;
                    bool use_virtual = image->using_virtual_resolution;
                    u16 w = 0, h = 0;
                    R_UnsetColor(image);
                    R_SetBlending(image, 0);
                    R_SetImageFilter(image, R_FILTER_NEAREST);
                    if (use_virtual) {
                        w = image->w;
                        h = image->h;
                        R_UnsetImageVirtualResolution(image);
                    }

                    Blit(renderer, image, NULL, target, (float)(image->w / 2), (float)(image->h / 2));

                    // Restore the saved settings
                    R_SetColor(image, ToEngineColor(color));
                    R_SetBlending(image, use_blending);
                    R_SetImageFilter(image, filter_mode);
                    if (use_virtual) {
                        R_SetImageVirtualResolution(image, w, h);
                    }
                }
            }
            break;
        case R_FORMAT_LUMINANCE:
        case R_FORMAT_LUMINANCE_ALPHA:
        case R_FORMAT_ALPHA:
        case R_FORMAT_RG:
            // Copy via texture download and upload (slow)
            {
                GLenum internal_format;
                int w;
                int h;
                unsigned char *texture_data = getRawImageData(renderer, image);
                if (texture_data == NULL) {
                    R_PushErrorCode("R_CopyImage", R_ERROR_BACKEND_ERROR, "Failed to get raw texture data.");
                    return NULL;
                }

                result = CreateUninitializedImage(renderer, image->texture_w, image->texture_h, image->format);
                if (result == NULL) {
                    ME_FREE(texture_data);
                    R_PushErrorCode("R_CopyImage", R_ERROR_BACKEND_ERROR, "Failed to create new image.");
                    return NULL;
                }

                changeTexturing(renderer, 1);
                bindTexture(renderer, result);

                internal_format = ((R_IMAGE_DATA *)(result->data))->format;
                w = result->w;
                h = result->h;
                if (!(renderer->enabled_features & R_FEATURE_NON_POWER_OF_TWO)) {
                    if (!isPowerOfTwo(w)) w = getNearestPowerOf2(w);
                    if (!isPowerOfTwo(h)) h = getNearestPowerOf2(h);
                }

                upload_new_texture(texture_data, R_MakeRect(0, 0, (float)w, (float)h), internal_format, 1, w, result->bytes_per_pixel);

                // Tell SDL_gpu what we got.
                result->texture_w = (u16)w;
                result->texture_h = (u16)h;

                ME_FREE(texture_data);
            }
            break;
        default:
            R_PushErrorCode("R_CopyImage", R_ERROR_BACKEND_ERROR, "Could not copy the given image format.");
            break;
    }

    return result;
}

R_Image *CopyImage(R_Renderer *renderer, R_Image *image) {
    R_Image *result = NULL;

    if (image == NULL) return NULL;

    result = gpu_copy_image_pixels_only(renderer, image);

    if (result != NULL) {
        // Copy the image settings
        R_SetColor(result, image->color);
        R_SetBlending(result, image->use_blending);
        result->blend_mode = image->blend_mode;
        R_SetImageFilter(result, image->filter_mode);
        R_SetSnapMode(result, image->snap_mode);
        R_SetWrapMode(result, image->wrap_mode_x, image->wrap_mode_y);
        if (image->has_mipmaps) R_GenerateMipmaps(result);
        if (image->using_virtual_resolution) R_SetImageVirtualResolution(result, image->w, image->h);
    }

    return result;
}

void UpdateImage(R_Renderer *renderer, R_Image *image, const MErect *image_rect, void *surface, const MErect *surface_rect) {
    R_IMAGE_DATA *data;
    GLenum original_format;

    SDL_Surface *newSurface;
    MErect updateRect;
    MErect sourceRect;
    int alignment;
    u8 *pixels;

    if (image == NULL || surface == NULL) return;

    data = (R_IMAGE_DATA *)image->data;
    original_format = data->format;

    newSurface = copySurfaceIfNeeded(renderer, data->format, (SDL_Surface *)surface, &original_format);
    if (newSurface == NULL) {
        R_PushErrorCode("R_UpdateImage", R_ERROR_BACKEND_ERROR, "Failed to convert surface to proper pixel format.");
        return;
    }

    if (image_rect != NULL) {
        updateRect = *image_rect;
        if (updateRect.x < 0) {
            updateRect.w += updateRect.x;
            updateRect.x = 0;
        }
        if (updateRect.y < 0) {
            updateRect.h += updateRect.y;
            updateRect.y = 0;
        }
        if (updateRect.x + updateRect.w > image->base_w) updateRect.w += image->base_w - (updateRect.x + updateRect.w);
        if (updateRect.y + updateRect.h > image->base_h) updateRect.h += image->base_h - (updateRect.y + updateRect.h);

        if (updateRect.w <= 0) updateRect.w = 0;
        if (updateRect.h <= 0) updateRect.h = 0;
    } else {
        updateRect.x = 0;
        updateRect.y = 0;
        updateRect.w = image->base_w;
        updateRect.h = image->base_h;
        if (updateRect.w < 0.0f || updateRect.h < 0.0f) {
            R_PushErrorCode("R_UpdateImage", R_ERROR_USER_ERROR, "Given negative image rectangle.");
            return;
        }
    }

    if (surface_rect != NULL) {
        sourceRect = *surface_rect;
        if (sourceRect.x < 0) {
            sourceRect.w += sourceRect.x;
            sourceRect.x = 0;
        }
        if (sourceRect.y < 0) {
            sourceRect.h += sourceRect.y;
            sourceRect.y = 0;
        }
        if (sourceRect.x + sourceRect.w > newSurface->w) sourceRect.w += newSurface->w - (sourceRect.x + sourceRect.w);
        if (sourceRect.y + sourceRect.h > newSurface->h) sourceRect.h += newSurface->h - (sourceRect.y + sourceRect.h);

        if (sourceRect.w <= 0) sourceRect.w = 0;
        if (sourceRect.h <= 0) sourceRect.h = 0;
    } else {
        sourceRect.x = 0;
        sourceRect.y = 0;
        sourceRect.w = (float)newSurface->w;
        sourceRect.h = (float)newSurface->h;
    }

    changeTexturing(renderer, 1);
    if (image->target != NULL && isCurrentTarget(renderer, image->target)) FlushBlitBuffer(renderer);
    bindTexture(renderer, image);
    alignment = 8;
    while (newSurface->pitch % alignment) alignment >>= 1;

    // Use the smaller of the image and surface rect dimensions
    if (sourceRect.w < updateRect.w) updateRect.w = sourceRect.w;
    if (sourceRect.h < updateRect.h) updateRect.h = sourceRect.h;

    pixels = (u8 *)newSurface->pixels;
    // Shift the pixels pointer to the proper source position
    pixels += (int)(newSurface->pitch * sourceRect.y + (newSurface->format->BytesPerPixel) * sourceRect.x);

    upload_texture(pixels, updateRect, original_format, alignment, newSurface->pitch / newSurface->format->BytesPerPixel, newSurface->pitch, newSurface->format->BytesPerPixel);

    // Delete temporary surface
    if (surface != newSurface) SDL_FreeSurface(newSurface);
}

void UpdateImageBytes(R_Renderer *renderer, R_Image *image, const MErect *image_rect, const unsigned char *bytes, int bytes_per_row) {
    R_IMAGE_DATA *data;
    GLenum original_format;

    MErect updateRect;
    int alignment;

    if (image == NULL || bytes == NULL) return;

    data = (R_IMAGE_DATA *)image->data;
    original_format = data->format;

    if (image_rect != NULL) {
        updateRect = *image_rect;
        if (updateRect.x < 0) {
            updateRect.w += updateRect.x;
            updateRect.x = 0;
        }
        if (updateRect.y < 0) {
            updateRect.h += updateRect.y;
            updateRect.y = 0;
        }
        if (updateRect.x + updateRect.w > image->base_w) updateRect.w += image->base_w - (updateRect.x + updateRect.w);
        if (updateRect.y + updateRect.h > image->base_h) updateRect.h += image->base_h - (updateRect.y + updateRect.h);

        if (updateRect.w <= 0) updateRect.w = 0;
        if (updateRect.h <= 0) updateRect.h = 0;
    } else {
        updateRect.x = 0;
        updateRect.y = 0;
        updateRect.w = image->base_w;
        updateRect.h = image->base_h;
        if (updateRect.w < 0.0f || updateRect.h < 0.0f) {
            R_PushErrorCode("R_UpdateImage", R_ERROR_USER_ERROR, "Given negative image rectangle.");
            return;
        }
    }

    changeTexturing(renderer, 1);
    if (image->target != NULL && isCurrentTarget(renderer, image->target)) FlushBlitBuffer(renderer);
    bindTexture(renderer, image);
    alignment = 8;
    while (bytes_per_row % alignment) alignment >>= 1;

    upload_texture(bytes, updateRect, original_format, alignment, bytes_per_row / image->bytes_per_pixel, bytes_per_row, image->bytes_per_pixel);
}

bool ReplaceImage(R_Renderer *renderer, R_Image *image, void *surface, const MErect *surface_rect) {
    R_IMAGE_DATA *data;
    MErect sourceRect;
    SDL_Surface *newSurface;
    GLenum internal_format;
    u8 *pixels;
    int w, h;
    int alignment;

    if (image == NULL) {
        R_PushErrorCode("R_ReplaceImage", R_ERROR_NULL_ARGUMENT, "image");
        return false;
    }

    if (surface == NULL) {
        R_PushErrorCode("R_ReplaceImage", R_ERROR_NULL_ARGUMENT, "surface");
        return false;
    }

    data = (R_IMAGE_DATA *)image->data;
    internal_format = data->format;

    newSurface = copySurfaceIfNeeded(renderer, internal_format, (SDL_Surface *)surface, &internal_format);
    if (newSurface == NULL) {
        R_PushErrorCode("R_ReplaceImage", R_ERROR_BACKEND_ERROR, "Failed to convert surface to proper pixel format.");
        return false;
    }

    // Free the attached framebuffer
    if ((renderer->enabled_features & R_FEATURE_RENDER_TARGETS) && image->target != NULL) {
        R_TARGET_DATA *tdata = (R_TARGET_DATA *)image->target->data;
        if (renderer->current_context_target != NULL) flushAndClearBlitBufferIfCurrentFramebuffer(renderer, image->target);
        if (tdata->handle != 0) glDeleteFramebuffersPROC(1, &tdata->handle);
        tdata->handle = 0;
    }

    // Free the old texture
    if (data->owns_handle) glDeleteTextures(1, &data->handle);
    data->handle = 0;

    // Get the area of the surface we'll use
    if (surface_rect == NULL) {
        sourceRect.x = 0;
        sourceRect.y = 0;
        sourceRect.w = (float)((SDL_Surface *)surface)->w;
        sourceRect.h = (float)((SDL_Surface *)surface)->h;
    } else
        sourceRect = *surface_rect;

    // Clip the source rect to the surface
    if (sourceRect.x < 0) {
        sourceRect.w += sourceRect.x;
        sourceRect.x = 0;
    }
    if (sourceRect.y < 0) {
        sourceRect.h += sourceRect.y;
        sourceRect.y = 0;
    }
    if (sourceRect.x >= ((SDL_Surface *)surface)->w) sourceRect.x = (float)((SDL_Surface *)surface)->w - 1;
    if (sourceRect.y >= ((SDL_Surface *)surface)->h) sourceRect.y = (float)((SDL_Surface *)surface)->h - 1;

    if (sourceRect.x + sourceRect.w > ((SDL_Surface *)surface)->w) sourceRect.w = (float)((SDL_Surface *)surface)->w - sourceRect.x;
    if (sourceRect.y + sourceRect.h > ((SDL_Surface *)surface)->h) sourceRect.h = (float)((SDL_Surface *)surface)->h - sourceRect.y;

    if (sourceRect.w <= 0 || sourceRect.h <= 0) {
        R_PushErrorCode("R_ReplaceImage", R_ERROR_DATA_ERROR, "Clipped source rect has zero size.");
        return false;
    }

    // Allocate new texture
    data->handle = CreateUninitializedTexture(renderer);
    data->owns_handle = 1;
    if (data->handle == 0) {
        R_PushErrorCode("R_ReplaceImage", R_ERROR_BACKEND_ERROR, "Failed to create a new texture handle.");
        return false;
    }

    // Update image members
    w = (int)sourceRect.w;
    h = (int)sourceRect.h;

    if (!image->using_virtual_resolution) {
        image->w = (u16)w;
        image->h = (u16)h;
    }
    image->base_w = (u16)w;
    image->base_h = (u16)h;

    if (!(renderer->enabled_features & R_FEATURE_NON_POWER_OF_TWO)) {
        if (!isPowerOfTwo(w)) w = getNearestPowerOf2(w);
        if (!isPowerOfTwo(h)) h = getNearestPowerOf2(h);
    }
    image->texture_w = (u16)w;
    image->texture_h = (u16)h;

    image->has_mipmaps = false;

    // Upload surface pixel data
    alignment = 8;
    while (newSurface->pitch % alignment) alignment >>= 1;

    pixels = (u8 *)newSurface->pixels;
    // Shift the pixels pointer to the proper source position
    pixels += (int)(newSurface->pitch * sourceRect.y + (newSurface->format->BytesPerPixel) * sourceRect.x);

    upload_new_texture(pixels, R_MakeRect(0, 0, (float)w, (float)h), internal_format, alignment, (newSurface->pitch / newSurface->format->BytesPerPixel), newSurface->format->BytesPerPixel);

    // Delete temporary surface
    if (surface != newSurface) SDL_FreeSurface(newSurface);

    // Update target members
    if ((renderer->enabled_features & R_FEATURE_RENDER_TARGETS) && image->target != NULL) {
        GLenum status;
        R_Target *target = image->target;
        R_TARGET_DATA *tdata = (R_TARGET_DATA *)target->data;

        // Create framebuffer object
        glGenFramebuffersPROC(1, &tdata->handle);
        if (tdata->handle == 0) {
            R_PushErrorCode("R_ReplaceImage", R_ERROR_BACKEND_ERROR, "Failed to create new framebuffer target.");
            return false;
        }

        flushAndBindFramebuffer(renderer, tdata->handle);

        // Attach the texture to it
        glFramebufferTexture2DPROC(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data->handle, 0);

        status = glCheckFramebufferStatusPROC(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            R_PushErrorCode("R_ReplaceImage", R_ERROR_BACKEND_ERROR, "Failed to recreate framebuffer target.");
            return false;
        }

        if (!target->using_virtual_resolution) {
            target->w = image->base_w;
            target->h = image->base_h;
        }
        target->base_w = image->texture_w;
        target->base_h = image->texture_h;

        // Reset viewport?
        target->viewport = R_MakeRect(0, 0, target->w, target->h);
    }

    return true;
}

static_inline u32 getPixel(SDL_Surface *Surface, int x, int y) {
    u8 *bits;
    u32 bpp;

    if (x < 0 || x >= Surface->w) return 0;  // Best I could do for errors

    bpp = Surface->format->BytesPerPixel;
    bits = ((u8 *)Surface->pixels) + y * Surface->pitch + x * bpp;

    switch (bpp) {
        case 1:
            return *((u8 *)Surface->pixels + y * Surface->pitch + x);
            break;
        case 2:
            return *((u16 *)Surface->pixels + y * Surface->pitch / 2 + x);
            break;
        case 3:
            // Endian-correct, but slower
            {
                u8 r, g, b;
                r = *((bits) + Surface->format->Rshift / 8);
                g = *((bits) + Surface->format->Gshift / 8);
                b = *((bits) + Surface->format->Bshift / 8);
                return SDL_MapRGB(Surface->format, r, g, b);
            }
            break;
        case 4:
            return *((u32 *)Surface->pixels + y * Surface->pitch / 4 + x);
            break;
    }

    return 0;  // FIXME: Handle errors better
}

R_Image *CopyImageFromSurface(R_Renderer *renderer, void *surface, const MErect *surface_rect) {
    R_FormatEnum format;
    R_Image *image;
    int sw, sh;

    if (surface == NULL) {
        R_PushErrorCode("R_CopyImageFromSurface", R_ERROR_NULL_ARGUMENT, "surface");
        return NULL;
    }
    sw = surface_rect == NULL ? ((SDL_Surface *)surface)->w : surface_rect->w;
    sh = surface_rect == NULL ? ((SDL_Surface *)surface)->h : surface_rect->h;

    if (((SDL_Surface *)surface)->w == 0 || ((SDL_Surface *)surface)->h == 0) {
        R_PushErrorCode("R_CopyImageFromSurface", R_ERROR_DATA_ERROR, "Surface has a zero dimension.");
        return NULL;
    }

    // See what the best image format is.
    if (((SDL_Surface *)surface)->format->Amask == 0) {
        if (has_colorkey((SDL_Surface *)surface) || is_alpha_format(((SDL_Surface *)surface)->format))
            format = R_FORMAT_RGBA;
        else
            format = R_FORMAT_RGB;
    } else {
        // TODO: Choose the best format for the texture depending on endianness.
        format = R_FORMAT_RGBA;
    }

    image = CreateImage(renderer, (u16)sw, (u16)sh, format);
    if (image == NULL) return NULL;

    UpdateImage(renderer, image, NULL, surface, surface_rect);

    return image;
}

R_Image *CopyImageFromTarget(R_Renderer *renderer, R_Target *target) {
    R_Image *result;

    if (target == NULL) return NULL;

    if (target->image != NULL) {
        result = gpu_copy_image_pixels_only(renderer, target->image);
    } else {
        SDL_Surface *surface = (SDL_Surface *)CopySurfaceFromTarget(renderer, target);
        result = CopyImageFromSurface(renderer, surface, NULL);
        SDL_FreeSurface(surface);
    }

    return result;
}

void FreeImage(R_Renderer *renderer, R_Image *image) {
    R_IMAGE_DATA *data;

    if (image == NULL) return;

    if (image->refcount > 1) {
        image->refcount--;
        return;
    }

    // Delete the attached target first
    if (image->target != NULL) {
        R_Target *target = image->target;
        image->target = NULL;

        // Freeing it will decrement the refcount.  If this is the only increment, it will be freed.  This means R_LoadTarget() needs to be paired with R_FreeTarget().
        target->refcount++;
        FreeTarget(renderer, target);
    }

    flushAndClearBlitBufferIfCurrentTexture(renderer, image);

    // Does the renderer data need to be freed too?
    data = (R_IMAGE_DATA *)image->data;
    if (data->refcount > 1) {
        data->refcount--;
    } else {
        if (data->owns_handle && image->renderer == R_GetCurrentRenderer()) {
            R_MakeCurrent(image->context_target, image->context_target->context->windowID);
            glDeleteTextures(1, &data->handle);
        }
        ME_FREE(data);
    }

    ME_FREE(image);
}

R_Target *GetTarget(R_Renderer *renderer, R_Image *image) {
    GLuint handle;
    GLenum status;
    R_Target *result;
    R_TARGET_DATA *data;

    if (image == NULL) return NULL;

    if (image->target != NULL) return image->target;

    if (!(renderer->enabled_features & R_FEATURE_RENDER_TARGETS)) return NULL;

    // Create framebuffer object
    glGenFramebuffersPROC(1, &handle);
    flushAndBindFramebuffer(renderer, handle);

    // Attach the texture to it
    glFramebufferTexture2DPROC(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ((R_IMAGE_DATA *)image->data)->handle, 0);

    status = glCheckFramebufferStatusPROC(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        R_PushErrorCode("R_GetTarget", R_ERROR_DATA_ERROR,
                        "Framebuffer incomplete with status: 0x%x.  Format 0x%x for framebuffers might not "
                        "be supported on this hardware.",
                        status, ((R_IMAGE_DATA *)image->data)->format);
        return NULL;
    }

    result = (R_Target *)ME_MALLOC(sizeof(R_Target));
    memset(result, 0, sizeof(R_Target));
    result->refcount = 0;
    data = (R_TARGET_DATA *)ME_MALLOC(sizeof(R_TARGET_DATA));
    data->refcount = 1;
    result->data = data;
    data->handle = handle;
    data->format = ((R_IMAGE_DATA *)image->data)->format;

    result->renderer = renderer;
    result->context_target = renderer->current_context_target;
    result->context = NULL;
    result->image = image;
    result->w = image->w;
    result->h = image->h;
    result->base_w = image->texture_w;
    result->base_h = image->texture_h;
    result->using_virtual_resolution = image->using_virtual_resolution;

    result->viewport = R_MakeRect(0, 0, result->w, result->h);

    result->matrix_mode = R_MODEL;
    R_InitMatrixStack(&result->projection_matrix);
    R_InitMatrixStack(&result->view_matrix);
    R_InitMatrixStack(&result->model_matrix);

    result->camera = R_GetDefaultCamera();
    result->use_camera = true;

    // Set up default projection matrix
    R_ResetProjection(result);

    result->use_depth_test = false;
    result->use_depth_write = true;

    result->use_clip_rect = false;
    result->clip_rect.x = 0;
    result->clip_rect.y = 0;
    result->clip_rect.w = result->w;
    result->clip_rect.h = result->h;
    result->use_color = false;

    image->target = result;
    return result;
}

void FreeTargetData(R_Renderer *renderer, R_TARGET_DATA *data) {
    if (data == NULL) return;

    if (data->refcount > 1) {
        data->refcount--;
        return;
    }

    // Time to actually free this target data
    if (renderer->enabled_features & R_FEATURE_RENDER_TARGETS) {
        // It might be possible to check against the default framebuffer (save that binding in the context data) and avoid deleting that...  Is that desired?
        glDeleteFramebuffersPROC(1, &data->handle);
    }

    ME_FREE(data);
}

void FreeContext(R_Context *context) {
    R_CONTEXT_DATA *cdata;

    if (context == NULL) return;

    if (context->refcount > 1) {
        context->refcount--;
        return;
    }

    // Time to actually free this context and its data
    cdata = (R_CONTEXT_DATA *)context->data;

    ME_FREE(cdata->blit_buffer);
    ME_FREE(cdata->index_buffer);

    if (!context->failed) {
#ifdef R_USE_BUFFER_PIPELINE
        glDeleteBuffers(2, cdata->blit_VBO);
        glDeleteBuffers(1, &cdata->blit_IBO);
        glDeleteBuffers(16, cdata->attribute_VBO);
#if !defined(R_NO_VAO)
        glDeleteVertexArrays(1, &cdata->blit_VAO);
#endif
#endif
    }

    if (context->context != 0) SDL_GL_DeleteContext(context->context);

    ME_FREE(cdata);
    ME_FREE(context);
}

void FreeTarget(R_Renderer *renderer, R_Target *target) {
    if (target == NULL) return;

    if (target->refcount > 1) {
        target->refcount--;
        return;
    }

    // Time to actually free this target

    // Prepare to work in this target's context, if it has one
    if (target == renderer->current_context_target)
        FlushBlitBuffer(renderer);
    else if (target->context_target != NULL) {
        R_MakeCurrent(target->context_target, target->context_target->context->windowID);
    }

    // Release renderer data reference
    FreeTargetData(renderer, (R_TARGET_DATA *)target->data);

    // Release context reference
    if (target->context != NULL) {
        // Remove all of the window mappings that refer to this target
        R_RemoveWindowMappingByTarget(target);

        FreeContext(target->context);
    }

    // Clear references to this target
    if (target == renderer->current_context_target) renderer->current_context_target = NULL;

    // Make sure this target is not referenced by the context
    if (renderer->current_context_target != NULL) {
        R_CONTEXT_DATA *cdata = ((R_CONTEXT_DATA *)renderer->current_context_target->context_target->context->data);
        // Clear reference to image
        if (cdata->last_image == target->image) cdata->last_image = NULL;

        if (target == renderer->current_context_target->context->active_target) renderer->current_context_target->context->active_target = NULL;
    }

    if (target->image != NULL) {
        // Make sure this is not targeted by an image that will persist
        if (target->image->target == target) target->image->target = NULL;
    }

    // Delete matrices
    R_ClearMatrixStack(&target->projection_matrix);
    R_ClearMatrixStack(&target->view_matrix);
    R_ClearMatrixStack(&target->model_matrix);

    ME_FREE(target);
}

#define SET_TEXTURED_VERTEX(x, y, s, t, r, g, b, a)                                       \
    blit_buffer[vert_index] = x;                                                          \
    blit_buffer[vert_index + 1] = y;                                                      \
    blit_buffer[tex_index] = s;                                                           \
    blit_buffer[tex_index + 1] = t;                                                       \
    blit_buffer[color_index] = r;                                                         \
    blit_buffer[color_index + 1] = g;                                                     \
    blit_buffer[color_index + 2] = b;                                                     \
    blit_buffer[color_index + 3] = a;                                                     \
    index_buffer[cdata->index_buffer_num_vertices++] = cdata->blit_buffer_num_vertices++; \
    vert_index += R_BLIT_BUFFER_FLOATS_PER_VERTEX;                                        \
    tex_index += R_BLIT_BUFFER_FLOATS_PER_VERTEX;                                         \
    color_index += R_BLIT_BUFFER_FLOATS_PER_VERTEX;

#define SET_TEXTURED_VERTEX_UNINDEXED(x, y, s, t, r, g, b, a) \
    blit_buffer[vert_index] = x;                              \
    blit_buffer[vert_index + 1] = y;                          \
    blit_buffer[tex_index] = s;                               \
    blit_buffer[tex_index + 1] = t;                           \
    blit_buffer[color_index] = r;                             \
    blit_buffer[color_index + 1] = g;                         \
    blit_buffer[color_index + 2] = b;                         \
    blit_buffer[color_index + 3] = a;                         \
    vert_index += R_BLIT_BUFFER_FLOATS_PER_VERTEX;            \
    tex_index += R_BLIT_BUFFER_FLOATS_PER_VERTEX;             \
    color_index += R_BLIT_BUFFER_FLOATS_PER_VERTEX;

#define SET_UNTEXTURED_VERTEX(x, y, r, g, b, a)                                           \
    blit_buffer[vert_index] = x;                                                          \
    blit_buffer[vert_index + 1] = y;                                                      \
    blit_buffer[color_index] = r;                                                         \
    blit_buffer[color_index + 1] = g;                                                     \
    blit_buffer[color_index + 2] = b;                                                     \
    blit_buffer[color_index + 3] = a;                                                     \
    index_buffer[cdata->index_buffer_num_vertices++] = cdata->blit_buffer_num_vertices++; \
    vert_index += R_BLIT_BUFFER_FLOATS_PER_VERTEX;                                        \
    color_index += R_BLIT_BUFFER_FLOATS_PER_VERTEX;

#define SET_UNTEXTURED_VERTEX_UNINDEXED(x, y, r, g, b, a) \
    blit_buffer[vert_index] = x;                          \
    blit_buffer[vert_index + 1] = y;                      \
    blit_buffer[color_index] = r;                         \
    blit_buffer[color_index + 1] = g;                     \
    blit_buffer[color_index + 2] = b;                     \
    blit_buffer[color_index + 3] = a;                     \
    vert_index += R_BLIT_BUFFER_FLOATS_PER_VERTEX;        \
    color_index += R_BLIT_BUFFER_FLOATS_PER_VERTEX;

#define SET_INDEXED_VERTEX(offset) index_buffer[cdata->index_buffer_num_vertices++] = blit_buffer_starting_index + (unsigned short)(offset);

#define SET_RELATIVE_INDEXED_VERTEX(offset) index_buffer[cdata->index_buffer_num_vertices++] = cdata->blit_buffer_num_vertices + (unsigned short)(offset);

#define BEGIN_UNTEXTURED_SEGMENTS(x1, y1, x2, y2, r, g, b, a) \
    SET_UNTEXTURED_VERTEX(x1, y1, r, g, b, a);                \
    SET_UNTEXTURED_VERTEX(x2, y2, r, g, b, a);

// Finish previous triangles and start the next one
#define SET_UNTEXTURED_SEGMENTS(x1, y1, x2, y2, r, g, b, a) \
    SET_UNTEXTURED_VERTEX(x1, y1, r, g, b, a);              \
    SET_RELATIVE_INDEXED_VERTEX(-2);                        \
    SET_UNTEXTURED_VERTEX(x2, y2, r, g, b, a);              \
    SET_RELATIVE_INDEXED_VERTEX(-2);                        \
    SET_RELATIVE_INDEXED_VERTEX(-2);                        \
    SET_RELATIVE_INDEXED_VERTEX(-1);

// Finish previous triangles
#define LOOP_UNTEXTURED_SEGMENTS()   \
    SET_INDEXED_VERTEX(0);           \
    SET_RELATIVE_INDEXED_VERTEX(-1); \
    SET_INDEXED_VERTEX(1);           \
    SET_INDEXED_VERTEX(0);

#define END_UNTEXTURED_SEGMENTS(x1, y1, x2, y2, r, g, b, a) \
    SET_UNTEXTURED_VERTEX(x1, y1, r, g, b, a);              \
    SET_RELATIVE_INDEXED_VERTEX(-2);                        \
    SET_UNTEXTURED_VERTEX(x2, y2, r, g, b, a);              \
    SET_RELATIVE_INDEXED_VERTEX(-2);

void Blit(R_Renderer *renderer, R_Image *image, MErect *src_rect, R_Target *target, float x, float y) {
    u32 tex_w, tex_h;
    float w;
    float h;
    float x1, y1, x2, y2;
    float dx1, dy1, dx2, dy2;
    R_CONTEXT_DATA *cdata;
    float *blit_buffer;
    unsigned short *index_buffer;
    unsigned short blit_buffer_starting_index;
    int vert_index;
    int tex_index;
    int color_index;
    float r, g, b, a;

    if (image == NULL) {
        R_PushErrorCode("R_Blit", R_ERROR_NULL_ARGUMENT, "image");
        return;
    }
    if (target == NULL) {
        R_PushErrorCode("R_Blit", R_ERROR_NULL_ARGUMENT, "target");
        return;
    }
    if (renderer != image->renderer || renderer != target->renderer) {
        R_PushErrorCode("R_Blit", R_ERROR_USER_ERROR, "Mismatched renderer");
        return;
    }

    makeContextCurrent(renderer, target);
    if (renderer->current_context_target == NULL) {
        R_PushErrorCode("R_Blit", R_ERROR_USER_ERROR, "NULL context");
        return;
    }

    prepareToRenderToTarget(renderer, target);
    prepareToRenderImage(renderer, target, image);

    // Bind the texture to which subsequent calls refer
    bindTexture(renderer, image);

    // Bind the FBO
    if (!SetActiveTarget(renderer, target)) {
        R_PushErrorCode("R_Blit", R_ERROR_BACKEND_ERROR, "Failed to bind framebuffer.");
        return;
    }

    tex_w = image->texture_w;
    tex_h = image->texture_h;

    if (image->snap_mode == R_SNAP_POSITION || image->snap_mode == R_SNAP_POSITION_AND_DIMENSIONS) {
        // Avoid rounding errors in texture sampling by insisting on integral pixel positions
        x = floorf(x);
        y = floorf(y);
    }

    if (src_rect == NULL) {
        // Scale tex coords according to actual texture dims
        x1 = 0.0f;
        y1 = 0.0f;
        x2 = ((float)image->w) / tex_w;
        y2 = ((float)image->h) / tex_h;
        w = image->w;
        h = image->h;
    } else {
        // Scale src_rect tex coords according to actual texture dims
        x1 = src_rect->x / (float)tex_w;
        y1 = src_rect->y / (float)tex_h;
        x2 = (src_rect->x + src_rect->w) / (float)tex_w;
        y2 = (src_rect->y + src_rect->h) / (float)tex_h;
        w = src_rect->w;
        h = src_rect->h;
    }

    if (image->using_virtual_resolution) {
        // Scale texture coords to fit the original dims
        x1 *= image->base_w / (float)image->w;
        y1 *= image->base_h / (float)image->h;
        x2 *= image->base_w / (float)image->w;
        y2 *= image->base_h / (float)image->h;
    }

    // Center the image on the given coords
    dx1 = x - w * image->anchor_x;
    dy1 = y - h * image->anchor_y;
    dx2 = x + w * (1.0f - image->anchor_x);
    dy2 = y + h * (1.0f - image->anchor_y);

    if (image->snap_mode == R_SNAP_DIMENSIONS || image->snap_mode == R_SNAP_POSITION_AND_DIMENSIONS) {
        float fractional;
        fractional = w / 2.0f - floorf(w / 2.0f);
        dx1 += fractional;
        dx2 += fractional;
        fractional = h / 2.0f - floorf(h / 2.0f);
        dy1 += fractional;
        dy2 += fractional;
    }

    if (renderer->coordinate_mode) {
        float temp = dy1;
        dy1 = dy2;
        dy2 = temp;
    }

    cdata = (R_CONTEXT_DATA *)renderer->current_context_target->context->data;

    if (cdata->blit_buffer_num_vertices + 4 >= cdata->blit_buffer_max_num_vertices) {
        if (!growBlitBuffer(cdata, cdata->blit_buffer_num_vertices + 4)) FlushBlitBuffer(renderer);
    }
    if (cdata->index_buffer_num_vertices + 6 >= cdata->index_buffer_max_num_vertices) {
        if (!growIndexBuffer(cdata, cdata->index_buffer_num_vertices + 6)) FlushBlitBuffer(renderer);
    }

    blit_buffer = cdata->blit_buffer;
    index_buffer = cdata->index_buffer;

    blit_buffer_starting_index = cdata->blit_buffer_num_vertices;

    vert_index = R_BLIT_BUFFER_VERTEX_OFFSET + cdata->blit_buffer_num_vertices * R_BLIT_BUFFER_FLOATS_PER_VERTEX;
    tex_index = R_BLIT_BUFFER_TEX_COORD_OFFSET + cdata->blit_buffer_num_vertices * R_BLIT_BUFFER_FLOATS_PER_VERTEX;
    color_index = R_BLIT_BUFFER_COLOR_OFFSET + cdata->blit_buffer_num_vertices * R_BLIT_BUFFER_FLOATS_PER_VERTEX;
    if (target->use_color) {
        r = MIX_COLOR_COMPONENT_NORMALIZED_RESULT(target->color.r, image->color.r);
        g = MIX_COLOR_COMPONENT_NORMALIZED_RESULT(target->color.g, image->color.g);
        b = MIX_COLOR_COMPONENT_NORMALIZED_RESULT(target->color.b, image->color.b);
        a = MIX_COLOR_COMPONENT_NORMALIZED_RESULT(GET_ALPHA(target->color), GET_ALPHA(image->color));
    } else {
        r = image->color.r / 255.0f;
        g = image->color.g / 255.0f;
        b = image->color.b / 255.0f;
        a = GET_ALPHA(image->color) / 255.0f;
    }

    // 4 Quad vertices
    SET_TEXTURED_VERTEX_UNINDEXED(dx1, dy1, x1, y1, r, g, b, a);
    SET_TEXTURED_VERTEX_UNINDEXED(dx2, dy1, x2, y1, r, g, b, a);
    SET_TEXTURED_VERTEX_UNINDEXED(dx2, dy2, x2, y2, r, g, b, a);
    SET_TEXTURED_VERTEX_UNINDEXED(dx1, dy2, x1, y2, r, g, b, a);

    // 6 Triangle indices
    SET_INDEXED_VERTEX(0);
    SET_INDEXED_VERTEX(1);
    SET_INDEXED_VERTEX(2);

    SET_INDEXED_VERTEX(0);
    SET_INDEXED_VERTEX(2);
    SET_INDEXED_VERTEX(3);

    cdata->blit_buffer_num_vertices += R_BLIT_BUFFER_VERTICES_PER_SPRITE;
}

void BlitRotate(R_Renderer *renderer, R_Image *image, MErect *src_rect, R_Target *target, float x, float y, float degrees) {
    float w, h;
    if (image == NULL) {
        R_PushErrorCode("R_BlitRotate", R_ERROR_NULL_ARGUMENT, "image");
        return;
    }
    if (target == NULL) {
        R_PushErrorCode("R_BlitRotate", R_ERROR_NULL_ARGUMENT, "target");
        return;
    }

    w = (src_rect == NULL ? image->w : src_rect->w);
    h = (src_rect == NULL ? image->h : src_rect->h);
    BlitTransformX(renderer, image, src_rect, target, x, y, w * image->anchor_x, h * image->anchor_y, degrees, 1.0f, 1.0f);
}

void BlitScale(R_Renderer *renderer, R_Image *image, MErect *src_rect, R_Target *target, float x, float y, float scaleX, float scaleY) {
    float w, h;
    if (image == NULL) {
        R_PushErrorCode("R_BlitScale", R_ERROR_NULL_ARGUMENT, "image");
        return;
    }
    if (target == NULL) {
        R_PushErrorCode("R_BlitScale", R_ERROR_NULL_ARGUMENT, "target");
        return;
    }

    w = (src_rect == NULL ? image->w : src_rect->w);
    h = (src_rect == NULL ? image->h : src_rect->h);
    BlitTransformX(renderer, image, src_rect, target, x, y, w * image->anchor_x, h * image->anchor_y, 0.0f, scaleX, scaleY);
}

void BlitTransform(R_Renderer *renderer, R_Image *image, MErect *src_rect, R_Target *target, float x, float y, float degrees, float scaleX, float scaleY) {
    float w, h;
    if (image == NULL) {
        R_PushErrorCode("R_BlitTransform", R_ERROR_NULL_ARGUMENT, "image");
        return;
    }
    if (target == NULL) {
        R_PushErrorCode("R_BlitTransform", R_ERROR_NULL_ARGUMENT, "target");
        return;
    }

    w = (src_rect == NULL ? image->w : src_rect->w);
    h = (src_rect == NULL ? image->h : src_rect->h);
    BlitTransformX(renderer, image, src_rect, target, x, y, w * image->anchor_x, h * image->anchor_y, degrees, scaleX, scaleY);
}

void BlitTransformX(R_Renderer *renderer, R_Image *image, MErect *src_rect, R_Target *target, float x, float y, float pivot_x, float pivot_y, float degrees, float scaleX, float scaleY) {
    u32 tex_w, tex_h;
    float x1, y1, x2, y2;
    float dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4;
    float w, h;
    R_CONTEXT_DATA *cdata;
    float *blit_buffer;
    unsigned short *index_buffer;
    unsigned short blit_buffer_starting_index;
    int vert_index;
    int tex_index;
    int color_index;
    float r, g, b, a;

    if (image == NULL) {
        R_PushErrorCode("R_BlitTransformX", R_ERROR_NULL_ARGUMENT, "image");
        return;
    }
    if (target == NULL) {
        R_PushErrorCode("R_BlitTransformX", R_ERROR_NULL_ARGUMENT, "target");
        return;
    }
    if (renderer != image->renderer || renderer != target->renderer) {
        R_PushErrorCode("R_BlitTransformX", R_ERROR_USER_ERROR, "Mismatched renderer");
        return;
    }

    makeContextCurrent(renderer, target);

    prepareToRenderToTarget(renderer, target);
    prepareToRenderImage(renderer, target, image);

    // Bind the texture to which subsequent calls refer
    bindTexture(renderer, image);

    // Bind the FBO
    if (!SetActiveTarget(renderer, target)) {
        R_PushErrorCode("R_BlitTransformX", R_ERROR_BACKEND_ERROR, "Failed to bind framebuffer.");
        return;
    }

    tex_w = image->texture_w;
    tex_h = image->texture_h;

    if (image->snap_mode == R_SNAP_POSITION || image->snap_mode == R_SNAP_POSITION_AND_DIMENSIONS) {
        // Avoid rounding errors in texture sampling by insisting on integral pixel positions
        x = floorf(x);
        y = floorf(y);
    }

    /*
        1,1 --- 3,3
         |       |
         |       |
        4,4 --- 2,2
    */
    if (src_rect == NULL) {
        // Scale tex coords according to actual texture dims
        x1 = 0.0f;
        y1 = 0.0f;
        x2 = ((float)image->w) / tex_w;
        y2 = ((float)image->h) / tex_h;
        w = image->w;
        h = image->h;
    } else {
        // Scale src_rect tex coords according to actual texture dims
        x1 = src_rect->x / (float)tex_w;
        y1 = src_rect->y / (float)tex_h;
        x2 = (src_rect->x + src_rect->w) / (float)tex_w;
        y2 = (src_rect->y + src_rect->h) / (float)tex_h;
        w = src_rect->w;
        h = src_rect->h;
    }

    if (image->using_virtual_resolution) {
        // Scale texture coords to fit the original dims
        x1 *= image->base_w / (float)image->w;
        y1 *= image->base_h / (float)image->h;
        x2 *= image->base_w / (float)image->w;
        y2 *= image->base_h / (float)image->h;
    }

    // Create vertices about the anchor
    dx1 = -pivot_x;
    dy1 = -pivot_y;
    dx2 = w - pivot_x;
    dy2 = h - pivot_y;

    if (image->snap_mode == R_SNAP_DIMENSIONS || image->snap_mode == R_SNAP_POSITION_AND_DIMENSIONS) {
        // This is a little weird for rotating sprites, but oh well.
        float fractional;
        fractional = w / 2.0f - floorf(w / 2.0f);
        dx1 += fractional;
        dx2 += fractional;
        fractional = h / 2.0f - floorf(h / 2.0f);
        dy1 += fractional;
        dy2 += fractional;
    }

    if (renderer->coordinate_mode == 1) {
        float temp = dy1;
        dy1 = dy2;
        dy2 = temp;
    }

    // Apply transforms

    // Scale about the anchor
    if (scaleX != 1.0f || scaleY != 1.0f) {
        dx1 *= scaleX;
        dy1 *= scaleY;
        dx2 *= scaleX;
        dy2 *= scaleY;
    }

    // Get extra vertices for rotation
    dx3 = dx2;
    dy3 = dy1;
    dx4 = dx1;
    dy4 = dy2;

    // Rotate about the anchor
    if (degrees != 0.0f) {
        float cosA = cosf(degrees * RAD_PER_DEG);
        float sinA = sinf(degrees * RAD_PER_DEG);
        float tempX = dx1;
        dx1 = dx1 * cosA - dy1 * sinA;
        dy1 = tempX * sinA + dy1 * cosA;
        tempX = dx2;
        dx2 = dx2 * cosA - dy2 * sinA;
        dy2 = tempX * sinA + dy2 * cosA;
        tempX = dx3;
        dx3 = dx3 * cosA - dy3 * sinA;
        dy3 = tempX * sinA + dy3 * cosA;
        tempX = dx4;
        dx4 = dx4 * cosA - dy4 * sinA;
        dy4 = tempX * sinA + dy4 * cosA;
    }

    // Translate to final position
    dx1 += x;
    dx2 += x;
    dx3 += x;
    dx4 += x;
    dy1 += y;
    dy2 += y;
    dy3 += y;
    dy4 += y;

    cdata = (R_CONTEXT_DATA *)renderer->current_context_target->context->data;

    if (cdata->blit_buffer_num_vertices + 4 >= cdata->blit_buffer_max_num_vertices) {
        if (!growBlitBuffer(cdata, cdata->blit_buffer_num_vertices + 4)) FlushBlitBuffer(renderer);
    }
    if (cdata->index_buffer_num_vertices + 6 >= cdata->index_buffer_max_num_vertices) {
        if (!growIndexBuffer(cdata, cdata->index_buffer_num_vertices + 6)) FlushBlitBuffer(renderer);
    }

    blit_buffer = cdata->blit_buffer;
    index_buffer = cdata->index_buffer;

    blit_buffer_starting_index = cdata->blit_buffer_num_vertices;

    vert_index = R_BLIT_BUFFER_VERTEX_OFFSET + cdata->blit_buffer_num_vertices * R_BLIT_BUFFER_FLOATS_PER_VERTEX;
    tex_index = R_BLIT_BUFFER_TEX_COORD_OFFSET + cdata->blit_buffer_num_vertices * R_BLIT_BUFFER_FLOATS_PER_VERTEX;
    color_index = R_BLIT_BUFFER_COLOR_OFFSET + cdata->blit_buffer_num_vertices * R_BLIT_BUFFER_FLOATS_PER_VERTEX;

    if (target->use_color) {
        r = MIX_COLOR_COMPONENT_NORMALIZED_RESULT(target->color.r, image->color.r);
        g = MIX_COLOR_COMPONENT_NORMALIZED_RESULT(target->color.g, image->color.g);
        b = MIX_COLOR_COMPONENT_NORMALIZED_RESULT(target->color.b, image->color.b);
        a = MIX_COLOR_COMPONENT_NORMALIZED_RESULT(GET_ALPHA(target->color), GET_ALPHA(image->color));
    } else {
        r = image->color.r / 255.0f;
        g = image->color.g / 255.0f;
        b = image->color.b / 255.0f;
        a = GET_ALPHA(image->color) / 255.0f;
    }

    // 4 Quad vertices
    SET_TEXTURED_VERTEX_UNINDEXED(dx1, dy1, x1, y1, r, g, b, a);
    SET_TEXTURED_VERTEX_UNINDEXED(dx3, dy3, x2, y1, r, g, b, a);
    SET_TEXTURED_VERTEX_UNINDEXED(dx2, dy2, x2, y2, r, g, b, a);
    SET_TEXTURED_VERTEX_UNINDEXED(dx4, dy4, x1, y2, r, g, b, a);

    // 6 Triangle indices
    SET_INDEXED_VERTEX(0);
    SET_INDEXED_VERTEX(1);
    SET_INDEXED_VERTEX(2);

    SET_INDEXED_VERTEX(0);
    SET_INDEXED_VERTEX(2);
    SET_INDEXED_VERTEX(3);

    cdata->blit_buffer_num_vertices += R_BLIT_BUFFER_VERTICES_PER_SPRITE;
}

#ifdef R_USE_BUFFER_PIPELINE

static_inline int sizeof_R_type(R_TypeEnum type) {
    if (type == R_TYPE_DOUBLE) return sizeof(double);
    if (type == R_TYPE_FLOAT) return sizeof(float);
    if (type == R_TYPE_INT) return sizeof(int);
    if (type == R_TYPE_UNSIGNED_INT) return sizeof(unsigned int);
    if (type == R_TYPE_SHORT) return sizeof(short);
    if (type == R_TYPE_UNSIGNED_SHORT) return sizeof(unsigned short);
    if (type == R_TYPE_BYTE) return sizeof(char);
    if (type == R_TYPE_UNSIGNED_BYTE) return sizeof(unsigned char);
    return 0;
}

void refresh_attribute_data(R_CONTEXT_DATA *cdata) {
    int i;
    for (i = 0; i < 16; i++) {
        R_AttributeSource *a = &cdata->shader_attributes[i];
        if (a->attribute.values != NULL && a->attribute.location >= 0 && a->num_values > 0 && a->attribute.format.is_per_sprite) {
            // Expand the values to 4 vertices
            int n;
            void *storage_ptr = a->per_vertex_storage;
            void *values_ptr = (void *)((char *)a->attribute.values + a->attribute.format.offset_bytes);
            int value_size_bytes = a->attribute.format.num_elems_per_value * sizeof_R_type(a->attribute.format.type);
            for (n = 0; n < a->num_values; n += 4) {
                memcpy(storage_ptr, values_ptr, value_size_bytes);
                storage_ptr = (void *)((char *)storage_ptr + a->per_vertex_storage_stride_bytes);
                memcpy(storage_ptr, values_ptr, value_size_bytes);
                storage_ptr = (void *)((char *)storage_ptr + a->per_vertex_storage_stride_bytes);
                memcpy(storage_ptr, values_ptr, value_size_bytes);
                storage_ptr = (void *)((char *)storage_ptr + a->per_vertex_storage_stride_bytes);
                memcpy(storage_ptr, values_ptr, value_size_bytes);
                storage_ptr = (void *)((char *)storage_ptr + a->per_vertex_storage_stride_bytes);

                values_ptr = (void *)((char *)values_ptr + a->attribute.format.stride_bytes);
            }
        }
    }
}

void upload_attribute_data(R_CONTEXT_DATA *cdata, int num_vertices) {
    int i;
    for (i = 0; i < 16; i++) {
        R_AttributeSource *a = &cdata->shader_attributes[i];
        if (a->attribute.values != NULL && a->attribute.location >= 0 && a->num_values > 0) {
            int num_values_used = num_vertices;
            int bytes_used;

            if (a->num_values < num_values_used) num_values_used = a->num_values;

            glBindBuffer(GL_ARRAY_BUFFER, cdata->attribute_VBO[i]);

            bytes_used = a->per_vertex_storage_stride_bytes * num_values_used;
            glBufferData(GL_ARRAY_BUFFER, bytes_used, a->next_value, GL_STREAM_DRAW);

            glEnableVertexAttribArray(a->attribute.location);
            glVertexAttribPointer(a->attribute.location, a->attribute.format.num_elems_per_value, a->attribute.format.type, a->attribute.format.normalize, a->per_vertex_storage_stride_bytes,
                                  (void *)(intptr_t)a->per_vertex_storage_offset_bytes);

            a->enabled = true;
            // Move the data along so we use the next values for the next flush
            a->num_values -= num_values_used;
            if (a->num_values <= 0)
                a->next_value = a->per_vertex_storage;
            else
                a->next_value = (void *)(((char *)a->next_value) + bytes_used);
        }
    }
}

void disable_attribute_data(R_CONTEXT_DATA *cdata) {
    int i;
    for (i = 0; i < 16; i++) {
        R_AttributeSource *a = &cdata->shader_attributes[i];
        if (a->enabled) {
            glDisableVertexAttribArray(a->attribute.location);
            a->enabled = false;
        }
    }
}

#endif

int get_lowest_attribute_num_values(R_CONTEXT_DATA *cdata, int cap) {
    int lowest = cap;

#ifdef R_USE_BUFFER_PIPELINE
    int i;
    for (i = 0; i < 16; i++) {
        R_AttributeSource *a = &cdata->shader_attributes[i];
        if (a->attribute.values != NULL && a->attribute.location >= 0) {
            if (a->num_values < lowest) lowest = a->num_values;
        }
    }
#else
    (void)cdata;
#endif

    return lowest;
}

static_inline void submit_buffer_data(int bytes, float *values, int bytes_indices, unsigned short *indices) {
#ifdef R_USE_BUFFER_PIPELINE
#if defined(R_USE_BUFFER_RESET)
    glBufferData(GL_ARRAY_BUFFER, bytes, values, GL_STREAM_DRAW);
    if (indices != NULL) glBufferData(GL_ELEMENT_ARRAY_BUFFER, bytes_indices, indices, GL_DYNAMIC_DRAW);
#elif defined(R_USE_BUFFER_MAPPING)
    // NOTE: On the Raspberry Pi, you may have to use GL_DYNAMIC_DRAW instead of GL_STREAM_DRAW for buffers to work with glMapBuffer().
    float *data = (float *)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    unsigned short *data_i = (indices == NULL ? NULL : (unsigned short *)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));
    if (data != NULL) {
        memcpy(data, values, bytes);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
    if (data_i != NULL) {
        memcpy(data_i, indices, bytes_indices);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    }
#elif defined(R_USE_BUFFER_UPDATE)
    glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, values);
    if (indices != NULL) glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, bytes_indices, indices);
#else
#error "SDL_gpu's VBO upload needs to choose R_USE_BUFFER_RESET, R_USE_BUFFER_MAPPING, or R_USE_BUFFER_UPDATE and none is defined!"
#endif
#else
    (void)indices;
#endif
}

void SetAttributefv(R_Renderer *renderer, int location, int num_elements, float *value);

#ifdef R_USE_BUFFER_PIPELINE
void gpu_upload_modelviewprojection(R_Target *dest, R_Context *context) {
    if (context->current_shader_block.modelViewProjection_loc >= 0) {
        float mvp[16];

        // MVP = P * V * M

        // P
        R_MatrixCopy(mvp, R_GetTopMatrix(&dest->projection_matrix));

        // V
        if (dest->use_camera) {
            float cam_matrix[16];
            get_camera_matrix(dest, cam_matrix);

            R_MultiplyAndAssign(mvp, cam_matrix);
        } else {
            R_MultiplyAndAssign(mvp, R_GetTopMatrix(&dest->view_matrix));
        }

        // M
        R_MultiplyAndAssign(mvp, R_GetTopMatrix(&dest->model_matrix));

        glUniformMatrix4fv(context->current_shader_block.modelViewProjection_loc, 1, 0, mvp);
    }
}
#endif

// Assumes the right format
void PrimitiveBatchV(R_Renderer *renderer, R_Image *image, R_Target *target, R_PrimitiveEnum primitive_type, unsigned short num_vertices, void *values, unsigned int num_indices,
                     unsigned short *indices, R_BatchFlagEnum flags) {
    R_Context *context;
    R_CONTEXT_DATA *cdata;
    int stride;
    intptr_t offset_texcoords, offset_colors;
    int size_vertices, size_texcoords, size_colors;

    bool using_texture = (image != NULL);
    bool use_vertices = (flags & (R_BATCH_XY | R_BATCH_XYZ));
    bool use_texcoords = (flags & R_BATCH_ST);
    bool use_colors = (flags & (R_BATCH_RGB | R_BATCH_RGBA | R_BATCH_RGB8 | R_BATCH_RGBA8));
    bool use_byte_colors = (flags & (R_BATCH_RGB8 | R_BATCH_RGBA8));
    bool use_z = (flags & R_BATCH_XYZ);
    bool use_a = (flags & (R_BATCH_RGBA | R_BATCH_RGBA8));

    if (num_vertices == 0) return;

    if (target == NULL) {
        R_PushErrorCode("R_PrimitiveBatchX", R_ERROR_NULL_ARGUMENT, "target");
        return;
    }
    if ((image != NULL && renderer != image->renderer) || renderer != target->renderer) {
        R_PushErrorCode("R_PrimitiveBatchX", R_ERROR_USER_ERROR, "Mismatched renderer");
        return;
    }

    makeContextCurrent(renderer, target);

    // Bind the texture to which subsequent calls refer
    if (using_texture) bindTexture(renderer, image);

    // Bind the FBO
    if (!SetActiveTarget(renderer, target)) {
        R_PushErrorCode("R_PrimitiveBatchX", R_ERROR_BACKEND_ERROR, "Failed to bind framebuffer.");
        return;
    }

    prepareToRenderToTarget(renderer, target);
    if (using_texture)
        prepareToRenderImage(renderer, target, image);
    else
        prepareToRenderShapes(renderer, primitive_type);
    changeViewport(target);
    changeCamera(target);

    if (using_texture) changeTexturing(renderer, true);

    setClipRect(renderer, target);

    context = renderer->current_context_target->context;
    cdata = (R_CONTEXT_DATA *)context->data;

    FlushBlitBuffer(renderer);

    if (cdata->index_buffer_num_vertices + num_indices >= cdata->index_buffer_max_num_vertices) {
        growBlitBuffer(cdata, cdata->index_buffer_num_vertices + num_indices);
    }
    if (cdata->blit_buffer_num_vertices + num_vertices >= cdata->blit_buffer_max_num_vertices) {
        growBlitBuffer(cdata, cdata->blit_buffer_num_vertices + num_vertices);
    }

    // Only need to check the blit buffer because of the VBO storage
    if (cdata->blit_buffer_num_vertices + num_vertices >= cdata->blit_buffer_max_num_vertices) {
        if (!growBlitBuffer(cdata, cdata->blit_buffer_num_vertices + num_vertices)) {
            // Can't do all of these sprites!  Only do some of them...
            num_vertices = (cdata->blit_buffer_max_num_vertices - cdata->blit_buffer_num_vertices);
        }
    }
    if (cdata->index_buffer_num_vertices + num_indices >= cdata->index_buffer_max_num_vertices) {
        if (!growIndexBuffer(cdata, cdata->index_buffer_num_vertices + num_indices)) {
            // Can't do all of these sprites!  Only do some of them...
            num_indices = (cdata->index_buffer_max_num_vertices - cdata->index_buffer_num_vertices);
        }
    }

#ifdef R_USE_BUFFER_PIPELINE
    refresh_attribute_data(cdata);
#endif

    if (indices == NULL) num_indices = num_vertices;

    (void)stride;
    (void)offset_texcoords;
    (void)offset_colors;
    (void)size_vertices;
    (void)size_texcoords;
    (void)size_colors;

    stride = 0;
    offset_texcoords = offset_colors = 0;
    size_vertices = size_texcoords = size_colors = 0;

    // Determine stride, size, and offsets
    if (use_vertices) {
        if (use_z)
            size_vertices = 3;
        else
            size_vertices = 2;

        stride += size_vertices;

        offset_texcoords = stride;
        offset_colors = stride;
    }

    if (use_texcoords) {
        size_texcoords = 2;

        stride += size_texcoords;

        offset_colors = stride;
    }

    if (use_colors) {
        if (use_a)
            size_colors = 4;
        else
            size_colors = 3;
    }

    // Floating point color components (either 3 or 4 floats)
    if (use_colors && !use_byte_colors) {
        stride += size_colors;
    }

    // Convert offsets to a number of bytes
    stride *= sizeof(float);
    offset_texcoords *= sizeof(float);
    offset_colors *= sizeof(float);

    // Unsigned byte color components (either 3 or 4 bytes)
    if (use_colors && use_byte_colors) {
        stride += size_colors;
    }

#ifdef R_USE_BUFFER_PIPELINE
    {
        // Skip uploads if we have no attribute location
        if (context->current_shader_block.position_loc < 0) use_vertices = false;
        if (context->current_shader_block.texcoord_loc < 0) use_texcoords = false;
        if (context->current_shader_block.color_loc < 0) use_colors = false;

// Update the vertex array object's buffers
#if !defined(R_NO_VAO)
        glBindVertexArray(cdata->blit_VAO);
#endif

        gpu_upload_modelviewprojection(target, context);

        if (values != NULL) {
            // Upload blit buffer to a single buffer object
            glBindBuffer(GL_ARRAY_BUFFER, cdata->blit_VBO[cdata->blit_VBO_flop]);
            cdata->blit_VBO_flop = !cdata->blit_VBO_flop;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cdata->blit_IBO);

            // Copy the whole blit buffer to the GPU
            submit_buffer_data(stride * num_vertices, (float *)values, sizeof(unsigned short) * num_indices,
                               indices);  // Fills GPU buffer with data.

            // Specify the formatting of the blit buffer
            if (use_vertices) {
                glEnableVertexAttribArray(context->current_shader_block.position_loc);  // Tell GL to use client-side attribute data
                glVertexAttribPointer(context->current_shader_block.position_loc, size_vertices, GL_FLOAT, GL_FALSE, stride,
                                      0);  // Tell how the data is formatted
            }
            if (use_texcoords) {
                glEnableVertexAttribArray(context->current_shader_block.texcoord_loc);
                glVertexAttribPointer(context->current_shader_block.texcoord_loc, size_texcoords, GL_FLOAT, GL_FALSE, stride, (void *)(offset_texcoords));
            }
            if (use_colors) {
                glEnableVertexAttribArray(context->current_shader_block.color_loc);
                if (use_byte_colors) {
                    glVertexAttribPointer(context->current_shader_block.color_loc, size_colors, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void *)(offset_colors));
                } else {
                    glVertexAttribPointer(context->current_shader_block.color_loc, size_colors, GL_FLOAT, GL_FALSE, stride, (void *)(offset_colors));
                }
            } else {
                SDL_Color color = get_complete_mod_color(renderer, target, image);
                float default_color[4] = {color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, GET_ALPHA(color) / 255.0f};
                SetAttributefv(renderer, context->current_shader_block.color_loc, 4, default_color);
            }
        }

        upload_attribute_data(cdata, num_indices);

        if (indices == NULL)
            glDrawArrays(primitive_type, 0, num_indices);
        else
            glDrawElements(primitive_type, num_indices, GL_UNSIGNED_SHORT, (void *)0);

        // Disable the vertex arrays again
        if (use_vertices) glDisableVertexAttribArray(context->current_shader_block.position_loc);
        if (use_texcoords) glDisableVertexAttribArray(context->current_shader_block.texcoord_loc);
        if (use_colors) glDisableVertexAttribArray(context->current_shader_block.color_loc);

        disable_attribute_data(cdata);

#if !defined(R_NO_VAO)
        glBindVertexArray(0);
#endif
    }
#endif

    cdata->blit_buffer_num_vertices = 0;
    cdata->index_buffer_num_vertices = 0;

    unsetClipRect(renderer, target);
}

void GenerateMipmaps(R_Renderer *renderer, R_Image *image) {
#ifndef __IPHONEOS__
    GLint filter;
    if (image == NULL) return;

    if (image->target != NULL && isCurrentTarget(renderer, image->target)) FlushBlitBuffer(renderer);
    bindTexture(renderer, image);
    glGenerateMipmapPROC(GL_TEXTURE_2D);
    image->has_mipmaps = true;

    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &filter);
    if (filter == GL_LINEAR) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
#endif
}

MErect SetClip(R_Renderer *renderer, R_Target *target, Sint16 x, Sint16 y, u16 w, u16 h) {
    MErect r;
    if (target == NULL) {
        r.x = r.y = r.w = r.h = 0;
        return r;
    }

    if (isCurrentTarget(renderer, target)) FlushBlitBuffer(renderer);
    target->use_clip_rect = true;

    r = target->clip_rect;

    target->clip_rect.x = x;
    target->clip_rect.y = y;
    target->clip_rect.w = w;
    target->clip_rect.h = h;

    return r;
}

void UnsetClip(R_Renderer *renderer, R_Target *target) {
    if (target == NULL) return;

    if (isCurrentTarget(renderer, target)) FlushBlitBuffer(renderer);
    // Leave the clip rect values intact so they can still be useful as storage
    target->use_clip_rect = false;
}

void swizzle_for_format(SDL_Color *color, GLenum format, unsigned char pixel[4]) {
    switch (format) {
        case GL_LUMINANCE:
            color->b = color->g = color->r = pixel[0];
            GET_ALPHA(*color) = 255;
            break;
        case GL_LUMINANCE_ALPHA:
            color->b = color->g = color->r = pixel[0];
            GET_ALPHA(*color) = pixel[3];
            break;
#ifdef GL_BGR
        case GL_BGR:
            color->b = pixel[0];
            color->g = pixel[1];
            color->r = pixel[2];
            GET_ALPHA(*color) = 255;
            break;
#endif
#ifdef GL_BGRA
        case GL_BGRA:
            color->b = pixel[0];
            color->g = pixel[1];
            color->r = pixel[2];
            GET_ALPHA(*color) = pixel[3];
            break;
#endif
#ifdef GL_ABGR
        case GL_ABGR:
            GET_ALPHA(*color) = pixel[0];
            color->b = pixel[1];
            color->g = pixel[2];
            color->r = pixel[3];
            break;
#endif
        case GL_ALPHA:
            break;
        case GL_RG:
            color->r = pixel[0];
            color->g = pixel[1];
            color->b = 0;
            GET_ALPHA(*color) = 255;
            break;
        case GL_RGB:
            color->r = pixel[0];
            color->g = pixel[1];
            color->b = pixel[2];
            GET_ALPHA(*color) = 255;
            break;
        case GL_RGBA:
            color->r = pixel[0];
            color->g = pixel[1];
            color->b = pixel[2];
            GET_ALPHA(*color) = pixel[3];
            break;
        default:
            break;
    }
}

MEcolor GetPixel(R_Renderer *renderer, R_Target *target, Sint16 x, Sint16 y) {
    SDL_Color result = {0, 0, 0, 0};
    if (target == NULL) return ToEngineColor(result);
    if (renderer != target->renderer) return ToEngineColor(result);
    if (x < 0 || y < 0 || x >= target->w || y >= target->h) return ToEngineColor(result);

    if (isCurrentTarget(renderer, target)) FlushBlitBuffer(renderer);
    if (SetActiveTarget(renderer, target)) {
        unsigned char pixels[4];
        GLenum format = ((R_TARGET_DATA *)target->data)->format;
        glReadPixels(x, y, 1, 1, format, GL_UNSIGNED_BYTE, pixels);

        swizzle_for_format(&result, format, pixels);
    }

    return ToEngineColor(result);
}

void SetImageFilter(R_Renderer *renderer, R_Image *image, R_FilterEnum filter) {
    GLenum minFilter, magFilter;

    if (image == NULL) {
        R_PushErrorCode("R_SetImageFilter", R_ERROR_NULL_ARGUMENT, "image");
        return;
    }
    if (renderer != image->renderer) {
        R_PushErrorCode("R_SetImageFilter", R_ERROR_USER_ERROR, "Mismatched renderer");
        return;
    }

    switch (filter) {
        case R_FILTER_NEAREST:
            minFilter = GL_NEAREST;
            magFilter = GL_NEAREST;
            break;
        case R_FILTER_LINEAR:
            if (image->has_mipmaps)
                minFilter = GL_LINEAR_MIPMAP_NEAREST;
            else
                minFilter = GL_LINEAR;

            magFilter = GL_LINEAR;
            break;
        case R_FILTER_LINEAR_MIPMAP:
            if (image->has_mipmaps)
                minFilter = GL_LINEAR_MIPMAP_LINEAR;
            else
                minFilter = GL_LINEAR;

            magFilter = GL_LINEAR;
            break;
        default:
            R_PushErrorCode("R_SetImageFilter", R_ERROR_USER_ERROR, "Unsupported value for filter (0x%x)", filter);
            return;
    }

    flushBlitBufferIfCurrentTexture(renderer, image);
    bindTexture(renderer, image);

    image->filter_mode = filter;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}

void SetWrapMode(R_Renderer *renderer, R_Image *image, R_WrapEnum wrap_mode_x, R_WrapEnum wrap_mode_y) {
    GLenum wrap_x, wrap_y;

    if (image == NULL) {
        R_PushErrorCode("R_SetWrapMode", R_ERROR_NULL_ARGUMENT, "image");
        return;
    }
    if (renderer != image->renderer) {
        R_PushErrorCode("R_SetWrapMode", R_ERROR_USER_ERROR, "Mismatched renderer");
        return;
    }

    switch (wrap_mode_x) {
        case R_WRAP_NONE:
            wrap_x = GL_CLAMP_TO_EDGE;
            break;
        case R_WRAP_REPEAT:
            wrap_x = GL_REPEAT;
            break;
        case R_WRAP_MIRRORED:
            if (renderer->enabled_features & R_FEATURE_WRAP_REPEAT_MIRRORED)
                wrap_x = GL_MIRRORED_REPEAT;
            else {
                R_PushErrorCode("R_SetWrapMode", R_ERROR_BACKEND_ERROR, "This renderer does not support R_WRAP_MIRRORED.");
                return;
            }
            break;
        default:
            R_PushErrorCode("R_SetWrapMode", R_ERROR_USER_ERROR, "Unsupported value for wrap_mode_x (0x%x)", wrap_mode_x);
            return;
    }

    switch (wrap_mode_y) {
        case R_WRAP_NONE:
            wrap_y = GL_CLAMP_TO_EDGE;
            break;
        case R_WRAP_REPEAT:
            wrap_y = GL_REPEAT;
            break;
        case R_WRAP_MIRRORED:
            if (renderer->enabled_features & R_FEATURE_WRAP_REPEAT_MIRRORED)
                wrap_y = GL_MIRRORED_REPEAT;
            else {
                R_PushErrorCode("R_SetWrapMode", R_ERROR_BACKEND_ERROR, "This renderer does not support R_WRAP_MIRRORED.");
                return;
            }
            break;
        default:
            R_PushErrorCode("R_SetWrapMode", R_ERROR_USER_ERROR, "Unsupported value for wrap_mode_y (0x%x)", wrap_mode_y);
            return;
    }

    flushBlitBufferIfCurrentTexture(renderer, image);
    bindTexture(renderer, image);

    image->wrap_mode_x = wrap_mode_x;
    image->wrap_mode_y = wrap_mode_y;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_x);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_y);
}

R_TextureHandle GetTextureHandle(R_Renderer *renderer, R_Image *image) {
    (void)renderer;
    return ((R_IMAGE_DATA *)image->data)->handle;
}

void ClearRGBA(R_Renderer *renderer, R_Target *target, u8 r, u8 g, u8 b, u8 a) {
    if (target == NULL) return;
    if (renderer != target->renderer) return;

    makeContextCurrent(renderer, target);

    if (isCurrentTarget(renderer, target)) FlushBlitBuffer(renderer);
    if (SetActiveTarget(renderer, target)) {
        setClipRect(renderer, target);

        glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        unsetClipRect(renderer, target);
    }
}

void DoPartialFlush(R_Renderer *renderer, R_Target *dest, R_Context *context, unsigned short num_vertices, float *blit_buffer, unsigned int num_indices, unsigned short *index_buffer) {
    R_CONTEXT_DATA *cdata = (R_CONTEXT_DATA *)context->data;

#ifdef R_USE_BUFFER_PIPELINE
    {
// Update the vertex array object's buffers
#if !defined(R_NO_VAO)
        glBindVertexArray(cdata->blit_VAO);
#endif

        gpu_upload_modelviewprojection(dest, context);

        // Upload blit buffer to a single buffer object
        glBindBuffer(GL_ARRAY_BUFFER, cdata->blit_VBO[cdata->blit_VBO_flop]);
        cdata->blit_VBO_flop = !cdata->blit_VBO_flop;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cdata->blit_IBO);

        // Copy the whole blit buffer to the GPU
        submit_buffer_data(R_BLIT_BUFFER_STRIDE * num_vertices, blit_buffer, sizeof(unsigned short) * num_indices,
                           index_buffer);  // Fills GPU buffer with data.

        // Specify the formatting of the blit buffer
        if (context->current_shader_block.position_loc >= 0) {
            glEnableVertexAttribArray(context->current_shader_block.position_loc);  // Tell GL to use client-side attribute data
            glVertexAttribPointer(context->current_shader_block.position_loc, 2, GL_FLOAT, GL_FALSE, R_BLIT_BUFFER_STRIDE,
                                  0);  // Tell how the data is formatted
        }
        if (context->current_shader_block.texcoord_loc >= 0) {
            glEnableVertexAttribArray(context->current_shader_block.texcoord_loc);
            glVertexAttribPointer(context->current_shader_block.texcoord_loc, 2, GL_FLOAT, GL_FALSE, R_BLIT_BUFFER_STRIDE, (void *)(R_BLIT_BUFFER_TEX_COORD_OFFSET * sizeof(float)));
        }
        if (context->current_shader_block.color_loc >= 0) {
            glEnableVertexAttribArray(context->current_shader_block.color_loc);
            glVertexAttribPointer(context->current_shader_block.color_loc, 4, GL_FLOAT, GL_FALSE, R_BLIT_BUFFER_STRIDE, (void *)(R_BLIT_BUFFER_COLOR_OFFSET * sizeof(float)));
        }

        upload_attribute_data(cdata, num_vertices);

        glDrawElements(cdata->last_shape, num_indices, GL_UNSIGNED_SHORT, (void *)0);

        // Disable the vertex arrays again
        if (context->current_shader_block.position_loc >= 0) glDisableVertexAttribArray(context->current_shader_block.position_loc);
        if (context->current_shader_block.texcoord_loc >= 0) glDisableVertexAttribArray(context->current_shader_block.texcoord_loc);
        if (context->current_shader_block.color_loc >= 0) glDisableVertexAttribArray(context->current_shader_block.color_loc);

        disable_attribute_data(cdata);

#if !defined(R_NO_VAO)
        glBindVertexArray(0);
#endif
    }
#endif
}

void DoUntexturedFlush(R_Renderer *renderer, R_Target *dest, R_Context *context, unsigned short num_vertices, float *blit_buffer, unsigned int num_indices, unsigned short *index_buffer) {
    R_CONTEXT_DATA *cdata = (R_CONTEXT_DATA *)context->data;

#ifdef R_USE_BUFFER_PIPELINE
    {
// Update the vertex array object's buffers
#if !defined(R_NO_VAO)
        glBindVertexArray(cdata->blit_VAO);
#endif

        gpu_upload_modelviewprojection(dest, context);

        // Upload blit buffer to a single buffer object
        glBindBuffer(GL_ARRAY_BUFFER, cdata->blit_VBO[cdata->blit_VBO_flop]);
        cdata->blit_VBO_flop = !cdata->blit_VBO_flop;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cdata->blit_IBO);

        // Copy the whole blit buffer to the GPU
        submit_buffer_data(R_BLIT_BUFFER_STRIDE * num_vertices, blit_buffer, sizeof(unsigned short) * num_indices,
                           index_buffer);  // Fills GPU buffer with data.

        // Specify the formatting of the blit buffer
        if (context->current_shader_block.position_loc >= 0) {
            glEnableVertexAttribArray(context->current_shader_block.position_loc);  // Tell GL to use client-side attribute data
            glVertexAttribPointer(context->current_shader_block.position_loc, 2, GL_FLOAT, GL_FALSE, R_BLIT_BUFFER_STRIDE,
                                  0);  // Tell how the data is formatted
        }
        if (context->current_shader_block.color_loc >= 0) {
            glEnableVertexAttribArray(context->current_shader_block.color_loc);
            glVertexAttribPointer(context->current_shader_block.color_loc, 4, GL_FLOAT, GL_FALSE, R_BLIT_BUFFER_STRIDE, (void *)(R_BLIT_BUFFER_COLOR_OFFSET * sizeof(float)));
        }

        upload_attribute_data(cdata, num_vertices);

        glDrawElements(cdata->last_shape, num_indices, GL_UNSIGNED_SHORT, (void *)0);

        // Disable the vertex arrays again
        if (context->current_shader_block.position_loc >= 0) glDisableVertexAttribArray(context->current_shader_block.position_loc);
        if (context->current_shader_block.color_loc >= 0) glDisableVertexAttribArray(context->current_shader_block.color_loc);

        disable_attribute_data(cdata);

#if !defined(R_NO_VAO)
        glBindVertexArray(0);
#endif
    }
#endif
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))

void FlushBlitBuffer(R_Renderer *renderer) {
    R_Context *context;
    R_CONTEXT_DATA *cdata;
    if (renderer->current_context_target == NULL) return;

    context = renderer->current_context_target->context;
    cdata = (R_CONTEXT_DATA *)context->data;
    if (cdata->blit_buffer_num_vertices > 0 && context->active_target != NULL) {
        R_Target *dest = context->active_target;
        int num_vertices;
        int num_indices;
        float *blit_buffer;
        unsigned short *index_buffer;

        changeViewport(dest);
        changeCamera(dest);

        applyTexturing(renderer);

        setClipRect(renderer, dest);

#ifdef R_USE_BUFFER_PIPELINE
        refresh_attribute_data(cdata);
#endif

        blit_buffer = cdata->blit_buffer;
        index_buffer = cdata->index_buffer;

        if (cdata->last_use_texturing) {
            while (cdata->blit_buffer_num_vertices > 0) {
                num_vertices = MAX(cdata->blit_buffer_num_vertices, get_lowest_attribute_num_values(cdata, cdata->blit_buffer_num_vertices));
                num_indices = num_vertices * 3 / 2;  // 6 indices per sprite / 4 vertices per sprite = 3/2

                DoPartialFlush(renderer, dest, context, (unsigned short)num_vertices, blit_buffer, (unsigned int)num_indices, index_buffer);

                cdata->blit_buffer_num_vertices -= (unsigned short)num_vertices;
                // Move our pointers ahead
                blit_buffer += R_BLIT_BUFFER_FLOATS_PER_VERTEX * num_vertices;
                index_buffer += num_indices;
            }
        } else {
            DoUntexturedFlush(renderer, dest, context, cdata->blit_buffer_num_vertices, blit_buffer, cdata->index_buffer_num_vertices, index_buffer);
        }

        cdata->blit_buffer_num_vertices = 0;
        cdata->index_buffer_num_vertices = 0;

        unsetClipRect(renderer, dest);
    }
}

void Flip(R_Renderer *renderer, R_Target *target) {
    FlushBlitBuffer(renderer);

    if (target != NULL && target->context != NULL) {
        makeContextCurrent(renderer, target);

        SDL_GL_SwapWindow(SDL_GetWindowFromID(renderer->current_context_target->context->windowID));
    }

    if (vendor_is_Intel) apply_Intel_attrib_workaround = true;
}

// Shader API

#include <string.h>

// On some platforms (e.g. Android), it might not be possible to just create a rwops and get the expected #included files.
// To do it, I might want to add an optional argument that specifies a base directory to prepend to #include file names.

u32 GetShaderSourceSize(const char *filename);
u32 GetShaderSource(const char *filename, char *result);

void read_until_end_of_comment(SDL_RWops *rwops, char multiline) {
    char buffer;
    while (SDL_RWread(rwops, &buffer, 1, 1) > 0) {
        if (!multiline) {
            if (buffer == '\n') break;
        } else {
            if (buffer == '*') {
                // If the stream ends at the next character or it is a '/', then we're done.
                if (SDL_RWread(rwops, &buffer, 1, 1) <= 0 || buffer == '/') break;
            }
        }
    }
}

u32 GetShaderSourceSize_RW(SDL_RWops *shader_source) {
    u32 size;
    char last_char;
    char buffer[512];
    long len;

    if (shader_source == NULL) return 0;

    size = 0;

    // Read 1 byte at a time until we reach the end
    last_char = ' ';
    len = 0;
    while ((len = SDL_RWread(shader_source, &buffer, 1, 1)) > 0) {
        // Follow through an #include directive?
        if (buffer[0] == '#') {
            // Get the rest of the line
            int line_size = 1;
            unsigned long line_len;
            char *token;
            while ((line_len = SDL_RWread(shader_source, buffer + line_size, 1, 1)) > 0) {
                line_size += line_len;
                if (buffer[line_size - line_len] == '\n') break;
            }
            buffer[line_size] = '\0';

            // Is there "include" after '#'?
            token = strtok(buffer, "# \t");

            if (token != NULL && strcmp(token, "include") == 0) {
                // Get filename token
                token = strtok(NULL, "\"");  // Skip the empty token before the quote
                if (token != NULL) {
                    // Add the size of the included file and a newline character
                    size += GetShaderSourceSize(token) + 1;
                }
            } else
                size += line_size;
            last_char = ' ';
            continue;
        }

        size += len;

        if (last_char == '/') {
            if (buffer[0] == '/') {
                read_until_end_of_comment(shader_source, 0);
                size++;  // For the end of the comment
            } else if (buffer[0] == '*') {
                read_until_end_of_comment(shader_source, 1);
                size += 2;  // For the end of the comments
            }
            last_char = ' ';
        } else
            last_char = buffer[0];
    }

    // Go back to the beginning of the stream
    SDL_RWseek(shader_source, 0, SEEK_SET);
    return size;
}

u32 GetShaderSource_RW(SDL_RWops *shader_source, char *result) {
    u32 size;
    char last_char;
    char buffer[512];
    long len;

    if (shader_source == NULL) {
        result[0] = '\0';
        return 0;
    }

    size = 0;

    // Read 1 byte at a time until we reach the end
    last_char = ' ';
    len = 0;
    while ((len = SDL_RWread(shader_source, &buffer, 1, 1)) > 0) {
        // Follow through an #include directive?
        if (buffer[0] == '#') {
            // Get the rest of the line
            int line_size = 1;
            unsigned long line_len;
            char token_buffer[512];  // strtok() is destructive
            char *token;
            while ((line_len = SDL_RWread(shader_source, buffer + line_size, 1, 1)) > 0) {
                line_size += line_len;
                if (buffer[line_size - line_len] == '\n') break;
            }

            // Is there "include" after '#'?
            memcpy(token_buffer, buffer, line_size + 1);
            token_buffer[line_size] = '\0';
            token = strtok(token_buffer, "# \t");

            if (token != NULL && strcmp(token, "include") == 0) {
                // Get filename token
                token = strtok(NULL, "\"");  // Skip the empty token before the quote
                if (token != NULL) {
                    // Add the size of the included file and a newline character
                    size += GetShaderSource(token, result + size);
                    result[size] = '\n';
                    size++;
                }
            } else {
                memcpy(result + size, buffer, line_size);
                size += line_size;
            }
            last_char = ' ';
            continue;
        }

        memcpy(result + size, buffer, len);
        size += len;

        if (last_char == '/') {
            if (buffer[0] == '/') {
                read_until_end_of_comment(shader_source, 0);
                memcpy(result + size, "\n", 1);
                size++;
            } else if (buffer[0] == '*') {
                read_until_end_of_comment(shader_source, 1);
                memcpy(result + size, "*/", 2);
                size += 2;
            }
            last_char = ' ';
        } else
            last_char = buffer[0];
    }
    result[size] = '\0';

    // Go back to the beginning of the stream
    SDL_RWseek(shader_source, 0, SEEK_SET);
    return size;
}

u32 GetShaderSource(const char *filename, char *result) {
    SDL_RWops *rwops;
    u32 size;

    if (filename == NULL) return 0;
    rwops = SDL_RWFromFile(filename, "r");

    size = GetShaderSource_RW(rwops, result);

    SDL_RWclose(rwops);
    return size;
}

u32 GetShaderSourceSize(const char *filename) {
    SDL_RWops *rwops;
    u32 result;

    if (filename == NULL) return 0;
    rwops = SDL_RWFromFile(filename, "r");

    result = GetShaderSourceSize_RW(rwops);

    SDL_RWclose(rwops);
    return result;
}

u32 compile_shader_source(R_ShaderEnum shader_type, const char *shader_source) {
    // Create the proper new shader object
    GLuint shader_object = 0;
    (void)shader_type;
    (void)shader_source;

    GLint compiled;

    switch (shader_type) {
        case R_VERTEX_SHADER:
            shader_object = glCreateShader(GL_VERTEX_SHADER);
            break;
        case R_FRAGMENT_SHADER:
            shader_object = glCreateShader(GL_FRAGMENT_SHADER);
            break;
        case R_GEOMETRY_SHADER:
#ifdef GL_GEOMETRY_SHADER
            shader_object = glCreateShader(GL_GEOMETRY_SHADER);
#else
            R_PushErrorCode("R_CompileShader", R_ERROR_BACKEND_ERROR, "Hardware does not support R_GEOMETRY_SHADER.");
            snprintf(shader_message, 256, "Failed to create geometry shader object.\n");
            return 0;
#endif
            break;
    }

    if (shader_object == 0) {
        R_PushErrorCode("R_CompileShader", R_ERROR_BACKEND_ERROR, "Failed to create new shader object");
        snprintf(shader_message, 256, "Failed to create new shader object.\n");
        return 0;
    }

    glShaderSource(shader_object, 1, &shader_source, NULL);

    // Compile the shader source

    glCompileShader(shader_object);

    glGetShaderiv(shader_object, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        R_PushErrorCode("R_CompileShader", R_ERROR_DATA_ERROR, "Failed to compile shader source");
        glGetShaderInfoLog(shader_object, 256, NULL, shader_message);
        glDeleteShader(shader_object);
        return 0;
    }

    return shader_object;
}

u32 CompileShaderInternal(R_Renderer *renderer, R_ShaderEnum shader_type, const char *shader_source) { return compile_shader_source(shader_type, shader_source); }

u32 CompileShader(R_Renderer *renderer, R_ShaderEnum shader_type, const char *shader_source) {
    u32 size = (u32)strlen(shader_source);
    if (size == 0) return 0;
    return CompileShaderInternal(renderer, shader_type, shader_source);
}

u32 CreateShaderProgram(R_Renderer *renderer) {
    GLuint p;

    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return 0;

    p = glCreateProgram();

    return p;
}

bool LinkShaderProgram(R_Renderer *renderer, u32 program_object) {
    int linked;

    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return false;

    // Bind the position attribute to location 0.
    // We always pass position data (right?), but on some systems (e.g. GL 2 on OS X), color is bound to 0
    // and the shader won't run when TriangleBatch uses R_BATCH_XY_ST (no color array).  Guess they didn't consider default attribute values...
    glBindAttribLocation(program_object, 0, "gpu_Vertex");
    glLinkProgram(program_object);

    glGetProgramiv(program_object, GL_LINK_STATUS, &linked);

    if (!linked) {
        R_PushErrorCode("R_LinkShaderProgram", R_ERROR_BACKEND_ERROR, "Failed to link shader program");
        glGetProgramInfoLog(program_object, 256, NULL, shader_message);
        glDeleteProgram(program_object);
        return false;
    }

    return true;
}

void FreeShader(R_Renderer *renderer, u32 shader_object) {
    if (IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) glDeleteShader(shader_object);
}

void FreeShaderProgram(R_Renderer *renderer, u32 program_object) {
    if (IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) glDeleteProgram(program_object);
}

void AttachShader(R_Renderer *renderer, u32 program_object, u32 shader_object) {
    if (IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) glAttachShader(program_object, shader_object);
}

void DetachShader(R_Renderer *renderer, u32 program_object, u32 shader_object) {
    if (IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) glDetachShader(program_object, shader_object);
}

void ActivateShaderProgram(R_Renderer *renderer, u32 program_object, R_ShaderBlock *block) {
    R_Target *target = renderer->current_context_target;
    (void)block;

    if (IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) {
        if (program_object == 0)  // Implies default shader
        {
            // Already using a default shader?
            if (target->context->current_shader_program == target->context->default_textured_shader_program ||
                target->context->current_shader_program == target->context->default_untextured_shader_program)
                return;

            program_object = target->context->default_untextured_shader_program;
        }

        FlushBlitBuffer(renderer);
        glUseProgram(program_object);

        {
            // Set up our shader attribute and uniform locations
            if (block == NULL) {
                if (program_object == target->context->default_textured_shader_program)
                    target->context->current_shader_block = target->context->default_textured_shader_block;
                else if (program_object == target->context->default_untextured_shader_program)
                    target->context->current_shader_block = target->context->default_untextured_shader_block;
                else {
                    R_ShaderBlock b;
                    b.position_loc = -1;
                    b.texcoord_loc = -1;
                    b.color_loc = -1;
                    b.modelViewProjection_loc = -1;
                    target->context->current_shader_block = b;
                }
            } else
                target->context->current_shader_block = *block;
        }
    }

    target->context->current_shader_program = program_object;
}

void DeactivateShaderProgram(R_Renderer *renderer) { ActivateShaderProgram(renderer, 0, NULL); }

const char *GetShaderMessage(R_Renderer *renderer) {
    (void)renderer;
    return shader_message;
}

int GetAttributeLocation(R_Renderer *renderer, u32 program_object, const char *attrib_name) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return -1;
    program_object = get_proper_program_id(renderer, program_object);
    if (program_object == 0) return -1;
    return glGetAttribLocation(program_object, attrib_name);
}

int GetUniformLocation(R_Renderer *renderer, u32 program_object, const char *uniform_name) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return -1;
    program_object = get_proper_program_id(renderer, program_object);
    if (program_object == 0) return -1;
    return glGetUniformLocation(program_object, uniform_name);
}

R_ShaderBlock LoadShaderBlock(R_Renderer *renderer, u32 program_object, const char *position_name, const char *texcoord_name, const char *color_name, const char *modelViewMatrix_name) {
    R_ShaderBlock b;
    program_object = get_proper_program_id(renderer, program_object);
    if (program_object == 0 || !IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) {
        b.position_loc = -1;
        b.texcoord_loc = -1;
        b.color_loc = -1;
        b.modelViewProjection_loc = -1;
        return b;
    }

    if (position_name == NULL)
        b.position_loc = -1;
    else
        b.position_loc = GetAttributeLocation(renderer, program_object, position_name);

    if (texcoord_name == NULL)
        b.texcoord_loc = -1;
    else
        b.texcoord_loc = GetAttributeLocation(renderer, program_object, texcoord_name);

    if (color_name == NULL)
        b.color_loc = -1;
    else
        b.color_loc = GetAttributeLocation(renderer, program_object, color_name);

    if (modelViewMatrix_name == NULL)
        b.modelViewProjection_loc = -1;
    else
        b.modelViewProjection_loc = GetUniformLocation(renderer, program_object, modelViewMatrix_name);

    return b;
}

void SetShaderImage(R_Renderer *renderer, R_Image *image, int location, int image_unit) {
    // TODO: OpenGL 1 needs to check for ARB_multitexture to use glActiveTexture().
    u32 new_texture;

    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;

    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0 || image_unit < 0) return;

    new_texture = 0;
    if (image != NULL) new_texture = ((R_IMAGE_DATA *)image->data)->handle;

    // Set the new image unit
    glUniform1i(location, image_unit);
    glActiveTexture(GL_TEXTURE0 + image_unit);
    glBindTexture(GL_TEXTURE_2D, new_texture);

    if (image_unit != 0) glActiveTexture(GL_TEXTURE0);

    (void)renderer;
    (void)image;
    (void)location;
    (void)image_unit;
}

void GetUniformiv(R_Renderer *renderer, u32 program_object, int location, int *values) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    program_object = get_proper_program_id(renderer, program_object);
    if (program_object != 0) glGetUniformiv(program_object, location, values);
}

void SetUniformi(R_Renderer *renderer, int location, int value) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;
    glUniform1i(location, value);
}

void SetUniformiv(R_Renderer *renderer, int location, int num_elements_per_value, int num_values, int *values) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;
    switch (num_elements_per_value) {
        case 1:
            glUniform1iv(location, num_values, values);
            break;
        case 2:
            glUniform2iv(location, num_values, values);
            break;
        case 3:
            glUniform3iv(location, num_values, values);
            break;
        case 4:
            glUniform4iv(location, num_values, values);
            break;
    }
}

void GetUniformuiv(R_Renderer *renderer, u32 program_object, int location, unsigned int *values) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    program_object = get_proper_program_id(renderer, program_object);
    if (program_object != 0) glGetUniformuiv(program_object, location, values);
}

void SetUniformui(R_Renderer *renderer, int location, unsigned int value) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;
    glUniform1ui(location, value);
}

void SetUniformuiv(R_Renderer *renderer, int location, int num_elements_per_value, int num_values, unsigned int *values) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;
    switch (num_elements_per_value) {
        case 1:
            glUniform1uiv(location, num_values, values);
            break;
        case 2:
            glUniform2uiv(location, num_values, values);
            break;
        case 3:
            glUniform3uiv(location, num_values, values);
            break;
        case 4:
            glUniform4uiv(location, num_values, values);
            break;
    }
}

void GetUniformfv(R_Renderer *renderer, u32 program_object, int location, float *values) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    program_object = get_proper_program_id(renderer, program_object);
    if (program_object != 0) glGetUniformfv(program_object, location, values);
}

void SetUniformf(R_Renderer *renderer, int location, float value) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;
    glUniform1f(location, value);
}

void SetUniformfv(R_Renderer *renderer, int location, int num_elements_per_value, int num_values, float *values) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;
    switch (num_elements_per_value) {
        case 1:
            glUniform1fv(location, num_values, values);
            break;
        case 2:
            glUniform2fv(location, num_values, values);
            break;
        case 3:
            glUniform3fv(location, num_values, values);
            break;
        case 4:
            glUniform4fv(location, num_values, values);
            break;
    }
}

void SetUniformMatrixfv(R_Renderer *renderer, int location, int num_matrices, int num_rows, int num_columns, bool transpose, float *values) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;
    if (num_rows < 2 || num_rows > 4 || num_columns < 2 || num_columns > 4) {
        R_PushErrorCode("R_SetUniformMatrixfv", R_ERROR_DATA_ERROR, "Given invalid dimensions (%dx%d)", num_rows, num_columns);
        return;
    }

    switch (num_rows) {
        case 2:
            if (num_columns == 2)
                glUniformMatrix2fv(location, num_matrices, transpose, values);
            else if (num_columns == 3)
                glUniformMatrix2x3fv(location, num_matrices, transpose, values);
            else if (num_columns == 4)
                glUniformMatrix2x4fv(location, num_matrices, transpose, values);
            break;
        case 3:
            if (num_columns == 2)
                glUniformMatrix3x2fv(location, num_matrices, transpose, values);
            else if (num_columns == 3)
                glUniformMatrix3fv(location, num_matrices, transpose, values);
            else if (num_columns == 4)
                glUniformMatrix3x4fv(location, num_matrices, transpose, values);
            break;
        case 4:
            if (num_columns == 2)
                glUniformMatrix4x2fv(location, num_matrices, transpose, values);
            else if (num_columns == 3)
                glUniformMatrix4x3fv(location, num_matrices, transpose, values);
            else if (num_columns == 4)
                glUniformMatrix4fv(location, num_matrices, transpose, values);
            break;
    }
}

void SetAttributef(R_Renderer *renderer, int location, float value) {
    (void)renderer;
    (void)location;
    (void)value;

    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;

    if (apply_Intel_attrib_workaround && location == 0) {
        apply_Intel_attrib_workaround = false;
        glBegin(GL_TRIANGLES);
        glEnd();
    }

    glVertexAttrib1f(location, value);
}

void SetAttributei(R_Renderer *renderer, int location, int value) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;

    if (apply_Intel_attrib_workaround && location == 0) {
        apply_Intel_attrib_workaround = false;
        glBegin(GL_TRIANGLES);
        glEnd();
    }

    glVertexAttribI1i(location, value);
}

void SetAttributeui(R_Renderer *renderer, int location, unsigned int value) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;

    if (apply_Intel_attrib_workaround && location == 0) {
        apply_Intel_attrib_workaround = false;
        glBegin(GL_TRIANGLES);
        glEnd();
    }

    glVertexAttribI1ui(location, value);
}

void SetAttributefv(R_Renderer *renderer, int location, int num_elements, float *value) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;

    if (apply_Intel_attrib_workaround && location == 0) {
        apply_Intel_attrib_workaround = false;
        glBegin(GL_TRIANGLES);
        glEnd();
    }

    switch (num_elements) {
        case 1:
            glVertexAttrib1f(location, value[0]);
            break;
        case 2:
            glVertexAttrib2f(location, value[0], value[1]);
            break;
        case 3:
            glVertexAttrib3f(location, value[0], value[1], value[2]);
            break;
        case 4:
            glVertexAttrib4f(location, value[0], value[1], value[2], value[3]);
            break;
    }
}

void SetAttributeiv(R_Renderer *renderer, int location, int num_elements, int *value) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;

    if (apply_Intel_attrib_workaround && location == 0) {
        apply_Intel_attrib_workaround = false;
        glBegin(GL_TRIANGLES);
        glEnd();
    }

    switch (num_elements) {
        case 1:
            glVertexAttribI1i(location, value[0]);
            break;
        case 2:
            glVertexAttribI2i(location, value[0], value[1]);
            break;
        case 3:
            glVertexAttribI3i(location, value[0], value[1], value[2]);
            break;
        case 4:
            glVertexAttribI4i(location, value[0], value[1], value[2], value[3]);
            break;
    }
}

void SetAttributeuiv(R_Renderer *renderer, int location, int num_elements, unsigned int *value) {
    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    FlushBlitBuffer(renderer);
    if (renderer->current_context_target->context->current_shader_program == 0) return;

    if (apply_Intel_attrib_workaround && location == 0) {
        apply_Intel_attrib_workaround = false;
        glBegin(GL_TRIANGLES);
        glEnd();
    }

    switch (num_elements) {
        case 1:
            glVertexAttribI1ui(location, value[0]);
            break;
        case 2:
            glVertexAttribI2ui(location, value[0], value[1]);
            break;
        case 3:
            glVertexAttribI3ui(location, value[0], value[1], value[2]);
            break;
        case 4:
            glVertexAttribI4ui(location, value[0], value[1], value[2], value[3]);
            break;
    }
}

void SetAttributeSource(R_Renderer *renderer, int num_values, R_Attribute source) {
    R_CONTEXT_DATA *cdata;
    R_AttributeSource *a;

    if (!IsFeatureEnabled(renderer, R_FEATURE_BASIC_SHADERS)) return;
    if (source.location < 0 || source.location >= 16) return;

    FlushBlitBuffer(renderer);
    cdata = (R_CONTEXT_DATA *)renderer->current_context_target->context->data;
    a = &cdata->shader_attributes[source.location];
    if (source.format.is_per_sprite) {
        int needed_size;

        a->per_vertex_storage_offset_bytes = 0;
        a->per_vertex_storage_stride_bytes = source.format.num_elems_per_value * sizeof_R_type(source.format.type);
        a->num_values = 4 * num_values;  // 4 vertices now
        needed_size = a->num_values * a->per_vertex_storage_stride_bytes;

        // Make sure we have enough room for converted per-vertex data
        if (a->per_vertex_storage_size < needed_size) {
            ME_FREE(a->per_vertex_storage);
            a->per_vertex_storage = ME_MALLOC(needed_size);
            a->per_vertex_storage_size = needed_size;
        }
    } else if (a->per_vertex_storage_size > 0) {
        ME_FREE(a->per_vertex_storage);
        a->per_vertex_storage = NULL;
        a->per_vertex_storage_size = 0;
    }

    a->enabled = false;
    a->attribute = source;

    if (!source.format.is_per_sprite) {
        a->per_vertex_storage = source.values;
        a->num_values = num_values;
        a->per_vertex_storage_stride_bytes = source.format.stride_bytes;
        a->per_vertex_storage_offset_bytes = source.format.offset_bytes;
    }

    a->next_value = a->per_vertex_storage;
}

#if 0
#define SET_COMMON_FUNCTIONS(impl)                            \
    impl->init = &init;                                       \
    impl->CreateTargetFromWindow = &CreateTargetFromWindow;   \
    impl->SetActiveTarget = &SetActiveTarget;                 \
    impl->CreateAliasTarget = &CreateAliasTarget;             \
    impl->MakeCurrent = &MakeCurrent;                         \
    impl->SetAsCurrent = &SetAsCurrent;                       \
    impl->ResetRendererState = &ResetRendererState;           \
    impl->AddDepthBuffer = &AddDepthBuffer;                   \
    impl->SetWindowResolution = &SetWindowResolution;         \
    impl->SetVirtualResolution = &SetVirtualResolution;       \
    impl->UnsetVirtualResolution = &UnsetVirtualResolution;   \
    impl->Quit = &Quit;                                       \
                                                              \
    impl->SetFullscreen = &SetFullscreen;                     \
    impl->SetCamera = &SetCamera;                             \
                                                              \
    impl->CreateImage = &CreateImage;                         \
    impl->CreateImageUsingTexture = &CreateImageUsingTexture; \
    impl->CreateAliasImage = &CreateAliasImage;               \
    impl->CopyImage = &CopyImage;                             \
    impl->UpdateImage = &UpdateImage;                         \
    impl->UpdateImageBytes = &UpdateImageBytes;               \
    impl->ReplaceImage = &ReplaceImage;                       \
    impl->CopyImageFromSurface = &CopyImageFromSurface;       \
    impl->CopyImageFromTarget = &CopyImageFromTarget;         \
    impl->CopySurfaceFromTarget = &CopySurfaceFromTarget;     \
    impl->CopySurfaceFromImage = &CopySurfaceFromImage;       \
    impl->FreeImage = &FreeImage;                             \
                                                              \
    impl->GetTarget = &GetTarget;                             \
    impl->FreeTarget = &FreeTarget;                           \
                                                              \
    impl->Blit = &Blit;                                       \
    impl->BlitRotate = &BlitRotate;                           \
    impl->BlitScale = &BlitScale;                             \
    impl->BlitTransform = &BlitTransform;                     \
    impl->BlitTransformX = &BlitTransformX;                   \
    impl->PrimitiveBatchV = &PrimitiveBatchV;                 \
                                                              \
    impl->GenerateMipmaps = &GenerateMipmaps;                 \
                                                              \
    impl->SetClip = &SetClip;                                 \
    impl->UnsetClip = &UnsetClip;                             \
                                                              \
    impl->GetPixel = &GetPixel;                               \
    impl->SetImageFilter = &SetImageFilter;                   \
    impl->SetWrapMode = &SetWrapMode;                         \
    impl->GetTextureHandle = &GetTextureHandle;               \
                                                              \
    impl->ClearRGBA = &ClearRGBA;                             \
    impl->FlushBlitBuffer = &FlushBlitBuffer;                 \
    impl->Flip = &Flip;                                       \
                                                              \
    impl->CompileShaderInternal = &CompileShaderInternal;     \
    impl->CompileShader = &CompileShader;                     \
    impl->CreateShaderProgram = &CreateShaderProgram;         \
    impl->LinkShaderProgram = &LinkShaderProgram;             \
    impl->FreeShader = &FreeShader;                           \
    impl->FreeShaderProgram = &FreeShaderProgram;             \
    impl->AttachShader = &AttachShader;                       \
    impl->DetachShader = &DetachShader;                       \
    impl->ActivateShaderProgram = &ActivateShaderProgram;     \
    impl->DeactivateShaderProgram = &DeactivateShaderProgram; \
    impl->GetShaderMessage = &GetShaderMessage;               \
    impl->GetAttributeLocation = &GetAttributeLocation;       \
    impl->GetUniformLocation = &GetUniformLocation;           \
    impl->LoadShaderBlock = &LoadShaderBlock;                 \
    impl->SetShaderImage = &SetShaderImage;                   \
    impl->GetUniformiv = &GetUniformiv;                       \
    impl->SetUniformi = &SetUniformi;                         \
    impl->SetUniformiv = &SetUniformiv;                       \
    impl->GetUniformuiv = &GetUniformuiv;                     \
    impl->SetUniformui = &SetUniformui;                       \
    impl->SetUniformuiv = &SetUniformuiv;                     \
    impl->GetUniformfv = &GetUniformfv;                       \
    impl->SetUniformf = &SetUniformf;                         \
    impl->SetUniformfv = &SetUniformfv;                       \
    impl->SetUniformMatrixfv = &SetUniformMatrixfv;           \
    impl->SetAttributef = &SetAttributef;                     \
    impl->SetAttributei = &SetAttributei;                     \
    impl->SetAttributeui = &SetAttributeui;                   \
    impl->SetAttributefv = &SetAttributefv;                   \
    impl->SetAttributeiv = &SetAttributeiv;                   \
    impl->SetAttributeuiv = &SetAttributeuiv;                 \
    impl->SetAttributeSource = &SetAttributeSource;           \
                                                              \
    /* Shape rendering */                                     \
                                                              \
    impl->SetLineThickness = &SetLineThickness;               \
    impl->GetLineThickness = &GetLineThickness;               \
    impl->DrawPixel = &DrawPixel;                             \
    impl->Line = &Line;                                       \
    impl->Arc = &Arc;                                         \
    impl->ArcFilled = &ArcFilled;                             \
    impl->Circle = &Circle;                                   \
    impl->CircleFilled = &CircleFilled;                       \
    impl->Ellipse = &Ellipse;                                 \
    impl->EllipseFilled = &EllipseFilled;                     \
    impl->Sector = &Sector;                                   \
    impl->SectorFilled = &SectorFilled;                       \
    impl->Tri = &Tri;                                         \
    impl->TriFilled = &TriFilled;                             \
    impl->Rectangle = &Rectangle;                             \
    impl->RectangleFilled = &RectangleFilled;                 \
    impl->RectangleRound = &RectangleRound;                   \
    impl->RectangleRoundFilled = &RectangleRoundFilled;       \
    impl->Polygon = &Polygon;                                 \
    impl->Polyline = &Polyline;                               \
    impl->PolygonFilled = &PolygonFilled;

#endif

// All shapes start this way for setup and so they can access the blit buffer properly
#define BEGIN_UNTEXTURED(function_name, shape, num_additional_vertices, num_additional_indices)                              \
    R_CONTEXT_DATA *cdata;                                                                                                   \
    float *blit_buffer;                                                                                                      \
    unsigned short *index_buffer;                                                                                            \
    int vert_index;                                                                                                          \
    int color_index;                                                                                                         \
    float r, g, b, a;                                                                                                        \
    unsigned short blit_buffer_starting_index;                                                                               \
    if (target == NULL) {                                                                                                    \
        R_PushErrorCode(function_name, R_ERROR_NULL_ARGUMENT, "target");                                                     \
        return;                                                                                                              \
    }                                                                                                                        \
    if (renderer != target->renderer) {                                                                                      \
        R_PushErrorCode(function_name, R_ERROR_USER_ERROR, "Mismatched renderer");                                           \
        return;                                                                                                              \
    }                                                                                                                        \
                                                                                                                             \
    makeContextCurrent(renderer, target);                                                                                    \
    if (renderer->current_context_target == NULL) {                                                                          \
        R_PushErrorCode(function_name, R_ERROR_USER_ERROR, "NULL context");                                                  \
        return;                                                                                                              \
    }                                                                                                                        \
                                                                                                                             \
    if (!SetActiveTarget(renderer, target)) {                                                                                \
        R_PushErrorCode(function_name, R_ERROR_BACKEND_ERROR, "Failed to bind framebuffer.");                                \
        return;                                                                                                              \
    }                                                                                                                        \
                                                                                                                             \
    prepareToRenderToTarget(renderer, target);                                                                               \
    prepareToRenderShapes(renderer, shape);                                                                                  \
                                                                                                                             \
    cdata = (R_CONTEXT_DATA *)renderer->current_context_target->context->data;                                               \
                                                                                                                             \
    if (cdata->blit_buffer_num_vertices + (num_additional_vertices) >= cdata->blit_buffer_max_num_vertices) {                \
        if (!growBlitBuffer(cdata, cdata->blit_buffer_num_vertices + (num_additional_vertices))) FlushBlitBuffer(renderer);  \
    }                                                                                                                        \
    if (cdata->index_buffer_num_vertices + (num_additional_indices) >= cdata->index_buffer_max_num_vertices) {               \
        if (!growIndexBuffer(cdata, cdata->index_buffer_num_vertices + (num_additional_indices))) FlushBlitBuffer(renderer); \
    }                                                                                                                        \
                                                                                                                             \
    blit_buffer = cdata->blit_buffer;                                                                                        \
    index_buffer = cdata->index_buffer;                                                                                      \
                                                                                                                             \
    vert_index = R_BLIT_BUFFER_VERTEX_OFFSET + cdata->blit_buffer_num_vertices * R_BLIT_BUFFER_FLOATS_PER_VERTEX;            \
    color_index = R_BLIT_BUFFER_COLOR_OFFSET + cdata->blit_buffer_num_vertices * R_BLIT_BUFFER_FLOATS_PER_VERTEX;            \
                                                                                                                             \
    if (target->use_color) {                                                                                                 \
        r = MIX_COLOR_COMPONENT_NORMALIZED_RESULT(target->color.r, color.r);                                                 \
        g = MIX_COLOR_COMPONENT_NORMALIZED_RESULT(target->color.g, color.g);                                                 \
        b = MIX_COLOR_COMPONENT_NORMALIZED_RESULT(target->color.b, color.b);                                                 \
        a = MIX_COLOR_COMPONENT_NORMALIZED_RESULT(GET_ALPHA(target->color), GET_ALPHA(color));                               \
    } else {                                                                                                                 \
        r = color.r / 255.0f;                                                                                                \
        g = color.g / 255.0f;                                                                                                \
        b = color.b / 255.0f;                                                                                                \
        a = GET_ALPHA(color) / 255.0f;                                                                                       \
    }                                                                                                                        \
    blit_buffer_starting_index = cdata->blit_buffer_num_vertices;                                                            \
    (void)blit_buffer_starting_index;

#define R_CIRCLE_SEGMENT_ANGLE_FACTOR 0.625f

#define CALCULATE_CIRCLE_DT_AND_SEGMENTS(radius)                                                                     \
    dt = R_CIRCLE_SEGMENT_ANGLE_FACTOR / sqrtf(radius); /* s = rA, so dA = ds/r.  ds of 1.25*sqrt(radius) is good */ \
    numSegments = (int)(2 * PI / dt) + 1;                                                                            \
                                                                                                                     \
    if (numSegments < 16) {                                                                                          \
        numSegments = 16;                                                                                            \
        dt = 2 * PI / (numSegments - 1);                                                                             \
    }

float SetLineThickness(R_Renderer *renderer, float thickness) {
    float old;

    if (renderer->current_context_target == NULL) return 1.0f;

    old = renderer->current_context_target->context->line_thickness;
    if (old != thickness) FlushBlitBuffer(renderer);

    renderer->current_context_target->context->line_thickness = thickness;
#ifndef R_SKIP_LINE_WIDTH
    glLineWidth(thickness);
#endif
    return old;
}

float GetLineThickness(R_Renderer *renderer) { return renderer->current_context_target->context->line_thickness; }

void DrawPixel(R_Renderer *renderer, R_Target *target, float x, float y, MEcolor color) {
    BEGIN_UNTEXTURED("R_DrawPixel", GL_POINTS, 1, 1);

    SET_UNTEXTURED_VERTEX(x, y, r, g, b, a);
}

void Line(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, MEcolor color) {
    float thickness = GetLineThickness(renderer);

    float t = thickness / 2;
    float line_angle = atan2f(y2 - y1, x2 - x1);
    float tc = t * cosf(line_angle);
    float ts = t * sinf(line_angle);

    BEGIN_UNTEXTURED("R_Line", GL_TRIANGLES, 4, 6);

    SET_UNTEXTURED_VERTEX(x1 + ts, y1 - tc, r, g, b, a);
    SET_UNTEXTURED_VERTEX(x1 - ts, y1 + tc, r, g, b, a);
    SET_UNTEXTURED_VERTEX(x2 + ts, y2 - tc, r, g, b, a);

    SET_INDEXED_VERTEX(1);
    SET_INDEXED_VERTEX(2);
    SET_UNTEXTURED_VERTEX(x2 - ts, y2 + tc, r, g, b, a);
}

// Arc() might call Circle()
void Circle(R_Renderer *renderer, R_Target *target, float x, float y, float radius, MEcolor color);

void Arc(R_Renderer *renderer, R_Target *target, float x, float y, float radius, float start_angle, float end_angle, MEcolor color) {
    float dx, dy;
    int i;

    float t = GetLineThickness(renderer) / 2;
    float inner_radius = radius - t;
    float outer_radius = radius + t;

    float dt;
    int numSegments;

    float tempx;
    float c, s;

    if (inner_radius < 0.0f) inner_radius = 0.0f;

    if (start_angle > end_angle) {
        float swapa = end_angle;
        end_angle = start_angle;
        start_angle = swapa;
    }
    if (start_angle == end_angle) return;

    // Big angle
    if (end_angle - start_angle >= 360) {
        Circle(renderer, target, x, y, radius, color);
        return;
    }

    // Shift together
    while (start_angle < 0 && end_angle < 0) {
        start_angle += 360;
        end_angle += 360;
    }
    while (start_angle > 360 && end_angle > 360) {
        start_angle -= 360;
        end_angle -= 360;
    }

    dt = ((end_angle - start_angle) / 360) * (R_CIRCLE_SEGMENT_ANGLE_FACTOR / sqrtf(outer_radius));  // s = rA, so dA = ds/r.  ds of 1.25*sqrt(radius) is good, use A in degrees.

    numSegments = (int)((fabs(end_angle - start_angle) * PI / 180) / dt);
    if (numSegments == 0) return;

    {
        BEGIN_UNTEXTURED("R_Arc", GL_TRIANGLES, 2 * (numSegments), 6 * (numSegments));

        c = cosf(dt);
        s = sinf(dt);

        // Rotate to start
        start_angle *= RAD_PER_DEG;
        dx = cosf(start_angle);
        dy = sinf(start_angle);

        BEGIN_UNTEXTURED_SEGMENTS(x + inner_radius * dx, y + inner_radius * dy, x + outer_radius * dx, y + outer_radius * dy, r, g, b, a);

        for (i = 1; i < numSegments; i++) {
            tempx = c * dx - s * dy;
            dy = s * dx + c * dy;
            dx = tempx;
            SET_UNTEXTURED_SEGMENTS(x + inner_radius * dx, y + inner_radius * dy, x + outer_radius * dx, y + outer_radius * dy, r, g, b, a);
        }

        // Last point
        end_angle *= RAD_PER_DEG;
        dx = cosf(end_angle);
        dy = sinf(end_angle);
        END_UNTEXTURED_SEGMENTS(x + inner_radius * dx, y + inner_radius * dy, x + outer_radius * dx, y + outer_radius * dy, r, g, b, a);
    }
}

// ArcFilled() might call CircleFilled()
void CircleFilled(R_Renderer *renderer, R_Target *target, float x, float y, float radius, MEcolor color);

void ArcFilled(R_Renderer *renderer, R_Target *target, float x, float y, float radius, float start_angle, float end_angle, MEcolor color) {
    float dx, dy;
    int i;

    float dt;
    int numSegments;

    float tempx;
    float c, s;

    if (start_angle > end_angle) {
        float swapa = end_angle;
        end_angle = start_angle;
        start_angle = swapa;
    }
    if (start_angle == end_angle) return;

    // Big angle
    if (end_angle - start_angle >= 360) {
        CircleFilled(renderer, target, x, y, radius, color);
        return;
    }

    // Shift together
    while (start_angle < 0 && end_angle < 0) {
        start_angle += 360;
        end_angle += 360;
    }
    while (start_angle > 360 && end_angle > 360) {
        start_angle -= 360;
        end_angle -= 360;
    }

    dt = ((end_angle - start_angle) / 360) * (R_CIRCLE_SEGMENT_ANGLE_FACTOR / sqrtf(radius));  // s = rA, so dA = ds/r.  ds of 1.25*sqrt(radius) is good, use A in degrees.

    numSegments = (int)((fabs(end_angle - start_angle) * RAD_PER_DEG) / dt);
    if (numSegments == 0) return;

    {
        BEGIN_UNTEXTURED("R_ArcFilled", GL_TRIANGLES, 3 + (numSegments - 1) + 1, 3 + (numSegments - 1) * 3 + 3);

        c = cosf(dt);
        s = sinf(dt);

        // Rotate to start
        start_angle *= RAD_PER_DEG;
        dx = cosf(start_angle);
        dy = sinf(start_angle);

        // First triangle
        SET_UNTEXTURED_VERTEX(x, y, r, g, b, a);
        SET_UNTEXTURED_VERTEX(x + radius * dx, y + radius * dy, r, g, b, a);  // first point

        tempx = c * dx - s * dy;
        dy = s * dx + c * dy;
        dx = tempx;
        SET_UNTEXTURED_VERTEX(x + radius * dx, y + radius * dy, r, g, b, a);  // new point

        for (i = 2; i < numSegments + 1; i++) {
            tempx = c * dx - s * dy;
            dy = s * dx + c * dy;
            dx = tempx;
            SET_INDEXED_VERTEX(0);                                                // center
            SET_INDEXED_VERTEX(i);                                                // last point
            SET_UNTEXTURED_VERTEX(x + radius * dx, y + radius * dy, r, g, b, a);  // new point
        }

        // Last triangle
        end_angle *= RAD_PER_DEG;
        dx = cosf(end_angle);
        dy = sinf(end_angle);
        SET_INDEXED_VERTEX(0);                                                // center
        SET_INDEXED_VERTEX(i);                                                // last point
        SET_UNTEXTURED_VERTEX(x + radius * dx, y + radius * dy, r, g, b, a);  // new point
    }
}

/*
Incremental rotation circle algorithm
*/

void Circle(R_Renderer *renderer, R_Target *target, float x, float y, float radius, MEcolor color) {
    float thickness = GetLineThickness(renderer);
    float dx, dy;
    int i;
    float t = thickness / 2;
    float inner_radius = radius - t;
    float outer_radius = radius + t;
    float dt;
    int numSegments;

    CALCULATE_CIRCLE_DT_AND_SEGMENTS(outer_radius);

    float tempx;
    float c = cosf(dt);
    float s = sinf(dt);

    BEGIN_UNTEXTURED("R_Circle", GL_TRIANGLES, 2 * (numSegments), 6 * (numSegments));

    if (inner_radius < 0.0f) inner_radius = 0.0f;

    dx = 1.0f;
    dy = 0.0f;

    BEGIN_UNTEXTURED_SEGMENTS(x + inner_radius * dx, y + inner_radius * dy, x + outer_radius * dx, y + outer_radius * dy, r, g, b, a);

    for (i = 1; i < numSegments; i++) {
        tempx = c * dx - s * dy;
        dy = s * dx + c * dy;
        dx = tempx;

        SET_UNTEXTURED_SEGMENTS(x + inner_radius * dx, y + inner_radius * dy, x + outer_radius * dx, y + outer_radius * dy, r, g, b, a);
    }

    LOOP_UNTEXTURED_SEGMENTS();  // back to the beginning
}

void CircleFilled(R_Renderer *renderer, R_Target *target, float x, float y, float radius, MEcolor color) {
    float dt;
    float dx, dy;
    int numSegments;
    int i;

    CALCULATE_CIRCLE_DT_AND_SEGMENTS(radius);

    float tempx;
    float c = cosf(dt);
    float s = sinf(dt);

    BEGIN_UNTEXTURED("R_CircleFilled", GL_TRIANGLES, 3 + (numSegments - 2), 3 + (numSegments - 2) * 3 + 3);

    // First triangle
    SET_UNTEXTURED_VERTEX(x, y, r, g, b, a);  // Center

    dx = 1.0f;
    dy = 0.0f;
    SET_UNTEXTURED_VERTEX(x + radius * dx, y + radius * dy, r, g, b, a);  // first point

    tempx = c * dx - s * dy;
    dy = s * dx + c * dy;
    dx = tempx;
    SET_UNTEXTURED_VERTEX(x + radius * dx, y + radius * dy, r, g, b, a);  // new point

    for (i = 2; i < numSegments; i++) {
        tempx = c * dx - s * dy;
        dy = s * dx + c * dy;
        dx = tempx;
        SET_INDEXED_VERTEX(0);                                                // center
        SET_INDEXED_VERTEX(i);                                                // last point
        SET_UNTEXTURED_VERTEX(x + radius * dx, y + radius * dy, r, g, b, a);  // new point
    }

    SET_INDEXED_VERTEX(0);  // center
    SET_INDEXED_VERTEX(i);  // last point
    SET_INDEXED_VERTEX(1);  // first point
}

void Ellipse(R_Renderer *renderer, R_Target *target, float x, float y, float rx, float ry, float degrees, MEcolor color) {
    float thickness = GetLineThickness(renderer);
    float dx, dy;
    int i;
    float t = thickness / 2;
    float rot_x = cosf(degrees * RAD_PER_DEG);
    float rot_y = sinf(degrees * RAD_PER_DEG);
    float inner_radius_x = rx - t;
    float outer_radius_x = rx + t;
    float inner_radius_y = ry - t;
    float outer_radius_y = ry + t;
    float dt;
    int numSegments;

    CALCULATE_CIRCLE_DT_AND_SEGMENTS(outer_radius_x > outer_radius_y ? outer_radius_x : outer_radius_y);

    float tempx;
    float c = cosf(dt);
    float s = sinf(dt);
    float inner_trans_x, inner_trans_y;
    float outer_trans_x, outer_trans_y;

    BEGIN_UNTEXTURED("R_Ellipse", GL_TRIANGLES, 2 * (numSegments), 6 * (numSegments));

    if (inner_radius_x < 0.0f) inner_radius_x = 0.0f;
    if (inner_radius_y < 0.0f) inner_radius_y = 0.0f;

    dx = 1.0f;
    dy = 0.0f;

    inner_trans_x = rot_x * inner_radius_x * dx - rot_y * inner_radius_y * dy;
    inner_trans_y = rot_y * inner_radius_x * dx + rot_x * inner_radius_y * dy;
    outer_trans_x = rot_x * outer_radius_x * dx - rot_y * outer_radius_y * dy;
    outer_trans_y = rot_y * outer_radius_x * dx + rot_x * outer_radius_y * dy;
    BEGIN_UNTEXTURED_SEGMENTS(x + inner_trans_x, y + inner_trans_y, x + outer_trans_x, y + outer_trans_y, r, g, b, a);

    for (i = 1; i < numSegments; i++) {
        tempx = c * dx - s * dy;
        dy = (s * dx + c * dy);
        dx = tempx;

        inner_trans_x = rot_x * inner_radius_x * dx - rot_y * inner_radius_y * dy;
        inner_trans_y = rot_y * inner_radius_x * dx + rot_x * inner_radius_y * dy;
        outer_trans_x = rot_x * outer_radius_x * dx - rot_y * outer_radius_y * dy;
        outer_trans_y = rot_y * outer_radius_x * dx + rot_x * outer_radius_y * dy;
        SET_UNTEXTURED_SEGMENTS(x + inner_trans_x, y + inner_trans_y, x + outer_trans_x, y + outer_trans_y, r, g, b, a);
    }

    LOOP_UNTEXTURED_SEGMENTS();  // back to the beginning
}

void EllipseFilled(R_Renderer *renderer, R_Target *target, float x, float y, float rx, float ry, float degrees, MEcolor color) {
    float dx, dy;
    int i;
    float rot_x = cosf(degrees * RAD_PER_DEG);
    float rot_y = sinf(degrees * RAD_PER_DEG);
    float dt;
    int numSegments;
    CALCULATE_CIRCLE_DT_AND_SEGMENTS(rx > ry ? rx : ry);

    float tempx;
    float c = cosf(dt);
    float s = sinf(dt);
    float trans_x, trans_y;

    BEGIN_UNTEXTURED("R_EllipseFilled", GL_TRIANGLES, 3 + (numSegments - 2), 3 + (numSegments - 2) * 3 + 3);

    // First triangle
    SET_UNTEXTURED_VERTEX(x, y, r, g, b, a);  // Center

    dx = 1.0f;
    dy = 0.0f;
    trans_x = rot_x * rx * dx - rot_y * ry * dy;
    trans_y = rot_y * rx * dx + rot_x * ry * dy;
    SET_UNTEXTURED_VERTEX(x + trans_x, y + trans_y, r, g, b, a);  // first point

    tempx = c * dx - s * dy;
    dy = s * dx + c * dy;
    dx = tempx;

    trans_x = rot_x * rx * dx - rot_y * ry * dy;
    trans_y = rot_y * rx * dx + rot_x * ry * dy;
    SET_UNTEXTURED_VERTEX(x + trans_x, y + trans_y, r, g, b, a);  // new point

    for (i = 2; i < numSegments; i++) {
        tempx = c * dx - s * dy;
        dy = (s * dx + c * dy);
        dx = tempx;

        trans_x = rot_x * rx * dx - rot_y * ry * dy;
        trans_y = rot_y * rx * dx + rot_x * ry * dy;

        SET_INDEXED_VERTEX(0);                                        // center
        SET_INDEXED_VERTEX(i);                                        // last point
        SET_UNTEXTURED_VERTEX(x + trans_x, y + trans_y, r, g, b, a);  // new point
    }

    SET_INDEXED_VERTEX(0);  // center
    SET_INDEXED_VERTEX(i);  // last point
    SET_INDEXED_VERTEX(1);  // first point
}

void Sector(R_Renderer *renderer, R_Target *target, float x, float y, float inner_radius, float outer_radius, float start_angle, float end_angle, MEcolor color) {
    bool circled;
    float dx1, dy1, dx2, dy2, dx3, dy3, dx4, dy4;

    if (inner_radius < 0.0f) inner_radius = 0.0f;
    if (outer_radius < 0.0f) outer_radius = 0.0f;

    if (inner_radius > outer_radius) {
        float s = inner_radius;
        inner_radius = outer_radius;
        outer_radius = s;
    }

    if (start_angle > end_angle) {
        float swapa = end_angle;
        end_angle = start_angle;
        start_angle = swapa;
    }
    if (start_angle == end_angle) return;

    if (inner_radius == outer_radius) {
        Arc(renderer, target, x, y, inner_radius, start_angle, end_angle, color);
        return;
    }

    circled = (end_angle - start_angle >= 360);
    // Composited shape...  But that means error codes may be confusing. :-/
    Arc(renderer, target, x, y, inner_radius, start_angle, end_angle, color);

    if (!circled) {
        dx1 = inner_radius * cosf(end_angle * RAD_PER_DEG);
        dy1 = inner_radius * sinf(end_angle * RAD_PER_DEG);
        dx2 = outer_radius * cosf(end_angle * RAD_PER_DEG);
        dy2 = outer_radius * sinf(end_angle * RAD_PER_DEG);
        Line(renderer, target, x + dx1, y + dy1, x + dx2, y + dy2, color);
    }

    Arc(renderer, target, x, y, outer_radius, start_angle, end_angle, color);

    if (!circled) {
        dx3 = inner_radius * cosf(start_angle * RAD_PER_DEG);
        dy3 = inner_radius * sinf(start_angle * RAD_PER_DEG);
        dx4 = outer_radius * cosf(start_angle * RAD_PER_DEG);
        dy4 = outer_radius * sinf(start_angle * RAD_PER_DEG);
        Line(renderer, target, x + dx3, y + dy3, x + dx4, y + dy4, color);
    }
}

void SectorFilled(R_Renderer *renderer, R_Target *target, float x, float y, float inner_radius, float outer_radius, float start_angle, float end_angle, MEcolor color) {
    float t;
    float dt;
    float dx, dy;

    int numSegments;

    if (inner_radius < 0.0f) inner_radius = 0.0f;
    if (outer_radius < 0.0f) outer_radius = 0.0f;

    if (inner_radius > outer_radius) {
        float s = inner_radius;
        inner_radius = outer_radius;
        outer_radius = s;
    }

    if (inner_radius == outer_radius) {
        Arc(renderer, target, x, y, inner_radius, start_angle, end_angle, color);
        return;
    }

    if (start_angle > end_angle) {
        float swapa = end_angle;
        end_angle = start_angle;
        start_angle = swapa;
    }
    if (start_angle == end_angle) return;

    if (end_angle - start_angle >= 360) end_angle = start_angle + 360;

    t = start_angle;
    dt = ((end_angle - start_angle) / 360) * (R_CIRCLE_SEGMENT_ANGLE_FACTOR / sqrtf(outer_radius)) * DEG_PER_RAD;  // s = rA, so dA = ds/r.  ds of 1.25*sqrt(radius) is good, use A in degrees.

    numSegments = (int)(fabs(end_angle - start_angle) / dt);
    if (numSegments == 0) return;

    {
        int i;
        bool use_inner;
        BEGIN_UNTEXTURED("R_SectorFilled", GL_TRIANGLES, 3 + (numSegments - 1) + 1, 3 + (numSegments - 1) * 3 + 3);

        use_inner = false;  // Switches between the radii for the next point

        // First triangle
        dx = inner_radius * cosf(t * RAD_PER_DEG);
        dy = inner_radius * sinf(t * RAD_PER_DEG);
        SET_UNTEXTURED_VERTEX(x + dx, y + dy, r, g, b, a);

        dx = outer_radius * cosf(t * RAD_PER_DEG);
        dy = outer_radius * sinf(t * RAD_PER_DEG);
        SET_UNTEXTURED_VERTEX(x + dx, y + dy, r, g, b, a);
        t += dt;
        dx = inner_radius * cosf(t * RAD_PER_DEG);
        dy = inner_radius * sinf(t * RAD_PER_DEG);
        SET_UNTEXTURED_VERTEX(x + dx, y + dy, r, g, b, a);
        t += dt;

        for (i = 2; i < numSegments + 1; i++) {
            SET_INDEXED_VERTEX(i - 1);
            SET_INDEXED_VERTEX(i);
            if (use_inner) {
                dx = inner_radius * cosf(t * RAD_PER_DEG);
                dy = inner_radius * sinf(t * RAD_PER_DEG);
            } else {
                dx = outer_radius * cosf(t * RAD_PER_DEG);
                dy = outer_radius * sinf(t * RAD_PER_DEG);
            }
            SET_UNTEXTURED_VERTEX(x + dx, y + dy, r, g, b, a);  // new point
            t += dt;
            use_inner = !use_inner;
        }

        // Last quad
        t = end_angle;
        if (use_inner) {
            dx = inner_radius * cosf(t * RAD_PER_DEG);
            dy = inner_radius * sinf(t * RAD_PER_DEG);
        } else {
            dx = outer_radius * cosf(t * RAD_PER_DEG);
            dy = outer_radius * sinf(t * RAD_PER_DEG);
        }
        SET_INDEXED_VERTEX(i - 1);
        SET_INDEXED_VERTEX(i);
        SET_UNTEXTURED_VERTEX(x + dx, y + dy, r, g, b, a);  // new point
        use_inner = !use_inner;
        i++;

        if (use_inner) {
            dx = inner_radius * cosf(t * RAD_PER_DEG);
            dy = inner_radius * sinf(t * RAD_PER_DEG);
        } else {
            dx = outer_radius * cosf(t * RAD_PER_DEG);
            dy = outer_radius * sinf(t * RAD_PER_DEG);
        }
        SET_INDEXED_VERTEX(i - 1);
        SET_INDEXED_VERTEX(i);
        SET_UNTEXTURED_VERTEX(x + dx, y + dy, r, g, b, a);  // new point
    }
}

void Tri(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, float x3, float y3, MEcolor color) {
    BEGIN_UNTEXTURED("R_Tri", GL_LINES, 3, 6);

    SET_UNTEXTURED_VERTEX(x1, y1, r, g, b, a);
    SET_UNTEXTURED_VERTEX(x2, y2, r, g, b, a);

    SET_INDEXED_VERTEX(1);
    SET_UNTEXTURED_VERTEX(x3, y3, r, g, b, a);

    SET_INDEXED_VERTEX(2);
    SET_INDEXED_VERTEX(0);
}

void TriFilled(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, float x3, float y3, MEcolor color) {
    BEGIN_UNTEXTURED("R_TriFilled", GL_TRIANGLES, 3, 3);

    SET_UNTEXTURED_VERTEX(x1, y1, r, g, b, a);
    SET_UNTEXTURED_VERTEX(x2, y2, r, g, b, a);
    SET_UNTEXTURED_VERTEX(x3, y3, r, g, b, a);
}

void Rectangle(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, MEcolor color) {
    if (y2 < y1) {
        float y = y1;
        y1 = y2;
        y2 = y;
    }
    if (x2 < x1) {
        float x = x1;
        x1 = x2;
        x2 = x;
    }

    {
        float thickness = GetLineThickness(renderer);

        // Thickness offsets
        float outer = thickness / 2;
        float inner_x = outer;
        float inner_y = outer;

        // Thick lines via filled triangles

        BEGIN_UNTEXTURED("R_Rectangle", GL_TRIANGLES, 12, 24);

        // Adjust inner thickness offsets to avoid overdraw on narrow/small rects
        if (x1 + inner_x > x2 - inner_x) inner_x = (x2 - x1) / 2;
        if (y1 + inner_y > y2 - inner_y) inner_y = (y2 - y1) / 2;

        // First triangle
        SET_UNTEXTURED_VERTEX(x1 - outer, y1 - outer, r, g, b, a);    // 0
        SET_UNTEXTURED_VERTEX(x1 - outer, y1 + inner_y, r, g, b, a);  // 1
        SET_UNTEXTURED_VERTEX(x2 + outer, y1 - outer, r, g, b, a);    // 2

        SET_INDEXED_VERTEX(2);
        SET_INDEXED_VERTEX(1);
        SET_UNTEXTURED_VERTEX(x2 + outer, y1 + inner_y, r, g, b, a);  // 3

        SET_INDEXED_VERTEX(3);
        SET_UNTEXTURED_VERTEX(x2 - inner_x, y1 + inner_y, r, g, b, a);  // 4
        SET_UNTEXTURED_VERTEX(x2 - inner_x, y2 - inner_y, r, g, b, a);  // 5

        SET_INDEXED_VERTEX(3);
        SET_INDEXED_VERTEX(5);
        SET_UNTEXTURED_VERTEX(x2 + outer, y2 - inner_y, r, g, b, a);  // 6

        SET_INDEXED_VERTEX(6);
        SET_UNTEXTURED_VERTEX(x1 - outer, y2 - inner_y, r, g, b, a);  // 7
        SET_UNTEXTURED_VERTEX(x2 + outer, y2 + outer, r, g, b, a);    // 8

        SET_INDEXED_VERTEX(7);
        SET_UNTEXTURED_VERTEX(x1 - outer, y2 + outer, r, g, b, a);  // 9
        SET_INDEXED_VERTEX(8);

        SET_INDEXED_VERTEX(7);
        SET_UNTEXTURED_VERTEX(x1 + inner_x, y2 - inner_y, r, g, b, a);  // 10
        SET_INDEXED_VERTEX(1);

        SET_INDEXED_VERTEX(1);
        SET_INDEXED_VERTEX(10);
        SET_UNTEXTURED_VERTEX(x1 + inner_x, y1 + inner_y, r, g, b, a);  // 11
    }
}

void RectangleFilled(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, MEcolor color) {
    BEGIN_UNTEXTURED("R_RectangleFilled", GL_TRIANGLES, 4, 6);

    SET_UNTEXTURED_VERTEX(x1, y1, r, g, b, a);
    SET_UNTEXTURED_VERTEX(x1, y2, r, g, b, a);
    SET_UNTEXTURED_VERTEX(x2, y1, r, g, b, a);

    SET_INDEXED_VERTEX(1);
    SET_INDEXED_VERTEX(2);
    SET_UNTEXTURED_VERTEX(x2, y2, r, g, b, a);
}

#define INCREMENT_CIRCLE     \
    tempx = c * dx - s * dy; \
    dy = s * dx + c * dy;    \
    dx = tempx;              \
    ++i;

void RectangleRound(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, float radius, MEcolor color) {
    if (y2 < y1) {
        float temp = y2;
        y2 = y1;
        y1 = temp;
    }
    if (x2 < x1) {
        float temp = x2;
        x2 = x1;
        x1 = temp;
    }

    if (radius > (x2 - x1) / 2) radius = (x2 - x1) / 2;
    if (radius > (y2 - y1) / 2) radius = (y2 - y1) / 2;

    x1 += radius;
    y1 += radius;
    x2 -= radius;
    y2 -= radius;

    {
        float thickness = GetLineThickness(renderer);
        float dx, dy;
        int i = 0;
        float t = thickness / 2;
        float inner_radius = radius - t;
        float outer_radius = radius + t;
        float dt;
        int numSegments;
        CALCULATE_CIRCLE_DT_AND_SEGMENTS(outer_radius);

        // Make a multiple of 4 so we can have even corners
        numSegments += numSegments % 4;

        dt = 2 * PI / (numSegments - 1);

        {
            float x, y;
            int go_to_second = numSegments / 4;
            int go_to_third = numSegments / 2;
            int go_to_fourth = 3 * numSegments / 4;

            float tempx;
            float c = cosf(dt);
            float s = sinf(dt);

            // Add another 4 for the extra corner vertices
            BEGIN_UNTEXTURED("R_RectangleRound", GL_TRIANGLES, 2 * (numSegments + 4), 6 * (numSegments + 4));

            if (inner_radius < 0.0f) inner_radius = 0.0f;

            dx = 1.0f;
            dy = 0.0f;

            x = x2;
            y = y2;
            BEGIN_UNTEXTURED_SEGMENTS(x + inner_radius, y, x + outer_radius, y, r, g, b, a);
            while (i < go_to_second - 1) {
                INCREMENT_CIRCLE;

                SET_UNTEXTURED_SEGMENTS(x + inner_radius * dx, y + inner_radius * dy, x + outer_radius * dx, y + outer_radius * dy, r, g, b, a);
            }
            INCREMENT_CIRCLE;

            SET_UNTEXTURED_SEGMENTS(x, y + inner_radius, x, y + outer_radius, r, g, b, a);
            x = x1;
            y = y2;
            SET_UNTEXTURED_SEGMENTS(x, y + inner_radius, x, y + outer_radius, r, g, b, a);
            while (i < go_to_third - 1) {
                INCREMENT_CIRCLE;

                SET_UNTEXTURED_SEGMENTS(x + inner_radius * dx, y + inner_radius * dy, x + outer_radius * dx, y + outer_radius * dy, r, g, b, a);
            }
            INCREMENT_CIRCLE;

            SET_UNTEXTURED_SEGMENTS(x - inner_radius, y, x - outer_radius, y, r, g, b, a);
            x = x1;
            y = y1;
            SET_UNTEXTURED_SEGMENTS(x - inner_radius, y, x - outer_radius, y, r, g, b, a);
            while (i < go_to_fourth - 1) {
                INCREMENT_CIRCLE;

                SET_UNTEXTURED_SEGMENTS(x + inner_radius * dx, y + inner_radius * dy, x + outer_radius * dx, y + outer_radius * dy, r, g, b, a);
            }
            INCREMENT_CIRCLE;

            SET_UNTEXTURED_SEGMENTS(x, y - inner_radius, x, y - outer_radius, r, g, b, a);
            x = x2;
            y = y1;
            SET_UNTEXTURED_SEGMENTS(x, y - inner_radius, x, y - outer_radius, r, g, b, a);
            while (i < numSegments - 1) {
                INCREMENT_CIRCLE;

                SET_UNTEXTURED_SEGMENTS(x + inner_radius * dx, y + inner_radius * dy, x + outer_radius * dx, y + outer_radius * dy, r, g, b, a);
            }
            SET_UNTEXTURED_SEGMENTS(x + inner_radius, y, x + outer_radius, y, r, g, b, a);

            LOOP_UNTEXTURED_SEGMENTS();  // back to the beginning
        }
    }
}

void RectangleRoundFilled(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, float radius, MEcolor color) {
    if (y2 < y1) {
        float temp = y2;
        y2 = y1;
        y1 = temp;
    }
    if (x2 < x1) {
        float temp = x2;
        x2 = x1;
        x1 = temp;
    }

    if (radius > (x2 - x1) / 2) radius = (x2 - x1) / 2;
    if (radius > (y2 - y1) / 2) radius = (y2 - y1) / 2;

    {
        float tau = 2 * PI;

        int verts_per_corner = 7;
        float corner_angle_increment = (tau / 4) / (verts_per_corner - 1);  // 0, 15, 30, 45, 60, 75, 90

        // Starting angle
        float angle = tau * 0.75f;
        int last_index = 2;
        int i;

        BEGIN_UNTEXTURED("R_RectangleRoundFilled", GL_TRIANGLES, 6 + 4 * (verts_per_corner - 1) - 1, 15 + 4 * (verts_per_corner - 1) * 3 - 3);

        // First triangle
        SET_UNTEXTURED_VERTEX((x2 + x1) / 2, (y2 + y1) / 2, r, g, b, a);  // Center
        SET_UNTEXTURED_VERTEX(x2 - radius + cosf(angle) * radius, y1 + radius + sinf(angle) * radius, r, g, b, a);
        angle += corner_angle_increment;
        SET_UNTEXTURED_VERTEX(x2 - radius + cosf(angle) * radius, y1 + radius + sinf(angle) * radius, r, g, b, a);
        angle += corner_angle_increment;

        for (i = 2; i < verts_per_corner; i++) {
            SET_INDEXED_VERTEX(0);
            SET_INDEXED_VERTEX(last_index++);
            SET_UNTEXTURED_VERTEX(x2 - radius + cosf(angle) * radius, y1 + radius + sinf(angle) * radius, r, g, b, a);
            angle += corner_angle_increment;
        }

        SET_INDEXED_VERTEX(0);
        SET_INDEXED_VERTEX(last_index++);
        SET_UNTEXTURED_VERTEX(x2 - radius + cosf(angle) * radius, y2 - radius + sinf(angle) * radius, r, g, b, a);
        for (i = 1; i < verts_per_corner; i++) {
            SET_INDEXED_VERTEX(0);
            SET_INDEXED_VERTEX(last_index++);
            SET_UNTEXTURED_VERTEX(x2 - radius + cosf(angle) * radius, y2 - radius + sinf(angle) * radius, r, g, b, a);
            angle += corner_angle_increment;
        }

        SET_INDEXED_VERTEX(0);
        SET_INDEXED_VERTEX(last_index++);
        SET_UNTEXTURED_VERTEX(x1 + radius + cosf(angle) * radius, y2 - radius + sinf(angle) * radius, r, g, b, a);
        for (i = 1; i < verts_per_corner; i++) {
            SET_INDEXED_VERTEX(0);
            SET_INDEXED_VERTEX(last_index++);
            SET_UNTEXTURED_VERTEX(x1 + radius + cosf(angle) * radius, y2 - radius + sinf(angle) * radius, r, g, b, a);
            angle += corner_angle_increment;
        }

        SET_INDEXED_VERTEX(0);
        SET_INDEXED_VERTEX(last_index++);
        SET_UNTEXTURED_VERTEX(x1 + radius + cosf(angle) * radius, y1 + radius + sinf(angle) * radius, r, g, b, a);
        for (i = 1; i < verts_per_corner; i++) {
            SET_INDEXED_VERTEX(0);
            SET_INDEXED_VERTEX(last_index++);
            SET_UNTEXTURED_VERTEX(x1 + radius + cosf(angle) * radius, y1 + radius + sinf(angle) * radius, r, g, b, a);
            angle += corner_angle_increment;
        }

        // Last triangle
        SET_INDEXED_VERTEX(0);
        SET_INDEXED_VERTEX(last_index++);
        SET_INDEXED_VERTEX(1);
    }
}

void Polygon(R_Renderer *renderer, R_Target *target, unsigned int num_vertices, float *vertices, MEcolor color) {
    if (num_vertices < 3) return;

    {
        int numSegments = 2 * num_vertices;
        int last_index = 0;
        int i;

        BEGIN_UNTEXTURED("R_Polygon", GL_LINES, num_vertices, numSegments);

        SET_UNTEXTURED_VERTEX(vertices[0], vertices[1], r, g, b, a);
        for (i = 2; i < numSegments; i += 2) {
            SET_UNTEXTURED_VERTEX(vertices[i], vertices[i + 1], r, g, b, a);
            last_index++;
            SET_INDEXED_VERTEX(last_index);  // Double the last one for the next line
        }
        SET_INDEXED_VERTEX(0);
    }
}

void Polyline(R_Renderer *renderer, R_Target *target, unsigned int num_vertices, float *vertices, MEcolor color, bool close_loop) {
    if (num_vertices < 2) return;

    float t = GetLineThickness(renderer) * 0.5f;
    float x1, x2, y1, y2, line_angle, tc, ts;

    int num_v = num_vertices * 4;
    int num_i = num_v + 2;
    int last_vert = num_vertices;

    if (!close_loop) {
        num_v -= 4;
        num_i = num_v;
        last_vert--;
    }

    BEGIN_UNTEXTURED("R_Polygon", GL_TRIANGLE_STRIP, num_v, num_i);

    int i = 0;
    do {
        x1 = vertices[i * 2];
        y1 = vertices[i * 2 + 1];
        i++;
        if (i == (int)num_vertices) {
            x2 = vertices[0];
            y2 = vertices[1];
        } else {
            x2 = vertices[i * 2];
            y2 = vertices[i * 2 + 1];
        }

        line_angle = atan2f(y2 - y1, x2 - x1);
        tc = t * cosf(line_angle);
        ts = t * sinf(line_angle);

        SET_UNTEXTURED_VERTEX(x1 + ts, y1 - tc, r, g, b, a);
        SET_UNTEXTURED_VERTEX(x1 - ts, y1 + tc, r, g, b, a);
        SET_UNTEXTURED_VERTEX(x2 + ts, y2 - tc, r, g, b, a);
        SET_UNTEXTURED_VERTEX(x2 - ts, y2 + tc, r, g, b, a);

    } while (i < last_vert);

    if (close_loop)  // end cap for closed
    {
        SET_INDEXED_VERTEX(0);
        SET_INDEXED_VERTEX(1)
    }
}

void PolygonFilled(R_Renderer *renderer, R_Target *target, unsigned int num_vertices, float *vertices, MEcolor color) {
    if (num_vertices < 3) return;

    {
        int numSegments = 2 * num_vertices;

        // Using a fan of triangles assumes that the polygon is convex
        BEGIN_UNTEXTURED("R_PolygonFilled", GL_TRIANGLES, num_vertices, 3 + (num_vertices - 3) * 3);

        // First triangle
        SET_UNTEXTURED_VERTEX(vertices[0], vertices[1], r, g, b, a);
        SET_UNTEXTURED_VERTEX(vertices[2], vertices[3], r, g, b, a);
        SET_UNTEXTURED_VERTEX(vertices[4], vertices[5], r, g, b, a);

        if (num_vertices > 3) {
            int last_index = 2;

            int i;
            for (i = 6; i < numSegments; i += 2) {
                SET_INDEXED_VERTEX(0);           // Start from the first vertex
                SET_INDEXED_VERTEX(last_index);  // Double the last one
                SET_UNTEXTURED_VERTEX(vertices[i], vertices[i + 1], r, g, b, a);
                last_index++;
            }
        }
    }
}

R_Renderer *R_CreateRenderer_OpenGL_3(R_RendererID request) {
    R_Renderer *renderer = (R_Renderer *)ME_MALLOC(sizeof(R_Renderer));
    if (renderer == NULL) return NULL;

    memset(renderer, 0, sizeof(R_Renderer));

    renderer->id = request;
    renderer->shader_language = R_LANGUAGE_GLSL;
    renderer->min_shader_version = 110;
    renderer->max_shader_version = R_GLSL_VERSION;

    renderer->default_image_anchor_x = 0.5f;
    renderer->default_image_anchor_y = 0.5f;

    renderer->current_context_target = NULL;

    // renderer->impl = (R_RendererImpl *)ME_MALLOC(sizeof(R_RendererImpl));
    // memset(renderer->impl, 0, sizeof(R_RendererImpl));
    // SET_COMMON_FUNCTIONS(renderer->impl);

    return renderer;
}

void R_FreeRenderer_OpenGL_3(R_Renderer *renderer) {
    if (renderer == NULL) return;

    // ME_FREE(renderer->impl);
    ME_FREE(renderer);
}

}  // namespace ME