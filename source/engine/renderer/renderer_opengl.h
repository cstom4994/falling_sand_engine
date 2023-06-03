#ifndef ME_RENDERER_OPENGL_H
#define ME_RENDERER_OPENGL_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "engine/core/sdl_wrapper.h"
#include "libs/glad/glad.h"
#include "renderer_gpu.h"

#if defined(GL_EXT_bgr) && !defined(GL_BGR)
#define GL_BGR GL_BGR_EXT
#endif
#if defined(GL_EXT_bgra) && !defined(GL_BGRA)
#define GL_BGRA GL_BGRA_EXT
#endif
#if defined(GL_EXT_abgr) && !defined(GL_ABGR)
#define GL_ABGR GL_ABGR_EXT
#endif

#define ToSDLColor(_c) \
    SDL_Color { _c.r, _c.g, _c.b, _c.a }

#define ToEngineColor(_c) \
    ME_Color { _c.r, _c.g, _c.b, _c.a }

#define R_CONTEXT_DATA ContextData_OpenGL_3
#define R_IMAGE_DATA ImageData_OpenGL_3
#define R_TARGET_DATA TargetData_OpenGL_3

#define R_DEFAULT_TEXTURED_VERTEX_SHADER_SOURCE \
    "#version 330 core\n\
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

#define R_DEFAULT_UNTEXTURED_VERTEX_SHADER_SOURCE \
    "#version 330 core\n\
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

#define R_DEFAULT_TEXTURED_FRAGMENT_SHADER_SOURCE \
    "#version 330 core\n\
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

#define R_DEFAULT_UNTEXTURED_FRAGMENT_SHADER_SOURCE \
    "#version 330 core\n\
\
in vec4 color;\n\
\
out vec4 fragColor;\n\
\
void main(void)\n\
{\n\
    fragColor = color;\n\
}"

typedef struct ContextData_OpenGL_3 {
    ME_Color last_color;
    bool last_use_texturing;
    unsigned int last_shape;
    bool last_use_blending;
    R_BlendMode last_blend_mode;
    ME_rect last_viewport;
    R_Camera last_camera;
    bool last_camera_inverted;

    bool last_depth_test;
    bool last_depth_write;
    R_ComparisonEnum last_depth_function;

    R_Image *last_image;
    float *blit_buffer;  // Holds sets of 4 vertices, each with interleaved position, tex coords, and colors (e.g. [x0, y0, z0, s0, t0, r0, g0, b0, a0, ...]).
    unsigned short blit_buffer_num_vertices;
    unsigned short blit_buffer_max_num_vertices;
    unsigned short *index_buffer;  // Indexes into the blit buffer so we can use 4 vertices for every 2 triangles (1 quad)
    unsigned int index_buffer_num_vertices;
    unsigned int index_buffer_max_num_vertices;

    // Tier 3 rendering
    unsigned int blit_VAO;
    unsigned int blit_VBO[2];  // For double-buffering
    unsigned int blit_IBO;
    bool blit_VBO_flop;

    R_AttributeSource shader_attributes[16];
    unsigned int attribute_VBO[16];
} ContextData_OpenGL_3;

typedef struct ImageData_OpenGL_3 {
    int refcount;
    bool owns_handle;
    u32 handle;
    u32 format;
} ImageData_OpenGL_3;

typedef struct TargetData_OpenGL_3 {
    int refcount;
    u32 handle;
    u32 format;
} TargetData_OpenGL_3;

#define ME_OPENGL

#if defined(ME_OPENGL)

void row_upload_texture(const unsigned char *pixels, ME_rect update_rect, Uint32 format, int alignment, unsigned int pitch, int bytes_per_pixel);
void copy_upload_texture(const unsigned char *pixels, ME_rect update_rect, Uint32 format, int alignment, unsigned int pitch, int bytes_per_pixel);
void init_features(R_Renderer *renderer);
void extBindFramebuffer(R_Renderer *renderer, GLuint handle);
void bindTexture(R_Renderer *renderer, R_Image *image);
void makeContextCurrent(R_Renderer *renderer, R_Target *target);
bool SetActiveTarget(R_Renderer *renderer, R_Target *target);
bool growBlitBuffer(R_CONTEXT_DATA *cdata, unsigned int minimum_vertices_needed);
bool growIndexBuffer(R_CONTEXT_DATA *cdata, unsigned int minimum_vertices_needed);
void setClipRect(R_Renderer *renderer, R_Target *target);
void unsetClipRect(R_Renderer *renderer, R_Target *target);
void changeDepthTest(R_Renderer *renderer, bool enable);
void changeDepthWrite(R_Renderer *renderer, bool enable);
void changeDepthFunction(R_Renderer *renderer, R_ComparisonEnum compare_operation);
void prepareToRenderToTarget(R_Renderer *renderer, R_Target *target);
void changeColor(R_Renderer *renderer, SDL_Color color);
void changeBlending(R_Renderer *renderer, bool enable);
void forceChangeBlendMode(R_Renderer *renderer, R_BlendMode mode);
void changeBlendMode(R_Renderer *renderer, R_BlendMode mode);
Uint32 get_proper_program_id(R_Renderer *renderer, Uint32 program_object);
void applyTexturing(R_Renderer *renderer);
void changeTexturing(R_Renderer *renderer, bool enable);
void enableTexturing(R_Renderer *renderer);
void disableTexturing(R_Renderer *renderer);
SDL_Color get_complete_mod_color(R_Renderer *renderer, R_Target *target, R_Image *image);
void prepareToRenderImage(R_Renderer *renderer, R_Target *target, R_Image *image);
void prepareToRenderShapes(R_Renderer *renderer, unsigned int shape);
void forceChangeViewport(R_Target *target, ME_rect viewport);
void changeViewport(R_Target *target);
void applyTargetCamera(R_Target *target);
bool equal_cameras(R_Camera a, R_Camera b);
void changeCamera(R_Target *target);
void get_camera_matrix(R_Target *target, float *result);
int get_lowest_attribute_num_values(R_CONTEXT_DATA *cdata, int cap);
void read_until_end_of_comment(SDL_RWops *rwops, char multiline);
void swizzle_for_format(SDL_Color *color, GLenum format, unsigned char pixel[4]);
R_Image *gpu_copy_image_pixels_only(R_Renderer *renderer, R_Image *image);
int compareFormats(R_Renderer *renderer, GLenum glFormat, SDL_Surface *surface, GLenum *surfaceFormatResult);
bool readTargetPixels(R_Renderer *renderer, R_Target *source, GLint format, GLubyte *pixels);
bool readImagePixels(R_Renderer *renderer, R_Image *source, GLint format, GLubyte *pixels);
unsigned char *getRawTargetData(R_Renderer *renderer, R_Target *target);
unsigned char *getRawImageData(R_Renderer *renderer, R_Image *image);
bool get_GL_version(int *major, int *minor);
bool get_GLSL_version(int *version);
bool get_API_versions(R_Renderer *renderer);
void update_stored_dimensions(R_Target *target);

R_Target *Init(R_Renderer *renderer, R_RendererID renderer_request, Uint16 w, Uint16 h, R_WindowFlagEnum SDL_flags);
bool IsFeatureEnabled(R_Renderer *renderer, R_FeatureEnum feature);
R_Target *CreateTargetFromWindow(R_Renderer *renderer, Uint32 windowID, R_Target *target);
R_Target *CreateAliasTarget(R_Renderer *renderer, R_Target *target);
void MakeCurrent(R_Renderer *renderer, R_Target *target, Uint32 windowID);
void SetAsCurrent(R_Renderer *renderer);
void ResetRendererState(R_Renderer *renderer);
bool AddDepthBuffer(R_Renderer *renderer, R_Target *target);
bool SetWindowResolution(R_Renderer *renderer, Uint16 w, Uint16 h);
void SetVirtualResolution(R_Renderer *renderer, R_Target *target, Uint16 w, Uint16 h);
void UnsetVirtualResolution(R_Renderer *renderer, R_Target *target);
void Quit(R_Renderer *renderer);
bool SetFullscreen(R_Renderer *renderer, bool enable_fullscreen, bool use_desktop_resolution);
R_Camera SetCamera(R_Renderer *renderer, R_Target *target, R_Camera *cam);
GLuint CreateUninitializedTexture(R_Renderer *renderer);
R_Image *CreateUninitializedImage(R_Renderer *renderer, Uint16 w, Uint16 h, R_FormatEnum format);
R_Image *CreateImage(R_Renderer *renderer, Uint16 w, Uint16 h, R_FormatEnum format);
R_Image *CreateImageUsingTexture(R_Renderer *renderer, R_TextureHandle handle, bool take_ownership);
R_Image *CreateAliasImage(R_Renderer *renderer, R_Image *image);
void *CopySurfaceFromTarget(R_Renderer *renderer, R_Target *target);
void *CopySurfaceFromImage(R_Renderer *renderer, R_Image *image);
SDL_PixelFormat *AllocFormat(GLenum glFormat);
void FreeFormat(SDL_PixelFormat *format);
SDL_Surface *copySurfaceIfNeeded(R_Renderer *renderer, GLenum glFormat, SDL_Surface *surface, GLenum *surfaceFormatResult);
R_Image *CopyImage(R_Renderer *renderer, R_Image *image);
void UpdateImage(R_Renderer *renderer, R_Image *image, const ME_rect *image_rect, void *surface, const ME_rect *surface_rect);
void UpdateImageBytes(R_Renderer *renderer, R_Image *image, const ME_rect *image_rect, const unsigned char *bytes, int bytes_per_row);
bool ReplaceImage(R_Renderer *renderer, R_Image *image, void *surface, const ME_rect *surface_rect);
R_Image *CopyImageFromSurface(R_Renderer *renderer, void *surface, const ME_rect *surface_rect);
R_Image *CopyImageFromTarget(R_Renderer *renderer, R_Target *target);
void FreeImage(R_Renderer *renderer, R_Image *image);
R_Target *GetTarget(R_Renderer *renderer, R_Image *image);
void FreeTargetData(R_Renderer *renderer, R_TARGET_DATA *data);
void FreeContext(R_Context *context);
void FreeTarget(R_Renderer *renderer, R_Target *target);
void Blit(R_Renderer *renderer, R_Image *image, ME_rect *src_rect, R_Target *target, float x, float y);
void BlitRotate(R_Renderer *renderer, R_Image *image, ME_rect *src_rect, R_Target *target, float x, float y, float degrees);
void BlitScale(R_Renderer *renderer, R_Image *image, ME_rect *src_rect, R_Target *target, float x, float y, float scaleX, float scaleY);
void BlitTransform(R_Renderer *renderer, R_Image *image, ME_rect *src_rect, R_Target *target, float x, float y, float degrees, float scaleX, float scaleY);
void BlitTransformX(R_Renderer *renderer, R_Image *image, ME_rect *src_rect, R_Target *target, float x, float y, float pivot_x, float pivot_y, float degrees, float scaleX, float scaleY);

#ifdef R_USE_BUFFER_PIPELINE
void refresh_attribute_data(R_CONTEXT_DATA *cdata);
void upload_attribute_data(R_CONTEXT_DATA *cdata, int num_vertices);
void disable_attribute_data(R_CONTEXT_DATA *cdata);
#endif

void SetAttributefv(R_Renderer *renderer, int location, int num_elements, float *value);
void PrimitiveBatchV(R_Renderer *renderer, R_Image *image, R_Target *target, R_PrimitiveEnum primitive_type, unsigned short num_vertices, void *values, unsigned int num_indices,
                     unsigned short *indices, R_BatchFlagEnum flags);
void GenerateMipmaps(R_Renderer *renderer, R_Image *image);
ME_rect SetClip(R_Renderer *renderer, R_Target *target, Sint16 x, Sint16 y, Uint16 w, Uint16 h);
void UnsetClip(R_Renderer *renderer, R_Target *target);
ME_Color GetPixel(R_Renderer *renderer, R_Target *target, Sint16 x, Sint16 y);
void SetImageFilter(R_Renderer *renderer, R_Image *image, R_FilterEnum filter);
void SetWrapMode(R_Renderer *renderer, R_Image *image, R_WrapEnum wrap_mode_x, R_WrapEnum wrap_mode_y);
R_TextureHandle GetTextureHandle(R_Renderer *renderer, R_Image *image);
void ClearRGBA(R_Renderer *renderer, R_Target *target, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
void DoPartialFlush(R_Renderer *renderer, R_Target *dest, R_Context *context, unsigned short num_vertices, float *blit_buffer, unsigned int num_indices, unsigned short *index_buffer);
void DoUntexturedFlush(R_Renderer *renderer, R_Target *dest, R_Context *context, unsigned short num_vertices, float *blit_buffer, unsigned int num_indices, unsigned short *index_buffer);
void FlushBlitBuffer(R_Renderer *renderer);
void Flip(R_Renderer *renderer, R_Target *target);
Uint32 GetShaderSourceSize(const char *filename);
Uint32 GetShaderSource(const char *filename, char *result);
Uint32 GetShaderSourceSize_RW(SDL_RWops *shader_source);
Uint32 GetShaderSource_RW(SDL_RWops *shader_source, char *result);
Uint32 GetShaderSource(const char *filename, char *result);
Uint32 GetShaderSourceSize(const char *filename);
Uint32 compile_shader_source(R_ShaderEnum shader_type, const char *shader_source);
Uint32 CompileShaderInternal(R_Renderer *renderer, R_ShaderEnum shader_type, const char *shader_source);
Uint32 CompileShader(R_Renderer *renderer, R_ShaderEnum shader_type, const char *shader_source);
Uint32 CreateShaderProgram(R_Renderer *renderer);
bool LinkShaderProgram(R_Renderer *renderer, Uint32 program_object);
void FreeShader(R_Renderer *renderer, Uint32 shader_object);
void FreeShaderProgram(R_Renderer *renderer, Uint32 program_object);
void AttachShader(R_Renderer *renderer, Uint32 program_object, Uint32 shader_object);
void DetachShader(R_Renderer *renderer, Uint32 program_object, Uint32 shader_object);
void ActivateShaderProgram(R_Renderer *renderer, Uint32 program_object, R_ShaderBlock *block);
void DeactivateShaderProgram(R_Renderer *renderer);
const char *GetShaderMessage(R_Renderer *renderer);
int GetAttributeLocation(R_Renderer *renderer, Uint32 program_object, const char *attrib_name);
int GetUniformLocation(R_Renderer *renderer, Uint32 program_object, const char *uniform_name);
R_ShaderBlock LoadShaderBlock(R_Renderer *renderer, Uint32 program_object, const char *position_name, const char *texcoord_name, const char *color_name, const char *modelViewMatrix_name);
void SetShaderImage(R_Renderer *renderer, R_Image *image, int location, int image_unit);
void GetUniformiv(R_Renderer *renderer, Uint32 program_object, int location, int *values);
void SetUniformi(R_Renderer *renderer, int location, int value);
void SetUniformiv(R_Renderer *renderer, int location, int num_elements_per_value, int num_values, int *values);
void GetUniformuiv(R_Renderer *renderer, Uint32 program_object, int location, unsigned int *values);
void SetUniformui(R_Renderer *renderer, int location, unsigned int value);
void SetUniformuiv(R_Renderer *renderer, int location, int num_elements_per_value, int num_values, unsigned int *values);
void GetUniformfv(R_Renderer *renderer, Uint32 program_object, int location, float *values);
void SetUniformf(R_Renderer *renderer, int location, float value);
void SetUniformfv(R_Renderer *renderer, int location, int num_elements_per_value, int num_values, float *values);
void SetUniformMatrixfv(R_Renderer *renderer, int location, int num_matrices, int num_rows, int num_columns, bool transpose, float *values);
void SetAttributef(R_Renderer *renderer, int location, float value);
void SetAttributei(R_Renderer *renderer, int location, int value);
void SetAttributeui(R_Renderer *renderer, int location, unsigned int value);
void SetAttributefv(R_Renderer *renderer, int location, int num_elements, float *value);
void SetAttributeiv(R_Renderer *renderer, int location, int num_elements, int *value);
void SetAttributeuiv(R_Renderer *renderer, int location, int num_elements, unsigned int *value);
void SetAttributeSource(R_Renderer *renderer, int num_values, R_Attribute source);
float SetLineThickness(R_Renderer *renderer, float thickness);
float GetLineThickness(R_Renderer *renderer);
void DrawPixel(R_Renderer *renderer, R_Target *target, float x, float y, ME_Color color);
void Line(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, ME_Color color);
void Arc(R_Renderer *renderer, R_Target *target, float x, float y, float radius, float start_angle, float end_angle, ME_Color color);
void ArcFilled(R_Renderer *renderer, R_Target *target, float x, float y, float radius, float start_angle, float end_angle, ME_Color color);
void Circle(R_Renderer *renderer, R_Target *target, float x, float y, float radius, ME_Color color);
void CircleFilled(R_Renderer *renderer, R_Target *target, float x, float y, float radius, ME_Color color);
void Ellipse(R_Renderer *renderer, R_Target *target, float x, float y, float rx, float ry, float degrees, ME_Color color);
void EllipseFilled(R_Renderer *renderer, R_Target *target, float x, float y, float rx, float ry, float degrees, ME_Color color);
void Sector(R_Renderer *renderer, R_Target *target, float x, float y, float inner_radius, float outer_radius, float start_angle, float end_angle, ME_Color color);
void SectorFilled(R_Renderer *renderer, R_Target *target, float x, float y, float inner_radius, float outer_radius, float start_angle, float end_angle, ME_Color color);
void Tri(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, float x3, float y3, ME_Color color);
void TriFilled(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, float x3, float y3, ME_Color color);
void Rectangle(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, ME_Color color);
void RectangleFilled(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, ME_Color color);
void RectangleRound(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, float radius, ME_Color color);
void RectangleRoundFilled(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2, float radius, ME_Color color);
void Polygon(R_Renderer *renderer, R_Target *target, unsigned int num_vertices, float *vertices, ME_Color color);
void Polyline(R_Renderer *renderer, R_Target *target, unsigned int num_vertices, float *vertices, ME_Color color, bool close_loop);
void PolygonFilled(R_Renderer *renderer, R_Target *target, unsigned int num_vertices, float *vertices, ME_Color color);

#endif

#endif
