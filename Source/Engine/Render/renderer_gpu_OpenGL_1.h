#ifndef _SDL_METAENGINE_Render_OPENGL_1_H__
#define _SDL_METAENGINE_Render_OPENGL_1_H__

#include "renderer_gpu.h"

#if !defined(SDL_METAENGINE_Render_DISABLE_OPENGL) && !defined(SDL_METAENGINE_Render_DISABLE_OPENGL_1)

    // Hacks to fix compile errors due to polluted namespace
    #ifdef _WIN32
    #define _WINUSER_H
    #define _WINGDI_H
    #endif

#include "glad/glad.h"
	
	#if defined(GL_EXT_bgr) && !defined(GL_BGR)
		#define GL_BGR GL_BGR_EXT
	#endif
	#if defined(GL_EXT_bgra) && !defined(GL_BGRA)
		#define GL_BGRA GL_BGRA_EXT
	#endif
	#if defined(GL_EXT_abgr) && !defined(GL_ABGR)
		#define GL_ABGR GL_ABGR_EXT
	#endif

	
#endif



#define METAENGINE_Render_CONTEXT_DATA ContextData_OpenGL_1
#define METAENGINE_Render_IMAGE_DATA ImageData_OpenGL_1
#define METAENGINE_Render_TARGET_DATA TargetData_OpenGL_1




#define METAENGINE_Render_DEFAULT_TEXTURED_VERTEX_SHADER_SOURCE \
"#version 110\n\
\
attribute vec2 gpu_Vertex;\n\
attribute vec2 gpu_TexCoord;\n\
attribute vec4 gpu_Color;\n\
uniform mat4 gpu_ModelViewProjectionMatrix;\n\
\
varying vec4 color;\n\
varying vec2 texCoord;\n\
\
void main(void)\n\
{\n\
	color = gpu_Color;\n\
	texCoord = vec2(gpu_TexCoord);\n\
	gl_Position = gpu_ModelViewProjectionMatrix * vec4(gpu_Vertex, 0.0, 1.0);\n\
}"

// Tier 3 uses shader attributes to send position, texcoord, and color data for each vertex.
#define METAENGINE_Render_DEFAULT_UNTEXTURED_VERTEX_SHADER_SOURCE \
"#version 110\n\
\
attribute vec2 gpu_Vertex;\n\
attribute vec4 gpu_Color;\n\
uniform mat4 gpu_ModelViewProjectionMatrix;\n\
\
varying vec4 color;\n\
\
void main(void)\n\
{\n\
	color = gpu_Color;\n\
	gl_Position = gpu_ModelViewProjectionMatrix * vec4(gpu_Vertex, 0.0, 1.0);\n\
}"


#define METAENGINE_Render_DEFAULT_TEXTURED_FRAGMENT_SHADER_SOURCE \
"#version 110\n\
\
varying vec4 color;\n\
varying vec2 texCoord;\n\
\
uniform sampler2D tex;\n\
\
void main(void)\n\
{\n\
    gl_FragColor = texture2D(tex, texCoord) * color;\n\
}"

#define METAENGINE_Render_DEFAULT_UNTEXTURED_FRAGMENT_SHADER_SOURCE \
"#version 110\n\
\
varying vec4 color;\n\
\
void main(void)\n\
{\n\
    gl_FragColor = color;\n\
}"





typedef struct ContextData_OpenGL_1
{
	SDL_Color last_color;
	METAENGINE_Render_bool last_use_texturing;
	unsigned int last_shape;
	METAENGINE_Render_bool last_use_blending;
	METAENGINE_Render_BlendMode last_blend_mode;
	METAENGINE_Render_Rect last_viewport;
	METAENGINE_Render_Camera last_camera;
	METAENGINE_Render_bool last_camera_inverted;
	
	METAENGINE_Render_bool last_depth_test;
	METAENGINE_Render_bool last_depth_write;
	METAENGINE_Render_ComparisonEnum last_depth_function;
	
	METAENGINE_Render_Image* last_image;
	float* blit_buffer;  // Holds sets of 4 vertices and 4 tex coords interleaved (e.g. [x0, y0, z0, s0, t0, ...]).
	unsigned short blit_buffer_num_vertices;
	unsigned short blit_buffer_max_num_vertices;
	unsigned short* index_buffer;  // Indexes into the blit buffer so we can use 4 vertices for every 2 triangles (1 quad)
	unsigned int index_buffer_num_vertices;
	unsigned int index_buffer_max_num_vertices;
	
    
    unsigned int blit_VBO[2];  // For double-buffering
    unsigned int blit_IBO;
    METAENGINE_Render_bool blit_VBO_flop;
    
	METAENGINE_Render_AttributeSource shader_attributes[16];
	unsigned int attribute_VBO[16];
} ContextData_OpenGL_1;

typedef struct ImageData_OpenGL_1
{
    int refcount;
    METAENGINE_Render_bool owns_handle;
	Uint32 handle;
	Uint32 format;
} ImageData_OpenGL_1;

typedef struct TargetData_OpenGL_1
{
    int refcount;
	Uint32 handle;
	Uint32 format;
} TargetData_OpenGL_1;



#endif
