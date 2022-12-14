// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _R_H__
#define _R_H__

#include "core/core.h"
// #include "engine/game_utils/PixelColor.h"

#include "libs/external/stb_image.h"
#include "libs/external/stb_rect_pack.h"
#include "libs/external/stb_truetype.h"
#include "libs/glad/glad.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES// So M_PI and company get defined on MSVC when we include math.h
#endif
#include <math.h>// Must be included before SDL.h, otherwise both try to define M_PI and we get a warning

#include <stdarg.h>
#include <stdio.h>

// For now, SDL2 can only create opengl context successfully with 3.2 on macos
#define R_GL_VERSION_MAJOR 3
#define R_GL_VERSION_MINOR 2

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT 2

#define METAENGINE_ALPHA_TRANSPARENT 0

// Check for R_bool support
#ifdef __STDC_VERSION__
#define R_HAVE_STDC 1
#else
#define R_HAVE_STDC 0
#endif

#define R_HAVE_C99 (R_HAVE_STDC && (__STDC_VERSION__ >= 199901L))

#ifdef __GNUC__// catches both gcc and clang I believe
#define R_HAVE_GNUC 1
#else
#define R_HAVE_GNUC 0
#endif

#ifdef _MSC_VER
#define R_HAVE_MSVC 1
#else
#define R_HAVE_MSVC 0
#endif

#define R_HAVE_MSVC18 (R_HAVE_MSVC && (_MSC_VER >= 1800))// VS2013+

#if defined(_MSC_VER) || (defined(__INTEL_COMPILER) && defined(_WIN32))
#if defined(_M_X64)
#define R_BITNESS 64
#else
#define R_BITNESS 32
#endif
#define R_LONG_SIZE 4
#elif defined(__clang__) || defined(__INTEL_COMPILER) || defined(__GNUC__)
#if defined(__x86_64)
#define R_BITNESS 64
#else
#define R_BITNESS 32
#endif
#if __LONG_MAX__ == 2147483647L
#define R_LONG_SIZE 4
#else
#define R_LONG_SIZE 8
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// Struct padding for 32 or 64 bit alignment
#if R_BITNESS == 32
#define R_PAD_1_TO_32 char _padding[1];
#define R_PAD_2_TO_32 char _padding[2];
#define R_PAD_3_TO_32 char _padding[3];
#define R_PAD_1_TO_64 char _padding[1];
#define R_PAD_2_TO_64 char _padding[2];
#define R_PAD_3_TO_64 char _padding[3];
#define R_PAD_4_TO_64
#define R_PAD_5_TO_64 char _padding[1];
#define R_PAD_6_TO_64 char _padding[2];
#define R_PAD_7_TO_64 char _padding[3];
#elif R_BITNESS == 64
#define R_PAD_1_TO_32 char _padding[1];
#define R_PAD_2_TO_32 char _padding[2];
#define R_PAD_3_TO_32 char _padding[3];
#define R_PAD_1_TO_64 char _padding[1];
#define R_PAD_2_TO_64 char _padding[2];
#define R_PAD_3_TO_64 char _padding[3];
#define R_PAD_4_TO_64 char _padding[4];
#define R_PAD_5_TO_64 char _padding[5];
#define R_PAD_6_TO_64 char _padding[6];
#define R_PAD_7_TO_64 char _padding[7];
#endif

typedef struct
{
    U8 r;
    U8 g;
    U8 b;
    U8 a;
} METAENGINE_Color;

typedef struct R_Renderer R_Renderer;
typedef struct R_Target R_Target;

typedef struct R_Rect
{
    float x, y;
    float w, h;
} R_Rect;

/*! \ingroup Initialization
 * \ingroup RendererSetup
 * \ingroup RendererControls
 * Renderer ID object for identifying a specific renderer.
 * \see R_MakeRendererID()
 * \see R_InitRendererByID()
 */
typedef struct R_RendererID
{
    const char *name;
    int major_version;
    int minor_version;

    R_PAD_4_TO_64
} R_RendererID;

/*! \ingroup TargetControls
 * Comparison operations (for depth testing)
 * \see R_SetDepthFunction()
 * Values chosen for direct OpenGL compatibility.
 */
typedef enum {
    R_NEVER = 0x0200,
    R_LESS = 0x0201,
    R_EQUAL = 0x0202,
    R_LEQUAL = 0x0203,
    R_GREATER = 0x0204,
    R_NOTEQUAL = 0x0205,
    R_GEQUAL = 0x0206,
    R_ALWAYS = 0x0207
} R_ComparisonEnum;

/*! \ingroup ImageControls
 * Blend component functions
 * \see R_SetBlendFunction()
 * Values chosen for direct OpenGL compatibility.
 */
typedef enum {
    R_FUNC_ZERO = 0,
    R_FUNC_ONE = 1,
    R_FUNC_SRC_COLOR = 0x0300,
    R_FUNC_DST_COLOR = 0x0306,
    R_FUNC_ONE_MINUS_SRC = 0x0301,
    R_FUNC_ONE_MINUS_DST = 0x0307,
    R_FUNC_SRC_ALPHA = 0x0302,
    R_FUNC_DST_ALPHA = 0x0304,
    R_FUNC_ONE_MINUS_SRC_ALPHA = 0x0303,
    R_FUNC_ONE_MINUS_DST_ALPHA = 0x0305
} R_BlendFuncEnum;

/*! \ingroup ImageControls
 * Blend component equations
 * \see R_SetBlendEquation()
 * Values chosen for direct OpenGL compatibility.
 */
typedef enum {
    R_EQ_ADD = 0x8006,
    R_EQ_SUBTRACT = 0x800A,
    R_EQ_REVERSE_SUBTRACT = 0x800B
} R_BlendEqEnum;

/*! \ingroup ImageControls
 * Blend mode storage struct */
typedef struct R_BlendMode
{
    R_BlendFuncEnum source_color;
    R_BlendFuncEnum dest_color;
    R_BlendFuncEnum source_alpha;
    R_BlendFuncEnum dest_alpha;

    R_BlendEqEnum color_equation;
    R_BlendEqEnum alpha_equation;
} R_BlendMode;

/*! \ingroup ImageControls
 * Blend mode presets 
 * \see R_SetBlendMode()
 * \see R_GetBlendModeFromPreset()
 */
typedef enum {
    R_BLEND_NORMAL = 0,
    R_BLEND_PREMULTIPLIED_ALPHA = 1,
    R_BLEND_MULTIPLY = 2,
    R_BLEND_ADD = 3,
    R_BLEND_SUBTRACT = 4,
    R_BLEND_MOD_ALPHA = 5,
    R_BLEND_SET_ALPHA = 6,
    R_BLEND_SET = 7,
    R_BLEND_NORMAL_KEEP_ALPHA = 8,
    R_BLEND_NORMAL_ADD_ALPHA = 9,
    R_BLEND_NORMAL_FACTOR_ALPHA = 10
} R_BlendPresetEnum;

/*! \ingroup ImageControls
 * Image filtering options.  These affect the quality/interpolation of colors when images are scaled. 
 * \see R_SetImageFilter()
 */
typedef enum {
    R_FILTER_NEAREST = 0,
    R_FILTER_LINEAR = 1,
    R_FILTER_LINEAR_MIPMAP = 2
} R_FilterEnum;

/*! \ingroup ImageControls
 * Snap modes.  Blitting with these modes will align the sprite with the target's pixel grid.
 * \see R_SetSnapMode()
 * \see R_GetSnapMode()
 */
typedef enum {
    R_SNAP_NONE = 0,
    R_SNAP_POSITION = 1,
    R_SNAP_DIMENSIONS = 2,
    R_SNAP_POSITION_AND_DIMENSIONS = 3
} R_SnapEnum;

/*! \ingroup ImageControls
 * Image wrapping options.  These affect how images handle src_rect coordinates beyond their dimensions when blitted.
 * \see R_SetWrapMode()
 */
typedef enum {
    R_WRAP_NONE = 0,
    R_WRAP_REPEAT = 1,
    R_WRAP_MIRRORED = 2
} R_WrapEnum;

/*! \ingroup ImageControls
 * Image format enum
 * \see R_CreateImage()
 */
typedef enum {
    R_FORMAT_LUMINANCE = 1,
    R_FORMAT_LUMINANCE_ALPHA = 2,
    R_FORMAT_RGB = 3,
    R_FORMAT_RGBA = 4,
    R_FORMAT_ALPHA = 5,
    R_FORMAT_RG = 6,
    R_FORMAT_YCbCr422 = 7,
    R_FORMAT_YCbCr420P = 8,
    R_FORMAT_BGR = 9,
    R_FORMAT_BGRA = 10,
    R_FORMAT_ABGR = 11
} R_FormatEnum;

typedef enum {
    R_FILE_AUTO = 0,
    R_FILE_PNG,
    R_FILE_BMP,
    R_FILE_TGA
} R_FileFormatEnum;

typedef struct R_Image
{
    struct R_Renderer *renderer;
    R_Target *context_target;
    R_Target *target;
    void *data;

    U16 w, h;
    R_FormatEnum format;
    int num_layers;
    int bytes_per_pixel;
    U16 base_w, base_h;      // Original image dimensions
    U16 texture_w, texture_h;// Underlying texture dimensions

    float anchor_x;// Normalized coords for the point at which the image is blitted.  Default is (0.5, 0.5), that is, the image is drawn centered.
    float anchor_y;// These are interpreted according to R_SetCoordinateMode() and range from (0.0 - 1.0) normally.

    METAENGINE_Color color;
    R_BlendMode blend_mode;
    R_FilterEnum filter_mode;
    R_SnapEnum snap_mode;
    R_WrapEnum wrap_mode_x;
    R_WrapEnum wrap_mode_y;

    int refcount;

    R_bool using_virtual_resolution;
    R_bool has_mipmaps;
    R_bool use_blending;
    R_bool is_alias;
} R_Image;

/*! \ingroup ImageControls
 * A backend-neutral type that is intended to hold a backend-specific handle/pointer to a texture.
 * \see R_CreateImageUsingTexture()
 * \see R_GetTextureHandle()
 */
typedef uintptr_t R_TextureHandle;

/*! \ingroup TargetControls
 * Camera object that determines viewing transform.
 * \see R_SetCamera() 
 * \see R_GetDefaultCamera() 
 * \see R_GetCamera()
 */
typedef struct R_Camera
{
    float x, y, z;
    float angle;
    float zoom_x, zoom_y;
    float z_near, z_far;       // z clipping planes
    R_bool use_centered_origin;// move rotation/scaling origin to the center of the camera's view

    R_PAD_7_TO_64
} R_Camera;

/*! \ingroup ShaderInterface
 * Container for the built-in shader attribute and uniform locations (indices).
 * \see R_LoadShaderBlock()
 * \see R_SetShaderBlock()
 */
typedef struct R_ShaderBlock
{
    // Attributes
    int position_loc;
    int texcoord_loc;
    int color_loc;
    // Uniforms
    int modelViewProjection_loc;
} R_ShaderBlock;

#define R_MODEL 0
#define R_VIEW 1
#define R_PROJECTION 2

/*! \ingroup Matrix
 * Matrix stack data structure for global vertex transforms.  */
typedef struct R_MatrixStack
{
    unsigned int storage_size;
    unsigned int size;
    float **matrix;
} R_MatrixStack;

/*! \ingroup ContextControls
 * Rendering context data.  Only R_Targets which represent windows will store this. */
typedef struct R_Context
{
    /*! SDL_GLContext */
    void *context;

    /*! Last target used */
    R_Target *active_target;

    R_ShaderBlock current_shader_block;
    R_ShaderBlock default_textured_shader_block;
    R_ShaderBlock default_untextured_shader_block;

    /*! SDL window ID */
    U32 windowID;

    /*! Actual window dimensions */
    int window_w;
    int window_h;

    /*! Drawable region dimensions */
    int drawable_w;
    int drawable_h;

    /*! Window dimensions for restoring windowed mode after R_SetFullscreen(1,1). */
    int stored_window_w;
    int stored_window_h;

    /*! Shader handles used in the default shader programs */
    U32 default_textured_vertex_shader_id;
    U32 default_textured_fragment_shader_id;
    U32 default_untextured_vertex_shader_id;
    U32 default_untextured_fragment_shader_id;

    /*! Internal state */
    U32 current_shader_program;
    U32 default_textured_shader_program;
    U32 default_untextured_shader_program;

    R_BlendMode shapes_blend_mode;
    float line_thickness;

    int refcount;

    void *data;

    R_bool failed;
    R_bool use_texturing;
    R_bool shapes_use_blending;

    R_PAD_5_TO_64
} R_Context;

/*! \ingroup TargetControls
 * Render target object for use as a blitting destination.
 * A R_Target can be created from a R_Image with R_LoadTarget().
 * A R_Target can also represent a separate window with R_CreateTargetFromWindow().  In that case, 'context' is allocated and filled in.
 * Note: You must have passed the SDL_WINDOW_OPENGL flag to SDL_CreateWindow() for OpenGL renderers to work with new windows.
 * Free the memory with R_FreeTarget() when you're done.
 * \see R_LoadTarget()
 * \see R_CreateTargetFromWindow()
 * \see R_FreeTarget()
 */
struct R_Target
{
    struct R_Renderer *renderer;
    R_Target *context_target;
    R_Image *image;
    void *data;
    U16 w, h;
    U16 base_w, base_h;// The true dimensions of the underlying image or window
    R_Rect clip_rect;
    METAENGINE_Color color;

    R_Rect viewport;

    /*! Perspective and object viewing transforms. */
    int matrix_mode;
    R_MatrixStack projection_matrix;
    R_MatrixStack view_matrix;
    R_MatrixStack model_matrix;

    R_Camera camera;

    R_bool using_virtual_resolution;
    R_bool use_clip_rect;
    R_bool use_color;
    R_bool use_camera;

    R_ComparisonEnum depth_function;

    /*! Renderer context data.  NULL if the target does not represent a window or rendering context. */
    R_Context *context;
    int refcount;

    R_bool use_depth_test;
    R_bool use_depth_write;
    R_bool is_alias;

    R_PAD_1_TO_64
};

/*! \ingroup Initialization
 * Important GPU features which may not be supported depending on a device's extension support.  Can be bitwise OR'd together.
 * \see R_IsFeatureEnabled()
 * \see R_SetRequiredFeatures()
 */
typedef U32 R_FeatureEnum;
static const R_FeatureEnum R_FEATURE_NON_POWER_OF_TWO = 0x1;
static const R_FeatureEnum R_FEATURE_RENDER_TARGETS = 0x2;
static const R_FeatureEnum R_FEATURE_BLEND_EQUATIONS = 0x4;
static const R_FeatureEnum R_FEATURE_BLEND_FUNC_SEPARATE = 0x8;
static const R_FeatureEnum R_FEATURE_BLEND_EQUATIONS_SEPARATE = 0x10;
static const R_FeatureEnum R_FEATURE_GL_BGR = 0x20;
static const R_FeatureEnum R_FEATURE_GL_BGRA = 0x40;
static const R_FeatureEnum R_FEATURE_GL_ABGR = 0x80;
static const R_FeatureEnum R_FEATURE_VERTEX_SHADER = 0x100;
static const R_FeatureEnum R_FEATURE_FRAGMENT_SHADER = 0x200;
static const R_FeatureEnum R_FEATURE_PIXEL_SHADER = 0x200;
static const R_FeatureEnum R_FEATURE_GEOMETRY_SHADER = 0x400;
static const R_FeatureEnum R_FEATURE_WRAP_REPEAT_MIRRORED = 0x800;
static const R_FeatureEnum R_FEATURE_CORE_FRAMEBUFFER_OBJECTS = 0x1000;

/*! Combined feature flags */
#define R_FEATURE_ALL_BASE R_FEATURE_RENDER_TARGETS
#define R_FEATURE_ALL_BLEND_PRESETS (R_FEATURE_BLEND_EQUATIONS | R_FEATURE_BLEND_FUNC_SEPARATE)
#define R_FEATURE_ALL_GL_FORMATS (R_FEATURE_GL_BGR | R_FEATURE_GL_BGRA | R_FEATURE_GL_ABGR)
#define R_FEATURE_BASIC_SHADERS (R_FEATURE_FRAGMENT_SHADER | R_FEATURE_VERTEX_SHADER)
#define R_FEATURE_ALL_SHADERS                                                                      \
    (R_FEATURE_FRAGMENT_SHADER | R_FEATURE_VERTEX_SHADER | R_FEATURE_GEOMETRY_SHADER)

typedef U32 R_WindowFlagEnum;

/*! \ingroup Initialization
 * Initialization flags for changing default init parameters.  Can be bitwise OR'ed together.
 * Default (0) is to use late swap vsync and double buffering.
 * \see R_SetPreInitFlags()
 * \see R_GetPreInitFlags()
 */
typedef U32 R_InitFlagEnum;
static const R_InitFlagEnum R_INIT_ENABLE_VSYNC = 0x1;
static const R_InitFlagEnum R_INIT_DISABLE_VSYNC = 0x2;
static const R_InitFlagEnum R_INIT_DISABLE_DOUBLE_BUFFER = 0x4;
static const R_InitFlagEnum R_INIT_DISABLE_AUTO_VIRTUAL_RESOLUTION = 0x8;
static const R_InitFlagEnum R_INIT_REQUEST_COMPATIBILITY_PROFILE = 0x10;
static const R_InitFlagEnum R_INIT_USE_ROW_BY_ROW_TEXTURE_UPLOAD_FALLBACK = 0x20;
static const R_InitFlagEnum R_INIT_USE_COPY_TEXTURE_UPLOAD_FALLBACK = 0x40;

#define R_DEFAULT_INIT_FLAGS 0

static const U32 R_NONE = 0x0;

/*! \ingroup Rendering
 * Primitive types for rendering arbitrary geometry.  The values are intentionally identical to the GL_* primitives.
 * \see R_PrimitiveBatch()
 * \see R_PrimitiveBatchV()
 */
typedef U32 R_PrimitiveEnum;
static const R_PrimitiveEnum R_POINTS = 0x0;
static const R_PrimitiveEnum R_LINES = 0x1;
static const R_PrimitiveEnum R_LINE_LOOP = 0x2;
static const R_PrimitiveEnum R_LINE_STRIP = 0x3;
static const R_PrimitiveEnum R_TRIANGLES = 0x4;
static const R_PrimitiveEnum R_TRIANGLE_STRIP = 0x5;
static const R_PrimitiveEnum R_TRIANGLE_FAN = 0x6;

/*! Bit flags for geometry batching.
 * \see R_TriangleBatch()
 * \see R_TriangleBatchX()
 * \see R_PrimitiveBatch()
 * \see R_PrimitiveBatchV()
 */
typedef U32 R_BatchFlagEnum;
static const R_BatchFlagEnum R_BATCH_XY = 0x1;
static const R_BatchFlagEnum R_BATCH_XYZ = 0x2;
static const R_BatchFlagEnum R_BATCH_ST = 0x4;
static const R_BatchFlagEnum R_BATCH_RGB = 0x8;
static const R_BatchFlagEnum R_BATCH_RGBA = 0x10;
static const R_BatchFlagEnum R_BATCH_RGB8 = 0x20;
static const R_BatchFlagEnum R_BATCH_RGBA8 = 0x40;

#define R_BATCH_XY_ST (R_BATCH_XY | R_BATCH_ST)
#define R_BATCH_XYZ_ST (R_BATCH_XYZ | R_BATCH_ST)
#define R_BATCH_XY_RGB (R_BATCH_XY | R_BATCH_RGB)
#define R_BATCH_XYZ_RGB (R_BATCH_XYZ | R_BATCH_RGB)
#define R_BATCH_XY_RGBA (R_BATCH_XY | R_BATCH_RGBA)
#define R_BATCH_XYZ_RGBA (R_BATCH_XYZ | R_BATCH_RGBA)
#define R_BATCH_XY_ST_RGBA (R_BATCH_XY | R_BATCH_ST | R_BATCH_RGBA)
#define R_BATCH_XYZ_ST_RGBA (R_BATCH_XYZ | R_BATCH_ST | R_BATCH_RGBA)
#define R_BATCH_XY_RGB8 (R_BATCH_XY | R_BATCH_RGB8)
#define R_BATCH_XYZ_RGB8 (R_BATCH_XYZ | R_BATCH_RGB8)
#define R_BATCH_XY_RGBA8 (R_BATCH_XY | R_BATCH_RGBA8)
#define R_BATCH_XYZ_RGBA8 (R_BATCH_XYZ | R_BATCH_RGBA8)
#define R_BATCH_XY_ST_RGBA8 (R_BATCH_XY | R_BATCH_ST | R_BATCH_RGBA8)
#define R_BATCH_XYZ_ST_RGBA8 (R_BATCH_XYZ | R_BATCH_ST | R_BATCH_RGBA8)

/*! Bit flags for blitting into a rectangular region.
 * \see R_BlitRect
 * \see R_BlitRectX
 */
typedef U32 R_FlipEnum;
static const R_FlipEnum R_FLIP_NONE = 0x0;
static const R_FlipEnum R_FLIP_HORIZONTAL = 0x1;
static const R_FlipEnum R_FLIP_VERTICAL = 0x2;

/*! \ingroup ShaderInterface
 * Type enumeration for R_AttributeFormat specifications.
 */
typedef U32 R_TypeEnum;
// Use OpenGL's values for simpler translation
static const R_TypeEnum R_TYPE_BYTE = 0x1400;
static const R_TypeEnum R_TYPE_UNSIGNED_BYTE = 0x1401;
static const R_TypeEnum R_TYPE_SHORT = 0x1402;
static const R_TypeEnum R_TYPE_UNSIGNED_SHORT = 0x1403;
static const R_TypeEnum R_TYPE_INT = 0x1404;
static const R_TypeEnum R_TYPE_UNSIGNED_INT = 0x1405;
static const R_TypeEnum R_TYPE_FLOAT = 0x1406;
static const R_TypeEnum R_TYPE_DOUBLE = 0x140A;

typedef enum {
    R_VERTEX_SHADER = 0,
    R_FRAGMENT_SHADER = 1,
    R_PIXEL_SHADER = 1,
    R_GEOMETRY_SHADER = 2
} R_ShaderEnum;

/*! \ingroup ShaderInterface
 * Type enumeration for the shader language used by the renderer.
 */
typedef enum {
    R_LANGUAGE_NONE = 0,
    R_LANGUAGE_ARB_ASSEMBLY = 1,
    R_LANGUAGE_GLSL = 2,
    R_LANGUAGE_GLSLES = 3,
    R_LANGUAGE_HLSL = 4,
    R_LANGUAGE_CG = 5
} R_ShaderLanguageEnum;

/*! \ingroup ShaderInterface */
typedef struct R_AttributeFormat
{
    int num_elems_per_value;
    R_TypeEnum type;     // R_TYPE_FLOAT, R_TYPE_INT, R_TYPE_UNSIGNED_INT, etc.
    int stride_bytes;    // Number of bytes between two vertex specifications
    int offset_bytes;    // Number of bytes to skip at the beginning of 'values'
    R_bool is_per_sprite;// Per-sprite values are expanded to 4 vertices
    R_bool normalize;

    R_PAD_2_TO_32
} R_AttributeFormat;

/*! \ingroup ShaderInterface */
typedef struct R_Attribute
{
    void *values;// Expect 4 values for each sprite
    R_AttributeFormat format;
    int location;

    R_PAD_4_TO_64
} R_Attribute;

/*! \ingroup ShaderInterface */
typedef struct R_AttributeSource
{
    void *next_value;
    void *per_vertex_storage;// Could point to the attribute's values or to allocated storage

    int num_values;
    // Automatic storage format
    int per_vertex_storage_stride_bytes;
    int per_vertex_storage_offset_bytes;
    int per_vertex_storage_size;// Over 0 means that the per-vertex storage has been automatically allocated
    R_Attribute attribute;
    R_bool enabled;

    R_PAD_7_TO_64
} R_AttributeSource;

/*! \ingroup Logging
 * Type enumeration for error codes.
 * \see R_PushErrorCode()
 * \see R_PopErrorCode()
 */
typedef enum {
    R_ERROR_NONE = 0,
    R_ERROR_BACKEND_ERROR = 1,
    R_ERROR_DATA_ERROR = 2,
    R_ERROR_USER_ERROR = 3,
    R_ERROR_UNSUPPORTED_FUNCTION = 4,
    R_ERROR_NULL_ARGUMENT = 5,
    R_ERROR_FILE_NOT_FOUND = 6
} R_ErrorEnum;

/*! \ingroup Logging */
typedef struct R_ErrorObject
{
    char *function;
    char *details;
    R_ErrorEnum error;

    R_PAD_4_TO_64
} R_ErrorObject;

/* Private implementation of renderer members */
struct R_RendererImpl;

/*! Renderer object which specializes the API to a particular backend. */
struct R_Renderer
{
    /*! Struct identifier of the renderer. */
    R_RendererID id;
    R_RendererID requested_id;
    R_WindowFlagEnum SDL_init_flags;
    R_InitFlagEnum R_init_flags;

    R_ShaderLanguageEnum shader_language;
    int min_shader_version;
    int max_shader_version;
    R_FeatureEnum enabled_features;

    /*! Current display target */
    R_Target *current_context_target;

    /*! Default is (0.5, 0.5) - images draw centered. */
    float default_image_anchor_x;
    float default_image_anchor_y;

    struct R_RendererImpl *impl;

    /*! 0 for inverted, 1 for mathematical */
    R_bool coordinate_mode;

    R_PAD_7_TO_64
};

/*! The window corresponding to 'windowID' will be used to create the rendering context instead of creating a new window. */
void R_SetInitWindow(U32 windowID);

/*! Returns the window ID that has been set via R_SetInitWindow(). */
U32 R_GetInitWindow(void);

/*! Set special flags to use for initialization. Set these before calling R_Init().
 * \param R_flags An OR'ed combination of R_InitFlagEnum flags.  Default flags (0) enable late swap vsync and double buffering. */
void R_SetPreInitFlags(R_InitFlagEnum R_flags);

/*! Returns the current special flags to use for initialization. */
R_InitFlagEnum R_GetPreInitFlags(void);

/*! Set required features to use for initialization. Set these before calling R_Init().
 * \param features An OR'ed combination of R_FeatureEnum flags.  Required features will force R_Init() to create a renderer that supports all of the given flags or else fail. */
void R_SetRequiredFeatures(R_FeatureEnum features);

/*! Returns the current required features to use for initialization. */
R_FeatureEnum R_GetRequiredFeatures(void);

/*! Gets the current renderer ID order for initialization copied into the 'order' array and the number of renderer IDs into 'order_size'.  Pass NULL for 'order' to just get the size of the renderer order array. */
void R_GetRendererOrder(int *order_size, R_RendererID *order);

/*! Initializes SDL's video subsystem (if necessary) and all of SDL_gpu's internal structures.
 * Chooses a renderer and creates a window with the given dimensions and window creation flags.
 * A pointer to the resulting window's render target is returned.
 * 
 * \param w Desired window width in pixels
 * \param h Desired window height in pixels
 * \param SDL_flags The bit flags to pass to SDL when creating the window.  Use R_DEFAULT_INIT_FLAGS if you don't care.
 * \return On success, returns the new context target (i.e. render target backed by a window).  On failure, returns NULL.
 * 
 * Initializes these systems:
 *  The 'error queue': Stores error codes and description strings.
 *  The 'renderer registry': An array of information about the supported renderers on the current platform,
 *    such as the renderer name and id and its life cycle functions.
 *  The SDL library and its video subsystem: Calls SDL_Init() if SDL has not already been initialized.
 *    Use SDL_InitSubsystem() to initialize more parts of SDL.
 *  The current renderer:  Walks through each renderer in the renderer registry and tries to initialize them until one succeeds.
 *
 * \see R_RendererID
 * \see R_InitRenderer()
 * \see R_InitRendererByID()
 * \see R_PushErrorCode()
 */
R_Target *R_Init(U16 w, U16 h, R_WindowFlagEnum SDL_flags);

/*! Initializes SDL and SDL_gpu.  Creates a window and the requested renderer context. */
R_Target *R_InitRenderer(U16 w, U16 h, R_WindowFlagEnum SDL_flags);

/*! Initializes SDL and SDL_gpu.  Creates a window and the requested renderer context.
 * By requesting a renderer via ID, you can specify the major and minor versions of an individual renderer backend.
 * \see R_MakeRendererID
 */
R_Target *R_InitRendererByID(R_RendererID renderer_request, U16 w, U16 h,
                             R_WindowFlagEnum SDL_flags);

/*! Checks for important GPU features which may not be supported depending on a device's extension support.  Feature flags (R_FEATURE_*) can be bitwise OR'd together.
 * \return 1 if all of the passed features are enabled/supported
 * \return 0 if any of the passed features are disabled/unsupported
 */
R_bool R_IsFeatureEnabled(R_FeatureEnum feature);

/*! Clean up the renderer state. */
void R_CloseCurrentRenderer(void);

/*! Clean up the renderer state and shut down SDL_gpu. */
void R_Quit(void);

// End of Initialization
/*! @} */

/*! Pushes a new error code into the error queue.  If the queue is full, the queue is not modified.
 * \param function The name of the function that pushed the error
 * \param error The error code to push on the error queue
 * \param details Additional information string, can be NULL.
 */
void R_PushErrorCode(const char *function, R_ErrorEnum error, const char *details, ...);

/*! Pops an error object from the error queue and returns it.  If the error queue is empty, it returns an error object with NULL function, R_ERROR_NONE error, and NULL details. */
R_ErrorObject R_PopErrorCode(void);

/*! Gets the string representation of an error code. */
const char *R_GetErrorString(R_ErrorEnum error);

/*! Changes the maximum number of error objects that SDL_gpu will store.  This deletes all currently stored errors. */
void R_SetErrorQueueMax(unsigned int max);

// End of Logging
/*! @} */

/*! \ingroup RendererSetup
 *  @{ */

/*! Returns an initialized R_RendererID. */
R_RendererID R_MakeRendererID(const char *name, int major_version, int minor_version);

R_RendererID R_GetRendererID();

/*! Prepares a renderer for use by SDL_gpu. */
void R_RegisterRenderer(R_RendererID id, R_Renderer *(*create_renderer)(R_RendererID request),
                        void (*free_renderer)(R_Renderer *renderer));

// End of RendererSetup
/*! @} */

/*! \return The current renderer */
R_Renderer *R_GetCurrentRenderer(void);

/*! Switches the current renderer to the renderer matching the given identifier. */
void R_SetCurrentRenderer(R_RendererID id);

/*! \return The renderer matching the given identifier. */
R_Renderer *R_GetRenderer(R_RendererID id);

void R_FreeRenderer(R_Renderer *renderer);

/*! Reapplies the renderer state to the backend API (e.g. OpenGL, Direct3D).  Use this if you want SDL_gpu to be able to render after you've used direct backend calls. */
void R_ResetRendererState(void);

/*! Sets the coordinate mode for this renderer.  Target and image coordinates will be either "inverted" (0,0 is the upper left corner, y increases downward) or "mathematical" (0,0 is the bottom-left corner, y increases upward).
 * The default is inverted (0), as this is traditional for 2D graphics.
 * \param inverted 0 is for inverted coordinates, 1 is for mathematical coordinates */
void R_SetCoordinateMode(R_bool use_math_coords);

R_bool R_GetCoordinateMode(void);

/*! Sets the default image blitting anchor for newly created images.
 * \see R_SetAnchor
 */
void R_SetDefaultAnchor(float anchor_x, float anchor_y);

/*! Returns the default image blitting anchor through the given variables.
 * \see R_GetAnchor
 */
void R_GetDefaultAnchor(float *anchor_x, float *anchor_y);

// End of RendererControls
/*! @} */

// Context / window controls

/*! \ingroup ContextControls
 *  @{ */

/*! \return The renderer's current context target. */
R_Target *R_GetContextTarget(void);

/*! \return The target that is associated with the given windowID. */
R_Target *R_GetWindowTarget(U32 windowID);

/*! Creates a separate context for the given window using the current renderer and returns a R_Target that represents it. */
R_Target *R_CreateTargetFromWindow(U32 windowID);

/*! Makes the given window the current rendering destination for the given context target.
 * This also makes the target the current context for image loading and window operations.
 * If the target does not represent a window, this does nothing.
 */
void R_MakeCurrent(R_Target *target, U32 windowID);

/*! Change the actual size of the current context target's window.  This resets the virtual resolution and viewport of the context target.
 * Aside from direct resolution changes, this should also be called in response to SDL_WINDOWEVENT_RESIZED window events for resizable windows. */
R_bool R_SetWindowResolution(U16 w, U16 h);

/*! Enable/disable fullscreen mode for the current context target's window.
 * On some platforms, this may destroy the renderer context and require that textures be reloaded.  Unfortunately, SDL does not provide a notification mechanism for this.
 * \param enable_fullscreen If true, make the application go fullscreen.  If false, make the application go to windowed mode.
 * \param use_desktop_resolution If true, lets the window change its resolution when it enters fullscreen mode (via SDL_WINDOW_FULLSCREEN_DESKTOP).
 * \return 0 if the new mode is windowed, 1 if the new mode is fullscreen.  */
R_bool R_SetFullscreen(R_bool enable_fullscreen, R_bool use_desktop_resolution);

/*! Returns true if the current context target's window is in fullscreen mode. */
R_bool R_GetFullscreen(void);

/*! \return Returns the last active target. */
R_Target *R_GetActiveTarget(void);

/*! \return Sets the currently active target for matrix modification functions. */
R_bool R_SetActiveTarget(R_Target *target);

/*! Enables/disables alpha blending for shape rendering on the current window. */
void R_SetShapeBlending(R_bool enable);

/*! Translates a blend preset into a blend mode. */
R_BlendMode R_GetBlendModeFromPreset(R_BlendPresetEnum preset);

/*! Sets the blending component functions for shape rendering. */
void R_SetShapeBlendFunction(R_BlendFuncEnum source_color, R_BlendFuncEnum dest_color,
                             R_BlendFuncEnum source_alpha, R_BlendFuncEnum dest_alpha);

/*! Sets the blending component equations for shape rendering. */
void R_SetShapeBlendEquation(R_BlendEqEnum color_equation, R_BlendEqEnum alpha_equation);

/*! Sets the blending mode for shape rendering on the current window, if supported by the renderer. */
void R_SetShapeBlendMode(R_BlendPresetEnum mode);

/*! Sets the thickness of lines for the current context.
 * \param thickness New line thickness in pixels measured across the line.  Default is 1.0f.
 * \return The old thickness value
 */
float R_SetLineThickness(float thickness);

/*! Returns the current line thickness value. */
float R_GetLineThickness(void);

// End of ContextControls
/*! @} */

/*! \ingroup TargetControls
 *  @{ */

/*! Creates a target that aliases the given target.  Aliases can be used to store target settings (e.g. viewports) for easy switching.
 * R_FreeTarget() frees the alias's memory, but does not affect the original. */
R_Target *R_CreateAliasTarget(R_Target *target);

/*! Creates a new render target from the given image.  It can then be accessed from image->target.  This increments the internal refcount of the target, so it should be matched with a R_FreeTarget(). */
R_Target *R_LoadTarget(R_Image *image);

/*! Creates a new render target from the given image.  It can then be accessed from image->target.  This does not increment the internal refcount of the target, so it will be invalidated when the image is freed. */
R_Target *R_GetTarget(R_Image *image);

/*! Deletes a render target in the proper way for this renderer. */
void R_FreeTarget(R_Target *target);

/*! Change the logical size of the given target.  Rendering to this target will be scaled as if the dimensions were actually the ones given. */
void R_SetVirtualResolution(R_Target *target, U16 w, U16 h);

/*! Query the logical size of the given target. */
void R_GetVirtualResolution(R_Target *target, U16 *w, U16 *h);

/*! Converts screen space coordinates (such as from mouse input) to logical drawing coordinates.  This interacts with R_SetCoordinateMode() when the y-axis is flipped (screen space is assumed to be inverted: (0,0) in the upper-left corner). */
void R_GetVirtualCoords(R_Target *target, float *x, float *y, float displayX, float displayY);

/*! Reset the logical size of the given target to its original value. */
void R_UnsetVirtualResolution(R_Target *target);

/*! \return A R_Rect with the given values. */
R_Rect R_MakeRect(float x, float y, float w, float h);

/*! \return An METAENGINE_Color with the given values. */
METAENGINE_Color R_MakeColor(U8 r, U8 g, U8 b, U8 a);

/*! Sets the given target's viewport. */
void R_SetViewport(R_Target *target, R_Rect viewport);

/*! Resets the given target's viewport to the entire target area. */
void R_UnsetViewport(R_Target *target);

/*! \return A R_Camera with position (0, 0, 0), angle of 0, zoom of 1, centered origin, and near/far clipping planes of -100 and 100. */
R_Camera R_GetDefaultCamera(void);

/*! \return The camera of the given render target.  If target is NULL, returns the default camera. */
R_Camera R_GetCamera(R_Target *target);

/*! Sets the current render target's current camera.
 * \param target A pointer to the target that will copy this camera.
 * \param cam A pointer to the camera data to use or NULL to use the default camera.
 * \return The old camera. */
R_Camera R_SetCamera(R_Target *target, R_Camera *cam);

/*! Enables or disables using the built-in camera matrix transforms. */
void R_EnableCamera(R_Target *target, R_bool use_camera);

/*! Returns 1 if the camera transforms are enabled, 0 otherwise. */
R_bool R_IsCameraEnabled(R_Target *target);

/*! Attach a new depth buffer to the given target so that it can use depth testing.  Context targets automatically have a depth buffer already.
 *  If successful, also enables depth testing for this target.
 */
R_bool R_AddDepthBuffer(R_Target *target);

/*! Enables or disables the depth test, which will skip drawing pixels/fragments behind other fragments.  Disabled by default.
 *  This has implications for alpha blending, where compositing might not work correctly depending on render order.
 */
void R_SetDepthTest(R_Target *target, R_bool enable);

/*! Enables or disables writing the depth (effective view z-coordinate) of new pixels to the depth buffer.  Enabled by default, but you must call R_SetDepthTest() to use it. */
void R_SetDepthWrite(R_Target *target, R_bool enable);

/*! Sets the operation to perform when depth testing. */
void R_SetDepthFunction(R_Target *target, R_ComparisonEnum compare_operation);

/*! \return The RGBA color of a pixel. */
METAENGINE_Color R_GetPixel(R_Target *target, I16 x, I16 y);

/*! Sets the clipping rect for the given render target. */
R_Rect R_SetClipRect(R_Target *target, R_Rect rect);

/*! Sets the clipping rect for the given render target. */
R_Rect R_SetClip(R_Target *target, I16 x, I16 y, U16 w, U16 h);

/*! Turns off clipping for the given target. */
void R_UnsetClip(R_Target *target);

/*! Returns true if the given rects A and B overlap, in which case it also fills the given result rect with the intersection.  `result` can be NULL if you don't need the intersection. */
R_bool R_IntersectRect(R_Rect A, R_Rect B, R_Rect *result);

/*! Returns true if the given target's clip rect and the given B rect overlap, in which case it also fills the given result rect with the intersection.  `result` can be NULL if you don't need the intersection.
 * If the target doesn't have a clip rect enabled, this uses the whole target area.
 */
R_bool R_IntersectClipRect(R_Target *target, R_Rect B, R_Rect *result);

/*! Sets the modulation color for subsequent drawing of images and shapes on the given target.
 *  This has a cumulative effect with the image coloring functions.
 *  e.g. R_SetRGB(image, 255, 128, 0); R_SetTargetRGB(target, 128, 128, 128);
 *  Would make the image draw with color of roughly (128, 64, 0).
 */
void R_SetTargetColor(R_Target *target, METAENGINE_Color color);

/*! Sets the modulation color for subsequent drawing of images and shapes on the given target.
 *  This has a cumulative effect with the image coloring functions.
 *  e.g. R_SetRGB(image, 255, 128, 0); R_SetTargetRGB(target, 128, 128, 128);
 *  Would make the image draw with color of roughly (128, 64, 0).
 */
void R_SetTargetRGB(R_Target *target, U8 r, U8 g, U8 b);

/*! Sets the modulation color for subsequent drawing of images and shapes on the given target.
 *  This has a cumulative effect with the image coloring functions.
 *  e.g. R_SetRGB(image, 255, 128, 0); R_SetTargetRGB(target, 128, 128, 128);
 *  Would make the image draw with color of roughly (128, 64, 0).
 */
void R_SetTargetRGBA(R_Target *target, U8 r, U8 g, U8 b, U8 a);

/*! Unsets the modulation color for subsequent drawing of images and shapes on the given target.
 *  This has the same effect as coloring with pure opaque white (255, 255, 255, 255).
 */
void R_UnsetTargetColor(R_Target *target);

// End of SurfaceControls
/*! @} */

/*! \ingroup ImageControls
 *  @{ */

/*! Create a new, blank image with the given format.  Don't forget to R_FreeImage() it.
	 * \param w Image width in pixels
	 * \param h Image height in pixels
	 * \param format Format of color channels.
	 */
R_Image *R_CreateImage(U16 w, U16 h, R_FormatEnum format);

/*! Create a new image that uses the given native texture handle as the image texture. */
R_Image *R_CreateImageUsingTexture(R_TextureHandle handle, R_bool take_ownership);

/*! Creates an image that aliases the given image.  Aliases can be used to store image settings (e.g. modulation color) for easy switching.
 * R_FreeImage() frees the alias's memory, but does not affect the original. */
R_Image *R_CreateAliasImage(R_Image *image);

/*! Copy an image to a new image.  Don't forget to R_FreeImage() both. */
R_Image *R_CopyImage(R_Image *image);

/*! Deletes an image in the proper way for this renderer.  Also deletes the corresponding R_Target if applicable.  Be careful not to use that target afterward! */
void R_FreeImage(R_Image *image);

/*! Change the logical size of the given image.  Rendering this image will scaled it as if the dimensions were actually the ones given. */
void R_SetImageVirtualResolution(R_Image *image, U16 w, U16 h);

/*! Reset the logical size of the given image to its original value. */
void R_UnsetImageVirtualResolution(R_Image *image);

/*! Update an image from surface data.  Ignores virtual resolution on the image so the number of pixels needed from the surface is known. */
void R_UpdateImage(R_Image *image, const R_Rect *image_rect, void *surface,
                   const R_Rect *surface_rect);

/*! Update an image from an array of pixel data.  Ignores virtual resolution on the image so the number of pixels needed from the surface is known. */
void R_UpdateImageBytes(R_Image *image, const R_Rect *image_rect, const unsigned char *bytes,
                        int bytes_per_row);

/*! Update an image from surface data, replacing its underlying texture to allow for size changes.  Ignores virtual resolution on the image so the number of pixels needed from the surface is known. */
R_bool R_ReplaceImage(R_Image *image, void *surface, const R_Rect *surface_rect);

/*! Loads mipmaps for the given image, if supported by the renderer. */
void R_GenerateMipmaps(R_Image *image);

/*! Sets the modulation color for subsequent drawing of the given image. */
void R_SetColor(R_Image *image, METAENGINE_Color color);

/*! Sets the modulation color for subsequent drawing of the given image. */
void R_SetRGB(R_Image *image, U8 r, U8 g, U8 b);

/*! Sets the modulation color for subsequent drawing of the given image. */
void R_SetRGBA(R_Image *image, U8 r, U8 g, U8 b, U8 a);

/*! Unsets the modulation color for subsequent drawing of the given image.
 *  This is equivalent to coloring with pure opaque white (255, 255, 255, 255). */
void R_UnsetColor(R_Image *image);

/*! Gets the current alpha blending setting. */
R_bool R_GetBlending(R_Image *image);

/*! Enables/disables alpha blending for the given image. */
void R_SetBlending(R_Image *image, R_bool enable);

/*! Sets the blending component functions. */
void R_SetBlendFunction(R_Image *image, R_BlendFuncEnum source_color, R_BlendFuncEnum dest_color,
                        R_BlendFuncEnum source_alpha, R_BlendFuncEnum dest_alpha);

/*! Sets the blending component equations. */
void R_SetBlendEquation(R_Image *image, R_BlendEqEnum color_equation, R_BlendEqEnum alpha_equation);

/*! Sets the blending mode, if supported by the renderer. */
void R_SetBlendMode(R_Image *image, R_BlendPresetEnum mode);

/*! Sets the image filtering mode, if supported by the renderer. */
void R_SetImageFilter(R_Image *image, R_FilterEnum filter);

/*! Sets the image anchor, which is the point about which the image is blitted.  The default is to blit the image on-center (0.5, 0.5).  The anchor is in normalized coordinates (0.0-1.0). */
void R_SetAnchor(R_Image *image, float anchor_x, float anchor_y);

/*! Returns the image anchor via the passed parameters.  The anchor is in normalized coordinates (0.0-1.0). */
void R_GetAnchor(R_Image *image, float *anchor_x, float *anchor_y);

/*! Gets the current pixel snap setting.  The default value is R_SNAP_POSITION_AND_DIMENSIONS.  */
R_SnapEnum R_GetSnapMode(R_Image *image);

/*! Sets the pixel grid snapping mode for the given image. */
void R_SetSnapMode(R_Image *image, R_SnapEnum mode);

/*! Sets the image wrapping mode, if supported by the renderer. */
void R_SetWrapMode(R_Image *image, R_WrapEnum wrap_mode_x, R_WrapEnum wrap_mode_y);

/*! Returns the backend-specific texture handle associated with the given image.  Note that SDL_gpu will be unaware of changes made to the texture.  */
R_TextureHandle R_GetTextureHandle(R_Image *image);

// End of ImageControls
/*! @} */

// Surface / Image / Target conversions
/*! \ingroup Conversions
 *  @{ */

/*! Copy SDL_Surface data into a new R_Image.  Don't forget to SDL_FreeSurface() the surface and R_FreeImage() the image.*/
R_Image *R_CopyImageFromSurface(void *surface);

/*! Like R_CopyImageFromSurface but enable to copy only part of the surface.*/
R_Image *R_CopyImageFromSurfaceRect(void *surface, R_Rect *surface_rect);

/*! Copy R_Target data into a new R_Image.  Don't forget to R_FreeImage() the image.*/
R_Image *R_CopyImageFromTarget(R_Target *target);

/*! Copy R_Target data into a new SDL_Surface.  Don't forget to SDL_FreeSurface() the surface.*/
void *R_CopySurfaceFromTarget(R_Target *target);

/*! Copy R_Image data into a new SDL_Surface.  Don't forget to SDL_FreeSurface() the surface and R_FreeImage() the image.*/
void *R_CopySurfaceFromImage(R_Image *image);

// End of Conversions
/*! @} */

/*! \ingroup Matrix
 *  @{ */

// Basic vector operations (3D)

/*! Returns the magnitude (length) of the given vector. */
float R_VectorLength(const float *vec3);

/*! Modifies the given vector so that it has a new length of 1. */
void R_VectorNormalize(float *vec3);

/*! Returns the dot product of two vectors. */
float R_VectorDot(const float *A, const float *B);

/*! Performs the cross product of vectors A and B (result = A x B).  Do not use A or B as 'result'. */
void R_VectorCross(float *result, const float *A, const float *B);

/*! Overwrite 'result' vector with the values from vector A. */
void R_VectorCopy(float *result, const float *A);

/*! Multiplies the given matrix into the given vector (vec3 = matrix*vec3). */
void R_VectorApplyMatrix(float *vec3, const float *matrix_4x4);

/*! Multiplies the given matrix into the given vector (vec4 = matrix*vec4). */
void R_Vector4ApplyMatrix(float *vec4, const float *matrix_4x4);

// Basic matrix operations (4x4)

/*! Overwrite 'result' matrix with the values from matrix A. */
void R_MatrixCopy(float *result, const float *A);

/*! Fills 'result' matrix with the identity matrix. */
void R_MatrixIdentity(float *result);

/*! Multiplies an orthographic projection matrix into the given matrix. */
void R_MatrixOrtho(float *result, float left, float right, float bottom, float top, float z_near,
                   float z_far);

/*! Multiplies a perspective projection matrix into the given matrix. */
void R_MatrixFrustum(float *result, float left, float right, float bottom, float top, float z_near,
                     float z_far);

/*! Multiplies a perspective projection matrix into the given matrix. */
void R_MatrixPerspective(float *result, float fovy, float aspect, float z_near, float z_far);

/*! Multiplies a view matrix into the given matrix. */
void R_MatrixLookAt(float *matrix, float eye_x, float eye_y, float eye_z, float target_x,
                    float target_y, float target_z, float up_x, float up_y, float up_z);

/*! Adds a translation into the given matrix. */
void R_MatrixTranslate(float *result, float x, float y, float z);

/*! Multiplies a scaling matrix into the given matrix. */
void R_MatrixScale(float *result, float sx, float sy, float sz);

/*! Multiplies a rotation matrix into the given matrix. */
void R_MatrixRotate(float *result, float degrees, float x, float y, float z);

/*! Multiplies matrices A and B and stores the result in the given 'result' matrix (result = A*B).  Do not use A or B as 'result'.
 * \see R_MultiplyAndAssign
*/
void R_MatrixMultiply(float *result, const float *A, const float *B);

/*! Multiplies matrices 'result' and B and stores the result in the given 'result' matrix (result = result * B). */
void R_MultiplyAndAssign(float *result, const float *B);

// Matrix stack accessors

/*! Returns an internal string that represents the contents of matrix A. */
const char *R_GetMatrixString(const float *A);

/*! Returns the current matrix from the active target.  Returns NULL if stack is empty. */
float *R_GetCurrentMatrix(void);

/*! Returns the current matrix from the top of the matrix stack.  Returns NULL if stack is empty. */
float *R_GetTopMatrix(R_MatrixStack *stack);

/*! Returns the current model matrix from the active target.  Returns NULL if stack is empty. */
float *R_GetModel(void);

/*! Returns the current view matrix from the active target.  Returns NULL if stack is empty. */
float *R_GetView(void);

/*! Returns the current projection matrix from the active target.  Returns NULL if stack is empty. */
float *R_GetProjection(void);

/*! Copies the current modelview-projection matrix from the active target into the given 'result' matrix (result = P*V*M). */
void R_GetModelViewProjection(float *result);

// Matrix stack manipulators

/*! Returns a newly allocated matrix stack that has already been initialized. */
R_MatrixStack *R_CreateMatrixStack(void);

/*! Frees the memory for the matrix stack and any matrices it contains. */
void R_FreeMatrixStack(R_MatrixStack *stack);

/*! Allocate new matrices for the given stack. */
void R_InitMatrixStack(R_MatrixStack *stack);

/*! Copies matrices from one stack to another. */
void R_CopyMatrixStack(const R_MatrixStack *source, R_MatrixStack *dest);

/*! Deletes matrices in the given stack. */
void R_ClearMatrixStack(R_MatrixStack *stack);

/*! Reapplies the default orthographic projection matrix, based on camera and coordinate settings. */
void R_ResetProjection(R_Target *target);

/*! Sets the active target and changes matrix mode to R_PROJECTION, R_VIEW, or R_MODEL.  Further matrix stack operations manipulate that particular stack. */
void R_MatrixMode(R_Target *target, int matrix_mode);

/*! Copies the given matrix to the active target's projection matrix. */
void R_SetProjection(const float *A);

/*! Copies the given matrix to the active target's view matrix. */
void R_SetView(const float *A);

/*! Copies the given matrix to the active target's model matrix. */
void R_SetModel(const float *A);

/*! Copies the given matrix to the active target's projection matrix. */
void R_SetProjectionFromStack(R_MatrixStack *stack);

/*! Copies the given matrix to the active target's view matrix. */
void R_SetViewFromStack(R_MatrixStack *stack);

/*! Copies the given matrix to the active target's model matrix. */
void R_SetModelFromStack(R_MatrixStack *stack);

/*! Pushes the current matrix as a new matrix stack item to be restored later. */
void R_PushMatrix(void);

/*! Removes the current matrix from the stack, restoring the previously pushed matrix. */
void R_PopMatrix(void);

/*! Fills current matrix with the identity matrix. */
void R_LoadIdentity(void);

/*! Copies a given matrix to be the current matrix. */
void R_LoadMatrix(const float *matrix4x4);

/*! Multiplies an orthographic projection matrix into the current matrix. */
void R_Ortho(float left, float right, float bottom, float top, float z_near, float z_far);

/*! Multiplies a perspective projection matrix into the current matrix. */
void R_Frustum(float left, float right, float bottom, float top, float z_near, float z_far);

/*! Multiplies a perspective projection matrix into the current matrix. */
void R_Perspective(float fovy, float aspect, float z_near, float z_far);

/*! Multiplies a view matrix into the current matrix. */
void R_LookAt(float eye_x, float eye_y, float eye_z, float target_x, float target_y, float target_z,
              float up_x, float up_y, float up_z);

/*! Adds a translation into the current matrix. */
void R_Translate(float x, float y, float z);

/*! Multiplies a scaling matrix into the current matrix. */
void R_Scale(float sx, float sy, float sz);

/*! Multiplies a rotation matrix into the current matrix. */
void R_Rotate(float degrees, float x, float y, float z);

/*! Multiplies a given matrix into the current matrix. */
void R_MultMatrix(const float *matrix4x4);

// End of Matrix
/*! @} */

/*! \ingroup Rendering
 *  @{ */

/*! Clears the contents of the given render target.  Fills the target with color {0, 0, 0, 0}. */
void R_Clear(R_Target *target);

/*! Fills the given render target with a color. */
void R_ClearColor(R_Target *target, METAENGINE_Color color);

/*! Fills the given render target with a color (alpha is 255, fully opaque). */
void R_ClearRGB(R_Target *target, U8 r, U8 g, U8 b);

/*! Fills the given render target with a color. */
void R_ClearRGBA(R_Target *target, U8 r, U8 g, U8 b, U8 a);

/*! Draws the given image to the given render target.
    * \param src_rect The region of the source image to use.  Pass NULL for the entire image.
    * \param x Destination x-position
    * \param y Destination y-position */
void R_Blit(R_Image *image, R_Rect *src_rect, R_Target *target, float x, float y);

/*! Rotates and draws the given image to the given render target.
    * \param src_rect The region of the source image to use.  Pass NULL for the entire image.
    * \param x Destination x-position
    * \param y Destination y-position
    * \param degrees Rotation angle (in degrees) */
void R_BlitRotate(R_Image *image, R_Rect *src_rect, R_Target *target, float x, float y,
                  float degrees);

/*! Scales and draws the given image to the given render target.
    * \param src_rect The region of the source image to use.  Pass NULL for the entire image.
    * \param x Destination x-position
    * \param y Destination y-position
    * \param scaleX Horizontal stretch factor
    * \param scaleY Vertical stretch factor */
void R_BlitScale(R_Image *image, R_Rect *src_rect, R_Target *target, float x, float y, float scaleX,
                 float scaleY);

/*! Scales, rotates, and draws the given image to the given render target.
    * \param src_rect The region of the source image to use.  Pass NULL for the entire image.
    * \param x Destination x-position
    * \param y Destination y-position
    * \param degrees Rotation angle (in degrees)
    * \param scaleX Horizontal stretch factor
    * \param scaleY Vertical stretch factor */
void R_BlitTransform(R_Image *image, R_Rect *src_rect, R_Target *target, float x, float y,
                     float degrees, float scaleX, float scaleY);

/*! Scales, rotates around a pivot point, and draws the given image to the given render target.  The drawing point (x, y) coincides with the pivot point on the src image (pivot_x, pivot_y).
	* \param src_rect The region of the source image to use.  Pass NULL for the entire image.
	* \param x Destination x-position
	* \param y Destination y-position
	* \param pivot_x Pivot x-position (in image coordinates)
	* \param pivot_y Pivot y-position (in image coordinates)
	* \param degrees Rotation angle (in degrees)
	* \param scaleX Horizontal stretch factor
	* \param scaleY Vertical stretch factor */
void R_BlitTransformX(R_Image *image, R_Rect *src_rect, R_Target *target, float x, float y,
                      float pivot_x, float pivot_y, float degrees, float scaleX, float scaleY);

/*! Draws the given image to the given render target, scaling it to fit the destination region.
    * \param src_rect The region of the source image to use.  Pass NULL for the entire image.
    * \param dest_rect The region of the destination target image to draw upon.  Pass NULL for the entire target.
    */
void R_BlitRect(R_Image *image, R_Rect *src_rect, R_Target *target, R_Rect *dest_rect);

/*! Draws the given image to the given render target, scaling it to fit the destination region.
    * \param src_rect The region of the source image to use.  Pass NULL for the entire image.
    * \param dest_rect The region of the destination target image to draw upon.  Pass NULL for the entire target.
	* \param degrees Rotation angle (in degrees)
	* \param pivot_x Pivot x-position (in image coordinates)
	* \param pivot_y Pivot y-position (in image coordinates)
	* \param flip_direction A R_FlipEnum value (or bitwise OR'd combination) that specifies which direction the image should be flipped.
    */
void R_BlitRectX(R_Image *image, R_Rect *src_rect, R_Target *target, R_Rect *dest_rect,
                 float degrees, float pivot_x, float pivot_y, R_FlipEnum flip_direction);

/*! Renders triangles from the given set of vertices.  This lets you render arbitrary geometry.  It is a direct path to the GPU, so the format is different than typical SDL_gpu calls.
 * \param values A tightly-packed array of vertex position (e.g. x,y), texture coordinates (e.g. s,t), and color (e.g. r,g,b,a) values.  Texture coordinates and color values are expected to be already normalized to 0.0 - 1.0.  Pass NULL to render with only custom shader attributes.
 * \param indices If not NULL, this is used to specify which vertices to use and in what order (i.e. it indexes the vertices in the 'values' array).
 * \param flags Bit flags to control the interpretation of the 'values' array parameters.
 */
void R_TriangleBatch(R_Image *image, R_Target *target, unsigned short num_vertices, float *values,
                     unsigned int num_indices, unsigned short *indices, R_BatchFlagEnum flags);

/*! Renders triangles from the given set of vertices.  This lets you render arbitrary geometry.  It is a direct path to the GPU, so the format is different than typical SDL_gpu calls.
 * \param values A tightly-packed array of vertex position (e.g. x,y), texture coordinates (e.g. s,t), and color (e.g. r,g,b,a) values.  Texture coordinates and color values are expected to be already normalized to 0.0 - 1.0 (or 0 - 255 for 8-bit color components).  Pass NULL to render with only custom shader attributes.
 * \param indices If not NULL, this is used to specify which vertices to use and in what order (i.e. it indexes the vertices in the 'values' array).
 * \param flags Bit flags to control the interpretation of the 'values' array parameters.
 */
void R_TriangleBatchX(R_Image *image, R_Target *target, unsigned short num_vertices, void *values,
                      unsigned int num_indices, unsigned short *indices, R_BatchFlagEnum flags);

/*! Renders primitives from the given set of vertices.  This lets you render arbitrary geometry.  It is a direct path to the GPU, so the format is different than typical SDL_gpu calls.
 * \param primitive_type The kind of primitive to render.
 * \param values A tightly-packed array of vertex position (e.g. x,y), texture coordinates (e.g. s,t), and color (e.g. r,g,b,a) values.  Texture coordinates and color values are expected to be already normalized to 0.0 - 1.0 (or 0 - 255 for 8-bit color components).  Pass NULL to render with only custom shader attributes.
 * \param indices If not NULL, this is used to specify which vertices to use and in what order (i.e. it indexes the vertices in the 'values' array).
 * \param flags Bit flags to control the interpretation of the 'values' array parameters.
 */
void R_PrimitiveBatch(R_Image *image, R_Target *target, R_PrimitiveEnum primitive_type,
                      unsigned short num_vertices, float *values, unsigned int num_indices,
                      unsigned short *indices, R_BatchFlagEnum flags);

/*! Renders primitives from the given set of vertices.  This lets you render arbitrary geometry.  It is a direct path to the GPU, so the format is different than typical SDL_gpu calls.
 * \param primitive_type The kind of primitive to render.
 * \param values A tightly-packed array of vertex position (e.g. x,y), texture coordinates (e.g. s,t), and color (e.g. r,g,b,a) values.  Texture coordinates and color values are expected to be already normalized to 0.0 - 1.0 (or 0 - 255 for 8-bit color components).  Pass NULL to render with only custom shader attributes.
 * \param indices If not NULL, this is used to specify which vertices to use and in what order (i.e. it indexes the vertices in the 'values' array).
 * \param flags Bit flags to control the interpretation of the 'values' array parameters.
 */
void R_PrimitiveBatchV(R_Image *image, R_Target *target, R_PrimitiveEnum primitive_type,
                       unsigned short num_vertices, void *values, unsigned int num_indices,
                       unsigned short *indices, R_BatchFlagEnum flags);

/*! Send all buffered blitting data to the current context target. */
void R_FlushBlitBuffer(void);

/*! Updates the given target's associated window.  For non-context targets (e.g. image targets), this will flush the blit buffer. */
void R_Flip(R_Target *target);

// End of Rendering
/*! @} */

/*! \ingroup Shapes
 *  @{ */

/*! Renders a colored point.
 * \param target The destination render target
 * \param x x-coord of the point
 * \param y y-coord of the point
 * \param color The color of the shape to render
 */
void R_Pixel(R_Target *target, float x, float y, METAENGINE_Color color);

/*! Renders a colored line.
 * \param target The destination render target
 * \param x1 x-coord of starting point
 * \param y1 y-coord of starting point
 * \param x2 x-coord of ending point
 * \param y2 y-coord of ending point
 * \param color The color of the shape to render
 */
void R_Line(R_Target *target, float x1, float y1, float x2, float y2, METAENGINE_Color color);

/*! Renders a colored arc curve (circle segment).
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param radius The radius of the circle / distance from the center point that rendering will occur
 * \param start_angle The angle to start from, in degrees.  Measured clockwise from the positive x-axis.
 * \param end_angle The angle to end at, in degrees.  Measured clockwise from the positive x-axis.
 * \param color The color of the shape to render
 */
void R_Arc(R_Target *target, float x, float y, float radius, float start_angle, float end_angle,
           METAENGINE_Color color);

/*! Renders a colored filled arc (circle segment / pie piece).
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param radius The radius of the circle / distance from the center point that rendering will occur
 * \param start_angle The angle to start from, in degrees.  Measured clockwise from the positive x-axis.
 * \param end_angle The angle to end at, in degrees.  Measured clockwise from the positive x-axis.
 * \param color The color of the shape to render
 */
void R_ArcFilled(R_Target *target, float x, float y, float radius, float start_angle,
                 float end_angle, METAENGINE_Color color);

/*! Renders a colored circle outline.
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param radius The radius of the circle / distance from the center point that rendering will occur
 * \param color The color of the shape to render
 */
void R_Circle(R_Target *target, float x, float y, float radius, METAENGINE_Color color);

/*! Renders a colored filled circle.
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param radius The radius of the circle / distance from the center point that rendering will occur
 * \param color The color of the shape to render
 */
void R_CircleFilled(R_Target *target, float x, float y, float radius, METAENGINE_Color color);

/*! Renders a colored ellipse outline.
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param rx x-radius of ellipse
 * \param ry y-radius of ellipse
 * \param degrees The angle to rotate the ellipse
 * \param color The color of the shape to render
 */
void R_Ellipse(R_Target *target, float x, float y, float rx, float ry, float degrees,
               METAENGINE_Color color);

/*! Renders a colored filled ellipse.
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param rx x-radius of ellipse
 * \param ry y-radius of ellipse
 * \param degrees The angle to rotate the ellipse
 * \param color The color of the shape to render
 */
void R_EllipseFilled(R_Target *target, float x, float y, float rx, float ry, float degrees,
                     METAENGINE_Color color);

/*! Renders a colored annular sector outline (ring segment).
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param inner_radius The inner radius of the ring
 * \param outer_radius The outer radius of the ring
 * \param start_angle The angle to start from, in degrees.  Measured clockwise from the positive x-axis.
 * \param end_angle The angle to end at, in degrees.  Measured clockwise from the positive x-axis.
 * \param color The color of the shape to render
 */
void R_Sector(R_Target *target, float x, float y, float inner_radius, float outer_radius,
              float start_angle, float end_angle, METAENGINE_Color color);

/*! Renders a colored filled annular sector (ring segment).
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param inner_radius The inner radius of the ring
 * \param outer_radius The outer radius of the ring
 * \param start_angle The angle to start from, in degrees.  Measured clockwise from the positive x-axis.
 * \param end_angle The angle to end at, in degrees.  Measured clockwise from the positive x-axis.
 * \param color The color of the shape to render
 */
void R_SectorFilled(R_Target *target, float x, float y, float inner_radius, float outer_radius,
                    float start_angle, float end_angle, METAENGINE_Color color);

/*! Renders a colored triangle outline.
 * \param target The destination render target
 * \param x1 x-coord of first point
 * \param y1 y-coord of first point
 * \param x2 x-coord of second point
 * \param y2 y-coord of second point
 * \param x3 x-coord of third point
 * \param y3 y-coord of third point
 * \param color The color of the shape to render
 */
void R_Tri(R_Target *target, float x1, float y1, float x2, float y2, float x3, float y3,
           METAENGINE_Color color);

/*! Renders a colored filled triangle.
 * \param target The destination render target
 * \param x1 x-coord of first point
 * \param y1 y-coord of first point
 * \param x2 x-coord of second point
 * \param y2 y-coord of second point
 * \param x3 x-coord of third point
 * \param y3 y-coord of third point
 * \param color The color of the shape to render
 */
void R_TriFilled(R_Target *target, float x1, float y1, float x2, float y2, float x3, float y3,
                 METAENGINE_Color color);

/*! Renders a colored rectangle outline.
 * \param target The destination render target
 * \param x1 x-coord of top-left corner
 * \param y1 y-coord of top-left corner
 * \param x2 x-coord of bottom-right corner
 * \param y2 y-coord of bottom-right corner
 * \param color The color of the shape to render
 */
void R_Rectangle(R_Target *target, float x1, float y1, float x2, float y2, METAENGINE_Color color);

/*! Renders a colored rectangle outline.
 * \param target The destination render target
 * \param rect The rectangular area to draw
 * \param color The color of the shape to render
 */
void R_Rectangle2(R_Target *target, R_Rect rect, METAENGINE_Color color);

/*! Renders a colored filled rectangle.
 * \param target The destination render target
 * \param x1 x-coord of top-left corner
 * \param y1 y-coord of top-left corner
 * \param x2 x-coord of bottom-right corner
 * \param y2 y-coord of bottom-right corner
 * \param color The color of the shape to render
 */
void R_RectangleFilled(R_Target *target, float x1, float y1, float x2, float y2,
                       METAENGINE_Color color);

/*! Renders a colored filled rectangle.
 * \param target The destination render target
 * \param rect The rectangular area to draw
 * \param color The color of the shape to render
 */
void R_RectangleFilled2(R_Target *target, R_Rect rect, METAENGINE_Color color);

/*! Renders a colored rounded (filleted) rectangle outline.
 * \param target The destination render target
 * \param x1 x-coord of top-left corner
 * \param y1 y-coord of top-left corner
 * \param x2 x-coord of bottom-right corner
 * \param y2 y-coord of bottom-right corner
 * \param radius The radius of the corners
 * \param color The color of the shape to render
 */
void R_RectangleRound(R_Target *target, float x1, float y1, float x2, float y2, float radius,
                      METAENGINE_Color color);

/*! Renders a colored rounded (filleted) rectangle outline.
 * \param target The destination render target
 * \param rect The rectangular area to draw
 * \param radius The radius of the corners
 * \param color The color of the shape to render
 */
void R_RectangleRound2(R_Target *target, R_Rect rect, float radius, METAENGINE_Color color);

/*! Renders a colored filled rounded (filleted) rectangle.
 * \param target The destination render target
 * \param x1 x-coord of top-left corner
 * \param y1 y-coord of top-left corner
 * \param x2 x-coord of bottom-right corner
 * \param y2 y-coord of bottom-right corner
 * \param radius The radius of the corners
 * \param color The color of the shape to render
 */
void R_RectangleRoundFilled(R_Target *target, float x1, float y1, float x2, float y2, float radius,
                            METAENGINE_Color color);

/*! Renders a colored filled rounded (filleted) rectangle.
 * \param target The destination render target
 * \param rect The rectangular area to draw
 * \param radius The radius of the corners
 * \param color The color of the shape to render
 */
void R_RectangleRoundFilled2(R_Target *target, R_Rect rect, float radius, METAENGINE_Color color);

/*! Renders a colored polygon outline.  The vertices are expected to define a convex polygon.
 * \param target The destination render target
 * \param num_vertices Number of vertices (x and y pairs)
 * \param vertices An array of vertex positions stored as interlaced x and y coords, e.g. {x1, y1, x2, y2, ...}
 * \param color The color of the shape to render
 */
void R_Polygon(R_Target *target, unsigned int num_vertices, float *vertices,
               METAENGINE_Color color);

/*! Renders a colored sequence of line segments.
 * \param target The destination render target
 * \param num_vertices Number of vertices (x and y pairs)
 * \param vertices An array of vertex positions stored as interlaced x and y coords, e.g. {x1, y1, x2, y2, ...}
 * \param color The color of the shape to render
 * \param close_loop Make a closed polygon by drawing a line at the end back to the start point
 */
void R_Polyline(R_Target *target, unsigned int num_vertices, float *vertices,
                METAENGINE_Color color, R_bool close_loop);

/*! Renders a colored filled polygon.  The vertices are expected to define a convex polygon.
 * \param target The destination render target
 * \param num_vertices Number of vertices (x and y pairs)
 * \param vertices An array of vertex positions stored as interlaced x and y coords, e.g. {x1, y1, x2, y2, ...}
 * \param color The color of the shape to render
 */
void R_PolygonFilled(R_Target *target, unsigned int num_vertices, float *vertices,
                     METAENGINE_Color color);

// End of Shapes
/*! @} */

/*! \ingroup ShaderInterface
 *  @{ */

/*! Creates a new, empty shader program.  You will need to compile shaders, attach them to the program, then link the program.
 * \see R_AttachShader
 * \see R_LinkShaderProgram
 */
U32 R_CreateShaderProgram(void);

/*! Deletes a shader program. */
void R_FreeShaderProgram(U32 program_object);

/*! Compiles shader source and returns the new shader object. */
U32 R_CompileShader(R_ShaderEnum shader_type, const char *shader_source);

/*! Creates and links a shader program with the given shader objects. */
U32 R_LinkShaders(U32 shader_object1, U32 shader_object2);

/*! Creates and links a shader program with the given shader objects. */
U32 R_LinkManyShaders(U32 *shader_objects, int count);

/*! Deletes a shader object. */
void R_FreeShader(U32 shader_object);

/*! Attaches a shader object to a shader program for future linking. */
void R_AttachShader(U32 program_object, U32 shader_object);

/*! Detaches a shader object from a shader program. */
void R_DetachShader(U32 program_object, U32 shader_object);

/*! Links a shader program with any attached shader objects. */
R_bool R_LinkShaderProgram(U32 program_object);

/*! \return The current shader program */
U32 R_GetCurrentShaderProgram(void);

/*! Returns 1 if the given shader program is a default shader for the current context, 0 otherwise. */
R_bool R_IsDefaultShaderProgram(U32 program_object);

/*! Activates the given shader program.  Passing NULL for 'block' will disable the built-in shader variables for custom shaders until a R_ShaderBlock is set again. */
void R_ActivateShaderProgram(U32 program_object, R_ShaderBlock *block);

/*! Deactivates the current shader program (activates program 0). */
void R_DeactivateShaderProgram(void);

/*! Returns the last shader log message. */
const char *R_GetShaderMessage(void);

/*! Returns an integer representing the location of the specified attribute shader variable. */
int R_GetAttributeLocation(U32 program_object, const char *attrib_name);

/*! Returns a filled R_AttributeFormat object. */
R_AttributeFormat R_MakeAttributeFormat(int num_elems_per_vertex, R_TypeEnum type, R_bool normalize,
                                        int stride_bytes, int offset_bytes);

/*! Returns a filled R_Attribute object. */
R_Attribute R_MakeAttribute(int location, void *values, R_AttributeFormat format);

/*! Returns an integer representing the location of the specified uniform shader variable. */
int R_GetUniformLocation(U32 program_object, const char *uniform_name);

/*! Loads the given shader program's built-in attribute and uniform locations. */
R_ShaderBlock R_LoadShaderBlock(U32 program_object, const char *position_name,
                                const char *texcoord_name, const char *color_name,
                                const char *modelViewMatrix_name);

/*! Sets the current shader block to use the given attribute and uniform locations. */
void R_SetShaderBlock(R_ShaderBlock block);

/*! Gets the shader block for the current shader. */
R_ShaderBlock R_GetShaderBlock(void);

/*! Sets the given image unit to the given image so that a custom shader can sample multiple textures.
    \param image The source image/texture.  Pass NULL to disable the image unit.
    \param location The uniform location of a texture sampler
    \param image_unit The index of the texture unit to set.  0 is the first unit, which is used by SDL_gpu's blitting functions.  1 would be the second unit. */
void R_SetShaderImage(R_Image *image, int location, int image_unit);

/*! Fills "values" with the value of the uniform shader variable at the given location. */
void R_GetUniformiv(U32 program_object, int location, int *values);

/*! Sets the value of the integer uniform shader variable at the given location.
    This is equivalent to calling R_SetUniformiv(location, 1, 1, &value). */
void R_SetUniformi(int location, int value);

/*! Sets the value of the integer uniform shader variable at the given location. */
void R_SetUniformiv(int location, int num_elements_per_value, int num_values, int *values);

/*! Fills "values" with the value of the uniform shader variable at the given location. */
void R_GetUniformuiv(U32 program_object, int location, unsigned int *values);

/*! Sets the value of the unsigned integer uniform shader variable at the given location.
    This is equivalent to calling R_SetUniformuiv(location, 1, 1, &value). */
void R_SetUniformui(int location, unsigned int value);

/*! Sets the value of the unsigned integer uniform shader variable at the given location. */
void R_SetUniformuiv(int location, int num_elements_per_value, int num_values,
                     unsigned int *values);

/*! Fills "values" with the value of the uniform shader variable at the given location. */
void R_GetUniformfv(U32 program_object, int location, float *values);

/*! Sets the value of the floating point uniform shader variable at the given location.
    This is equivalent to calling R_SetUniformfv(location, 1, 1, &value). */
void R_SetUniformf(int location, float value);

/*! Sets the value of the floating point uniform shader variable at the given location. */
void R_SetUniformfv(int location, int num_elements_per_value, int num_values, float *values);

/*! Fills "values" with the value of the uniform shader variable at the given location.  The results are identical to calling R_GetUniformfv().  Matrices are gotten in column-major order. */
void R_GetUniformMatrixfv(U32 program_object, int location, float *values);

/*! Sets the value of the matrix uniform shader variable at the given location.  The size of the matrices sent is specified by num_rows and num_columns.  Rows and columns must be between 2 and 4. */
void R_SetUniformMatrixfv(int location, int num_matrices, int num_rows, int num_columns,
                          R_bool transpose, float *values);

/*! Sets a constant-value shader attribute that will be used for each rendered vertex. */
void R_SetAttributef(int location, float value);

/*! Sets a constant-value shader attribute that will be used for each rendered vertex. */
void R_SetAttributei(int location, int value);

/*! Sets a constant-value shader attribute that will be used for each rendered vertex. */
void R_SetAttributeui(int location, unsigned int value);

/*! Sets a constant-value shader attribute that will be used for each rendered vertex. */
void R_SetAttributefv(int location, int num_elements, float *value);

/*! Sets a constant-value shader attribute that will be used for each rendered vertex. */
void R_SetAttributeiv(int location, int num_elements, int *value);

/*! Sets a constant-value shader attribute that will be used for each rendered vertex. */
void R_SetAttributeuiv(int location, int num_elements, unsigned int *value);

/*! Enables a shader attribute and sets its source data. */
void R_SetAttributeSource(int num_values, R_Attribute source);

// End of ShaderInterface
/*! @} */

#define R_Text_NULL 0
#define R_Text_NULL_HANDLE 0

#define R_Text_LEFT 0
#define R_Text_TOP 0

#define R_Text_CENTER 1

#define R_Text_RIGHT 2
#define R_Text_BOTTOM 2

static GLboolean R_Text_Initialized = GL_FALSE;

typedef struct R_Texttext R_Texttext;

GLboolean R_Text_Init(void);
void R_Text_Terminate(void);

R_Texttext *R_Text_CreateText(void);
void R_Text_DeleteText(R_Texttext *text);
#define R_Text_DestroyText R_Text_DeleteText

R_bool R_Text_SetText(R_Texttext *text, const char *string);
const char *R_Text_GetText(R_Texttext *text);

void R_Text_Viewport(GLsizei width, GLsizei height);

void R_Text_BeginDraw();
void R_Text_EndDraw();

void R_Text_DrawText(R_Texttext *text, const GLfloat mvp[16]);

void R_Text_DrawText2D(R_Texttext *text, GLfloat x, GLfloat y, GLfloat scale);
void R_Text_DrawText2DAligned(R_Texttext *text, GLfloat x, GLfloat y, GLfloat scale,
                              int horizontalAlignment, int verticalAlignment);

void R_Text_DrawText3D(R_Texttext *text, GLfloat x, GLfloat y, GLfloat z, GLfloat scale,
                       GLfloat view[16], GLfloat projection[16]);

void R_Text_Color(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void R_Text_GetColor(GLfloat *r, GLfloat *g, GLfloat *b, GLfloat *a);

GLfloat R_Text_GetLineHeight(GLfloat scale);

GLfloat R_Text_GetTextWidth(const R_Texttext *text, GLfloat scale);
GLfloat R_Text_GetTextHeight(const R_Texttext *text, GLfloat scale);

GLboolean R_Text_IsCharacterSupported(const char c);
GLint R_Text_CountSupportedCharacters(const char *str);

GLboolean R_Text_IsCharacterDrawable(const char c);
GLint R_Text_CountDrawableCharacters(const char *str);

GLint R_Text_CountNewLines(const char *str);

// --------------------------------------------------------------------

// --------------------------------------------------------------------

// Internal API for managing window mappings
void R_AddWindowMapping(R_Target *target);
void R_RemoveWindowMapping(U32 windowID);
void R_RemoveWindowMappingByTarget(R_Target *target);

/*! Private implementation of renderer members. */
typedef struct R_RendererImpl
{
    /*! \see R_Init()
	 *  \see R_InitRenderer()
	 *  \see R_InitRendererByID()
	 */
    R_Target *(*Init)(R_Renderer *renderer, R_RendererID renderer_request, U16 w, U16 h,
                      R_WindowFlagEnum SDL_flags);

    /*! \see R_CreateTargetFromWindow
     * The extra parameter is used internally to reuse/reinit a target. */
    R_Target *(*CreateTargetFromWindow)(R_Renderer *renderer, U32 windowID, R_Target *target);

    /*! \see R_SetActiveTarget() */
    R_bool (*SetActiveTarget)(R_Renderer *renderer, R_Target *target);

    /*! \see R_CreateAliasTarget() */
    R_Target *(*CreateAliasTarget)(R_Renderer *renderer, R_Target *target);

    /*! \see R_MakeCurrent */
    void (*MakeCurrent)(R_Renderer *renderer, R_Target *target, U32 windowID);

    /*! Sets up this renderer to act as the current renderer.  Called automatically by R_SetCurrentRenderer(). */
    void (*SetAsCurrent)(R_Renderer *renderer);

    /*! \see R_ResetRendererState() */
    void (*ResetRendererState)(R_Renderer *renderer);

    /*! \see R_AddDepthBuffer() */
    R_bool (*AddDepthBuffer)(R_Renderer *renderer, R_Target *target);

    /*! \see R_SetWindowResolution() */
    R_bool (*SetWindowResolution)(R_Renderer *renderer, U16 w, U16 h);

    /*! \see R_SetVirtualResolution() */
    void (*SetVirtualResolution)(R_Renderer *renderer, R_Target *target, U16 w, U16 h);

    /*! \see R_UnsetVirtualResolution() */
    void (*UnsetVirtualResolution)(R_Renderer *renderer, R_Target *target);

    /*! Clean up the renderer state. */
    void (*Quit)(R_Renderer *renderer);

    /*! \see R_SetFullscreen() */
    R_bool (*SetFullscreen)(R_Renderer *renderer, R_bool enable_fullscreen,
                            R_bool use_desktop_resolution);

    /*! \see R_SetCamera() */
    R_Camera (*SetCamera)(R_Renderer *renderer, R_Target *target, R_Camera *cam);

    /*! \see R_CreateImage() */
    R_Image *(*CreateImage)(R_Renderer *renderer, U16 w, U16 h, R_FormatEnum format);

    /*! \see R_CreateImageUsingTexture() */
    R_Image *(*CreateImageUsingTexture)(R_Renderer *renderer, R_TextureHandle handle,
                                        R_bool take_ownership);

    /*! \see R_CreateAliasImage() */
    R_Image *(*CreateAliasImage)(R_Renderer *renderer, R_Image *image);

    /*! \see R_CopyImage() */
    R_Image *(*CopyImage)(R_Renderer *renderer, R_Image *image);

    /*! \see R_UpdateImage */
    void (*UpdateImage)(R_Renderer *renderer, R_Image *image, const R_Rect *image_rect,
                        void *surface, const R_Rect *surface_rect);

    /*! \see R_UpdateImageBytes */
    void (*UpdateImageBytes)(R_Renderer *renderer, R_Image *image, const R_Rect *image_rect,
                             const unsigned char *bytes, int bytes_per_row);

    /*! \see R_ReplaceImage */
    R_bool (*ReplaceImage)(R_Renderer *renderer, R_Image *image, void *surface,
                           const R_Rect *surface_rect);

    /*! \see R_CopyImageFromSurface() */
    R_Image *(*CopyImageFromSurface)(R_Renderer *renderer, void *surface,
                                     const R_Rect *surface_rect);

    /*! \see R_CopyImageFromTarget() */
    R_Image *(*CopyImageFromTarget)(R_Renderer *renderer, R_Target *target);

    /*! \see R_CopySurfaceFromTarget() */
    void *(*CopySurfaceFromTarget)(R_Renderer *renderer, R_Target *target);

    /*! \see R_CopySurfaceFromImage() */
    void *(*CopySurfaceFromImage)(R_Renderer *renderer, R_Image *image);

    /*! \see R_FreeImage() */
    void (*FreeImage)(R_Renderer *renderer, R_Image *image);

    /*! \see R_GetTarget() */
    R_Target *(*GetTarget)(R_Renderer *renderer, R_Image *image);

    /*! \see R_FreeTarget() */
    void (*FreeTarget)(R_Renderer *renderer, R_Target *target);

    /*! \see R_Blit() */
    void (*Blit)(R_Renderer *renderer, R_Image *image, R_Rect *src_rect, R_Target *target, float x,
                 float y);

    /*! \see R_BlitRotate() */
    void (*BlitRotate)(R_Renderer *renderer, R_Image *image, R_Rect *src_rect, R_Target *target,
                       float x, float y, float degrees);

    /*! \see R_BlitScale() */
    void (*BlitScale)(R_Renderer *renderer, R_Image *image, R_Rect *src_rect, R_Target *target,
                      float x, float y, float scaleX, float scaleY);

    /*! \see R_BlitTransform */
    void (*BlitTransform)(R_Renderer *renderer, R_Image *image, R_Rect *src_rect, R_Target *target,
                          float x, float y, float degrees, float scaleX, float scaleY);

    /*! \see R_BlitTransformX() */
    void (*BlitTransformX)(R_Renderer *renderer, R_Image *image, R_Rect *src_rect, R_Target *target,
                           float x, float y, float pivot_x, float pivot_y, float degrees,
                           float scaleX, float scaleY);

    /*! \see R_PrimitiveBatchV() */
    void (*PrimitiveBatchV)(R_Renderer *renderer, R_Image *image, R_Target *target,
                            R_PrimitiveEnum primitive_type, unsigned short num_vertices,
                            void *values, unsigned int num_indices, unsigned short *indices,
                            R_BatchFlagEnum flags);

    /*! \see R_GenerateMipmaps() */
    void (*GenerateMipmaps)(R_Renderer *renderer, R_Image *image);

    /*! \see R_SetClip() */
    R_Rect (*SetClip)(R_Renderer *renderer, R_Target *target, I16 x, I16 y, U16 w, U16 h);

    /*! \see R_UnsetClip() */
    void (*UnsetClip)(R_Renderer *renderer, R_Target *target);

    /*! \see R_GetPixel() */
    METAENGINE_Color (*GetPixel)(R_Renderer *renderer, R_Target *target, I16 x, I16 y);

    /*! \see R_SetImageFilter() */
    void (*SetImageFilter)(R_Renderer *renderer, R_Image *image, R_FilterEnum filter);

    /*! \see R_SetWrapMode() */
    void (*SetWrapMode)(R_Renderer *renderer, R_Image *image, R_WrapEnum wrap_mode_x,
                        R_WrapEnum wrap_mode_y);

    /*! \see R_GetTextureHandle() */
    R_TextureHandle (*GetTextureHandle)(R_Renderer *renderer, R_Image *image);

    /*! \see R_ClearRGBA() */
    void (*ClearRGBA)(R_Renderer *renderer, R_Target *target, U8 r, U8 g, U8 b, U8 a);
    /*! \see R_FlushBlitBuffer() */
    void (*FlushBlitBuffer)(R_Renderer *renderer);
    /*! \see R_Flip() */
    void (*Flip)(R_Renderer *renderer, R_Target *target);

    /*! \see R_CreateShaderProgram() */
    U32 (*CreateShaderProgram)(R_Renderer *renderer);

    /*! \see R_FreeShaderProgram() */
    void (*FreeShaderProgram)(R_Renderer *renderer, U32 program_object);

    U32 (*CompileShaderInternal)(R_Renderer *renderer, R_ShaderEnum shader_type,
                                    const char *shader_source);

    /*! \see R_CompileShader() */
    U32 (*CompileShader)(R_Renderer *renderer, R_ShaderEnum shader_type,
                            const char *shader_source);

    /*! \see R_FreeShader() */
    void (*FreeShader)(R_Renderer *renderer, U32 shader_object);

    /*! \see R_AttachShader() */
    void (*AttachShader)(R_Renderer *renderer, U32 program_object, U32 shader_object);

    /*! \see R_DetachShader() */
    void (*DetachShader)(R_Renderer *renderer, U32 program_object, U32 shader_object);

    /*! \see R_LinkShaderProgram() */
    R_bool (*LinkShaderProgram)(R_Renderer *renderer, U32 program_object);

    /*! \see R_ActivateShaderProgram() */
    void (*ActivateShaderProgram)(R_Renderer *renderer, U32 program_object,
                                  R_ShaderBlock *block);

    /*! \see R_DeactivateShaderProgram() */
    void (*DeactivateShaderProgram)(R_Renderer *renderer);

    /*! \see R_GetShaderMessage() */
    const char *(*GetShaderMessage)(R_Renderer *renderer);

    /*! \see R_GetAttribLocation() */
    int (*GetAttributeLocation)(R_Renderer *renderer, U32 program_object,
                                const char *attrib_name);

    /*! \see R_GetUniformLocation() */
    int (*GetUniformLocation)(R_Renderer *renderer, U32 program_object,
                              const char *uniform_name);

    /*! \see R_LoadShaderBlock() */
    R_ShaderBlock (*LoadShaderBlock)(R_Renderer *renderer, U32 program_object,
                                     const char *position_name, const char *texcoord_name,
                                     const char *color_name, const char *modelViewMatrix_name);

    /*! \see R_SetShaderBlock() */
    void (*SetShaderBlock)(R_Renderer *renderer, R_ShaderBlock block);

    /*! \see R_SetShaderImage() */
    void (*SetShaderImage)(R_Renderer *renderer, R_Image *image, int location, int image_unit);

    /*! \see R_GetUniformiv() */
    void (*GetUniformiv)(R_Renderer *renderer, U32 program_object, int location, int *values);

    /*! \see R_SetUniformi() */
    void (*SetUniformi)(R_Renderer *renderer, int location, int value);

    /*! \see R_SetUniformiv() */
    void (*SetUniformiv)(R_Renderer *renderer, int location, int num_elements_per_value,
                         int num_values, int *values);

    /*! \see R_GetUniformuiv() */
    void (*GetUniformuiv)(R_Renderer *renderer, U32 program_object, int location,
                          unsigned int *values);

    /*! \see R_SetUniformui() */
    void (*SetUniformui)(R_Renderer *renderer, int location, unsigned int value);

    /*! \see R_SetUniformuiv() */
    void (*SetUniformuiv)(R_Renderer *renderer, int location, int num_elements_per_value,
                          int num_values, unsigned int *values);

    /*! \see R_GetUniformfv() */
    void (*GetUniformfv)(R_Renderer *renderer, U32 program_object, int location, float *values);

    /*! \see R_SetUniformf() */
    void (*SetUniformf)(R_Renderer *renderer, int location, float value);

    /*! \see R_SetUniformfv() */
    void (*SetUniformfv)(R_Renderer *renderer, int location, int num_elements_per_value,
                         int num_values, float *values);

    /*! \see R_SetUniformMatrixfv() */
    void (*SetUniformMatrixfv)(R_Renderer *renderer, int location, int num_matrices, int num_rows,
                               int num_columns, R_bool transpose, float *values);

    /*! \see R_SetAttributef() */
    void (*SetAttributef)(R_Renderer *renderer, int location, float value);

    /*! \see R_SetAttributei() */
    void (*SetAttributei)(R_Renderer *renderer, int location, int value);

    /*! \see R_SetAttributeui() */
    void (*SetAttributeui)(R_Renderer *renderer, int location, unsigned int value);

    /*! \see R_SetAttributefv() */
    void (*SetAttributefv)(R_Renderer *renderer, int location, int num_elements, float *value);

    /*! \see R_SetAttributeiv() */
    void (*SetAttributeiv)(R_Renderer *renderer, int location, int num_elements, int *value);

    /*! \see R_SetAttributeuiv() */
    void (*SetAttributeuiv)(R_Renderer *renderer, int location, int num_elements,
                            unsigned int *value);

    /*! \see R_SetAttributeSource() */
    void (*SetAttributeSource)(R_Renderer *renderer, int num_values, R_Attribute source);

    // Shapes

    /*! \see R_SetLineThickness() */
    float (*SetLineThickness)(R_Renderer *renderer, float thickness);

    /*! \see R_GetLineThickness() */
    float (*GetLineThickness)(R_Renderer *renderer);

    /*! \see R_Pixel() */
    void (*Pixel)(R_Renderer *renderer, R_Target *target, float x, float y, METAENGINE_Color color);

    /*! \see R_Line() */
    void (*Line)(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2,
                 METAENGINE_Color color);

    /*! \see R_Arc() */
    void (*Arc)(R_Renderer *renderer, R_Target *target, float x, float y, float radius,
                float start_angle, float end_angle, METAENGINE_Color color);

    /*! \see R_ArcFilled() */
    void (*ArcFilled)(R_Renderer *renderer, R_Target *target, float x, float y, float radius,
                      float start_angle, float end_angle, METAENGINE_Color color);

    /*! \see R_Circle() */
    void (*Circle)(R_Renderer *renderer, R_Target *target, float x, float y, float radius,
                   METAENGINE_Color color);

    /*! \see R_CircleFilled() */
    void (*CircleFilled)(R_Renderer *renderer, R_Target *target, float x, float y, float radius,
                         METAENGINE_Color color);

    /*! \see R_Ellipse() */
    void (*Ellipse)(R_Renderer *renderer, R_Target *target, float x, float y, float rx, float ry,
                    float degrees, METAENGINE_Color color);

    /*! \see R_EllipseFilled() */
    void (*EllipseFilled)(R_Renderer *renderer, R_Target *target, float x, float y, float rx,
                          float ry, float degrees, METAENGINE_Color color);

    /*! \see R_Sector() */
    void (*Sector)(R_Renderer *renderer, R_Target *target, float x, float y, float inner_radius,
                   float outer_radius, float start_angle, float end_angle, METAENGINE_Color color);

    /*! \see R_SectorFilled() */
    void (*SectorFilled)(R_Renderer *renderer, R_Target *target, float x, float y,
                         float inner_radius, float outer_radius, float start_angle, float end_angle,
                         METAENGINE_Color color);

    /*! \see R_Tri() */
    void (*Tri)(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2, float y2,
                float x3, float y3, METAENGINE_Color color);

    /*! \see R_TriFilled() */
    void (*TriFilled)(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2,
                      float y2, float x3, float y3, METAENGINE_Color color);

    /*! \see R_Rectangle() */
    void (*Rectangle)(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2,
                      float y2, METAENGINE_Color color);

    /*! \see R_RectangleFilled() */
    void (*RectangleFilled)(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2,
                            float y2, METAENGINE_Color color);

    /*! \see R_RectangleRound() */
    void (*RectangleRound)(R_Renderer *renderer, R_Target *target, float x1, float y1, float x2,
                           float y2, float radius, METAENGINE_Color color);

    /*! \see R_RectangleRoundFilled() */
    void (*RectangleRoundFilled)(R_Renderer *renderer, R_Target *target, float x1, float y1,
                                 float x2, float y2, float radius, METAENGINE_Color color);

    /*! \see R_Polygon() */
    void (*Polygon)(R_Renderer *renderer, R_Target *target, unsigned int num_vertices,
                    float *vertices, METAENGINE_Color color);

    /*! \see R_Polyline() */
    void (*Polyline)(R_Renderer *renderer, R_Target *target, unsigned int num_vertices,
                     float *vertices, METAENGINE_Color color, R_bool close_loop);

    /*! \see R_PolygonFilled() */
    void (*PolygonFilled)(R_Renderer *renderer, R_Target *target, unsigned int num_vertices,
                          float *vertices, METAENGINE_Color color);

} R_RendererImpl;

#ifdef __cplusplus
}
#endif

#endif
