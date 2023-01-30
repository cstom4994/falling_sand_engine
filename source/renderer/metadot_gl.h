

// - METADOT_GL_UNIFORM_NAME_LENGTH (default: 32)
// - METADOT_GL_MAX_UNIFORMS (default: 32)
// - METADOT_GL_MAX_STATES (default: 32)

// Todo:
// -----
// - MSAA flag for examples
// - Better version handling
// - Shader examples
// - Scissor
// - Indexed buffers
// - Multiple texture units

#ifndef METADOT_GL_H
#define METADOT_GL_H

#include <stdarg.h>   // va_list, va_start, va_end
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdint.h>   // uint32_t, int32_t, uint64_t
#include <stdio.h>    // printf, vprintf

#include "libs/glad/glad.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OpenGL compatible size type
 */
typedef uint32_t metadot_gl_size_t;

/**
 * @brief Runtime error codes
 */
typedef enum {
    METADOT_GL_NO_ERROR,                       //!< No error
    METADOT_GL_INVALID_ENUM,                   //!< Invalid enumeration value
    METADOT_GL_INVALID_VALUE,                  //!< Invalid value
    METADOT_GL_INVALID_OPERATION,              //!< Invalid operation
    METADOT_GL_OUT_OF_MEMORY,                  //!< Out of memory
    METADOT_GL_INVALID_FRAMEBUFFER_OPERATION,  //!< Invalid framebuffer operation
    METADOT_GL_FRAMEBUFFER_INCOMPLETE,         //!< Framebuffer is incomplete
    METADOT_GL_SHADER_COMPILATION_ERROR,       //!< Shader compilation error
    METADOT_GL_SHADER_LINKING_ERROR,           //!< Shader linking error
    METADOT_GL_INVALID_TEXTURE_SIZE,           //!< Invalid texture dimensions
    METADOT_GL_INVALID_TEXTURE_FORMAT,         //!< Invalid texture format
    METADOT_GL_INVALID_ATTRIBUTE_COUNT,        //!< Invalid number of attributes
    METADOT_GL_INVALID_UNIFORM_COUNT,          //!< Invalid number of uniforms
    METADOT_GL_INVALID_UNIFORM_NAME,           //!< Invalid uniform name
    METADOT_GL_UNKNOWN_ERROR,                  //!< Unknown error
    METADOT_GL_ERROR_COUNT
} metadot_gl_error_t;

/**
 * @brief OpenGL versions used by the library
 */
typedef enum { METADOT_GL_GL3, METADOT_GL_GLES3, METADOT_GL_VERSION_UNSUPPORTED } metadot_gl_version_t;

/**
 * @brief Blend factors
 */
typedef enum {
    METADOT_GL_ZERO,                 //!< (0, 0, 0, 0)
    METADOT_GL_ONE,                  //!< (1, 1, 1, 1)
    METADOT_GL_SRC_COLOR,            //!< (src.r, src.g, src.b, src.a)
    METADOT_GL_ONE_MINUS_SRC_COLOR,  //!< (1, 1, 1, 1) - (src.r, src.g, src.b, src.a)
    METADOT_GL_DST_COLOR,            //!< (dst.r, dst.g, dst.b, dst.a)
    METADOT_GL_ONE_MINUS_DST_COLOR,  //!< (1, 1, 1, 1) - (dst.r, dst.g, dst.b, dst.a)
    METADOT_GL_SRC_ALPHA,            //!< (src.a, src.a, src.a, src.a)
    METADOT_GL_ONE_MINUS_SRC_ALPHA,  //!< (1, 1, 1, 1) - (src.a, src.a, src.a, src.a)
    METADOT_GL_DST_ALPHA,            //!< (dst.a, dst.a, dst.a, dst.a)
    METADOT_GL_ONE_MINUS_DST_ALPHA,  //!< (1, 1, 1, 1) - (dst.a, dst.a, dst.a, dst.a)
    METADOT_GL_FACTOR_COUNT
} metadot_gl_blend_factor_t;

/**
 * @brief Blend equations
 */
typedef enum {
    METADOT_GL_FUNC_ADD,               //!< result = src * src_factor + dst * dst_factor
    METADOT_GL_FUNC_SUBTRACT,          //!< result = src * src_factor - dst * dst_factor
    METADOT_GL_FUNC_REVERSE_SUBTRACT,  //!< result = dst * dst_factor - src * src_factor
    METADOT_GL_MIN,                    //!< result = min(src, dst)
    METADOT_GL_MAX,                    //!< result = max(src, dst)
    METADOT_GL_EQ_COUNT
} metadot_gl_blend_eq_t;

/**
 * @brief Blend mode
 *
 * Completely describes a blend operation.
 */
typedef struct {
    metadot_gl_blend_factor_t color_src;  //!< Color source blending factor
    metadot_gl_blend_factor_t color_dst;  //!< Color dsestination blending factor
    metadot_gl_blend_eq_t color_eq;       //!< Equation for blending colors
    metadot_gl_blend_factor_t alpha_src;  //!< Alpha source blending factor
    metadot_gl_blend_factor_t alpha_dst;  //!< Alpha destination blending factor
    metadot_gl_blend_eq_t alpha_eq;       //!< Equation for blending alpha values
} metadot_gl_blend_mode_t;

/**
 * @brief Drawing primitives
 */
typedef enum {
    METADOT_GL_POINTS,          //!< Array of points
    METADOT_GL_LINES,           //!< Each adjacent pair of points forms a line
    METADOT_GL_LINE_STRIP,      //!< Array of points where every pair forms a lines
    METADOT_GL_TRIANGLES,       //!< Each adjacent triple forms an individual triangle
    METADOT_GL_TRIANGLE_STRIP,  //!< Array of points where every triple forms a triangle
} metadot_gl_primitive_t;

/**
 * @brief A vertex describes a point and the data associated with it (color and
 * texture coordinates)
 */
typedef struct {
    float pos[3];
    float color[4];
    float uv[2];
} metadot_gl_vertex_t;

/**
 * @brief 2D floating point vector
 */
typedef float metadot_gl_v2f_t[2];

/**
 * @brief 3D floating point vector
 */
typedef float metadot_gl_v3f_t[3];

/**
 * @brief 4D floating point vector
 */
typedef float metadot_gl_v4f_t[4];

/**
 * @brief 2D integer vector
 */
typedef int32_t metadot_gl_v2i_t[2];

/**
 * @brief 3D integer vector
 */
typedef int32_t metadot_gl_v3i_t[3];

/**
 * @brief 4D integer vector
 */
typedef int32_t metadot_gl_v4i_t[4];

/**
 * @brief 2x2 floating point matrix
 */
typedef float metadot_gl_m2_t[4];

/**
 * @brief 3x3 floating point matrix
 */
typedef float metadot_gl_m3_t[9];

/**
 * @brief 4x4 floating point matrix
 */
typedef float metadot_gl_m4_t[16];

/**
 * @brief Contains core data/state for an instance of the renderer
 */
typedef struct metadot_gl_ctx_t metadot_gl_ctx_t;

/**
 * @brief Contains shader data/state
 */
typedef struct metadot_gl_shader_t metadot_gl_shader_t;

/**
 * @brief Contains texture data/state
 */
typedef struct metadot_gl_texture_t metadot_gl_texture_t;

/**
 * @brief Contains vertex buffer data/state
 */
typedef struct metadot_gl_buffer_t metadot_gl_buffer_t;

/**
 * @brief Defines an OpenGL (GLAD) function loader
 *
 * @param name The name of the function to load
 */
typedef void *(*metadot_gl_loader_fn)(const char *name);

/**
 * @brief Loads all supported OpenGL functions via GLAD
 *
 * IMPORTANT: A valid OpenGL context must exist for this function to succeed.
 * This function must be called before any other PGL functions.
 *
 * @param loader_fp Function loader (can be NULL except for GLES contexts)
 *
 * @returns -1 on failure and 0 otherwise
 */
int metadot_gl_global_init(metadot_gl_loader_fn loader_fp);

/**
 * @brief Returns the current error code
 */
metadot_gl_error_t metadot_gl_get_error(metadot_gl_ctx_t *ctx);

/**
 * @brief Returns the string associated with the specified error code
 *
 * @param code The error code to query
 *
 * @returns The string representation of the error code
 */
const char *metadot_gl_get_error_str(metadot_gl_error_t code);

/**
 * @brief Returns the current OpenGL version in use by the library.
 *
 * Currently the library does not use OpenGL features outside of 3.0 and ES 3.0.
 * This unlikely to change unless there is an incompatibility with more recent
 * versions or the library is ported to OpenGL 2.1.
 *
 * @returns METADOT_GL_VERSION_UNSUPPORTED if the version of OpenGL is not supported
 */
metadot_gl_version_t metadot_gl_get_version();

/**
 * @brief Prints system info
 */
void metadot_gl_print_info();

/**
 * @brief Creates an instance of the renderer
 *
 * @param w       The drawable width of the window in pixels
 * @param h       The drawable height of the window in pixels
 * @param depth   Depth test is enabled if true
 * @param samples The number of MSAA (anti-alising) samples (disabled if 0)
 * @param srgb    Enables support for the sRGB colorspace
 * @param mem_ctx User data provided to custom allocators
 *
 * @returns The context pointer or \em NULL on error
 */
metadot_gl_ctx_t *metadot_gl_create_context(uint32_t w, uint32_t h, bool depth, uint32_t samples, bool srgb, void *mem_ctx);

/**
 * @brief Destroys a renderer context, releasing it's resources
 *
 * @param ctx A pointer to the context
 */
void metadot_gl_destroy_context(metadot_gl_ctx_t *ctx);

/**
 * @brief Resizes the drawable dimensions
 *
 * @param ctx       A pointer to the context
 * @param w         The drawable width of the window in pixels
 * @param h         The drawable height of the window in pixels
 * @pawram reset_vp Resets the viewport if true
 */
void metadot_gl_resize(metadot_gl_ctx_t *ctx, uint32_t w, uint32_t h, bool reset_vp);

/**
 * @brief Creates a shader program
 *
 * If `vert_src` and `frag_src` are both `NULL`, then the shader is compiled
 * from the default vertex and fragment sources. If either `vert_src` or
 * `frag_src` are `NULL`, then the shader is compiled from the (respective)
 * default vertex or fragment source, together with the non-`NULL` argument.
 *
 * @param ctx      The relevant context
 * @param vert_src Vertex shader source
 * @param frag_src Fragment shader source

 * @returns Pointer to the shader program, or \em NULL on error
 */
metadot_gl_shader_t *metadot_gl_create_shader(metadot_gl_ctx_t *ctx, const char *vert_src, const char *frag_src);

/**
 * @brief Destroys a shader program
 *
 * @param shader Pointer to the shader program
 */
void metadot_gl_destroy_shader(metadot_gl_shader_t *shader);

/**
 * @brief Activates a shader program for rendering
 *
 * This function sets the context's currently active shader. If `shader` is
 * `NULL` the current shader is deactivated.
 *
 * @param ctx    The relevant context
 * @param shader The shader program to activate, or `NULL` to deactivate
 */
void metadot_gl_bind_shader(metadot_gl_ctx_t *ctx, metadot_gl_shader_t *shader);

/**
 * @brief Return the implementation specific shader ID
 *
 * @param shader The target shader
 *
 * @returns An unsigned 64-bit ID value
 */
uint64_t metadot_gl_get_shader_id(const metadot_gl_shader_t *shader);

/**
 * @brief Creates an empty texture
 *
 * @param ctx    The relevant context
 * @param target If true, the texture can be used as a render target
 * @param w      The texture's width
 * @param h      The texture's height
 * @param srgb   True if the internal format is sRGB
 * @param smooth High (true) or low (false) quality filtering
 * @param repeat Repeats or clamps when uv coordinates exceed 1.0
 */
metadot_gl_texture_t *metadot_gl_create_texture(metadot_gl_ctx_t *ctx, bool target, int32_t w, int32_t h, bool srgb, bool smooth, bool repeat);

/**
 * @brief Creates a texture from a bitmap
 *
 * @param ctx    The relevant context
 * @param w      The texture's width
 * @param h      The texture's height
 * @param srgb   True if the internal format is sRGB
 * @param smooth High (true) or low (false) quality filtering
 * @param repeat Repeats or clamps when uv coordinates exceed 1.0
 * @param bitmap Pixel data in RGBA format
 */
metadot_gl_texture_t *metadot_gl_texture_from_bitmap(metadot_gl_ctx_t *ctx, int32_t w, int32_t h, bool srgb, bool smooth, bool repeat, const uint8_t *bitmap);

/**
 * @brief Uploads data from a bitmap into a texture
 *
 * @param ctx     The relevant context
 * @param texture The target texture
 * @param w       The texture's width
 * @param h       The texture's height
 * @param bitmap  The pixel data in RGBA
 */
int metadot_gl_upload_texture(metadot_gl_ctx_t *ctx, metadot_gl_texture_t *texture, int32_t w, int32_t h, const uint8_t *bitmap);

/**
 * @brief Updates a region of an existing texture
 *
 * @param ctx     The relevant context
 * @param texture The texture to update
 * @param x       The x offset of the region
 * @param y       The y offset of the region
 * @param w       The width of the region
 * @param h       The height of the region
 * @param bitmap  The pixel data in RGBA
 */
void metadot_gl_update_texture(metadot_gl_ctx_t *ctx, metadot_gl_texture_t *texture, int x, int y, int w, int h, const uint8_t *bitmap);

/**
 * @brief Generate mipmaps for the specified texture
 *
 * Generates a sequence of smaller pre-filtered / pre-optimized textures
 * intended to reduce the effects of aliasing when rendering smaller versions
 * of the texture.
 *
 * @param texture Pointer to the target texture
 * @param linear  Determines the selection of the which mipmap to blend
 *
 * @returns 0 on success and -1 on failure
 */
int metadot_gl_generate_mipmap(metadot_gl_texture_t *texture, bool linear);

/**
 * @brief Destroys a texture
 *
 * @param texture Texture to destroy
 */
void metadot_gl_destroy_texture(metadot_gl_texture_t *texture);

/**
 * @brief Gets texture size
 *
 * @param texture The texture
 * @param w       Pointer to width (output)
 * @param h       Pointer to height (output)
 */
void metadot_gl_get_texture_size(const metadot_gl_texture_t *texture, int *w, int *h);

/**
 * Gets maximum texture size as reported by OpenGL
 *
 * @param w Pointer to width (output)
 * @param h Poineter to height (output)
 */
void metadot_gl_get_max_texture_size(int *w, int *h);

/**
 * @brief Return the implementation specific texture ID
 *
 * @param texture The target texture
 *
 * @returns An unsigned 64-bit ID value
 */
uint64_t metadot_gl_get_texture_id(const metadot_gl_texture_t *texture);

/**
 * @brief Activates a texture for rendering
 *
 * This function sets the context's currently texture. If `texture` is
 * `NULL` the current texture is deactivated.
 *
 * @param ctx     The relevant context
 * @param texture The texture to activate, or `NULL` to deactivate
 */
void metadot_gl_bind_texture(metadot_gl_ctx_t *ctx, metadot_gl_texture_t *texture);

/**
 * @brief Draw to texture
 *
 * The function set a texture to be the target for rendering until the texture
 * if replaced by another or set to `NULL`.
 *
 * @param ctx     The relevant context
 * @param texture The render target
 *
 * @returns 0 on success and -1 on failure
 */
int metadot_gl_set_render_target(metadot_gl_ctx_t *ctx, metadot_gl_texture_t *texture);

/**
 * @brief Clears the framebuffer to the specified color
 *
 * All of the parameters are in `[0.0, 1.0]`
 */
void metadot_gl_clear(float r, float g, float b, float a);

/**
 * Draws primitives according to a vertex array
 *
 * @param ctx       The relevant context
 * @param primitive The type of geometry to draw
 * @param vertices  A vertex array
 * @param count     The number of vertices
 * @param texture   The texture to draw from (can be `NULL`)
 * @param shader    The shader used to draw the array (cannot be `NULL`)
 */
void metadot_gl_draw_array(metadot_gl_ctx_t *ctx, metadot_gl_primitive_t primitive, const metadot_gl_vertex_t *vertices, metadot_gl_size_t count, metadot_gl_texture_t *texture,
                           metadot_gl_shader_t *shader);

/**
 * Draws primvities according to vertex and index arrays
 *
 * @param ctx          The relevant context
 * @param primitive    The type of geometry to draw
 * @param vertices     An array of vertices
 * @param vertex_count The number of vertices
 * @param indices      An array of indices
 * @param index_ count The number of indicies
 * @param texture      The texture to draw from (can be `NULL`)
 * @param shader       The shader used to draw the array (cannot be `NULL`)
 */
void metadot_gl_draw_indexed_array(metadot_gl_ctx_t *ctx, metadot_gl_primitive_t primitive, const metadot_gl_vertex_t *vertices, metadot_gl_size_t vertex_count, const uint32_t *indices,
                                   metadot_gl_size_t index_count, metadot_gl_texture_t *texture, metadot_gl_shader_t *shader);

/**
 * @brief Creates a buffer in VRAM to store an array of vertices that can then
 * be rendered without having upload the vertices every time they are drawn
 *
 * @param ctx       The relevant context
 * @param primitive The type of geometry to draw
 * @param vertices  A vertex array
 * @param count     The number of vertices to store in the buffer
 *
 * @returns A pointer to the buffer or `NULL` on error
 */
metadot_gl_buffer_t *metadot_gl_create_buffer(metadot_gl_ctx_t *ctx, metadot_gl_primitive_t primitive, const metadot_gl_vertex_t *vertices, metadot_gl_size_t count);

/**
 * @brief Destroys a previously created buffer
 */
void metadot_gl_destroy_buffer(metadot_gl_buffer_t *buffer);

/**
 * @brief Draw a previously created buffer
 *
 * @param ctx     The relevant context
 * @param buffer  The buffer to draw
 * @param start   The base vertex index
 * @param count   The number of vertices to draw from `start`
 * @param texture The texture to draw from (can be `NULL`)
 * @param shader  The shader used to draw the array (cannot be `NULL`)
 */
void metadot_gl_draw_buffer(metadot_gl_ctx_t *ctx, const metadot_gl_buffer_t *buffer, metadot_gl_size_t start, metadot_gl_size_t count, metadot_gl_texture_t *texture, metadot_gl_shader_t *shader);

/**
 * @brief Turns matrix transposition on/off
 */
void metadot_gl_set_transpose(metadot_gl_ctx_t *ctx, bool enabled);

/**
 * @brief Set the blending mode
 *
 * @param ctx  The relevant context
 * @param mode The blending mode (@see metadot_gl_blend_mode_t)
 */
void metadot_gl_set_blend_mode(metadot_gl_ctx_t *ctx, metadot_gl_blend_mode_t mode);

/**
 * @brief Resets the blend mode to standard alpha blending
 *
 * @param ctx The relevant context
 */
void metadot_gl_reset_blend_mode(metadot_gl_ctx_t *ctx);

/**
 * @brief Sets the context's global tranformation matrix
 *
 * @param ctx    The relevant context
 * @param matrix The global transform matrix
 */
void metadot_gl_set_transform(metadot_gl_ctx_t *ctx, const metadot_gl_m4_t matrix);

/**
 * @brief 3D variant of `metadot_gl_set_transform`
 *
 * @param ctx    The relevant context
 * @param matrix The 3D global transform matrix
 */
void metadot_gl_set_transform_3d(metadot_gl_ctx_t *ctx, const metadot_gl_m3_t matrix);

/**
 * @brief Resets the context's transform to the identity matrix
 *
 * @param ctx The relevant context
 */
void metadot_gl_reset_transform(metadot_gl_ctx_t *ctx);

/**
 * @brief Sets a context's global projecton matrix
 *
 * @param ctx    The relevant context
 * @param matrix The global projection matrix
 */
void metadot_gl_set_projection(metadot_gl_ctx_t *ctx, const metadot_gl_m4_t matrix);

/**
 * @brief 3D variant of `metadot_gl_set_projection`
 *
 * @param ctx    The relevant context
 * @param matrix The 3D global projection matrix
 */
void metadot_gl_set_projection_3d(metadot_gl_ctx_t *ctx, const metadot_gl_m3_t matrix);

/**
 * @brief Resets the context's project to the identity matrix
 *
 * @param ctx The relevant context
 */
void metadot_gl_reset_projection(metadot_gl_ctx_t *ctx);

/**
 * @brief Sets the location and dimensions of the rendering viewport
 *
 * @param ctx The relevant context
 * @param x   The left edge of the viewport
 * @param y   The bottom edge of the viewport
 * @param w   The width of the viewport
 * @param h   The height of the viewort
 */
void metadot_gl_set_viewport(metadot_gl_ctx_t *ctx, int32_t x, int32_t y, int32_t w, int32_t h);

/**
 * @brief Reset the viewport to the drawable dimensions of the context
 *
 * @param ctx The relevant context
 */
void metadot_gl_reset_viewport(metadot_gl_ctx_t *ctx);

/**
 @brief Sets the line primitive width
 */
void metadot_gl_set_line_width(metadot_gl_ctx_t *ctx, float line_width);

/**
 * @brief Resets the line width to 1.0f
 */
void metadot_gl_reset_line_width(metadot_gl_ctx_t *ctx);

/**
 * @brief Resets the current state of the context
 *
 * Resets the global transform, projection, blend mode, and viewport.
 *
 * @param ctx The relevant context
 */
void metadot_gl_reset_state(metadot_gl_ctx_t *ctx);

/**
 * @brief Pushes the current state onto the state stack, allowing it to be
 * restored later
 *
 * @param ctx The relevant context
 */
void metadot_gl_push_state(metadot_gl_ctx_t *ctx);

/**
 * @brief Pops a state off of the state stack and makes it the current state
 *
 * @param ctx The relevant context
 */
void metadot_gl_pop_state(metadot_gl_ctx_t *ctx);

/**
 * @brief Removes all states from the state stack
 *
 * @param ctx The relevant context
 */
void metadot_gl_clear_stack(metadot_gl_ctx_t *ctx);

/**
 * @brief Sets a boolean uniform
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param value  The boolean value
 */
void metadot_gl_set_bool(metadot_gl_shader_t *shader, const char *name, bool value);

/**
 * @brief Sets an integer uniform
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param value  The integer value
 */
void metadot_gl_set_1i(metadot_gl_shader_t *shader, const char *name, int32_t a);

/**
 * @brief Sets a 2D integer uniform
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param a      The first value
 * @param b      The second value
 */
void metadot_gl_set_2i(metadot_gl_shader_t *shader, const char *name, int32_t a, int32_t b);

/**
 * @brief Sets a 3D integer uniform
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param a      The first value
 * @param b      The second value
 * @param c      The third value
 */
void metadot_gl_set_3i(metadot_gl_shader_t *shader, const char *name, int32_t a, int32_t b, int32_t c);

/**
 * @brief Sets a 4D integer uniform
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param a      The first value
 * @param b      The second value
 * @param c      The third value
 * @param d      The fourth value
 */
void metadot_gl_set_4i(metadot_gl_shader_t *shader, const char *name, int32_t a, int32_t b, int32_t c, int32_t d);
/**
 * @brief Sets a 2D integer uniform by vector
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param vec    The vector
 */
void metadot_gl_set_v2i(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v2i_t vec);

/**
 * @brief Sets a 3D integer uniform by vector
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param vec    The vector
 */
void metadot_gl_set_v3i(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v3i_t vec);

/**
 * @brief Sets a 4D integer uniform by vector
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param vec    The vector
 */
void metadot_gl_set_v4i(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v4i_t vec);

/**
 * @brief Sets an floating point uniform
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param x      The float value
 */
void metadot_gl_set_1f(metadot_gl_shader_t *shader, const char *name, float x);

/**
 * @brief Sets a 2D floating point uniform
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param x      The first value
 * @param y      The second value
 */
void metadot_gl_set_2f(metadot_gl_shader_t *shader, const char *name, float x, float y);

/**
 * @brief Sets a 3D floating point uniform
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param x      The first value
 * @param y      The second value
 * @param z      The third value
 */
void metadot_gl_set_3f(metadot_gl_shader_t *shader, const char *name, float x, float y, float z);

/**
 * @brief Sets a 4D floating point uniform
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param x      The first value
 * @param y      The second value
 * @param w      The third value
 * @param z      The fourth value
 */
void metadot_gl_set_4f(metadot_gl_shader_t *shader, const char *name, float x, float y, float z, float w);

/**
 * @brief Sets a 2D floating point uniform by vector
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param vec    The vector
 */
void metadot_gl_set_v2f(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v2f_t vec);

/**
 * @brief Sets a 3D floating point uniform by vector
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param vec    The vector
 */
void metadot_gl_set_v3f(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v3f_t vec);

/**
 * @brief Sets a 4D floating point uniform by vector
 *
 * @param shader The uniform's shader program
 * @param name   The name of the uniform
 * @param vec    The vector
 */
void metadot_gl_set_v4f(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v4f_t vec);

/**
 * @brief Sends an array of floating point numbers
 *
 * @param shader The uniform's shader program
 * @param name The name of the uniform
 * @param array The array of floats
 * @param count The size of the array
 */
void metadot_gl_set_a1f(metadot_gl_shader_t *shader, const char *name, const float array[], metadot_gl_size_t count);

/**
 * @brief Sends an array of 2D floating point vectors
 *
 * @param shader The uniform's shader program
 * @param name The name of the uniform
 * @param array The array of vectors
 * @param count The size of the array
 */
void metadot_gl_set_a2f(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v2f_t array[], metadot_gl_size_t count);

/**
 * @brief Sends an array of 3D floating point vectors
 *
 * @param shader The uniform's shader program
 * @param name The name of the uniform
 * @param array The array of vectors
 * @param count The size of the array
 */
void metadot_gl_set_a3f(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v3f_t array[], metadot_gl_size_t count);

/**
 * @brief Sends an array of 4D floating point vectors
 *
 * @param shader The uniform's shader program
 * @param name The name of the uniform
 * @param array The array of vectors
 * @param count The size of the array
 */
void metadot_gl_set_a4f(metadot_gl_shader_t *shader, const char *name, const metadot_gl_v4f_t array[], metadot_gl_size_t count);

/**
 * @brief Sets a 2x2 floating point matrix
 *
 * @param shader The uniform's shader program
 * @param name The name of the uniform
 * @param matrix The matrix
 */
void metadot_gl_set_m2(metadot_gl_shader_t *shader, const char *name, const metadot_gl_m2_t matrix);

/**
 * @brief Sets a 3x3 floating point matrix
 *
 * @param shader The uniform's shader program
 * @param name The name of the uniform
 * @param matrix The matrix
 */
void metadot_gl_set_m3(metadot_gl_shader_t *shader, const char *name, const metadot_gl_m3_t matrix);

/**
 * @brief Sets a 4x4 floating point matrix
 *
 * @param shader The uniform's shader program
 * @param name The name of the uniform
 * @param matrix The matrix
 */
void metadot_gl_set_m4(metadot_gl_shader_t *shader, const char *name, const metadot_gl_m4_t matrix);

/**
 * @brief Sets a 2D sampler uniform
 *
 * @param shader The uniform's shader program
 * @param name The name of the uniform
 * @param value The sampler's texture unit
 */
void metadot_gl_set_s2d(metadot_gl_shader_t *shader, const char *name, int32_t value);

#ifdef __cplusplus
}
#endif

#endif  // METADOT_GL_H
