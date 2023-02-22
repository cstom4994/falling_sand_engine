

#include "metadot_gl.h"

#include "core/core.h"

#if !defined(METADOT__GL_MALLOC) || !defined(METADOT__GL_FREE)
#include <stdlib.h>
#define METADOT__GL_MALLOC(size, ctx) (malloc(size))
#define METADOT__GL_FREE(ptr, ctx) (free(ptr))
#endif

#ifdef NDEBUG
#define METADOT__GL_ASSERT(...)
#else
#ifndef METADOT__GL_ASSERT
#include <assert.h>
#define METADOT__GL_ASSERT(expr) (assert(expr))
#endif
#endif

#ifndef METADOT__GL_LOG
#include <stdio.h>
#define METADOT__GL_LOG(...) (METADOT_INFO(__VA_ARGS__))
#endif

#include <string.h>  // memset, strcmp

#ifndef METADOT__GL_UNIFORM_NAME_LENGTH
#define METADOT__GL_UNIFORM_NAME_LENGTH 32
#endif

#ifndef METADOT__GL_MAX_UNIFORMS
#define METADOT__GL_MAX_UNIFORMS 32
#endif

#ifndef METADOT__GL_MAX_STATES
#define METADOT__GL_MAX_STATES 32
#endif

/*=============================================================================
 * Internal aliases
 *============================================================================*/

#define METADOT_GL_MALLOC METADOT__GL_MALLOC
#define METADOT_GL_FREE METADOT__GL_FREE
#define METADOT_GL_ASSERT METADOT__GL_ASSERT
#define METADOT_GL_LOG METADOT__GL_LOG
#define METADOT_GL_UNIFORM_NAME_LENGTH METADOT__GL_UNIFORM_NAME_LENGTH
#define METADOT_GL_MAX_UNIFORMS METADOT__GL_MAX_UNIFORMS
#define METADOT_GL_MAX_STATES METADOT__GL_MAX_STATES

/*=============================================================================
 * Internal PGL enum to GL enum maps
 *============================================================================*/

#ifdef NDEBUG
#define METADOT_GL_CHECK(expr) (expr)
#else
#define METADOT_GL_CHECK(expr)                           \
    do {                                                 \
        expr;                                            \
        metadot_gl_log_error(__FILE__, __LINE__, #expr); \
    } while (false)
#endif

static bool metadot_gl_initialized = false;

static const char *metadot_gl_error_msg_map[] = {"No error",
                                                 "Invalid enumeration value",
                                                 "Invalid value",
                                                 "Invalid operation",
                                                 "Out of memory",
                                                 "Invalid framebuffer operation",
                                                 "Framebuffer is incomplete",
                                                 "Shader compilation error",
                                                 "Shader linking error",
                                                 "Invalid texture dimensions",
                                                 "Invalid texture format",
                                                 "Invalid number of attributes",
                                                 "Invalid number of uniforms",
                                                 "Invalid uniform name",
                                                 "Unknown error",
                                                 0};

static const GLenum metadot_gl_blend_factor_map[] = {
        GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA};

static const GLenum metadot_gl_blend_eq_map[] = {GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX};

static const GLenum metadot_gl_primitive_map[] = {GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP};

/*=============================================================================
 * Internal struct declarations
 *============================================================================*/

typedef uint32_t metadot_gl_hash_t;

typedef struct {
    bool active;
    GLint location;
    GLsizei size;
    GLvoid *offset;
} metadot_gl_attribute_t;

typedef struct {
    char name[METADOT_GL_UNIFORM_NAME_LENGTH];
    GLsizei size;
    GLenum type;
    GLint location;
    metadot_gl_hash_t hash;
} metadot_gl_uniform_t;

typedef struct {
    int32_t x, y, w, h;
} metadot_gl_viewport_t;

typedef struct {
    metadot_gl_blend_mode_t blend_mode;
    metadot_gl_m4_t transform;
    metadot_gl_m4_t projection;
    metadot_gl_viewport_t viewport;
    float line_width;
} metadot_gl_state_t;

typedef struct {
    metadot_gl_size_t size;
    metadot_gl_state_t state;
    metadot_gl_state_t array[METADOT_GL_MAX_STATES];
} metadot_gl_state_stack_t;

/*=============================================================================
 * Internal function declarations
 *============================================================================*/

static void metadot_gl_set_error(metadot_gl_ctx_t *ctx, metadot_gl_error_t code);

static const char *metadot_gl_get_default_vert_shader();
static const char *metadot_gl_get_default_frag_shader();

static metadot_gl_state_stack_t *metadot_gl_get_active_stack(metadot_gl_ctx_t *ctx);
static metadot_gl_state_t *metadot_gl_get_active_state(metadot_gl_ctx_t *ctx);

static void metadot_gl_reset_last_state(metadot_gl_ctx_t *ctx);

static void metadot_gl_apply_blend(metadot_gl_ctx_t *ctx, const metadot_gl_blend_mode_t *mode);
static void metadot_gl_apply_transform(metadot_gl_ctx_t *ctx, const metadot_gl_m4_t matrix);
static void metadot_gl_apply_projection(metadot_gl_ctx_t *ctx, const metadot_gl_m4_t matrix);
static void metadot_gl_apply_viewport(metadot_gl_ctx_t *ctx, const metadot_gl_viewport_t *viewport);

static void metadot_gl_before_draw(metadot_gl_ctx_t *ctx, metadot_gl_texture_t *texture, metadot_gl_shader_t *shader);
static void metadot_gl_after_draw(metadot_gl_ctx_t *ctx);

static int metadot_gl_load_uniforms(metadot_gl_shader_t *shader);
static const metadot_gl_uniform_t *metadot_gl_find_uniform(const metadot_gl_shader_t *shader, const char *name);

static void metadot_gl_bind_attributes();

static void metadot_gl_log(const char *fmt, ...);
static void metadot_gl_log_error(const char *file, unsigned line, const char *expr);
static metadot_gl_error_t metadot_gl_map_error(GLenum id);
static metadot_gl_hash_t metadot_gl_hash_str(const char *str);
static bool metadot_gl_str_equal(const char *str1, const char *str2);
static bool metadot_gl_mem_equal(const void *ptr1, const void *ptr2, size_t size);

/*=============================================================================
 * Internal constants
 *============================================================================*/

// 64-bit FNV1a Constants
// static const metadot_gl_hash_t METADOT_GL_OFFSET_BASIS = 0xCBF29CE484222325;
// static const metadot_gl_hash_t METADOT_GL_PRIME = 0x100000001B3;

static const metadot_gl_hash_t METADOT_GL_OFFSET_BASIS = 0x811C9DC5;
static const metadot_gl_hash_t METADOT_GL_PRIME = 0x1000193;

/*=============================================================================
 * Shaders GL3
 *============================================================================*/

#define METADOT_GL_GL_HDR \
    ""                    \
    "#version 330 core\n"

#define METADOT_GL_GLES_HDR \
    ""                      \
    "#version 310 es\n"

#define METADOT_GL_GL_VERT_BODY                                       \
    ""                                                                \
    "layout (location = 0) in vec3 a_pos;\n"                          \
    "layout (location = 1) in vec4 a_color;\n"                        \
    "layout (location = 2) in vec2 a_uv;\n"                           \
    "\n"                                                              \
    "out vec4 color;\n"                                               \
    "out vec2 uv;\n"                                                  \
    "\n"                                                              \
    "uniform mat4 u_transform;\n"                                     \
    "uniform mat4 u_projection;\n"                                    \
    "\n"                                                              \
    "void main()\n"                                                   \
    "{\n"                                                             \
    "   gl_Position = u_projection * u_transform * vec4(a_pos, 1);\n" \
    "   color = a_color;\n"                                           \
    "   uv = a_uv;\n"                                                 \
    "}\n"

#define METADOT_GL_GL_FRAG_BODY                     \
    ""                                              \
    "#ifdef GL_ES\n"                                \
    "#ifdef GL_FRAGMENT_PRECISION_HIGH\n"           \
    "   precision highp float;\n"                   \
    "#else\n"                                       \
    "   precision mediump float;\n"                 \
    "#endif\n"                                      \
    "#endif\n"                                      \
    "out vec4 frag_color;\n"                        \
    "\n"                                            \
    "in vec4 color;\n"                              \
    "in vec2 uv;\n"                                 \
    "\n"                                            \
    "uniform sampler2D u_tex;\n"                    \
    "\n"                                            \
    "void main()\n"                                 \
    "{\n"                                           \
    "   frag_color = texture(u_tex, uv) * color;\n" \
    "}\n"

static const GLchar *METADOT_GL_GL_VERT_SRC = METADOT_GL_GL_HDR METADOT_GL_GL_VERT_BODY;
static const GLchar *METADOT_GL_GL_FRAG_SRC = METADOT_GL_GL_HDR METADOT_GL_GL_FRAG_BODY;

static const GLchar *METADOT_GL_GLES_VERT_SRC = METADOT_GL_GLES_HDR METADOT_GL_GL_VERT_BODY;
static const GLchar *METADOT_GL_GLES_FRAG_SRC = METADOT_GL_GLES_HDR METADOT_GL_GL_FRAG_BODY;

/*=============================================================================
 * Public API implementation
 *============================================================================*/

struct metadot_gl_ctx_t {
    metadot_gl_error_t error_code;
    metadot_gl_shader_t *shader;
    metadot_gl_texture_t *texture;
    metadot_gl_texture_t *target;
    metadot_gl_state_t last_state;
    metadot_gl_state_stack_t stack;
    metadot_gl_state_stack_t target_stack;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    uint32_t w, h;
    uint32_t samples;
    bool srgb;
    bool depth;
    bool transpose;
    void *mem_ctx;
};

struct metadot_gl_shader_t {
    metadot_gl_ctx_t *ctx;

    GLuint program;

    metadot_gl_size_t uniform_count;
    metadot_gl_uniform_t uniforms[METADOT_GL_MAX_UNIFORMS];

    metadot_gl_attribute_t pos;
    metadot_gl_attribute_t color;
    metadot_gl_attribute_t uv;
};

struct metadot_gl_texture_t {
    GLuint id;
    metadot_gl_ctx_t *ctx;
    bool target;
    int32_t w, h;
    bool srgb;
    bool smooth;
    bool mipmap;
    GLuint fbo;
    GLuint fbo_msaa;
    GLuint rbo_msaa;
    GLuint depth_id;
    GLuint depth_rbo_msaa;
};

struct metadot_gl_buffer_t {
    GLenum primitive;
    GLuint vao;
    GLuint vbo;
    GLsizei count;
};

metadot_gl_error_t metadot_gl_get_error(metadot_gl_ctx_t *ctx) { return ctx->error_code; }

const char *metadot_gl_get_error_str(metadot_gl_error_t code) {
    METADOT_GL_ASSERT(code < METADOT_GL_ERROR_COUNT);

    if (code < METADOT_GL_ERROR_COUNT)
        return metadot_gl_error_msg_map[(metadot_gl_size_t)code];
    else
        return NULL;
}

metadot_gl_version_t metadot_gl_get_version() {
    if (!metadot_gl_initialized) {
        METADOT_GL_LOG("Library hasn't been initialized: call metadot_gl_global_init");
        return METADOT_GL_VERSION_UNSUPPORTED;
    }

    if (GLAD_GL_VERSION_3_3) return METADOT_GL_GL3;

    // if (GLAD_GL_ES_VERSION_3_1) return METADOT_GL_GLES3;

    return METADOT_GL_VERSION_UNSUPPORTED;
}

void metadot_gl_print_info() {
    if (!metadot_gl_initialized) {
        METADOT_GL_LOG("Library hasn't been initialized: call metadot_gl_global_init");
        return;
    }

    const unsigned char *vendor = glGetString(GL_VENDOR);
    const unsigned char *renderer = glGetString(GL_RENDERER);
    const unsigned char *gl_version = glGetString(GL_VERSION);
    const unsigned char *glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);

    int tex_w, tex_h;
    metadot_gl_get_max_texture_size(&tex_w, &tex_h);

    METADOT_GL_LOG("OpenGL info:");
    METADOT_GL_LOG("Vendor: %s", vendor);
    METADOT_GL_LOG("Renderer: %s", renderer);
    METADOT_GL_LOG("GL Version: %s", gl_version);
    METADOT_GL_LOG("GLSL Version: %s", glsl_version);
    METADOT_GL_LOG("Max texture size: %ix%i", tex_w, tex_h);
}

int metadot_gl_global_init(metadot_gl_loader_fn loader_fp) {

    // if (gles) {
    //     if (!gladLoadGLES2Loader((GLADloadproc)loader_fp)) {
    //         METADOT_GL_LOG("GLAD GLES2 loader failed");
    //         return -1;
    //     }

    //     metadot_gl_initialized = true;

    //     return 0;
    // }

    if (NULL == loader_fp) {
        if (!gladLoadGL()) {
            METADOT_GL_LOG("GLAD GL3 loader failed");
            return -1;
        }
    } else {
        if (!gladLoadGLLoader((GLADloadproc)loader_fp)) {
            METADOT_GL_LOG("GLAD GL3 loader failed");
            return -1;
        }
    }

    METADOT_GL_CHECK(glEnable(GL_BLEND));

    metadot_gl_initialized = true;

    return 0;
}

metadot_gl_ctx_t *metadot_gl_create_context(uint32_t w, uint32_t h, bool depth, uint32_t samples, bool srgb, void *mem_ctx) {
    if (!metadot_gl_initialized) {
        METADOT_GL_LOG("Library hasn't been initialized: call metadot_gl_global_init");
        return NULL;
    }

    metadot_gl_ctx_t *ctx = (metadot_gl_ctx_t *)METADOT_GL_MALLOC(sizeof(metadot_gl_ctx_t), mem_ctx);

    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(metadot_gl_ctx_t));

    ctx->w = w;
    ctx->h = h;
    ctx->samples = samples;
    ctx->srgb = srgb;
    ctx->depth = depth;
    ctx->mem_ctx = mem_ctx;

    // Create VBO/EBO
    METADOT_GL_CHECK(glGenVertexArrays(1, &ctx->vao));
    METADOT_GL_CHECK(glBindVertexArray(ctx->vao));
    METADOT_GL_CHECK(glGenBuffers(1, &ctx->vbo));
    METADOT_GL_CHECK(glGenBuffers(1, &ctx->ebo));
    METADOT_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->ebo));
    METADOT_GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW));
    METADOT_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo));
    METADOT_GL_CHECK(glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW));
    metadot_gl_bind_attributes();
    METADOT_GL_CHECK(glBindVertexArray(0));

    if (samples > 0) {
        GLint max_samples = 0;
        METADOT_GL_CHECK(glGetIntegerv(GL_MAX_SAMPLES, &max_samples));

        if (samples > (uint32_t)max_samples) ctx->samples = (uint32_t)max_samples;

        METADOT_GL_CHECK(glEnable(GL_MULTISAMPLE));
    }

    metadot_gl_clear_stack(ctx);
    metadot_gl_reset_state(ctx);

    return ctx;
}

void metadot_gl_destroy_context(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);

    METADOT_GL_CHECK(glDeleteBuffers(1, &ctx->vbo));
    METADOT_GL_CHECK(glDeleteBuffers(1, &ctx->ebo));
    METADOT_GL_CHECK(glDeleteVertexArrays(1, &ctx->vao));
    METADOT_GL_FREE(ctx, ctx->mem_ctx);
}

void metadot_gl_resize(metadot_gl_ctx_t *ctx, uint32_t w, uint32_t h, bool reset_vp) {
    METADOT_GL_ASSERT(ctx);

    ctx->w = w;
    ctx->h = h;

    if (reset_vp) metadot_gl_reset_viewport(ctx);
}

metadot_gl_shader_t *metadot_gl_create_shader(metadot_gl_ctx_t *ctx, const char *vert_src, const char *frag_src) {
    METADOT_GL_ASSERT(ctx);

    if (!vert_src && !frag_src) {
        vert_src = metadot_gl_get_default_vert_shader();
        frag_src = metadot_gl_get_default_frag_shader();
    } else if (!vert_src) {
        vert_src = metadot_gl_get_default_vert_shader();
    } else if (!frag_src) {
        frag_src = metadot_gl_get_default_frag_shader();
    }

    // Create shaders
    GLuint vs, fs, program;

    METADOT_GL_CHECK(vs = glCreateShader(GL_VERTEX_SHADER));
    METADOT_GL_CHECK(fs = glCreateShader(GL_FRAGMENT_SHADER));

    GLsizei length;
    GLchar msg[2048];

    // Compile vertex shader
    GLint is_compiled = GL_FALSE;

    METADOT_GL_CHECK(glShaderSource(vs, 1, &vert_src, NULL));
    METADOT_GL_CHECK(glCompileShader(vs));
    METADOT_GL_CHECK(glGetShaderiv(vs, GL_COMPILE_STATUS, &is_compiled));

    if (GL_FALSE == is_compiled) {
        METADOT_GL_CHECK(glGetShaderInfoLog(vs, sizeof(msg), &length, msg));
        METADOT_GL_CHECK(glDeleteShader(vs));
        METADOT_GL_LOG("Error compiling vertex shader: %s", msg);
        metadot_gl_set_error(ctx, METADOT_GL_SHADER_COMPILATION_ERROR);
        return NULL;
    }

    // Compile fragment shader
    is_compiled = GL_FALSE;

    METADOT_GL_CHECK(glShaderSource(fs, 1, &frag_src, NULL));
    METADOT_GL_CHECK(glCompileShader(fs));
    METADOT_GL_CHECK(glGetShaderiv(fs, GL_COMPILE_STATUS, &is_compiled));

    if (GL_FALSE == is_compiled) {
        METADOT_GL_CHECK(glGetShaderInfoLog(fs, sizeof(msg), &length, msg));
        METADOT_GL_CHECK(glDeleteShader(vs));
        METADOT_GL_CHECK(glDeleteShader(fs));
        METADOT_GL_LOG("Error compiling fragment shader: %s", msg);
        metadot_gl_set_error(ctx, METADOT_GL_SHADER_COMPILATION_ERROR);
        return NULL;
    }

    // Link program
    GLint is_linked = GL_FALSE;

    METADOT_GL_CHECK(program = glCreateProgram());

    METADOT_GL_CHECK(glAttachShader(program, vs));
    METADOT_GL_CHECK(glAttachShader(program, fs));
    METADOT_GL_CHECK(glLinkProgram(program));
    METADOT_GL_CHECK(glGetProgramiv(program, GL_LINK_STATUS, &is_linked));

    METADOT_GL_CHECK(glDetachShader(program, vs));
    METADOT_GL_CHECK(glDetachShader(program, fs));
    METADOT_GL_CHECK(glDeleteShader(vs));
    METADOT_GL_CHECK(glDeleteShader(fs));

    if (GL_FALSE == is_linked) {
        METADOT_GL_CHECK(glGetProgramInfoLog(program, sizeof(msg), &length, msg));
        METADOT_GL_CHECK(glDeleteProgram(program));
        METADOT_GL_LOG("Error linking shader program: %s", msg);
        metadot_gl_set_error(ctx, METADOT_GL_SHADER_LINKING_ERROR);
        return NULL;
    }

    metadot_gl_shader_t *shader = (metadot_gl_shader_t *)METADOT_GL_MALLOC(sizeof(metadot_gl_shader_t), ctx->mem_ctx);

    if (!shader) {
        metadot_gl_set_error(ctx, METADOT_GL_OUT_OF_MEMORY);
        return NULL;
    }

    memset(shader, 0, sizeof(metadot_gl_shader_t));

    shader->program = program;
    shader->ctx = ctx;

    metadot_gl_bind_shader(ctx, shader);
    metadot_gl_load_uniforms(shader);

    return shader;
}

void metadot_gl_destroy_shader(metadot_gl_shader_t *shader)

{
    METADOT_GL_ASSERT(shader);

    metadot_gl_bind_shader(shader->ctx, NULL);
    METADOT_GL_CHECK(glDeleteProgram(shader->program));
    METADOT_GL_FREE(shader, shader->ctx->mem_ctx);
}

uint64_t metadot_gl_get_shader_id(const metadot_gl_shader_t *shader) {
    METADOT_GL_ASSERT(shader);

    return shader->program;
}

void metadot_gl_bind_shader(metadot_gl_ctx_t *ctx, metadot_gl_shader_t *shader) {
    METADOT_GL_ASSERT(ctx);

    if (ctx->shader == shader) return;

    if (NULL != shader) {
        METADOT_GL_CHECK(glUseProgram(shader->program));
        ctx->shader = shader;
    } else {
        METADOT_GL_CHECK(glUseProgram(0));
        ctx->shader = NULL;
    }
}

static void metadot_gl_set_texture_params(GLuint tex_id, bool smooth, bool repeat) {
    METADOT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, tex_id));

    METADOT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smooth ? GL_LINEAR : GL_NEAREST));

    METADOT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST));

    METADOT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE));

    METADOT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE));
}

metadot_gl_texture_t *metadot_gl_create_texture(metadot_gl_ctx_t *ctx, bool target, int32_t w, int32_t h, bool srgb, bool smooth, bool repeat) {
    METADOT_GL_ASSERT(ctx);

    // Check texture dimensions
    if (w <= 0 || h <= 0) {
        METADOT_GL_LOG("Texture dimensions must be positive (w: %i, h: %i)", w, h);
        metadot_gl_set_error(ctx, METADOT_GL_INVALID_TEXTURE_SIZE);
        return NULL;
    }

    GLint max_size;
    METADOT_GL_CHECK(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size));

    if (w > max_size || h > max_size) {
        METADOT_GL_LOG("Texture dimensions exceed max size (w: %i, h: %i)", w, h);
        metadot_gl_set_error(ctx, METADOT_GL_INVALID_TEXTURE_SIZE);
        return NULL;
    }

    // Allocate texture
    metadot_gl_texture_t *tex = (metadot_gl_texture_t *)METADOT_GL_MALLOC(sizeof(metadot_gl_texture_t), ctx->mem_ctx);

    if (!tex) {
        metadot_gl_set_error(ctx, METADOT_GL_OUT_OF_MEMORY);
        return NULL;
    }

    METADOT_GL_CHECK(glGenTextures(1, &tex->id));

    tex->ctx = ctx;
    tex->w = w;
    tex->h = h;
    tex->srgb = srgb;
    tex->target = target;
    tex->smooth = smooth;

    if (-1 == metadot_gl_upload_texture(ctx, tex, w, h, NULL)) {
        METADOT_GL_CHECK(glDeleteTextures(1, &tex->id));
        METADOT_GL_FREE(tex, ctx->mem_ctx);
        return NULL;
    }

    metadot_gl_set_texture_params(tex->id, smooth, repeat);

    // Generate objects for render target
    if (target) {
        // Generate framebuffer
        METADOT_GL_CHECK(glGenFramebuffers(1, &tex->fbo));

        // Create depth texture
        if (ctx->depth) {
            METADOT_GL_CHECK(glGenTextures(1, &tex->depth_id));

            metadot_gl_set_texture_params(tex->depth_id, smooth, repeat);

            METADOT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, tex->depth_id));
            METADOT_GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL));
        }

        // Generate multi-sample buffers for MSAA
        if (ctx->samples > 0) {
            METADOT_GL_CHECK(glGenFramebuffers(1, &tex->fbo_msaa));
            METADOT_GL_CHECK(glGenRenderbuffers(1, &tex->rbo_msaa));

            // Generate depth buffer for MSAA
            if (ctx->depth) METADOT_GL_CHECK(glGenRenderbuffers(1, &tex->depth_rbo_msaa));
        }
    }

    // TODO: Consider breaking up this function
    // Create attachments
    if (target) {
        // Bind framebuffer
        METADOT_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, tex->fbo));

        // Create framebuffer attachment
        METADOT_GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->id, 0));

        // Create framebuffer depth attachment
        if (ctx->depth) {

            METADOT_GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex->depth_id, 0));
        }

        // Create attachments for MSAA
        if (ctx->samples > 0) {
            // Bind MSAA render buffer
            METADOT_GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, tex->rbo_msaa));

            // Create multi-sample buffer storage
            METADOT_GL_CHECK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, ctx->samples, ctx->srgb ? GL_SRGB8_ALPHA8 : GL_RGBA, tex->w, tex->h));

            METADOT_GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));

            // Bind MSAA framebuffer
            METADOT_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, tex->fbo_msaa));

            // Create color attachment
            METADOT_GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, tex->rbo_msaa));

            // Create attachment for MSAA with depth test
            if (ctx->depth) {
                // Bind MSAA depth buffer
                METADOT_GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, tex->depth_rbo_msaa));

                // Create multi-sample depth buffer storage
                METADOT_GL_CHECK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, ctx->samples, GL_DEPTH_COMPONENT24, tex->w, tex->h));

                // Create depth attachment
                METADOT_GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, tex->depth_rbo_msaa));
            }
        }

        // Check to see if framebuffer is complete
        GLenum status;
        METADOT_GL_CHECK(status = glCheckFramebufferStatus(GL_FRAMEBUFFER));

        if (status != GL_FRAMEBUFFER_COMPLETE) {
            METADOT_GL_LOG("Framebuffer incomplete");
            metadot_gl_set_error(ctx, METADOT_GL_FRAMEBUFFER_INCOMPLETE);

            // Framebuffer is incomplete, so release resources

            METADOT_GL_CHECK(glDeleteTextures(1, &tex->id));
            METADOT_GL_CHECK(glDeleteFramebuffers(1, &tex->fbo));

            if (ctx->depth) {
                METADOT_GL_CHECK(glDeleteTextures(1, &tex->depth_id));
            }

            if (ctx->samples > 0) {
                METADOT_GL_CHECK(glDeleteRenderbuffers(1, &tex->rbo_msaa));
                METADOT_GL_CHECK(glDeleteFramebuffers(1, &tex->fbo_msaa));

                if (ctx->depth) {
                    METADOT_GL_CHECK(glDeleteRenderbuffers(1, &tex->depth_rbo_msaa));
                }
            }

            METADOT_GL_FREE(tex, ctx->mem_ctx);

            return NULL;
        }

        // Ensure framebuffer objects are not bound
        METADOT_GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, 0));
        METADOT_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }

    return tex;
}

metadot_gl_texture_t *metadot_gl_texture_from_bitmap(metadot_gl_ctx_t *ctx, int32_t w, int32_t h, bool srgb, bool smooth, bool repeat, const unsigned char *bitmap)

{
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(bitmap);

    metadot_gl_texture_t *tex = metadot_gl_create_texture(ctx, false, w, h, srgb, smooth, repeat);

    if (!tex) return NULL;

    if (-1 == metadot_gl_upload_texture(ctx, tex, w, h, bitmap)) {
        METADOT_GL_CHECK(glDeleteTextures(1, &tex->id));
        METADOT_GL_FREE(tex, ctx->mem_ctx);
        return NULL;
    }

    return tex;
}

int metadot_gl_upload_texture(metadot_gl_ctx_t *ctx, metadot_gl_texture_t *texture, int32_t w, int32_t h, const unsigned char *bitmap) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(texture);

    metadot_gl_bind_texture(ctx, texture);

    METADOT_GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, texture->srgb ? GL_SRGB8_ALPHA8 : GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap));

    return 0;
}

void metadot_gl_update_texture(metadot_gl_ctx_t *ctx, metadot_gl_texture_t *texture, int x, int y, int w, int h, const uint8_t *bitmap) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(texture);
    METADOT_GL_ASSERT(bitmap);

    metadot_gl_bind_texture(ctx, texture);

    METADOT_GL_CHECK(glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, bitmap));
}

int metadot_gl_generate_mipmap(metadot_gl_texture_t *texture, bool linear) {
    METADOT_GL_ASSERT(texture);

    if (texture->mipmap)  // TODO: Return error? void?
        return 0;

    metadot_gl_bind_texture(texture->ctx, texture);

    if (linear) {
        METADOT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture->smooth ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR));
    } else {
        METADOT_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture->smooth ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST));
    }

    METADOT_GL_CHECK(glGenerateMipmap(GL_TEXTURE_2D));

    texture->mipmap = true;

    return 0;
}

void metadot_gl_destroy_texture(metadot_gl_texture_t *tex) {
    METADOT_GL_ASSERT(tex);

    metadot_gl_bind_texture(tex->ctx, NULL);
    METADOT_GL_CHECK(glDeleteTextures(1, &tex->id));

    if (tex->target) {
        METADOT_GL_CHECK(glDeleteFramebuffers(1, &tex->fbo));

        if (tex->ctx->depth) {
            METADOT_GL_CHECK(glDeleteTextures(1, &tex->depth_id));
        }

        if (tex->ctx->samples > 0) {
            METADOT_GL_CHECK(glDeleteRenderbuffers(1, &tex->rbo_msaa));
            METADOT_GL_CHECK(glDeleteFramebuffers(1, &tex->fbo_msaa));

            if (tex->ctx->depth) {
                METADOT_GL_CHECK(glDeleteRenderbuffers(1, &tex->depth_rbo_msaa));
            }
        }
    }

    METADOT_GL_FREE(tex, tex->ctx->mem_ctx);
}

void metadot_gl_get_texture_size(const metadot_gl_texture_t *texture, int *w, int *h) {
    METADOT_GL_ASSERT(texture);

    if (w) *w = texture->w;
    if (h) *h = texture->h;
}

void metadot_gl_get_max_texture_size(int *w, int *h) {
    int max_size = 0;
    METADOT_GL_CHECK(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size));

    if (w) *w = max_size;
    if (h) *h = max_size;
}

uint64_t metadot_gl_get_texture_id(const metadot_gl_texture_t *texture) {
    METADOT_GL_ASSERT(texture);
    return texture->id;
}

void metadot_gl_bind_texture(metadot_gl_ctx_t *ctx, metadot_gl_texture_t *texture) {
    METADOT_GL_ASSERT(ctx);

    if (ctx->texture == texture) return;

    if (NULL != texture) {
        METADOT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture->id));
        ctx->texture = texture;
    } else {
        METADOT_GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));
        ctx->texture = NULL;
    }
}

int metadot_gl_set_render_target(metadot_gl_ctx_t *ctx, metadot_gl_texture_t *target) {
    METADOT_GL_ASSERT(ctx);

    if (ctx->target == target) return 0;

    if (ctx->target && ctx->samples > 0) {
        METADOT_GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, ctx->target->fbo_msaa));
        METADOT_GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, ctx->target->fbo));
        METADOT_GL_CHECK(glBlitFramebuffer(0, 0, ctx->target->w, ctx->target->h, 0, 0, ctx->target->w, ctx->target->h, GL_COLOR_BUFFER_BIT, GL_LINEAR));
    }

    if (!target) {
        METADOT_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

        ctx->target = NULL;
        metadot_gl_reset_last_state(ctx);

        return 0;
    }

    if (ctx->samples > 0)
        METADOT_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, target->fbo_msaa));
    else
        METADOT_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, target->fbo));

    ctx->target = target;

    metadot_gl_clear_stack(ctx);
    metadot_gl_reset_state(ctx);
    metadot_gl_reset_last_state(ctx);
    metadot_gl_set_viewport(ctx, 0, 0, target->w, target->h);

    return 0;
}

void metadot_gl_clear(float r, float g, float b, float a) {
    METADOT_GL_CHECK(glClearColor(r, g, b, a));
    METADOT_GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void metadot_gl_draw_array(metadot_gl_ctx_t *ctx, metadot_gl_primitive_t primitive, const metadot_gl_vertex_t *vertices, metadot_gl_size_t count, metadot_gl_texture_t *texture,
                           metadot_gl_shader_t *shader) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(vertices);
    METADOT_GL_ASSERT(shader);

    metadot_gl_before_draw(ctx, texture, shader);

    METADOT_GL_CHECK(glBindVertexArray(ctx->vao));

    METADOT_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo));
    METADOT_GL_CHECK(glBufferData(GL_ARRAY_BUFFER, count * sizeof(metadot_gl_vertex_t), vertices, GL_STATIC_DRAW));

    METADOT_GL_CHECK(glDrawArrays(metadot_gl_primitive_map[primitive], 0, count));
    METADOT_GL_CHECK(glBindVertexArray(0));

    metadot_gl_after_draw(ctx);
}

void metadot_gl_draw_indexed_array(metadot_gl_ctx_t *ctx, metadot_gl_primitive_t primitive, const metadot_gl_vertex_t *vertices, metadot_gl_size_t vertex_count, const uint32_t *indices,
                                   metadot_gl_size_t index_count, metadot_gl_texture_t *texture, metadot_gl_shader_t *shader) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(vertices);
    METADOT_GL_ASSERT(indices);
    METADOT_GL_ASSERT(shader);

    metadot_gl_before_draw(ctx, texture, shader);

    METADOT_GL_CHECK(glBindVertexArray(ctx->vao));

    METADOT_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, ctx->vbo));
    METADOT_GL_CHECK(glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(metadot_gl_vertex_t), vertices, GL_STATIC_DRAW));

    METADOT_GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ctx->ebo));
    METADOT_GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(GLuint), indices, GL_STATIC_DRAW));

    METADOT_GL_CHECK(glDrawElements(metadot_gl_primitive_map[primitive], index_count, GL_UNSIGNED_INT, 0));
    METADOT_GL_CHECK(glBindVertexArray(0));

    metadot_gl_after_draw(ctx);
}

metadot_gl_buffer_t *metadot_gl_create_buffer(metadot_gl_ctx_t *ctx, metadot_gl_primitive_t primitive, const metadot_gl_vertex_t *vertices, metadot_gl_size_t count) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(vertices);

    metadot_gl_buffer_t *buffer = (metadot_gl_buffer_t *)METADOT_GL_MALLOC(sizeof(metadot_gl_buffer_t), ctx->mem_ctx);

    if (!buffer) {
        metadot_gl_set_error(ctx, METADOT_GL_OUT_OF_MEMORY);
        return NULL;
    }

    METADOT_GL_CHECK(glGenVertexArrays(1, &buffer->vao));
    METADOT_GL_CHECK(glGenBuffers(1, &buffer->vbo));

    METADOT_GL_CHECK(glBindVertexArray(buffer->vao));

    METADOT_GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, buffer->vbo));
    METADOT_GL_CHECK(glBufferData(GL_ARRAY_BUFFER, count * sizeof(metadot_gl_vertex_t), vertices, GL_STATIC_DRAW));

    metadot_gl_bind_attributes();
    METADOT_GL_CHECK(glBindVertexArray(0));

    buffer->primitive = metadot_gl_primitive_map[primitive];
    buffer->count = count;

    return buffer;
}

void metadot_gl_destroy_buffer(metadot_gl_buffer_t *buffer) {
    METADOT_GL_ASSERT(buffer);

    METADOT_GL_CHECK(glDeleteVertexArrays(1, &buffer->vao));
    METADOT_GL_CHECK(glDeleteBuffers(1, &buffer->vbo));

    METADOT_GL_FREE(buffer, buffer->ctx->mem_ctx);
}

void metadot_gl_draw_buffer(metadot_gl_ctx_t *ctx, const metadot_gl_buffer_t *buffer, metadot_gl_size_t start, metadot_gl_size_t count, metadot_gl_texture_t *texture, metadot_gl_shader_t *shader) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(buffer);
    METADOT_GL_ASSERT(shader);

    METADOT_GL_ASSERT(start + count <= (metadot_gl_size_t)buffer->count);

    metadot_gl_before_draw(ctx, texture, shader);

    METADOT_GL_CHECK(glBindVertexArray(buffer->vao));
    METADOT_GL_CHECK(glDrawArrays(buffer->primitive, start, count));
    METADOT_GL_CHECK(glBindVertexArray(0));

    metadot_gl_after_draw(ctx);
}

void metadot_gl_set_transpose(metadot_gl_ctx_t *ctx, bool enabled) {
    METADOT_GL_ASSERT(ctx);
    ctx->transpose = enabled;
}

void metadot_gl_set_blend_mode(metadot_gl_ctx_t *ctx, metadot_gl_blend_mode_t mode) {
    METADOT_GL_ASSERT(ctx);

    metadot_gl_state_t *state = metadot_gl_get_active_state(ctx);
    state->blend_mode = mode;
}

void metadot_gl_reset_blend_mode(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);

    metadot_gl_state_t *state = metadot_gl_get_active_state(ctx);

    metadot_gl_blend_mode_t mode = {METADOT_GL_SRC_ALPHA, METADOT_GL_ONE_MINUS_SRC_ALPHA, METADOT_GL_FUNC_ADD, METADOT_GL_ONE, METADOT_GL_ONE_MINUS_SRC_ALPHA, METADOT_GL_FUNC_ADD};

    state->blend_mode = mode;
}

void metadot_gl_set_transform(metadot_gl_ctx_t *ctx, const metadot_gl_m4_t matrix) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(matrix);

    metadot_gl_state_t *state = metadot_gl_get_active_state(ctx);
    memcpy(state->transform, matrix, sizeof(metadot_gl_m4_t));
}

void metadot_gl_set_transform_3d(metadot_gl_ctx_t *ctx, const metadot_gl_m3_t matrix) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(matrix);

    if (ctx->transpose) {
        const metadot_gl_m4_t matrix4 = {matrix[0], matrix[1], 0.0f, matrix[2], matrix[3], matrix[4], 0.0f, matrix[5], 0.0f, 0.0f, 1.0f, 0.0f, matrix[6], matrix[7], 0.0f, matrix[8]};

        metadot_gl_set_transform(ctx, matrix4);
    } else {
        const metadot_gl_m4_t matrix4 = {matrix[0], matrix[3], 0.0f, matrix[6], matrix[1], matrix[4], 0.0f, matrix[7], 0.0f, 0.0f, 1.0f, 0.0f, matrix[2], matrix[5], 0.0f, matrix[8]};

        metadot_gl_set_transform(ctx, matrix4);
    }
}

void metadot_gl_reset_transform(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);

    metadot_gl_state_t *state = metadot_gl_get_active_state(ctx);

    const metadot_gl_m4_t matrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    memcpy(state->transform, matrix, sizeof(metadot_gl_m4_t));
}

void metadot_gl_set_projection(metadot_gl_ctx_t *ctx, const metadot_gl_m4_t matrix) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(matrix);

    metadot_gl_state_t *state = metadot_gl_get_active_state(ctx);
    memcpy(state->projection, matrix, sizeof(metadot_gl_m4_t));
}

void metadot_gl_set_projection_3d(metadot_gl_ctx_t *ctx, const metadot_gl_m3_t matrix) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(matrix);

    if (ctx->transpose) {
        const metadot_gl_m4_t matrix4 = {matrix[0], matrix[1], 0.0f, matrix[2], matrix[3], matrix[4], 0.0f, matrix[5], 0.0f, 0.0f, 1.0f, 0.0f, matrix[6], matrix[7], 0.0f, matrix[8]};

        metadot_gl_set_projection(ctx, matrix4);
    } else {
        const metadot_gl_m4_t matrix4 = {matrix[0], matrix[3], 0.0f, matrix[6], matrix[1], matrix[4], 0.0f, matrix[7], 0.0f, 0.0f, 1.0f, 0.0f, matrix[2], matrix[5], 0.0f, matrix[8]};

        metadot_gl_set_projection(ctx, matrix4);
    }
}

void metadot_gl_reset_projection(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);

    metadot_gl_state_t *state = metadot_gl_get_active_state(ctx);

    const metadot_gl_m4_t matrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    memcpy(state->projection, matrix, sizeof(metadot_gl_m4_t));
}

void metadot_gl_set_viewport(metadot_gl_ctx_t *ctx, int32_t x, int32_t y, int32_t w, int32_t h) {
    METADOT_GL_ASSERT(ctx);

    metadot_gl_state_t *state = metadot_gl_get_active_state(ctx);
    state->viewport = (metadot_gl_viewport_t){x, y, w, h};
}

void metadot_gl_reset_viewport(metadot_gl_ctx_t *ctx)  // TODO: get GL viewport?
{
    METADOT_GL_ASSERT(ctx);

    metadot_gl_state_t *state = metadot_gl_get_active_state(ctx);
    state->viewport = (metadot_gl_viewport_t){0, 0, static_cast<int32_t>(ctx->w), static_cast<int32_t>(ctx->h)};
}

void metadot_gl_set_line_width(metadot_gl_ctx_t *ctx, float width) {
    METADOT_GL_ASSERT(ctx);

    metadot_gl_state_t *state = metadot_gl_get_active_state(ctx);
    state->line_width = width;
}

void metadot_gl_reset_line_width(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);

    metadot_gl_state_t *state = metadot_gl_get_active_state(ctx);
    state->line_width = 1.0f;
}

void metadot_gl_reset_state(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);
    metadot_gl_reset_blend_mode(ctx);
    metadot_gl_reset_transform(ctx);
    metadot_gl_reset_projection(ctx);
    metadot_gl_reset_viewport(ctx);
    metadot_gl_reset_line_width(ctx);
}

void metadot_gl_push_state(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);

    // TODO: Make into growable array

    metadot_gl_state_stack_t *stack = metadot_gl_get_active_stack(ctx);
    METADOT_GL_ASSERT(stack->size < METADOT_GL_MAX_STATES);

    stack->array[stack->size] = stack->state;
    stack->size++;
}

void metadot_gl_pop_state(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);

    metadot_gl_state_stack_t *stack = metadot_gl_get_active_stack(ctx);

    METADOT_GL_ASSERT(stack->size > 0);

    stack->state = stack->array[stack->size];
    stack->size--;
}

void metadot_gl_clear_stack(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);

    metadot_gl_state_stack_t *stack = metadot_gl_get_active_stack(ctx);
    stack->size = 0;
}

void metadot_gl_set_bool(metadot_gl_shader_t *shader, const char *name, bool value) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniform1i(uniform->location, value));
}

void metadot_gl_set_1i(metadot_gl_shader_t *shader, const char *name, int32_t a) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniform1i(uniform->location, a));
}

void metadot_gl_set_2i(metadot_gl_shader_t *shader, const char *name, int32_t a, int32_t b) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniform2i(uniform->location, a, b));
}

void metadot_gl_set_3i(metadot_gl_shader_t *shader, const char *name, int32_t a, int32_t b, int32_t c) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniform3i(uniform->location, a, b, c));
}

void metadot_gl_set_4i(metadot_gl_shader_t *shader, const char *name, int32_t a, int32_t b, int32_t c, int32_t d) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniform4i(uniform->location, a, b, c, d));
}

void metadot_gl_set_v2i(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v2i_t vec) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_set_2i(shader, name, vec[0], vec[1]);
}

void metadot_gl_set_v3i(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v3i_t vec) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_set_3i(shader, name, vec[0], vec[1], vec[2]);
}

void metadot_gl_set_v4i(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v4i_t vec) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_set_4i(shader, name, vec[0], vec[1], vec[2], vec[3]);
}

void metadot_gl_set_1f(metadot_gl_shader_t *shader, const char *name, float x) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniform1f(uniform->location, x));
}

void metadot_gl_set_2f(metadot_gl_shader_t *shader, const char *name, float x, float y) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniform2f(uniform->location, x, y));
}

void metadot_gl_set_3f(metadot_gl_shader_t *shader, const char *name, float x, float y, float z) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniform3f(uniform->location, x, y, z));
}

void metadot_gl_set_4f(metadot_gl_shader_t *shader, const char *name, float x, float y, float z, float w) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniform4f(uniform->location, x, y, z, w));
}

void metadot_gl_set_v2f(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v2f_t vec) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);
    METADOT_GL_ASSERT(vec);

    metadot_gl_set_2f(shader, name, vec[0], vec[1]);
}

void metadot_gl_set_v3f(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v3f_t vec) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);
    METADOT_GL_ASSERT(vec);

    metadot_gl_set_3f(shader, name, vec[0], vec[1], vec[2]);
}

void metadot_gl_set_v4f(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v4f_t vec) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);
    METADOT_GL_ASSERT(vec);

    metadot_gl_set_4f(shader, name, vec[0], vec[1], vec[2], vec[3]);
}

void metadot_gl_set_a1f(metadot_gl_shader_t *shader, const char *name, const float *values, metadot_gl_size_t count) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);
    METADOT_GL_ASSERT(values);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniform1fv(uniform->location, count, values));
}

void metadot_gl_set_a2f(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v2f_t *vec, metadot_gl_size_t count) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);
    METADOT_GL_ASSERT(vec);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    float values[2 * count];

    metadot_gl_size_t i, j;

    for (i = 0, j = 0; j < count; i += 2, j++) {
        values[i] = vec[j][0];
        values[i + 1] = vec[j][1];
    }

    METADOT_GL_CHECK(glUniform2fv(uniform->location, count, values));
}

void metadot_gl_set_a3f(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v3f_t *vec, metadot_gl_size_t count) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);
    METADOT_GL_ASSERT(vec);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    float values[3 * count];

    metadot_gl_size_t i, j;

    for (i = 0, j = 0; j < count; i += 3, j++) {
        values[i] = vec[j][0];
        values[i + 1] = vec[j][1];
        values[i + 2] = vec[j][2];
    }

    METADOT_GL_CHECK(glUniform3fv(uniform->location, count, values));
}

void metadot_gl_set_a4f(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v4f_t *vec, metadot_gl_size_t count) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);
    METADOT_GL_ASSERT(vec);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    float values[4 * count];

    metadot_gl_size_t i, j;

    for (i = 0, j = 0; j < count; i += 4, j++) {
        values[i] = vec[j][0];
        values[i + 1] = vec[j][1];
        values[i + 2] = vec[j][2];
        values[i + 3] = vec[j][3];
    }

    METADOT_GL_CHECK(glUniform4fv(uniform->location, count, values));
}

void metadot_gl_set_m2(metadot_gl_shader_t *shader, const char *name, const metadot_gl_m2_t matrix) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);
    METADOT_GL_ASSERT(matrix);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniformMatrix2fv(uniform->location, uniform->size, shader->ctx->transpose, (float *)matrix));
}

void metadot_gl_set_m3(metadot_gl_shader_t *shader, const char *name, const metadot_gl_m3_t matrix) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);
    METADOT_GL_ASSERT(matrix);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniformMatrix3fv(uniform->location, uniform->size, shader->ctx->transpose, (float *)matrix));
}

void metadot_gl_set_m4(metadot_gl_shader_t *shader, const char *name, const metadot_gl_m4_t matrix) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);
    METADOT_GL_ASSERT(matrix);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniformMatrix4fv(uniform->location, uniform->size, shader->ctx->transpose, (float *)matrix));
}

void metadot_gl_set_s2d(metadot_gl_shader_t *shader, const char *name, int32_t value) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name);

    metadot_gl_bind_shader(shader->ctx, shader);
    const metadot_gl_uniform_t *uniform = metadot_gl_find_uniform(shader, name);

    if (!uniform) return;

    METADOT_GL_CHECK(glUniform1i(uniform->location, value));
}

/*=============================================================================
 * Internal API implementation
 *============================================================================*/

static void metadot_gl_set_error(metadot_gl_ctx_t *ctx, metadot_gl_error_t code) {
    METADOT_GL_ASSERT(ctx);
    ctx->error_code = code;
}

static const char *metadot_gl_get_default_vert_shader() {
    metadot_gl_version_t ver = metadot_gl_get_version();

    switch (ver) {
        case METADOT_GL_GL3:
            return METADOT_GL_GL_VERT_SRC;

        case METADOT_GL_GLES3:
            return METADOT_GL_GLES_VERT_SRC;

        default:
            return NULL;
    }
}

static const char *metadot_gl_get_default_frag_shader() {
    metadot_gl_version_t ver = metadot_gl_get_version();

    switch (ver) {
        case METADOT_GL_GL3:
            return METADOT_GL_GL_FRAG_SRC;

        case METADOT_GL_GLES3:
            return METADOT_GL_GLES_FRAG_SRC;

        default:
            return NULL;
    }
}

static void metadot_gl_reset_last_state(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);

    metadot_gl_state_t *last = &ctx->last_state;

    memset(last, 0, sizeof(metadot_gl_state_t));

    metadot_gl_blend_mode_t mode = {METADOT_GL_FACTOR_COUNT, METADOT_GL_FACTOR_COUNT, METADOT_GL_EQ_COUNT, METADOT_GL_FACTOR_COUNT, METADOT_GL_FACTOR_COUNT, METADOT_GL_EQ_COUNT};

    last->blend_mode = mode;
}

static metadot_gl_state_stack_t *metadot_gl_get_active_stack(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);

    metadot_gl_state_stack_t *stack = &ctx->stack;

    if (ctx->target) stack = &ctx->target_stack;

    return stack;
}

static metadot_gl_state_t *metadot_gl_get_active_state(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);

    return &metadot_gl_get_active_stack(ctx)->state;
}

static void metadot_gl_apply_blend(metadot_gl_ctx_t *ctx, const metadot_gl_blend_mode_t *mode) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(mode);

    if (metadot_gl_mem_equal(mode, &ctx->last_state.blend_mode, sizeof(metadot_gl_blend_mode_t))) return;

    METADOT_GL_CHECK(glBlendFuncSeparate(metadot_gl_blend_factor_map[mode->color_src], metadot_gl_blend_factor_map[mode->color_dst], metadot_gl_blend_factor_map[mode->alpha_src],
                                         metadot_gl_blend_factor_map[mode->alpha_dst]));

    METADOT_GL_CHECK(glBlendEquationSeparate(metadot_gl_blend_eq_map[mode->color_eq], metadot_gl_blend_eq_map[mode->alpha_eq]));
}

static void metadot_gl_apply_transform(metadot_gl_ctx_t *ctx, const metadot_gl_m4_t matrix) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(matrix);

    if (metadot_gl_mem_equal(matrix, ctx->last_state.transform, sizeof(metadot_gl_m4_t))) return;

    metadot_gl_set_m4(ctx->shader, "u_transform", matrix);
}

static void metadot_gl_apply_projection(metadot_gl_ctx_t *ctx, const metadot_gl_m4_t matrix) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(matrix);

    if (metadot_gl_mem_equal(matrix, &ctx->last_state.projection, sizeof(metadot_gl_m4_t))) return;

    metadot_gl_set_m4(ctx->shader, "u_projection", matrix);
}

static void metadot_gl_apply_viewport(metadot_gl_ctx_t *ctx, const metadot_gl_viewport_t *viewport) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(viewport);

    if (viewport->w <= 0 && viewport->h <= 0) return;

    if (metadot_gl_mem_equal(viewport, &ctx->last_state.viewport, sizeof(metadot_gl_viewport_t))) return;

    METADOT_GL_CHECK(glViewport(viewport->x, viewport->y, viewport->w, viewport->h));
}

static void metadot_gl_apply_line_width(metadot_gl_ctx_t *ctx, float line_width) {
    METADOT_GL_ASSERT(ctx);

    if (line_width == ctx->last_state.line_width) return;

    METADOT_GL_CHECK(glLineWidth(line_width));
}

static void metadot_gl_before_draw(metadot_gl_ctx_t *ctx, metadot_gl_texture_t *texture, metadot_gl_shader_t *shader) {
    METADOT_GL_ASSERT(ctx);
    METADOT_GL_ASSERT(shader);

    metadot_gl_bind_texture(ctx, texture);
    metadot_gl_bind_shader(ctx, shader);

    metadot_gl_state_t *state = metadot_gl_get_active_state(ctx);

    metadot_gl_apply_viewport(ctx, &state->viewport);
    metadot_gl_apply_blend(ctx, &state->blend_mode);
    metadot_gl_apply_transform(ctx, state->transform);
    metadot_gl_apply_projection(ctx, state->projection);
    metadot_gl_apply_line_width(ctx, state->line_width);

    glEnable(GL_BLEND);

    if (ctx->depth)
        METADOT_GL_CHECK(glEnable(GL_DEPTH_TEST));
    else
        METADOT_GL_CHECK(glDisable(GL_DEPTH_TEST));

    if (ctx->srgb)
        METADOT_GL_CHECK(glEnable(GL_FRAMEBUFFER_SRGB));
    else
        METADOT_GL_CHECK(glDisable(GL_FRAMEBUFFER_SRGB));
}

static void metadot_gl_after_draw(metadot_gl_ctx_t *ctx) {
    METADOT_GL_ASSERT(ctx);

    ctx->last_state = *metadot_gl_get_active_state(ctx);
}

static int metadot_gl_load_uniforms(metadot_gl_shader_t *shader) {
    METADOT_GL_ASSERT(shader);

    // Get number of active uniforms
    GLint uniform_count;
    glGetProgramiv(shader->program, GL_ACTIVE_UNIFORMS, &uniform_count);
    shader->uniform_count = uniform_count;

    // Validate number of uniforms
    METADOT_GL_ASSERT(uniform_count < METADOT_GL_MAX_UNIFORMS);

    if (uniform_count >= METADOT_GL_MAX_UNIFORMS) {
        metadot_gl_set_error(shader->ctx, METADOT_GL_INVALID_UNIFORM_COUNT);
        return -1;
    }

    // Loop through active uniforms and add them to the uniform array
    for (GLint i = 0; i < uniform_count; i++) {
        metadot_gl_uniform_t uniform;
        GLsizei name_length;

        // Uniform index
        GLint index = i;

        // Get uniform information
        METADOT_GL_CHECK(glGetActiveUniform(shader->program, index,
                                            METADOT_GL_UNIFORM_NAME_LENGTH,  // Max name length
                                            &name_length, &uniform.size, &uniform.type, uniform.name));

        // Validate name length
        METADOT_GL_ASSERT(name_length <= METADOT_GL_UNIFORM_NAME_LENGTH);

        if (name_length > METADOT_GL_UNIFORM_NAME_LENGTH) {
            metadot_gl_set_error(shader->ctx, METADOT_GL_INVALID_UNIFORM_NAME);
            return -1;
        }

        // Get uniform location
        uniform.location = glGetUniformLocation(shader->program, uniform.name);

        // Hash name for fast lookups
        uniform.hash = metadot_gl_hash_str(uniform.name);

        // Store uniform in the array
        shader->uniforms[i] = uniform;
    }

    return 0;
}

static const metadot_gl_uniform_t *metadot_gl_find_uniform(const metadot_gl_shader_t *shader, const char *name) {
    METADOT_GL_ASSERT(shader);
    METADOT_GL_ASSERT(name && strlen(name) > 0);

    metadot_gl_size_t uniform_count = shader->uniform_count;
    const metadot_gl_uniform_t *uniforms = shader->uniforms;

    uint64_t hash = metadot_gl_hash_str(name);

    for (metadot_gl_size_t i = 0; i < uniform_count; i++) {
        const metadot_gl_uniform_t *uniform = &uniforms[i];

        if (uniform->hash == hash && metadot_gl_str_equal(name, uniform->name)) {
            return uniform;
        }
    }

    return NULL;
}

static void metadot_gl_bind_attributes() {
    // Position
    METADOT_GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(metadot_gl_vertex_t), (GLvoid *)offsetof(metadot_gl_vertex_t, pos)));

    METADOT_GL_CHECK(glEnableVertexAttribArray(0));

    // Color
    METADOT_GL_CHECK(glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(metadot_gl_vertex_t), (GLvoid *)offsetof(metadot_gl_vertex_t, color)));

    METADOT_GL_CHECK(glEnableVertexAttribArray(1));

    // UV
    METADOT_GL_CHECK(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(metadot_gl_vertex_t), (GLvoid *)offsetof(metadot_gl_vertex_t, uv)));

    METADOT_GL_CHECK(glEnableVertexAttribArray(2));
}

static void metadot_gl_log_error(const char *file, unsigned line, const char *expr) {
    METADOT_GL_ASSERT(file);

    metadot_gl_error_t code = metadot_gl_map_error(glGetError());

    if (METADOT_GL_NO_ERROR == code) return;

    METADOT_GL_LOG("GL error: file: %s, line: %u, msg: \"%s\", expr: \"%s\"", file, line, metadot_gl_error_msg_map[code], expr);
}

static metadot_gl_error_t metadot_gl_map_error(GLenum id) {
    switch (id) {
        case GL_NO_ERROR:
            return METADOT_GL_NO_ERROR;
        case GL_INVALID_ENUM:
            return METADOT_GL_INVALID_ENUM;
        case GL_INVALID_VALUE:
            return METADOT_GL_INVALID_VALUE;
        case GL_INVALID_OPERATION:
            return METADOT_GL_INVALID_OPERATION;
        case GL_OUT_OF_MEMORY:
            return METADOT_GL_OUT_OF_MEMORY;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            return METADOT_GL_INVALID_FRAMEBUFFER_OPERATION;
    }

    return METADOT_GL_UNKNOWN_ERROR;
}
static bool metadot_gl_str_equal(const char *str1, const char *str2) { return (0 == strcmp(str1, str2)); }

static bool metadot_gl_mem_equal(const void *ptr1, const void *ptr2, size_t size) { return (0 == memcmp(ptr1, ptr2, size)); }

// FNV-1a
static metadot_gl_hash_t metadot_gl_hash_str(const char *str) {
    METADOT_GL_ASSERT(str);

    metadot_gl_hash_t hash = METADOT_GL_OFFSET_BASIS;

    while ('\0' != str[0]) {
        hash ^= (metadot_gl_hash_t)str[0];
        hash *= METADOT_GL_PRIME;
        str++;
    }

    return hash;
}

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif  // __GNUC__
