#ifndef _R_OPENGL_3_H__
#define _R_OPENGL_3_H__

#include "engine/sdl_wrapper.h"
#include "renderer_gpu.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Hacks to fix compile errors due to polluted namespace
#ifdef _WIN32
#define _WINUSER_H
#define _WINGDI_H
#endif

#include "libs/glad/glad.h"

#if defined(GL_EXT_bgr) && !defined(GL_BGR)
#define GL_BGR GL_BGR_EXT
#endif
#if defined(GL_EXT_bgra) && !defined(GL_BGRA)
#define GL_BGRA GL_BGRA_EXT
#endif
#if defined(GL_EXT_abgr) && !defined(GL_ABGR)
#define GL_ABGR GL_ABGR_EXT
#endif

#define ToSDLColor(_c)                                                                             \
    (SDL_Color) { _c.r, _c.g, _c.b, _c.a }

#define ToEngineColor(_c)                                                                          \
    (METAENGINE_Color) { _c.r, _c.g, _c.b, _c.a }

#define R_CONTEXT_DATA ContextData_OpenGL_3
#define R_IMAGE_DATA ImageData_OpenGL_3
#define R_TARGET_DATA TargetData_OpenGL_3

#define R_DEFAULT_TEXTURED_VERTEX_SHADER_SOURCE                                                    \
    "#version 400\n\
\
in vec2 gpu_Vertex;\n\
in vec2 gpu_TexCoord;\n\
in vec4 gpu_Color;\n\
uniform mat4 gpu_ModelViewProjectionMatrix;\n\
\
out vec4 color;\n\
out vec2 texCoord;\n\
\
void main(void)\n\
{\n\
	color = gpu_Color;\n\
	texCoord = vec2(gpu_TexCoord);\n\
	gl_Position = gpu_ModelViewProjectionMatrix * vec4(gpu_Vertex, 0.0, 1.0);\n\
}"

#define R_DEFAULT_UNTEXTURED_VERTEX_SHADER_SOURCE                                                  \
    "#version 400\n\
\
in vec2 gpu_Vertex;\n\
in vec4 gpu_Color;\n\
uniform mat4 gpu_ModelViewProjectionMatrix;\n\
\
out vec4 color;\n\
\
void main(void)\n\
{\n\
	color = gpu_Color;\n\
	gl_Position = gpu_ModelViewProjectionMatrix * vec4(gpu_Vertex, 0.0, 1.0);\n\
}"

#define R_DEFAULT_TEXTURED_FRAGMENT_SHADER_SOURCE                                                  \
    "#version 400\n\
\
in vec4 color;\n\
in vec2 texCoord;\n\
\
uniform sampler2D tex;\n\
\
out vec4 fragColor;\n\
\
void main(void)\n\
{\n\
    fragColor = texture(tex, texCoord) * color;\n\
}"

#define R_DEFAULT_UNTEXTURED_FRAGMENT_SHADER_SOURCE                                                \
    "#version 400\n\
\
in vec4 color;\n\
\
out vec4 fragColor;\n\
\
void main(void)\n\
{\n\
    fragColor = color;\n\
}"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct ContextData_OpenGL_3
{
    METAENGINE_Color last_color;
    bool last_use_texturing;
    unsigned int last_shape;
    bool last_use_blending;
    R_BlendMode last_blend_mode;
    R_Rect last_viewport;
    R_Camera last_camera;
    bool last_camera_inverted;

    bool last_depth_test;
    bool last_depth_write;
    R_ComparisonEnum last_depth_function;

    R_Image *last_image;
    float *blit_buffer;// Holds sets of 4 vertices, each with interleaved position, tex coords, and colors (e.g. [x0, y0, z0, s0, t0, r0, g0, b0, a0, ...]).
    unsigned short blit_buffer_num_vertices;
    unsigned short blit_buffer_max_num_vertices;
    unsigned short *
            index_buffer;// Indexes into the blit buffer so we can use 4 vertices for every 2 triangles (1 quad)
    unsigned int index_buffer_num_vertices;
    unsigned int index_buffer_max_num_vertices;

    // Tier 3 rendering
    unsigned int blit_VAO;
    unsigned int blit_VBO[2];// For double-buffering
    unsigned int blit_IBO;
    bool blit_VBO_flop;

    R_AttributeSource shader_attributes[16];
    unsigned int attribute_VBO[16];
} ContextData_OpenGL_3;

typedef struct ImageData_OpenGL_3
{
    int refcount;
    bool owns_handle;
    U32 handle;
    U32 format;
} ImageData_OpenGL_3;

typedef struct TargetData_OpenGL_3
{
    int refcount;
    U32 handle;
    U32 format;
} TargetData_OpenGL_3;

#ifdef __cplusplus
}
#endif

#endif
