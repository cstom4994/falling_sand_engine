// Copyright(c) 2022, KaoruXun All rights reserved.

#ifndef _METAENGINE_Render_H__
#define _METAENGINE_Render_H__

#include "Core/Core.hpp"
#include "Engine/ImGuiImplement.hpp"
#include "Engine/Internal/BuiltinBox2d.h"
#include "Libs/glad/glad.h"
#include "external/stb_rect_pack.h"
#include "external/stb_truetype.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES// So M_PI and company get defined on MSVC when we include math.h
#endif
#include <cmath>// Must be included before SDL.h, otherwise both try to define M_PI and we get a warning

#include <cstdarg>
#include <cstdio>
#include <functional>
#include <unordered_map>

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT 2

#define METAENGINE_ALPHA_TRANSPARENT 0

// Compile-time version info
#define METAENGINE_Render_VERSION_MAJOR 0
#define METAENGINE_Render_VERSION_MINOR 12
#define METAENGINE_Render_VERSION_PATCH 0

// Check for bool support
#ifdef __STDC_VERSION__
#define METAENGINE_Render_HAVE_STDC 1
#else
#define METAENGINE_Render_HAVE_STDC 0
#endif

#define METAENGINE_Render_HAVE_C99 (METAENGINE_Render_HAVE_STDC && (__STDC_VERSION__ >= 199901L))

#ifdef __GNUC__// catches both gcc and clang I believe
#define METAENGINE_Render_HAVE_GNUC 1
#else
#define METAENGINE_Render_HAVE_GNUC 0
#endif

#ifdef _MSC_VER
#define METAENGINE_Render_HAVE_MSVC 1
#else
#define METAENGINE_Render_HAVE_MSVC 0
#endif

#define METAENGINE_Render_HAVE_MSVC18 (METAENGINE_Render_HAVE_MSVC && (_MSC_VER >= 1800))// VS2013+

#define METAENGINE_Render_bool bool

#if defined(_MSC_VER) || (defined(__INTEL_COMPILER) && defined(_WIN32))
#if defined(_M_X64)
#define METAENGINE_Render_BITNESS 64
#else
#define METAENGINE_Render_BITNESS 32
#endif
#define METAENGINE_Render_LONG_SIZE 4
#elif defined(__clang__) || defined(__INTEL_COMPILER) || defined(__GNUC__)
#if defined(__x86_64)
#define METAENGINE_Render_BITNESS 64
#else
#define METAENGINE_Render_BITNESS 32
#endif
#if __LONG_MAX__ == 2147483647L
#define METAENGINE_Render_LONG_SIZE 4
#else
#define METAENGINE_Render_LONG_SIZE 8
#endif
#endif

// Struct padding for 32 or 64 bit alignment
#if METAENGINE_Render_BITNESS == 32
#define METAENGINE_Render_PAD_1_TO_32 char _padding[1];
#define METAENGINE_Render_PAD_2_TO_32 char _padding[2];
#define METAENGINE_Render_PAD_3_TO_32 char _padding[3];
#define METAENGINE_Render_PAD_1_TO_64 char _padding[1];
#define METAENGINE_Render_PAD_2_TO_64 char _padding[2];
#define METAENGINE_Render_PAD_3_TO_64 char _padding[3];
#define METAENGINE_Render_PAD_4_TO_64
#define METAENGINE_Render_PAD_5_TO_64 char _padding[1];
#define METAENGINE_Render_PAD_6_TO_64 char _padding[2];
#define METAENGINE_Render_PAD_7_TO_64 char _padding[3];
#elif METAENGINE_Render_BITNESS == 64
#define METAENGINE_Render_PAD_1_TO_32 char _padding[1];
#define METAENGINE_Render_PAD_2_TO_32 char _padding[2];
#define METAENGINE_Render_PAD_3_TO_32 char _padding[3];
#define METAENGINE_Render_PAD_1_TO_64 char _padding[1];
#define METAENGINE_Render_PAD_2_TO_64 char _padding[2];
#define METAENGINE_Render_PAD_3_TO_64 char _padding[3];
#define METAENGINE_Render_PAD_4_TO_64 char _padding[4];
#define METAENGINE_Render_PAD_5_TO_64 char _padding[5];
#define METAENGINE_Render_PAD_6_TO_64 char _padding[6];
#define METAENGINE_Render_PAD_7_TO_64 char _padding[7];
#endif

#define METAENGINE_Render_FALSE 0
#define METAENGINE_Render_TRUE 1

typedef struct METAENGINE_Color
{
    UInt8 r;
    UInt8 g;
    UInt8 b;
    UInt8 a;
} METAENGINE_Color;

typedef struct METAENGINE_Render_Renderer METAENGINE_Render_Renderer;
typedef struct METAENGINE_Render_Target METAENGINE_Render_Target;

typedef struct METAENGINE_Render_Rect
{
    float x, y;
    float w, h;
} METAENGINE_Render_Rect;

#define METAENGINE_Render_RENDERER_ORDER_MAX 10

typedef UInt32 METAENGINE_Render_RendererEnum;
static const METAENGINE_Render_RendererEnum METAENGINE_Render_RENDERER_UNKNOWN = 0;// invalid value
static const METAENGINE_Render_RendererEnum METAENGINE_Render_RENDERER_OPENGL_3 = 1;
#define METAENGINE_Render_RENDERER_CUSTOM_0 1000

/*! \ingroup Initialization
 * \ingroup RendererSetup
 * \ingroup RendererControls
 * Renderer ID object for identifying a specific renderer.
 * \see METAENGINE_Render_MakeRendererID()
 * \see METAENGINE_Render_InitRendererByID()
 */
typedef struct METAENGINE_Render_RendererID
{
    const char *name;
    METAENGINE_Render_RendererEnum renderer;
    int major_version;
    int minor_version;

    METAENGINE_Render_PAD_4_TO_64
} METAENGINE_Render_RendererID;

/*! \ingroup TargetControls
 * Comparison operations (for depth testing)
 * \see METAENGINE_Render_SetDepthFunction()
 * Values chosen for direct OpenGL compatibility.
 */
typedef enum {
    METAENGINE_Render_NEVER = 0x0200,
    METAENGINE_Render_LESS = 0x0201,
    METAENGINE_Render_EQUAL = 0x0202,
    METAENGINE_Render_LEQUAL = 0x0203,
    METAENGINE_Render_GREATER = 0x0204,
    METAENGINE_Render_NOTEQUAL = 0x0205,
    METAENGINE_Render_GEQUAL = 0x0206,
    METAENGINE_Render_ALWAYS = 0x0207
} METAENGINE_Render_ComparisonEnum;

/*! \ingroup ImageControls
 * Blend component functions
 * \see METAENGINE_Render_SetBlendFunction()
 * Values chosen for direct OpenGL compatibility.
 */
typedef enum {
    METAENGINE_Render_FUNC_ZERO = 0,
    METAENGINE_Render_FUNC_ONE = 1,
    METAENGINE_Render_FUNC_SRC_COLOR = 0x0300,
    METAENGINE_Render_FUNC_DST_COLOR = 0x0306,
    METAENGINE_Render_FUNC_ONE_MINUS_SRC = 0x0301,
    METAENGINE_Render_FUNC_ONE_MINUS_DST = 0x0307,
    METAENGINE_Render_FUNC_SRC_ALPHA = 0x0302,
    METAENGINE_Render_FUNC_DST_ALPHA = 0x0304,
    METAENGINE_Render_FUNC_ONE_MINUS_SRC_ALPHA = 0x0303,
    METAENGINE_Render_FUNC_ONE_MINUS_DST_ALPHA = 0x0305
} METAENGINE_Render_BlendFuncEnum;

/*! \ingroup ImageControls
 * Blend component equations
 * \see METAENGINE_Render_SetBlendEquation()
 * Values chosen for direct OpenGL compatibility.
 */
typedef enum {
    METAENGINE_Render_EQ_ADD = 0x8006,
    METAENGINE_Render_EQ_SUBTRACT = 0x800A,
    METAENGINE_Render_EQ_REVERSE_SUBTRACT = 0x800B
} METAENGINE_Render_BlendEqEnum;

/*! \ingroup ImageControls
 * Blend mode storage struct */
typedef struct METAENGINE_Render_BlendMode
{
    METAENGINE_Render_BlendFuncEnum source_color;
    METAENGINE_Render_BlendFuncEnum dest_color;
    METAENGINE_Render_BlendFuncEnum source_alpha;
    METAENGINE_Render_BlendFuncEnum dest_alpha;

    METAENGINE_Render_BlendEqEnum color_equation;
    METAENGINE_Render_BlendEqEnum alpha_equation;
} METAENGINE_Render_BlendMode;

/*! \ingroup ImageControls
 * Blend mode presets 
 * \see METAENGINE_Render_SetBlendMode()
 * \see METAENGINE_Render_GetBlendModeFromPreset()
 */
typedef enum {
    METAENGINE_Render_BLEND_NORMAL = 0,
    METAENGINE_Render_BLEND_PREMULTIPLIED_ALPHA = 1,
    METAENGINE_Render_BLEND_MULTIPLY = 2,
    METAENGINE_Render_BLEND_ADD = 3,
    METAENGINE_Render_BLEND_SUBTRACT = 4,
    METAENGINE_Render_BLEND_MOD_ALPHA = 5,
    METAENGINE_Render_BLEND_SET_ALPHA = 6,
    METAENGINE_Render_BLEND_SET = 7,
    METAENGINE_Render_BLEND_NORMAL_KEEP_ALPHA = 8,
    METAENGINE_Render_BLEND_NORMAL_ADD_ALPHA = 9,
    METAENGINE_Render_BLEND_NORMAL_FACTOR_ALPHA = 10
} METAENGINE_Render_BlendPresetEnum;

/*! \ingroup ImageControls
 * Image filtering options.  These affect the quality/interpolation of colors when images are scaled. 
 * \see METAENGINE_Render_SetImageFilter()
 */
typedef enum {
    METAENGINE_Render_FILTER_NEAREST = 0,
    METAENGINE_Render_FILTER_LINEAR = 1,
    METAENGINE_Render_FILTER_LINEAR_MIPMAP = 2
} METAENGINE_Render_FilterEnum;

/*! \ingroup ImageControls
 * Snap modes.  Blitting with these modes will align the sprite with the target's pixel grid.
 * \see METAENGINE_Render_SetSnapMode()
 * \see METAENGINE_Render_GetSnapMode()
 */
typedef enum {
    METAENGINE_Render_SNAP_NONE = 0,
    METAENGINE_Render_SNAP_POSITION = 1,
    METAENGINE_Render_SNAP_DIMENSIONS = 2,
    METAENGINE_Render_SNAP_POSITION_AND_DIMENSIONS = 3
} METAENGINE_Render_SnapEnum;

/*! \ingroup ImageControls
 * Image wrapping options.  These affect how images handle src_rect coordinates beyond their dimensions when blitted.
 * \see METAENGINE_Render_SetWrapMode()
 */
typedef enum {
    METAENGINE_Render_WRAP_NONE = 0,
    METAENGINE_Render_WRAP_REPEAT = 1,
    METAENGINE_Render_WRAP_MIRRORED = 2
} METAENGINE_Render_WrapEnum;

/*! \ingroup ImageControls
 * Image format enum
 * \see METAENGINE_Render_CreateImage()
 */
typedef enum {
    METAENGINE_Render_FORMAT_LUMINANCE = 1,
    METAENGINE_Render_FORMAT_LUMINANCE_ALPHA = 2,
    METAENGINE_Render_FORMAT_RGB = 3,
    METAENGINE_Render_FORMAT_RGBA = 4,
    METAENGINE_Render_FORMAT_ALPHA = 5,
    METAENGINE_Render_FORMAT_RG = 6,
    METAENGINE_Render_FORMAT_YCbCr422 = 7,
    METAENGINE_Render_FORMAT_YCbCr420P = 8,
    METAENGINE_Render_FORMAT_BGR = 9,
    METAENGINE_Render_FORMAT_BGRA = 10,
    METAENGINE_Render_FORMAT_ABGR = 11
} METAENGINE_Render_FormatEnum;

typedef enum {
    METAENGINE_Render_FILE_AUTO = 0,
    METAENGINE_Render_FILE_PNG,
    METAENGINE_Render_FILE_BMP,
    METAENGINE_Render_FILE_TGA
} METAENGINE_Render_FileFormatEnum;

typedef struct METAENGINE_Render_Image
{
    struct METAENGINE_Render_Renderer *renderer;
    METAENGINE_Render_Target *context_target;
    METAENGINE_Render_Target *target;
    void *data;

    UInt16 w, h;
    METAENGINE_Render_FormatEnum format;
    int num_layers;
    int bytes_per_pixel;
    UInt16 base_w, base_h;      // Original image dimensions
    UInt16 texture_w, texture_h;// Underlying texture dimensions

    float anchor_x;// Normalized coords for the point at which the image is blitted.  Default is (0.5, 0.5), that is, the image is drawn centered.
    float anchor_y;// These are interpreted according to METAENGINE_Render_SetCoordinateMode() and range from (0.0 - 1.0) normally.

    METAENGINE_Color color;
    METAENGINE_Render_BlendMode blend_mode;
    METAENGINE_Render_FilterEnum filter_mode;
    METAENGINE_Render_SnapEnum snap_mode;
    METAENGINE_Render_WrapEnum wrap_mode_x;
    METAENGINE_Render_WrapEnum wrap_mode_y;

    int refcount;

    METAENGINE_Render_bool using_virtual_resolution;
    METAENGINE_Render_bool has_mipmaps;
    METAENGINE_Render_bool use_blending;
    METAENGINE_Render_bool is_alias;
} METAENGINE_Render_Image;

/*! \ingroup ImageControls
 * A backend-neutral type that is intended to hold a backend-specific handle/pointer to a texture.
 * \see METAENGINE_Render_CreateImageUsingTexture()
 * \see METAENGINE_Render_GetTextureHandle()
 */
typedef uintptr_t METAENGINE_Render_TextureHandle;

/*! \ingroup TargetControls
 * Camera object that determines viewing transform.
 * \see METAENGINE_Render_SetCamera() 
 * \see METAENGINE_Render_GetDefaultCamera() 
 * \see METAENGINE_Render_GetCamera()
 */
typedef struct METAENGINE_Render_Camera
{
    float x, y, z;
    float angle;
    float zoom_x, zoom_y;
    float z_near, z_far;// z clipping planes
    METAENGINE_Render_bool
            use_centered_origin;// move rotation/scaling origin to the center of the camera's view

    METAENGINE_Render_PAD_7_TO_64
} METAENGINE_Render_Camera;

/*! \ingroup ShaderInterface
 * Container for the built-in shader attribute and uniform locations (indices).
 * \see METAENGINE_Render_LoadShaderBlock()
 * \see METAENGINE_Render_SetShaderBlock()
 */
typedef struct METAENGINE_Render_ShaderBlock
{
    // Attributes
    int position_loc;
    int texcoord_loc;
    int color_loc;
    // Uniforms
    int modelViewProjection_loc;
} METAENGINE_Render_ShaderBlock;

#define METAENGINE_Render_MODEL 0
#define METAENGINE_Render_VIEW 1
#define METAENGINE_Render_PROJECTION 2

/*! \ingroup Matrix
 * Matrix stack data structure for global vertex transforms.  */
typedef struct METAENGINE_Render_MatrixStack
{
    unsigned int storage_size;
    unsigned int size;
    float **matrix;
} METAENGINE_Render_MatrixStack;

/*! \ingroup ContextControls
 * Rendering context data.  Only METAENGINE_Render_Targets which represent windows will store this. */
typedef struct METAENGINE_Render_Context
{
    /*! SDL_GLContext */
    void *context;

    /*! Last target used */
    METAENGINE_Render_Target *active_target;

    METAENGINE_Render_ShaderBlock current_shader_block;
    METAENGINE_Render_ShaderBlock default_textured_shader_block;
    METAENGINE_Render_ShaderBlock default_untextured_shader_block;

    /*! SDL window ID */
    UInt32 windowID;

    /*! Actual window dimensions */
    int window_w;
    int window_h;

    /*! Drawable region dimensions */
    int drawable_w;
    int drawable_h;

    /*! Window dimensions for restoring windowed mode after METAENGINE_Render_SetFullscreen(1,1). */
    int stored_window_w;
    int stored_window_h;

    /*! Shader handles used in the default shader programs */
    UInt32 default_textured_vertex_shader_id;
    UInt32 default_textured_fragment_shader_id;
    UInt32 default_untextured_vertex_shader_id;
    UInt32 default_untextured_fragment_shader_id;

    /*! Internal state */
    UInt32 current_shader_program;
    UInt32 default_textured_shader_program;
    UInt32 default_untextured_shader_program;

    METAENGINE_Render_BlendMode shapes_blend_mode;
    float line_thickness;

    int refcount;

    void *data;

    METAENGINE_Render_bool failed;
    METAENGINE_Render_bool use_texturing;
    METAENGINE_Render_bool shapes_use_blending;

    METAENGINE_Render_PAD_5_TO_64
} METAENGINE_Render_Context;

/*! \ingroup TargetControls
 * Render target object for use as a blitting destination.
 * A METAENGINE_Render_Target can be created from a METAENGINE_Render_Image with METAENGINE_Render_LoadTarget().
 * A METAENGINE_Render_Target can also represent a separate window with METAENGINE_Render_CreateTargetFromWindow().  In that case, 'context' is allocated and filled in.
 * Note: You must have passed the SDL_WINDOW_OPENGL flag to SDL_CreateWindow() for OpenGL renderers to work with new windows.
 * Free the memory with METAENGINE_Render_FreeTarget() when you're done.
 * \see METAENGINE_Render_LoadTarget()
 * \see METAENGINE_Render_CreateTargetFromWindow()
 * \see METAENGINE_Render_FreeTarget()
 */
struct METAENGINE_Render_Target
{
    struct METAENGINE_Render_Renderer *renderer;
    METAENGINE_Render_Target *context_target;
    METAENGINE_Render_Image *image;
    void *data;
    UInt16 w, h;
    UInt16 base_w, base_h;// The true dimensions of the underlying image or window
    METAENGINE_Render_Rect clip_rect;
    METAENGINE_Color color;

    METAENGINE_Render_Rect viewport;

    /*! Perspective and object viewing transforms. */
    int matrix_mode;
    METAENGINE_Render_MatrixStack projection_matrix;
    METAENGINE_Render_MatrixStack view_matrix;
    METAENGINE_Render_MatrixStack model_matrix;

    METAENGINE_Render_Camera camera;

    METAENGINE_Render_bool using_virtual_resolution;
    METAENGINE_Render_bool use_clip_rect;
    METAENGINE_Render_bool use_color;
    METAENGINE_Render_bool use_camera;

    METAENGINE_Render_ComparisonEnum depth_function;

    /*! Renderer context data.  NULL if the target does not represent a window or rendering context. */
    METAENGINE_Render_Context *context;
    int refcount;

    METAENGINE_Render_bool use_depth_test;
    METAENGINE_Render_bool use_depth_write;
    METAENGINE_Render_bool is_alias;

    METAENGINE_Render_PAD_1_TO_64
};

/*! \ingroup Initialization
 * Important GPU features which may not be supported depending on a device's extension support.  Can be bitwise OR'd together.
 * \see METAENGINE_Render_IsFeatureEnabled()
 * \see METAENGINE_Render_SetRequiredFeatures()
 */
typedef UInt32 METAENGINE_Render_FeatureEnum;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_NON_POWER_OF_TWO = 0x1;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_RENDER_TARGETS = 0x2;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_BLEND_EQUATIONS = 0x4;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_BLEND_FUNC_SEPARATE = 0x8;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_BLEND_EQUATIONS_SEPARATE =
        0x10;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_GL_BGR = 0x20;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_GL_BGRA = 0x40;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_GL_ABGR = 0x80;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_VERTEX_SHADER = 0x100;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_FRAGMENT_SHADER = 0x200;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_PIXEL_SHADER = 0x200;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_GEOMETRY_SHADER = 0x400;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_WRAP_REPEAT_MIRRORED = 0x800;
static const METAENGINE_Render_FeatureEnum METAENGINE_Render_FEATURE_CORE_FRAMEBUFFER_OBJECTS =
        0x1000;

/*! Combined feature flags */
#define METAENGINE_Render_FEATURE_ALL_BASE METAENGINE_Render_FEATURE_RENDER_TARGETS
#define METAENGINE_Render_FEATURE_ALL_BLEND_PRESETS                                                \
    (METAENGINE_Render_FEATURE_BLEND_EQUATIONS | METAENGINE_Render_FEATURE_BLEND_FUNC_SEPARATE)
#define METAENGINE_Render_FEATURE_ALL_GL_FORMATS                                                   \
    (METAENGINE_Render_FEATURE_GL_BGR | METAENGINE_Render_FEATURE_GL_BGRA |                        \
     METAENGINE_Render_FEATURE_GL_ABGR)
#define METAENGINE_Render_FEATURE_BASIC_SHADERS                                                    \
    (METAENGINE_Render_FEATURE_FRAGMENT_SHADER | METAENGINE_Render_FEATURE_VERTEX_SHADER)
#define METAENGINE_Render_FEATURE_ALL_SHADERS                                                      \
    (METAENGINE_Render_FEATURE_FRAGMENT_SHADER | METAENGINE_Render_FEATURE_VERTEX_SHADER |         \
     METAENGINE_Render_FEATURE_GEOMETRY_SHADER)

typedef UInt32 METAENGINE_Render_WindowFlagEnum;

/*! \ingroup Initialization
 * Initialization flags for changing default init parameters.  Can be bitwise OR'ed together.
 * Default (0) is to use late swap vsync and double buffering.
 * \see METAENGINE_Render_SetPreInitFlags()
 * \see METAENGINE_Render_GetPreInitFlags()
 */
typedef UInt32 METAENGINE_Render_InitFlagEnum;
static const METAENGINE_Render_InitFlagEnum METAENGINE_Render_INIT_ENABLE_VSYNC = 0x1;
static const METAENGINE_Render_InitFlagEnum METAENGINE_Render_INIT_DISABLE_VSYNC = 0x2;
static const METAENGINE_Render_InitFlagEnum METAENGINE_Render_INIT_DISABLE_DOUBLE_BUFFER = 0x4;
static const METAENGINE_Render_InitFlagEnum METAENGINE_Render_INIT_DISABLE_AUTO_VIRTUAL_RESOLUTION =
        0x8;
static const METAENGINE_Render_InitFlagEnum METAENGINE_Render_INIT_REQUEST_COMPATIBILITY_PROFILE =
        0x10;
static const METAENGINE_Render_InitFlagEnum
        METAENGINE_Render_INIT_USE_ROW_BY_ROW_TEXTURE_UPLOAD_FALLBACK = 0x20;
static const METAENGINE_Render_InitFlagEnum
        METAENGINE_Render_INIT_USE_COPY_TEXTURE_UPLOAD_FALLBACK = 0x40;

#define METAENGINE_Render_DEFAULT_INIT_FLAGS 0

static const UInt32 METAENGINE_Render_NONE = 0x0;

/*! \ingroup Rendering
 * Primitive types for rendering arbitrary geometry.  The values are intentionally identical to the GL_* primitives.
 * \see METAENGINE_Render_PrimitiveBatch()
 * \see METAENGINE_Render_PrimitiveBatchV()
 */
typedef UInt32 METAENGINE_Render_PrimitiveEnum;
static const METAENGINE_Render_PrimitiveEnum METAENGINE_Render_POINTS = 0x0;
static const METAENGINE_Render_PrimitiveEnum METAENGINE_Render_LINES = 0x1;
static const METAENGINE_Render_PrimitiveEnum METAENGINE_Render_LINE_LOOP = 0x2;
static const METAENGINE_Render_PrimitiveEnum METAENGINE_Render_LINE_STRIP = 0x3;
static const METAENGINE_Render_PrimitiveEnum METAENGINE_Render_TRIANGLES = 0x4;
static const METAENGINE_Render_PrimitiveEnum METAENGINE_Render_TRIANGLE_STRIP = 0x5;
static const METAENGINE_Render_PrimitiveEnum METAENGINE_Render_TRIANGLE_FAN = 0x6;

/*! Bit flags for geometry batching.
 * \see METAENGINE_Render_TriangleBatch()
 * \see METAENGINE_Render_TriangleBatchX()
 * \see METAENGINE_Render_PrimitiveBatch()
 * \see METAENGINE_Render_PrimitiveBatchV()
 */
typedef UInt32 METAENGINE_Render_BatchFlagEnum;
static const METAENGINE_Render_BatchFlagEnum METAENGINE_Render_BATCH_XY = 0x1;
static const METAENGINE_Render_BatchFlagEnum METAENGINE_Render_BATCH_XYZ = 0x2;
static const METAENGINE_Render_BatchFlagEnum METAENGINE_Render_BATCH_ST = 0x4;
static const METAENGINE_Render_BatchFlagEnum METAENGINE_Render_BATCH_RGB = 0x8;
static const METAENGINE_Render_BatchFlagEnum METAENGINE_Render_BATCH_RGBA = 0x10;
static const METAENGINE_Render_BatchFlagEnum METAENGINE_Render_BATCH_RGB8 = 0x20;
static const METAENGINE_Render_BatchFlagEnum METAENGINE_Render_BATCH_RGBA8 = 0x40;

#define METAENGINE_Render_BATCH_XY_ST (METAENGINE_Render_BATCH_XY | METAENGINE_Render_BATCH_ST)
#define METAENGINE_Render_BATCH_XYZ_ST (METAENGINE_Render_BATCH_XYZ | METAENGINE_Render_BATCH_ST)
#define METAENGINE_Render_BATCH_XY_RGB (METAENGINE_Render_BATCH_XY | METAENGINE_Render_BATCH_RGB)
#define METAENGINE_Render_BATCH_XYZ_RGB (METAENGINE_Render_BATCH_XYZ | METAENGINE_Render_BATCH_RGB)
#define METAENGINE_Render_BATCH_XY_RGBA (METAENGINE_Render_BATCH_XY | METAENGINE_Render_BATCH_RGBA)
#define METAENGINE_Render_BATCH_XYZ_RGBA                                                           \
    (METAENGINE_Render_BATCH_XYZ | METAENGINE_Render_BATCH_RGBA)
#define METAENGINE_Render_BATCH_XY_ST_RGBA                                                         \
    (METAENGINE_Render_BATCH_XY | METAENGINE_Render_BATCH_ST | METAENGINE_Render_BATCH_RGBA)
#define METAENGINE_Render_BATCH_XYZ_ST_RGBA                                                        \
    (METAENGINE_Render_BATCH_XYZ | METAENGINE_Render_BATCH_ST | METAENGINE_Render_BATCH_RGBA)
#define METAENGINE_Render_BATCH_XY_RGB8 (METAENGINE_Render_BATCH_XY | METAENGINE_Render_BATCH_RGB8)
#define METAENGINE_Render_BATCH_XYZ_RGB8                                                           \
    (METAENGINE_Render_BATCH_XYZ | METAENGINE_Render_BATCH_RGB8)
#define METAENGINE_Render_BATCH_XY_RGBA8                                                           \
    (METAENGINE_Render_BATCH_XY | METAENGINE_Render_BATCH_RGBA8)
#define METAENGINE_Render_BATCH_XYZ_RGBA8                                                          \
    (METAENGINE_Render_BATCH_XYZ | METAENGINE_Render_BATCH_RGBA8)
#define METAENGINE_Render_BATCH_XY_ST_RGBA8                                                        \
    (METAENGINE_Render_BATCH_XY | METAENGINE_Render_BATCH_ST | METAENGINE_Render_BATCH_RGBA8)
#define METAENGINE_Render_BATCH_XYZ_ST_RGBA8                                                       \
    (METAENGINE_Render_BATCH_XYZ | METAENGINE_Render_BATCH_ST | METAENGINE_Render_BATCH_RGBA8)

/*! Bit flags for blitting into a rectangular region.
 * \see METAENGINE_Render_BlitRect
 * \see METAENGINE_Render_BlitRectX
 */
typedef UInt32 METAENGINE_Render_FlipEnum;
static const METAENGINE_Render_FlipEnum METAENGINE_Render_FLIP_NONE = 0x0;
static const METAENGINE_Render_FlipEnum METAENGINE_Render_FLIP_HORIZONTAL = 0x1;
static const METAENGINE_Render_FlipEnum METAENGINE_Render_FLIP_VERTICAL = 0x2;

/*! \ingroup ShaderInterface
 * Type enumeration for METAENGINE_Render_AttributeFormat specifications.
 */
typedef UInt32 METAENGINE_Render_TypeEnum;
// Use OpenGL's values for simpler translation
static const METAENGINE_Render_TypeEnum METAENGINE_Render_TYPE_BYTE = 0x1400;
static const METAENGINE_Render_TypeEnum METAENGINE_Render_TYPE_UNSIGNED_BYTE = 0x1401;
static const METAENGINE_Render_TypeEnum METAENGINE_Render_TYPE_SHORT = 0x1402;
static const METAENGINE_Render_TypeEnum METAENGINE_Render_TYPE_UNSIGNED_SHORT = 0x1403;
static const METAENGINE_Render_TypeEnum METAENGINE_Render_TYPE_INT = 0x1404;
static const METAENGINE_Render_TypeEnum METAENGINE_Render_TYPE_UNSIGNED_INT = 0x1405;
static const METAENGINE_Render_TypeEnum METAENGINE_Render_TYPE_FLOAT = 0x1406;
static const METAENGINE_Render_TypeEnum METAENGINE_Render_TYPE_DOUBLE = 0x140A;

typedef enum {
    METAENGINE_Render_VERTEX_SHADER = 0,
    METAENGINE_Render_FRAGMENT_SHADER = 1,
    METAENGINE_Render_PIXEL_SHADER = 1,
    METAENGINE_Render_GEOMETRY_SHADER = 2
} METAENGINE_Render_ShaderEnum;

/*! \ingroup ShaderInterface
 * Type enumeration for the shader language used by the renderer.
 */
typedef enum {
    METAENGINE_Render_LANGUAGE_NONE = 0,
    METAENGINE_Render_LANGUAGE_ARB_ASSEMBLY = 1,
    METAENGINE_Render_LANGUAGE_GLSL = 2,
    METAENGINE_Render_LANGUAGE_GLSLES = 3,
    METAENGINE_Render_LANGUAGE_HLSL = 4,
    METAENGINE_Render_LANGUAGE_CG = 5
} METAENGINE_Render_ShaderLanguageEnum;

/*! \ingroup ShaderInterface */
typedef struct METAENGINE_Render_AttributeFormat
{
    int num_elems_per_value;
    METAENGINE_Render_TypeEnum
            type;// METAENGINE_Render_TYPE_FLOAT, METAENGINE_Render_TYPE_INT, METAENGINE_Render_TYPE_UNSIGNED_INT, etc.
    int stride_bytes;                    // Number of bytes between two vertex specifications
    int offset_bytes;                    // Number of bytes to skip at the beginning of 'values'
    METAENGINE_Render_bool is_per_sprite;// Per-sprite values are expanded to 4 vertices
    METAENGINE_Render_bool normalize;

    METAENGINE_Render_PAD_2_TO_32
} METAENGINE_Render_AttributeFormat;

/*! \ingroup ShaderInterface */
typedef struct METAENGINE_Render_Attribute
{
    void *values;// Expect 4 values for each sprite
    METAENGINE_Render_AttributeFormat format;
    int location;

    METAENGINE_Render_PAD_4_TO_64
} METAENGINE_Render_Attribute;

/*! \ingroup ShaderInterface */
typedef struct METAENGINE_Render_AttributeSource
{
    void *next_value;
    void *per_vertex_storage;// Could point to the attribute's values or to allocated storage

    int num_values;
    // Automatic storage format
    int per_vertex_storage_stride_bytes;
    int per_vertex_storage_offset_bytes;
    int per_vertex_storage_size;// Over 0 means that the per-vertex storage has been automatically allocated
    METAENGINE_Render_Attribute attribute;
    METAENGINE_Render_bool enabled;

    METAENGINE_Render_PAD_7_TO_64
} METAENGINE_Render_AttributeSource;

/*! \ingroup Logging
 * Type enumeration for error codes.
 * \see METAENGINE_Render_PushErrorCode()
 * \see METAENGINE_Render_PopErrorCode()
 */
typedef enum {
    METAENGINE_Render_ERROR_NONE = 0,
    METAENGINE_Render_ERROR_BACKEND_ERROR = 1,
    METAENGINE_Render_ERROR_DATA_ERROR = 2,
    METAENGINE_Render_ERROR_USER_ERROR = 3,
    METAENGINE_Render_ERROR_UNSUPPORTED_FUNCTION = 4,
    METAENGINE_Render_ERROR_NULL_ARGUMENT = 5,
    METAENGINE_Render_ERROR_FILE_NOT_FOUND = 6
} METAENGINE_Render_ErrorEnum;

/*! \ingroup Logging */
typedef struct METAENGINE_Render_ErrorObject
{
    char *function;
    char *details;
    METAENGINE_Render_ErrorEnum error;

    METAENGINE_Render_PAD_4_TO_64
} METAENGINE_Render_ErrorObject;

/* Private implementation of renderer members */
struct METAENGINE_Render_RendererImpl;

/*! Renderer object which specializes the API to a particular backend. */
struct METAENGINE_Render_Renderer
{
    /*! Struct identifier of the renderer. */
    METAENGINE_Render_RendererID id;
    METAENGINE_Render_RendererID requested_id;
    METAENGINE_Render_WindowFlagEnum SDL_init_flags;
    METAENGINE_Render_InitFlagEnum METAENGINE_Render_init_flags;

    METAENGINE_Render_ShaderLanguageEnum shader_language;
    int min_shader_version;
    int max_shader_version;
    METAENGINE_Render_FeatureEnum enabled_features;

    /*! Current display target */
    METAENGINE_Render_Target *current_context_target;

    /*! Default is (0.5, 0.5) - images draw centered. */
    float default_image_anchor_x;
    float default_image_anchor_y;

    struct METAENGINE_Render_RendererImpl *impl;

    /*! 0 for inverted, 1 for mathematical */
    METAENGINE_Render_bool coordinate_mode;

    METAENGINE_Render_PAD_7_TO_64
};

/*! The window corresponding to 'windowID' will be used to create the rendering context instead of creating a new window. */
void METAENGINE_Render_SetInitWindow(UInt32 windowID);

/*! Returns the window ID that has been set via METAENGINE_Render_SetInitWindow(). */
UInt32 METAENGINE_Render_GetInitWindow(void);

/*! Set special flags to use for initialization. Set these before calling METAENGINE_Render_Init().
 * \param METAENGINE_Render_flags An OR'ed combination of METAENGINE_Render_InitFlagEnum flags.  Default flags (0) enable late swap vsync and double buffering. */
void METAENGINE_Render_SetPreInitFlags(METAENGINE_Render_InitFlagEnum METAENGINE_Render_flags);

/*! Returns the current special flags to use for initialization. */
METAENGINE_Render_InitFlagEnum METAENGINE_Render_GetPreInitFlags(void);

/*! Set required features to use for initialization. Set these before calling METAENGINE_Render_Init().
 * \param features An OR'ed combination of METAENGINE_Render_FeatureEnum flags.  Required features will force METAENGINE_Render_Init() to create a renderer that supports all of the given flags or else fail. */
void METAENGINE_Render_SetRequiredFeatures(METAENGINE_Render_FeatureEnum features);

/*! Returns the current required features to use for initialization. */
METAENGINE_Render_FeatureEnum METAENGINE_Render_GetRequiredFeatures(void);

/*! Gets the default initialization renderer IDs for the current platform copied into the 'order' array and the number of renderer IDs into 'order_size'.  Pass NULL for 'order' to just get the size of the renderer order array.  Will return at most METAENGINE_Render_RENDERER_ORDER_MAX renderers. */
void METAENGINE_Render_GetDefaultRendererOrder(int *order_size,
                                               METAENGINE_Render_RendererID *order);

/*! Gets the current renderer ID order for initialization copied into the 'order' array and the number of renderer IDs into 'order_size'.  Pass NULL for 'order' to just get the size of the renderer order array. */
void METAENGINE_Render_GetRendererOrder(int *order_size, METAENGINE_Render_RendererID *order);

/*! Sets the renderer ID order to use for initialization.  If 'order' is NULL, it will restore the default order. */
void METAENGINE_Render_SetRendererOrder(int order_size, METAENGINE_Render_RendererID *order);

/*! Initializes SDL's video subsystem (if necessary) and all of SDL_gpu's internal structures.
 * Chooses a renderer and creates a window with the given dimensions and window creation flags.
 * A pointer to the resulting window's render target is returned.
 * 
 * \param w Desired window width in pixels
 * \param h Desired window height in pixels
 * \param SDL_flags The bit flags to pass to SDL when creating the window.  Use METAENGINE_Render_DEFAULT_INIT_FLAGS if you don't care.
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
 * \see METAENGINE_Render_RendererID
 * \see METAENGINE_Render_InitRenderer()
 * \see METAENGINE_Render_InitRendererByID()
 * \see METAENGINE_Render_SetRendererOrder()
 * \see METAENGINE_Render_PushErrorCode()
 */
METAENGINE_Render_Target *METAENGINE_Render_Init(UInt16 w, UInt16 h,
                                                 METAENGINE_Render_WindowFlagEnum SDL_flags);

/*! Initializes SDL and SDL_gpu.  Creates a window and the requested renderer context. */
METAENGINE_Render_Target *METAENGINE_Render_InitRenderer(
        METAENGINE_Render_RendererEnum renderer_enum, UInt16 w, UInt16 h,
        METAENGINE_Render_WindowFlagEnum SDL_flags);

/*! Initializes SDL and SDL_gpu.  Creates a window and the requested renderer context.
 * By requesting a renderer via ID, you can specify the major and minor versions of an individual renderer backend.
 * \see METAENGINE_Render_MakeRendererID
 */
METAENGINE_Render_Target *METAENGINE_Render_InitRendererByID(
        METAENGINE_Render_RendererID renderer_request, UInt16 w, UInt16 h,
        METAENGINE_Render_WindowFlagEnum SDL_flags);

/*! Checks for important GPU features which may not be supported depending on a device's extension support.  Feature flags (METAENGINE_Render_FEATURE_*) can be bitwise OR'd together.
 * \return 1 if all of the passed features are enabled/supported
 * \return 0 if any of the passed features are disabled/unsupported
 */
METAENGINE_Render_bool METAENGINE_Render_IsFeatureEnabled(METAENGINE_Render_FeatureEnum feature);

/*! Clean up the renderer state. */
void METAENGINE_Render_CloseCurrentRenderer(void);

/*! Clean up the renderer state and shut down SDL_gpu. */
void METAENGINE_Render_Quit(void);

// End of Initialization
/*! @} */

/*! Pushes a new error code into the error queue.  If the queue is full, the queue is not modified.
 * \param function The name of the function that pushed the error
 * \param error The error code to push on the error queue
 * \param details Additional information string, can be NULL.
 */
void METAENGINE_Render_PushErrorCode(const char *function, METAENGINE_Render_ErrorEnum error,
                                     const char *details, ...);

/*! Pops an error object from the error queue and returns it.  If the error queue is empty, it returns an error object with NULL function, METAENGINE_Render_ERROR_NONE error, and NULL details. */
METAENGINE_Render_ErrorObject METAENGINE_Render_PopErrorCode(void);

/*! Gets the string representation of an error code. */
const char *METAENGINE_Render_GetErrorString(METAENGINE_Render_ErrorEnum error);

/*! Changes the maximum number of error objects that SDL_gpu will store.  This deletes all currently stored errors. */
void METAENGINE_Render_SetErrorQueueMax(unsigned int max);

// End of Logging
/*! @} */

/*! \ingroup RendererSetup
 *  @{ */

/*! Returns an initialized METAENGINE_Render_RendererID. */
METAENGINE_Render_RendererID METAENGINE_Render_MakeRendererID(
        const char *name, METAENGINE_Render_RendererEnum renderer, int major_version,
        int minor_version);

/*! Gets the first registered renderer identifier for the given enum value. */
METAENGINE_Render_RendererID METAENGINE_Render_GetRendererID(
        METAENGINE_Render_RendererEnum renderer);

/*! Gets the number of registered (available) renderers. */
int METAENGINE_Render_GetNumRegisteredRenderers(void);

/*! Gets an array of identifiers for the registered (available) renderers. */
void METAENGINE_Render_GetRegisteredRendererList(METAENGINE_Render_RendererID *renderers_array);

/*! Prepares a renderer for use by SDL_gpu. */
void METAENGINE_Render_RegisterRenderer(
        METAENGINE_Render_RendererID id,
        METAENGINE_Render_Renderer *(*create_renderer)(METAENGINE_Render_RendererID request),
        void (*free_renderer)(METAENGINE_Render_Renderer *renderer));

// End of RendererSetup
/*! @} */

/*! \ingroup RendererControls
 *  @{ */

/*! Gets the next enum ID that can be used for a custom renderer. */
METAENGINE_Render_RendererEnum METAENGINE_Render_ReserveNextRendererEnum(void);

/*! Gets the number of active (created) renderers. */
int METAENGINE_Render_GetNumActiveRenderers(void);

/*! Gets an array of identifiers for the active renderers. */
void METAENGINE_Render_GetActiveRendererList(METAENGINE_Render_RendererID *renderers_array);

/*! \return The current renderer */
METAENGINE_Render_Renderer *METAENGINE_Render_GetCurrentRenderer(void);

/*! Switches the current renderer to the renderer matching the given identifier. */
void METAENGINE_Render_SetCurrentRenderer(METAENGINE_Render_RendererID id);

/*! \return The renderer matching the given identifier. */
METAENGINE_Render_Renderer *METAENGINE_Render_GetRenderer(METAENGINE_Render_RendererID id);

void METAENGINE_Render_FreeRenderer(METAENGINE_Render_Renderer *renderer);

/*! Reapplies the renderer state to the backend API (e.g. OpenGL, Direct3D).  Use this if you want SDL_gpu to be able to render after you've used direct backend calls. */
void METAENGINE_Render_ResetRendererState(void);

/*! Sets the coordinate mode for this renderer.  Target and image coordinates will be either "inverted" (0,0 is the upper left corner, y increases downward) or "mathematical" (0,0 is the bottom-left corner, y increases upward).
 * The default is inverted (0), as this is traditional for 2D graphics.
 * \param inverted 0 is for inverted coordinates, 1 is for mathematical coordinates */
void METAENGINE_Render_SetCoordinateMode(METAENGINE_Render_bool use_math_coords);

METAENGINE_Render_bool METAENGINE_Render_GetCoordinateMode(void);

/*! Sets the default image blitting anchor for newly created images.
 * \see METAENGINE_Render_SetAnchor
 */
void METAENGINE_Render_SetDefaultAnchor(float anchor_x, float anchor_y);

/*! Returns the default image blitting anchor through the given variables.
 * \see METAENGINE_Render_GetAnchor
 */
void METAENGINE_Render_GetDefaultAnchor(float *anchor_x, float *anchor_y);

// End of RendererControls
/*! @} */

// Context / window controls

/*! \ingroup ContextControls
 *  @{ */

/*! \return The renderer's current context target. */
METAENGINE_Render_Target *METAENGINE_Render_GetContextTarget(void);

/*! \return The target that is associated with the given windowID. */
METAENGINE_Render_Target *METAENGINE_Render_GetWindowTarget(UInt32 windowID);

/*! Creates a separate context for the given window using the current renderer and returns a METAENGINE_Render_Target that represents it. */
METAENGINE_Render_Target *METAENGINE_Render_CreateTargetFromWindow(UInt32 windowID);

/*! Makes the given window the current rendering destination for the given context target.
 * This also makes the target the current context for image loading and window operations.
 * If the target does not represent a window, this does nothing.
 */
void METAENGINE_Render_MakeCurrent(METAENGINE_Render_Target *target, UInt32 windowID);

/*! Change the actual size of the current context target's window.  This resets the virtual resolution and viewport of the context target.
 * Aside from direct resolution changes, this should also be called in response to SDL_WINDOWEVENT_RESIZED window events for resizable windows. */
METAENGINE_Render_bool METAENGINE_Render_SetWindowResolution(UInt16 w, UInt16 h);

/*! Enable/disable fullscreen mode for the current context target's window.
 * On some platforms, this may destroy the renderer context and require that textures be reloaded.  Unfortunately, SDL does not provide a notification mechanism for this.
 * \param enable_fullscreen If true, make the application go fullscreen.  If false, make the application go to windowed mode.
 * \param use_desktop_resolution If true, lets the window change its resolution when it enters fullscreen mode (via SDL_WINDOW_FULLSCREEN_DESKTOP).
 * \return 0 if the new mode is windowed, 1 if the new mode is fullscreen.  */
METAENGINE_Render_bool METAENGINE_Render_SetFullscreen(
        METAENGINE_Render_bool enable_fullscreen, METAENGINE_Render_bool use_desktop_resolution);

/*! Returns true if the current context target's window is in fullscreen mode. */
METAENGINE_Render_bool METAENGINE_Render_GetFullscreen(void);

/*! \return Returns the last active target. */
METAENGINE_Render_Target *METAENGINE_Render_GetActiveTarget(void);

/*! \return Sets the currently active target for matrix modification functions. */
METAENGINE_Render_bool METAENGINE_Render_SetActiveTarget(METAENGINE_Render_Target *target);

/*! Enables/disables alpha blending for shape rendering on the current window. */
void METAENGINE_Render_SetShapeBlending(METAENGINE_Render_bool enable);

/*! Translates a blend preset into a blend mode. */
METAENGINE_Render_BlendMode METAENGINE_Render_GetBlendModeFromPreset(
        METAENGINE_Render_BlendPresetEnum preset);

/*! Sets the blending component functions for shape rendering. */
void METAENGINE_Render_SetShapeBlendFunction(METAENGINE_Render_BlendFuncEnum source_color,
                                             METAENGINE_Render_BlendFuncEnum dest_color,
                                             METAENGINE_Render_BlendFuncEnum source_alpha,
                                             METAENGINE_Render_BlendFuncEnum dest_alpha);

/*! Sets the blending component equations for shape rendering. */
void METAENGINE_Render_SetShapeBlendEquation(METAENGINE_Render_BlendEqEnum color_equation,
                                             METAENGINE_Render_BlendEqEnum alpha_equation);

/*! Sets the blending mode for shape rendering on the current window, if supported by the renderer. */
void METAENGINE_Render_SetShapeBlendMode(METAENGINE_Render_BlendPresetEnum mode);

/*! Sets the thickness of lines for the current context.
 * \param thickness New line thickness in pixels measured across the line.  Default is 1.0f.
 * \return The old thickness value
 */
float METAENGINE_Render_SetLineThickness(float thickness);

/*! Returns the current line thickness value. */
float METAENGINE_Render_GetLineThickness(void);

// End of ContextControls
/*! @} */

/*! \ingroup TargetControls
 *  @{ */

/*! Creates a target that aliases the given target.  Aliases can be used to store target settings (e.g. viewports) for easy switching.
 * METAENGINE_Render_FreeTarget() frees the alias's memory, but does not affect the original. */
METAENGINE_Render_Target *METAENGINE_Render_CreateAliasTarget(METAENGINE_Render_Target *target);

/*! Creates a new render target from the given image.  It can then be accessed from image->target.  This increments the internal refcount of the target, so it should be matched with a METAENGINE_Render_FreeTarget(). */
METAENGINE_Render_Target *METAENGINE_Render_LoadTarget(METAENGINE_Render_Image *image);

/*! Creates a new render target from the given image.  It can then be accessed from image->target.  This does not increment the internal refcount of the target, so it will be invalidated when the image is freed. */
METAENGINE_Render_Target *METAENGINE_Render_GetTarget(METAENGINE_Render_Image *image);

/*! Deletes a render target in the proper way for this renderer. */
void METAENGINE_Render_FreeTarget(METAENGINE_Render_Target *target);

/*! Change the logical size of the given target.  Rendering to this target will be scaled as if the dimensions were actually the ones given. */
void METAENGINE_Render_SetVirtualResolution(METAENGINE_Render_Target *target, UInt16 w, UInt16 h);

/*! Query the logical size of the given target. */
void METAENGINE_Render_GetVirtualResolution(METAENGINE_Render_Target *target, UInt16 *w, UInt16 *h);

/*! Converts screen space coordinates (such as from mouse input) to logical drawing coordinates.  This interacts with METAENGINE_Render_SetCoordinateMode() when the y-axis is flipped (screen space is assumed to be inverted: (0,0) in the upper-left corner). */
void METAENGINE_Render_GetVirtualCoords(METAENGINE_Render_Target *target, float *x, float *y,
                                        float displayX, float displayY);

/*! Reset the logical size of the given target to its original value. */
void METAENGINE_Render_UnsetVirtualResolution(METAENGINE_Render_Target *target);

/*! \return A METAENGINE_Render_Rect with the given values. */
METAENGINE_Render_Rect METAENGINE_Render_MakeRect(float x, float y, float w, float h);

/*! \return An METAENGINE_Color with the given values. */
METAENGINE_Color METAENGINE_Render_MakeColor(UInt8 r, UInt8 g, UInt8 b, UInt8 a);

/*! Sets the given target's viewport. */
void METAENGINE_Render_SetViewport(METAENGINE_Render_Target *target,
                                   METAENGINE_Render_Rect viewport);

/*! Resets the given target's viewport to the entire target area. */
void METAENGINE_Render_UnsetViewport(METAENGINE_Render_Target *target);

/*! \return A METAENGINE_Render_Camera with position (0, 0, 0), angle of 0, zoom of 1, centered origin, and near/far clipping planes of -100 and 100. */
METAENGINE_Render_Camera METAENGINE_Render_GetDefaultCamera(void);

/*! \return The camera of the given render target.  If target is NULL, returns the default camera. */
METAENGINE_Render_Camera METAENGINE_Render_GetCamera(METAENGINE_Render_Target *target);

/*! Sets the current render target's current camera.
 * \param target A pointer to the target that will copy this camera.
 * \param cam A pointer to the camera data to use or NULL to use the default camera.
 * \return The old camera. */
METAENGINE_Render_Camera METAENGINE_Render_SetCamera(METAENGINE_Render_Target *target,
                                                     METAENGINE_Render_Camera *cam);

/*! Enables or disables using the built-in camera matrix transforms. */
void METAENGINE_Render_EnableCamera(METAENGINE_Render_Target *target,
                                    METAENGINE_Render_bool use_camera);

/*! Returns 1 if the camera transforms are enabled, 0 otherwise. */
METAENGINE_Render_bool METAENGINE_Render_IsCameraEnabled(METAENGINE_Render_Target *target);

/*! Attach a new depth buffer to the given target so that it can use depth testing.  Context targets automatically have a depth buffer already.
 *  If successful, also enables depth testing for this target.
 */
METAENGINE_Render_bool METAENGINE_Render_AddDepthBuffer(METAENGINE_Render_Target *target);

/*! Enables or disables the depth test, which will skip drawing pixels/fragments behind other fragments.  Disabled by default.
 *  This has implications for alpha blending, where compositing might not work correctly depending on render order.
 */
void METAENGINE_Render_SetDepthTest(METAENGINE_Render_Target *target,
                                    METAENGINE_Render_bool enable);

/*! Enables or disables writing the depth (effective view z-coordinate) of new pixels to the depth buffer.  Enabled by default, but you must call METAENGINE_Render_SetDepthTest() to use it. */
void METAENGINE_Render_SetDepthWrite(METAENGINE_Render_Target *target,
                                     METAENGINE_Render_bool enable);

/*! Sets the operation to perform when depth testing. */
void METAENGINE_Render_SetDepthFunction(METAENGINE_Render_Target *target,
                                        METAENGINE_Render_ComparisonEnum compare_operation);

/*! \return The RGBA color of a pixel. */
METAENGINE_Color METAENGINE_Render_GetPixel(METAENGINE_Render_Target *target, Int16 x, Int16 y);

/*! Sets the clipping rect for the given render target. */
METAENGINE_Render_Rect METAENGINE_Render_SetClipRect(METAENGINE_Render_Target *target,
                                                     METAENGINE_Render_Rect rect);

/*! Sets the clipping rect for the given render target. */
METAENGINE_Render_Rect METAENGINE_Render_SetClip(METAENGINE_Render_Target *target, Int16 x, Int16 y,
                                                 UInt16 w, UInt16 h);

/*! Turns off clipping for the given target. */
void METAENGINE_Render_UnsetClip(METAENGINE_Render_Target *target);

/*! Returns METAENGINE_Render_TRUE if the given rects A and B overlap, in which case it also fills the given result rect with the intersection.  `result` can be NULL if you don't need the intersection. */
METAENGINE_Render_bool METAENGINE_Render_IntersectRect(METAENGINE_Render_Rect A,
                                                       METAENGINE_Render_Rect B,
                                                       METAENGINE_Render_Rect *result);

/*! Returns METAENGINE_Render_TRUE if the given target's clip rect and the given B rect overlap, in which case it also fills the given result rect with the intersection.  `result` can be NULL if you don't need the intersection.
 * If the target doesn't have a clip rect enabled, this uses the whole target area.
 */
METAENGINE_Render_bool METAENGINE_Render_IntersectClipRect(METAENGINE_Render_Target *target,
                                                           METAENGINE_Render_Rect B,
                                                           METAENGINE_Render_Rect *result);

/*! Sets the modulation color for subsequent drawing of images and shapes on the given target.
 *  This has a cumulative effect with the image coloring functions.
 *  e.g. METAENGINE_Render_SetRGB(image, 255, 128, 0); METAENGINE_Render_SetTargetRGB(target, 128, 128, 128);
 *  Would make the image draw with color of roughly (128, 64, 0).
 */
void METAENGINE_Render_SetTargetColor(METAENGINE_Render_Target *target, METAENGINE_Color color);

/*! Sets the modulation color for subsequent drawing of images and shapes on the given target.
 *  This has a cumulative effect with the image coloring functions.
 *  e.g. METAENGINE_Render_SetRGB(image, 255, 128, 0); METAENGINE_Render_SetTargetRGB(target, 128, 128, 128);
 *  Would make the image draw with color of roughly (128, 64, 0).
 */
void METAENGINE_Render_SetTargetRGB(METAENGINE_Render_Target *target, UInt8 r, UInt8 g, UInt8 b);

/*! Sets the modulation color for subsequent drawing of images and shapes on the given target.
 *  This has a cumulative effect with the image coloring functions.
 *  e.g. METAENGINE_Render_SetRGB(image, 255, 128, 0); METAENGINE_Render_SetTargetRGB(target, 128, 128, 128);
 *  Would make the image draw with color of roughly (128, 64, 0).
 */
void METAENGINE_Render_SetTargetRGBA(METAENGINE_Render_Target *target, UInt8 r, UInt8 g, UInt8 b,
                                     UInt8 a);

/*! Unsets the modulation color for subsequent drawing of images and shapes on the given target.
 *  This has the same effect as coloring with pure opaque white (255, 255, 255, 255).
 */
void METAENGINE_Render_UnsetTargetColor(METAENGINE_Render_Target *target);

// End of SurfaceControls
/*! @} */

/*! \ingroup ImageControls
 *  @{ */

/*! Create a new, blank image with the given format.  Don't forget to METAENGINE_Render_FreeImage() it.
	 * \param w Image width in pixels
	 * \param h Image height in pixels
	 * \param format Format of color channels.
	 */
METAENGINE_Render_Image *METAENGINE_Render_CreateImage(UInt16 w, UInt16 h,
                                                       METAENGINE_Render_FormatEnum format);

/*! Create a new image that uses the given native texture handle as the image texture. */
METAENGINE_Render_Image *METAENGINE_Render_CreateImageUsingTexture(
        METAENGINE_Render_TextureHandle handle, METAENGINE_Render_bool take_ownership);

/*! Creates an image that aliases the given image.  Aliases can be used to store image settings (e.g. modulation color) for easy switching.
 * METAENGINE_Render_FreeImage() frees the alias's memory, but does not affect the original. */
METAENGINE_Render_Image *METAENGINE_Render_CreateAliasImage(METAENGINE_Render_Image *image);

/*! Copy an image to a new image.  Don't forget to METAENGINE_Render_FreeImage() both. */
METAENGINE_Render_Image *METAENGINE_Render_CopyImage(METAENGINE_Render_Image *image);

/*! Deletes an image in the proper way for this renderer.  Also deletes the corresponding METAENGINE_Render_Target if applicable.  Be careful not to use that target afterward! */
void METAENGINE_Render_FreeImage(METAENGINE_Render_Image *image);

/*! Change the logical size of the given image.  Rendering this image will scaled it as if the dimensions were actually the ones given. */
void METAENGINE_Render_SetImageVirtualResolution(METAENGINE_Render_Image *image, UInt16 w,
                                                 UInt16 h);

/*! Reset the logical size of the given image to its original value. */
void METAENGINE_Render_UnsetImageVirtualResolution(METAENGINE_Render_Image *image);

/*! Update an image from surface data.  Ignores virtual resolution on the image so the number of pixels needed from the surface is known. */
void METAENGINE_Render_UpdateImage(METAENGINE_Render_Image *image,
                                   const METAENGINE_Render_Rect *image_rect, void *surface,
                                   const METAENGINE_Render_Rect *surface_rect);

/*! Update an image from an array of pixel data.  Ignores virtual resolution on the image so the number of pixels needed from the surface is known. */
void METAENGINE_Render_UpdateImageBytes(METAENGINE_Render_Image *image,
                                        const METAENGINE_Render_Rect *image_rect,
                                        const unsigned char *bytes, int bytes_per_row);

/*! Update an image from surface data, replacing its underlying texture to allow for size changes.  Ignores virtual resolution on the image so the number of pixels needed from the surface is known. */
METAENGINE_Render_bool METAENGINE_Render_ReplaceImage(METAENGINE_Render_Image *image, void *surface,
                                                      const METAENGINE_Render_Rect *surface_rect);

/*! Loads mipmaps for the given image, if supported by the renderer. */
void METAENGINE_Render_GenerateMipmaps(METAENGINE_Render_Image *image);

/*! Sets the modulation color for subsequent drawing of the given image. */
void METAENGINE_Render_SetColor(METAENGINE_Render_Image *image, METAENGINE_Color color);

/*! Sets the modulation color for subsequent drawing of the given image. */
void METAENGINE_Render_SetRGB(METAENGINE_Render_Image *image, UInt8 r, UInt8 g, UInt8 b);

/*! Sets the modulation color for subsequent drawing of the given image. */
void METAENGINE_Render_SetRGBA(METAENGINE_Render_Image *image, UInt8 r, UInt8 g, UInt8 b, UInt8 a);

/*! Unsets the modulation color for subsequent drawing of the given image.
 *  This is equivalent to coloring with pure opaque white (255, 255, 255, 255). */
void METAENGINE_Render_UnsetColor(METAENGINE_Render_Image *image);

/*! Gets the current alpha blending setting. */
METAENGINE_Render_bool METAENGINE_Render_GetBlending(METAENGINE_Render_Image *image);

/*! Enables/disables alpha blending for the given image. */
void METAENGINE_Render_SetBlending(METAENGINE_Render_Image *image, METAENGINE_Render_bool enable);

/*! Sets the blending component functions. */
void METAENGINE_Render_SetBlendFunction(METAENGINE_Render_Image *image,
                                        METAENGINE_Render_BlendFuncEnum source_color,
                                        METAENGINE_Render_BlendFuncEnum dest_color,
                                        METAENGINE_Render_BlendFuncEnum source_alpha,
                                        METAENGINE_Render_BlendFuncEnum dest_alpha);

/*! Sets the blending component equations. */
void METAENGINE_Render_SetBlendEquation(METAENGINE_Render_Image *image,
                                        METAENGINE_Render_BlendEqEnum color_equation,
                                        METAENGINE_Render_BlendEqEnum alpha_equation);

/*! Sets the blending mode, if supported by the renderer. */
void METAENGINE_Render_SetBlendMode(METAENGINE_Render_Image *image,
                                    METAENGINE_Render_BlendPresetEnum mode);

/*! Sets the image filtering mode, if supported by the renderer. */
void METAENGINE_Render_SetImageFilter(METAENGINE_Render_Image *image,
                                      METAENGINE_Render_FilterEnum filter);

/*! Sets the image anchor, which is the point about which the image is blitted.  The default is to blit the image on-center (0.5, 0.5).  The anchor is in normalized coordinates (0.0-1.0). */
void METAENGINE_Render_SetAnchor(METAENGINE_Render_Image *image, float anchor_x, float anchor_y);

/*! Returns the image anchor via the passed parameters.  The anchor is in normalized coordinates (0.0-1.0). */
void METAENGINE_Render_GetAnchor(METAENGINE_Render_Image *image, float *anchor_x, float *anchor_y);

/*! Gets the current pixel snap setting.  The default value is METAENGINE_Render_SNAP_POSITION_AND_DIMENSIONS.  */
METAENGINE_Render_SnapEnum METAENGINE_Render_GetSnapMode(METAENGINE_Render_Image *image);

/*! Sets the pixel grid snapping mode for the given image. */
void METAENGINE_Render_SetSnapMode(METAENGINE_Render_Image *image, METAENGINE_Render_SnapEnum mode);

/*! Sets the image wrapping mode, if supported by the renderer. */
void METAENGINE_Render_SetWrapMode(METAENGINE_Render_Image *image,
                                   METAENGINE_Render_WrapEnum wrap_mode_x,
                                   METAENGINE_Render_WrapEnum wrap_mode_y);

/*! Returns the backend-specific texture handle associated with the given image.  Note that SDL_gpu will be unaware of changes made to the texture.  */
METAENGINE_Render_TextureHandle METAENGINE_Render_GetTextureHandle(METAENGINE_Render_Image *image);

// End of ImageControls
/*! @} */

// Surface / Image / Target conversions
/*! \ingroup Conversions
 *  @{ */

/*! Copy SDL_Surface data into a new METAENGINE_Render_Image.  Don't forget to SDL_FreeSurface() the surface and METAENGINE_Render_FreeImage() the image.*/
METAENGINE_Render_Image *METAENGINE_Render_CopyImageFromSurface(void *surface);

/*! Like METAENGINE_Render_CopyImageFromSurface but enable to copy only part of the surface.*/
METAENGINE_Render_Image *METAENGINE_Render_CopyImageFromSurfaceRect(
        void *surface, METAENGINE_Render_Rect *surface_rect);

/*! Copy METAENGINE_Render_Target data into a new METAENGINE_Render_Image.  Don't forget to METAENGINE_Render_FreeImage() the image.*/
METAENGINE_Render_Image *METAENGINE_Render_CopyImageFromTarget(METAENGINE_Render_Target *target);

/*! Copy METAENGINE_Render_Target data into a new SDL_Surface.  Don't forget to SDL_FreeSurface() the surface.*/
void *METAENGINE_Render_CopySurfaceFromTarget(METAENGINE_Render_Target *target);

/*! Copy METAENGINE_Render_Image data into a new SDL_Surface.  Don't forget to SDL_FreeSurface() the surface and METAENGINE_Render_FreeImage() the image.*/
void *METAENGINE_Render_CopySurfaceFromImage(METAENGINE_Render_Image *image);

// End of Conversions
/*! @} */

/*! \ingroup Matrix
 *  @{ */

// Basic vector operations (3D)

/*! Returns the magnitude (length) of the given vector. */
float METAENGINE_Render_VectorLength(const float *vec3);

/*! Modifies the given vector so that it has a new length of 1. */
void METAENGINE_Render_VectorNormalize(float *vec3);

/*! Returns the dot product of two vectors. */
float METAENGINE_Render_VectorDot(const float *A, const float *B);

/*! Performs the cross product of vectors A and B (result = A x B).  Do not use A or B as 'result'. */
void METAENGINE_Render_VectorCross(float *result, const float *A, const float *B);

/*! Overwrite 'result' vector with the values from vector A. */
void METAENGINE_Render_VectorCopy(float *result, const float *A);

/*! Multiplies the given matrix into the given vector (vec3 = matrix*vec3). */
void METAENGINE_Render_VectorApplyMatrix(float *vec3, const float *matrix_4x4);

/*! Multiplies the given matrix into the given vector (vec4 = matrix*vec4). */
void METAENGINE_Render_Vector4ApplyMatrix(float *vec4, const float *matrix_4x4);

// Basic matrix operations (4x4)

/*! Overwrite 'result' matrix with the values from matrix A. */
void METAENGINE_Render_MatrixCopy(float *result, const float *A);

/*! Fills 'result' matrix with the identity matrix. */
void METAENGINE_Render_MatrixIdentity(float *result);

/*! Multiplies an orthographic projection matrix into the given matrix. */
void METAENGINE_Render_MatrixOrtho(float *result, float left, float right, float bottom, float top,
                                   float z_near, float z_far);

/*! Multiplies a perspective projection matrix into the given matrix. */
void METAENGINE_Render_MatrixFrustum(float *result, float left, float right, float bottom,
                                     float top, float z_near, float z_far);

/*! Multiplies a perspective projection matrix into the given matrix. */
void METAENGINE_Render_MatrixPerspective(float *result, float fovy, float aspect, float z_near,
                                         float z_far);

/*! Multiplies a view matrix into the given matrix. */
void METAENGINE_Render_MatrixLookAt(float *matrix, float eye_x, float eye_y, float eye_z,
                                    float target_x, float target_y, float target_z, float up_x,
                                    float up_y, float up_z);

/*! Adds a translation into the given matrix. */
void METAENGINE_Render_MatrixTranslate(float *result, float x, float y, float z);

/*! Multiplies a scaling matrix into the given matrix. */
void METAENGINE_Render_MatrixScale(float *result, float sx, float sy, float sz);

/*! Multiplies a rotation matrix into the given matrix. */
void METAENGINE_Render_MatrixRotate(float *result, float degrees, float x, float y, float z);

/*! Multiplies matrices A and B and stores the result in the given 'result' matrix (result = A*B).  Do not use A or B as 'result'.
 * \see METAENGINE_Render_MultiplyAndAssign
*/
void METAENGINE_Render_MatrixMultiply(float *result, const float *A, const float *B);

/*! Multiplies matrices 'result' and B and stores the result in the given 'result' matrix (result = result * B). */
void METAENGINE_Render_MultiplyAndAssign(float *result, const float *B);

// Matrix stack accessors

/*! Returns an internal string that represents the contents of matrix A. */
const char *METAENGINE_Render_GetMatrixString(const float *A);

/*! Returns the current matrix from the active target.  Returns NULL if stack is empty. */
float *METAENGINE_Render_GetCurrentMatrix(void);

/*! Returns the current matrix from the top of the matrix stack.  Returns NULL if stack is empty. */
float *METAENGINE_Render_GetTopMatrix(METAENGINE_Render_MatrixStack *stack);

/*! Returns the current model matrix from the active target.  Returns NULL if stack is empty. */
float *METAENGINE_Render_GetModel(void);

/*! Returns the current view matrix from the active target.  Returns NULL if stack is empty. */
float *METAENGINE_Render_GetView(void);

/*! Returns the current projection matrix from the active target.  Returns NULL if stack is empty. */
float *METAENGINE_Render_GetProjection(void);

/*! Copies the current modelview-projection matrix from the active target into the given 'result' matrix (result = P*V*M). */
void METAENGINE_Render_GetModelViewProjection(float *result);

// Matrix stack manipulators

/*! Returns a newly allocated matrix stack that has already been initialized. */
METAENGINE_Render_MatrixStack *METAENGINE_Render_CreateMatrixStack(void);

/*! Frees the memory for the matrix stack and any matrices it contains. */
void METAENGINE_Render_FreeMatrixStack(METAENGINE_Render_MatrixStack *stack);

/*! Allocate new matrices for the given stack. */
void METAENGINE_Render_InitMatrixStack(METAENGINE_Render_MatrixStack *stack);

/*! Copies matrices from one stack to another. */
void METAENGINE_Render_CopyMatrixStack(const METAENGINE_Render_MatrixStack *source,
                                       METAENGINE_Render_MatrixStack *dest);

/*! Deletes matrices in the given stack. */
void METAENGINE_Render_ClearMatrixStack(METAENGINE_Render_MatrixStack *stack);

/*! Reapplies the default orthographic projection matrix, based on camera and coordinate settings. */
void METAENGINE_Render_ResetProjection(METAENGINE_Render_Target *target);

/*! Sets the active target and changes matrix mode to METAENGINE_Render_PROJECTION, METAENGINE_Render_VIEW, or METAENGINE_Render_MODEL.  Further matrix stack operations manipulate that particular stack. */
void METAENGINE_Render_MatrixMode(METAENGINE_Render_Target *target, int matrix_mode);

/*! Copies the given matrix to the active target's projection matrix. */
void METAENGINE_Render_SetProjection(const float *A);

/*! Copies the given matrix to the active target's view matrix. */
void METAENGINE_Render_SetView(const float *A);

/*! Copies the given matrix to the active target's model matrix. */
void METAENGINE_Render_SetModel(const float *A);

/*! Copies the given matrix to the active target's projection matrix. */
void METAENGINE_Render_SetProjectionFromStack(METAENGINE_Render_MatrixStack *stack);

/*! Copies the given matrix to the active target's view matrix. */
void METAENGINE_Render_SetViewFromStack(METAENGINE_Render_MatrixStack *stack);

/*! Copies the given matrix to the active target's model matrix. */
void METAENGINE_Render_SetModelFromStack(METAENGINE_Render_MatrixStack *stack);

/*! Pushes the current matrix as a new matrix stack item to be restored later. */
void METAENGINE_Render_PushMatrix(void);

/*! Removes the current matrix from the stack, restoring the previously pushed matrix. */
void METAENGINE_Render_PopMatrix(void);

/*! Fills current matrix with the identity matrix. */
void METAENGINE_Render_LoadIdentity(void);

/*! Copies a given matrix to be the current matrix. */
void METAENGINE_Render_LoadMatrix(const float *matrix4x4);

/*! Multiplies an orthographic projection matrix into the current matrix. */
void METAENGINE_Render_Ortho(float left, float right, float bottom, float top, float z_near,
                             float z_far);

/*! Multiplies a perspective projection matrix into the current matrix. */
void METAENGINE_Render_Frustum(float left, float right, float bottom, float top, float z_near,
                               float z_far);

/*! Multiplies a perspective projection matrix into the current matrix. */
void METAENGINE_Render_Perspective(float fovy, float aspect, float z_near, float z_far);

/*! Multiplies a view matrix into the current matrix. */
void METAENGINE_Render_LookAt(float eye_x, float eye_y, float eye_z, float target_x, float target_y,
                              float target_z, float up_x, float up_y, float up_z);

/*! Adds a translation into the current matrix. */
void METAENGINE_Render_Translate(float x, float y, float z);

/*! Multiplies a scaling matrix into the current matrix. */
void METAENGINE_Render_Scale(float sx, float sy, float sz);

/*! Multiplies a rotation matrix into the current matrix. */
void METAENGINE_Render_Rotate(float degrees, float x, float y, float z);

/*! Multiplies a given matrix into the current matrix. */
void METAENGINE_Render_MultMatrix(const float *matrix4x4);

// End of Matrix
/*! @} */

/*! \ingroup Rendering
 *  @{ */

/*! Clears the contents of the given render target.  Fills the target with color {0, 0, 0, 0}. */
void METAENGINE_Render_Clear(METAENGINE_Render_Target *target);

/*! Fills the given render target with a color. */
void METAENGINE_Render_ClearColor(METAENGINE_Render_Target *target, METAENGINE_Color color);

/*! Fills the given render target with a color (alpha is 255, fully opaque). */
void METAENGINE_Render_ClearRGB(METAENGINE_Render_Target *target, UInt8 r, UInt8 g, UInt8 b);

/*! Fills the given render target with a color. */
void METAENGINE_Render_ClearRGBA(METAENGINE_Render_Target *target, UInt8 r, UInt8 g, UInt8 b,
                                 UInt8 a);

/*! Draws the given image to the given render target.
    * \param src_rect The region of the source image to use.  Pass NULL for the entire image.
    * \param x Destination x-position
    * \param y Destination y-position */
void METAENGINE_Render_Blit(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                            METAENGINE_Render_Target *target, float x, float y);

/*! Rotates and draws the given image to the given render target.
    * \param src_rect The region of the source image to use.  Pass NULL for the entire image.
    * \param x Destination x-position
    * \param y Destination y-position
    * \param degrees Rotation angle (in degrees) */
void METAENGINE_Render_BlitRotate(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                                  METAENGINE_Render_Target *target, float x, float y,
                                  float degrees);

/*! Scales and draws the given image to the given render target.
    * \param src_rect The region of the source image to use.  Pass NULL for the entire image.
    * \param x Destination x-position
    * \param y Destination y-position
    * \param scaleX Horizontal stretch factor
    * \param scaleY Vertical stretch factor */
void METAENGINE_Render_BlitScale(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                                 METAENGINE_Render_Target *target, float x, float y, float scaleX,
                                 float scaleY);

/*! Scales, rotates, and draws the given image to the given render target.
    * \param src_rect The region of the source image to use.  Pass NULL for the entire image.
    * \param x Destination x-position
    * \param y Destination y-position
    * \param degrees Rotation angle (in degrees)
    * \param scaleX Horizontal stretch factor
    * \param scaleY Vertical stretch factor */
void METAENGINE_Render_BlitTransform(METAENGINE_Render_Image *image,
                                     METAENGINE_Render_Rect *src_rect,
                                     METAENGINE_Render_Target *target, float x, float y,
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
void METAENGINE_Render_BlitTransformX(METAENGINE_Render_Image *image,
                                      METAENGINE_Render_Rect *src_rect,
                                      METAENGINE_Render_Target *target, float x, float y,
                                      float pivot_x, float pivot_y, float degrees, float scaleX,
                                      float scaleY);

/*! Draws the given image to the given render target, scaling it to fit the destination region.
    * \param src_rect The region of the source image to use.  Pass NULL for the entire image.
    * \param dest_rect The region of the destination target image to draw upon.  Pass NULL for the entire target.
    */
void METAENGINE_Render_BlitRect(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                                METAENGINE_Render_Target *target,
                                METAENGINE_Render_Rect *dest_rect);

/*! Draws the given image to the given render target, scaling it to fit the destination region.
    * \param src_rect The region of the source image to use.  Pass NULL for the entire image.
    * \param dest_rect The region of the destination target image to draw upon.  Pass NULL for the entire target.
	* \param degrees Rotation angle (in degrees)
	* \param pivot_x Pivot x-position (in image coordinates)
	* \param pivot_y Pivot y-position (in image coordinates)
	* \param flip_direction A METAENGINE_Render_FlipEnum value (or bitwise OR'd combination) that specifies which direction the image should be flipped.
    */
void METAENGINE_Render_BlitRectX(METAENGINE_Render_Image *image, METAENGINE_Render_Rect *src_rect,
                                 METAENGINE_Render_Target *target,
                                 METAENGINE_Render_Rect *dest_rect, float degrees, float pivot_x,
                                 float pivot_y, METAENGINE_Render_FlipEnum flip_direction);

/*! Renders triangles from the given set of vertices.  This lets you render arbitrary geometry.  It is a direct path to the GPU, so the format is different than typical SDL_gpu calls.
 * \param values A tightly-packed array of vertex position (e.g. x,y), texture coordinates (e.g. s,t), and color (e.g. r,g,b,a) values.  Texture coordinates and color values are expected to be already normalized to 0.0 - 1.0.  Pass NULL to render with only custom shader attributes.
 * \param indices If not NULL, this is used to specify which vertices to use and in what order (i.e. it indexes the vertices in the 'values' array).
 * \param flags Bit flags to control the interpretation of the 'values' array parameters.
 */
void METAENGINE_Render_TriangleBatch(METAENGINE_Render_Image *image,
                                     METAENGINE_Render_Target *target, unsigned short num_vertices,
                                     float *values, unsigned int num_indices,
                                     unsigned short *indices,
                                     METAENGINE_Render_BatchFlagEnum flags);

/*! Renders triangles from the given set of vertices.  This lets you render arbitrary geometry.  It is a direct path to the GPU, so the format is different than typical SDL_gpu calls.
 * \param values A tightly-packed array of vertex position (e.g. x,y), texture coordinates (e.g. s,t), and color (e.g. r,g,b,a) values.  Texture coordinates and color values are expected to be already normalized to 0.0 - 1.0 (or 0 - 255 for 8-bit color components).  Pass NULL to render with only custom shader attributes.
 * \param indices If not NULL, this is used to specify which vertices to use and in what order (i.e. it indexes the vertices in the 'values' array).
 * \param flags Bit flags to control the interpretation of the 'values' array parameters.
 */
void METAENGINE_Render_TriangleBatchX(METAENGINE_Render_Image *image,
                                      METAENGINE_Render_Target *target, unsigned short num_vertices,
                                      void *values, unsigned int num_indices,
                                      unsigned short *indices,
                                      METAENGINE_Render_BatchFlagEnum flags);

/*! Renders primitives from the given set of vertices.  This lets you render arbitrary geometry.  It is a direct path to the GPU, so the format is different than typical SDL_gpu calls.
 * \param primitive_type The kind of primitive to render.
 * \param values A tightly-packed array of vertex position (e.g. x,y), texture coordinates (e.g. s,t), and color (e.g. r,g,b,a) values.  Texture coordinates and color values are expected to be already normalized to 0.0 - 1.0 (or 0 - 255 for 8-bit color components).  Pass NULL to render with only custom shader attributes.
 * \param indices If not NULL, this is used to specify which vertices to use and in what order (i.e. it indexes the vertices in the 'values' array).
 * \param flags Bit flags to control the interpretation of the 'values' array parameters.
 */
void METAENGINE_Render_PrimitiveBatch(METAENGINE_Render_Image *image,
                                      METAENGINE_Render_Target *target,
                                      METAENGINE_Render_PrimitiveEnum primitive_type,
                                      unsigned short num_vertices, float *values,
                                      unsigned int num_indices, unsigned short *indices,
                                      METAENGINE_Render_BatchFlagEnum flags);

/*! Renders primitives from the given set of vertices.  This lets you render arbitrary geometry.  It is a direct path to the GPU, so the format is different than typical SDL_gpu calls.
 * \param primitive_type The kind of primitive to render.
 * \param values A tightly-packed array of vertex position (e.g. x,y), texture coordinates (e.g. s,t), and color (e.g. r,g,b,a) values.  Texture coordinates and color values are expected to be already normalized to 0.0 - 1.0 (or 0 - 255 for 8-bit color components).  Pass NULL to render with only custom shader attributes.
 * \param indices If not NULL, this is used to specify which vertices to use and in what order (i.e. it indexes the vertices in the 'values' array).
 * \param flags Bit flags to control the interpretation of the 'values' array parameters.
 */
void METAENGINE_Render_PrimitiveBatchV(METAENGINE_Render_Image *image,
                                       METAENGINE_Render_Target *target,
                                       METAENGINE_Render_PrimitiveEnum primitive_type,
                                       unsigned short num_vertices, void *values,
                                       unsigned int num_indices, unsigned short *indices,
                                       METAENGINE_Render_BatchFlagEnum flags);

/*! Send all buffered blitting data to the current context target. */
void METAENGINE_Render_FlushBlitBuffer(void);

/*! Updates the given target's associated window.  For non-context targets (e.g. image targets), this will flush the blit buffer. */
void METAENGINE_Render_Flip(METAENGINE_Render_Target *target);

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
void METAENGINE_Render_Pixel(METAENGINE_Render_Target *target, float x, float y,
                             METAENGINE_Color color);

/*! Renders a colored line.
 * \param target The destination render target
 * \param x1 x-coord of starting point
 * \param y1 y-coord of starting point
 * \param x2 x-coord of ending point
 * \param y2 y-coord of ending point
 * \param color The color of the shape to render
 */
void METAENGINE_Render_Line(METAENGINE_Render_Target *target, float x1, float y1, float x2,
                            float y2, METAENGINE_Color color);

/*! Renders a colored arc curve (circle segment).
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param radius The radius of the circle / distance from the center point that rendering will occur
 * \param start_angle The angle to start from, in degrees.  Measured clockwise from the positive x-axis.
 * \param end_angle The angle to end at, in degrees.  Measured clockwise from the positive x-axis.
 * \param color The color of the shape to render
 */
void METAENGINE_Render_Arc(METAENGINE_Render_Target *target, float x, float y, float radius,
                           float start_angle, float end_angle, METAENGINE_Color color);

/*! Renders a colored filled arc (circle segment / pie piece).
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param radius The radius of the circle / distance from the center point that rendering will occur
 * \param start_angle The angle to start from, in degrees.  Measured clockwise from the positive x-axis.
 * \param end_angle The angle to end at, in degrees.  Measured clockwise from the positive x-axis.
 * \param color The color of the shape to render
 */
void METAENGINE_Render_ArcFilled(METAENGINE_Render_Target *target, float x, float y, float radius,
                                 float start_angle, float end_angle, METAENGINE_Color color);

/*! Renders a colored circle outline.
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param radius The radius of the circle / distance from the center point that rendering will occur
 * \param color The color of the shape to render
 */
void METAENGINE_Render_Circle(METAENGINE_Render_Target *target, float x, float y, float radius,
                              METAENGINE_Color color);

/*! Renders a colored filled circle.
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param radius The radius of the circle / distance from the center point that rendering will occur
 * \param color The color of the shape to render
 */
void METAENGINE_Render_CircleFilled(METAENGINE_Render_Target *target, float x, float y,
                                    float radius, METAENGINE_Color color);

/*! Renders a colored ellipse outline.
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param rx x-radius of ellipse
 * \param ry y-radius of ellipse
 * \param degrees The angle to rotate the ellipse
 * \param color The color of the shape to render
 */
void METAENGINE_Render_Ellipse(METAENGINE_Render_Target *target, float x, float y, float rx,
                               float ry, float degrees, METAENGINE_Color color);

/*! Renders a colored filled ellipse.
 * \param target The destination render target
 * \param x x-coord of center point
 * \param y y-coord of center point
 * \param rx x-radius of ellipse
 * \param ry y-radius of ellipse
 * \param degrees The angle to rotate the ellipse
 * \param color The color of the shape to render
 */
void METAENGINE_Render_EllipseFilled(METAENGINE_Render_Target *target, float x, float y, float rx,
                                     float ry, float degrees, METAENGINE_Color color);

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
void METAENGINE_Render_Sector(METAENGINE_Render_Target *target, float x, float y,
                              float inner_radius, float outer_radius, float start_angle,
                              float end_angle, METAENGINE_Color color);

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
void METAENGINE_Render_SectorFilled(METAENGINE_Render_Target *target, float x, float y,
                                    float inner_radius, float outer_radius, float start_angle,
                                    float end_angle, METAENGINE_Color color);

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
void METAENGINE_Render_Tri(METAENGINE_Render_Target *target, float x1, float y1, float x2, float y2,
                           float x3, float y3, METAENGINE_Color color);

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
void METAENGINE_Render_TriFilled(METAENGINE_Render_Target *target, float x1, float y1, float x2,
                                 float y2, float x3, float y3, METAENGINE_Color color);

/*! Renders a colored rectangle outline.
 * \param target The destination render target
 * \param x1 x-coord of top-left corner
 * \param y1 y-coord of top-left corner
 * \param x2 x-coord of bottom-right corner
 * \param y2 y-coord of bottom-right corner
 * \param color The color of the shape to render
 */
void METAENGINE_Render_Rectangle(METAENGINE_Render_Target *target, float x1, float y1, float x2,
                                 float y2, METAENGINE_Color color);

/*! Renders a colored rectangle outline.
 * \param target The destination render target
 * \param rect The rectangular area to draw
 * \param color The color of the shape to render
 */
void METAENGINE_Render_Rectangle2(METAENGINE_Render_Target *target, METAENGINE_Render_Rect rect,
                                  METAENGINE_Color color);

/*! Renders a colored filled rectangle.
 * \param target The destination render target
 * \param x1 x-coord of top-left corner
 * \param y1 y-coord of top-left corner
 * \param x2 x-coord of bottom-right corner
 * \param y2 y-coord of bottom-right corner
 * \param color The color of the shape to render
 */
void METAENGINE_Render_RectangleFilled(METAENGINE_Render_Target *target, float x1, float y1,
                                       float x2, float y2, METAENGINE_Color color);

/*! Renders a colored filled rectangle.
 * \param target The destination render target
 * \param rect The rectangular area to draw
 * \param color The color of the shape to render
 */
void METAENGINE_Render_RectangleFilled2(METAENGINE_Render_Target *target,
                                        METAENGINE_Render_Rect rect, METAENGINE_Color color);

/*! Renders a colored rounded (filleted) rectangle outline.
 * \param target The destination render target
 * \param x1 x-coord of top-left corner
 * \param y1 y-coord of top-left corner
 * \param x2 x-coord of bottom-right corner
 * \param y2 y-coord of bottom-right corner
 * \param radius The radius of the corners
 * \param color The color of the shape to render
 */
void METAENGINE_Render_RectangleRound(METAENGINE_Render_Target *target, float x1, float y1,
                                      float x2, float y2, float radius, METAENGINE_Color color);

/*! Renders a colored rounded (filleted) rectangle outline.
 * \param target The destination render target
 * \param rect The rectangular area to draw
 * \param radius The radius of the corners
 * \param color The color of the shape to render
 */
void METAENGINE_Render_RectangleRound2(METAENGINE_Render_Target *target,
                                       METAENGINE_Render_Rect rect, float radius,
                                       METAENGINE_Color color);

/*! Renders a colored filled rounded (filleted) rectangle.
 * \param target The destination render target
 * \param x1 x-coord of top-left corner
 * \param y1 y-coord of top-left corner
 * \param x2 x-coord of bottom-right corner
 * \param y2 y-coord of bottom-right corner
 * \param radius The radius of the corners
 * \param color The color of the shape to render
 */
void METAENGINE_Render_RectangleRoundFilled(METAENGINE_Render_Target *target, float x1, float y1,
                                            float x2, float y2, float radius,
                                            METAENGINE_Color color);

/*! Renders a colored filled rounded (filleted) rectangle.
 * \param target The destination render target
 * \param rect The rectangular area to draw
 * \param radius The radius of the corners
 * \param color The color of the shape to render
 */
void METAENGINE_Render_RectangleRoundFilled2(METAENGINE_Render_Target *target,
                                             METAENGINE_Render_Rect rect, float radius,
                                             METAENGINE_Color color);

/*! Renders a colored polygon outline.  The vertices are expected to define a convex polygon.
 * \param target The destination render target
 * \param num_vertices Number of vertices (x and y pairs)
 * \param vertices An array of vertex positions stored as interlaced x and y coords, e.g. {x1, y1, x2, y2, ...}
 * \param color The color of the shape to render
 */
void METAENGINE_Render_Polygon(METAENGINE_Render_Target *target, unsigned int num_vertices,
                               float *vertices, METAENGINE_Color color);

/*! Renders a colored sequence of line segments.
 * \param target The destination render target
 * \param num_vertices Number of vertices (x and y pairs)
 * \param vertices An array of vertex positions stored as interlaced x and y coords, e.g. {x1, y1, x2, y2, ...}
 * \param color The color of the shape to render
 * \param close_loop Make a closed polygon by drawing a line at the end back to the start point
 */
void METAENGINE_Render_Polyline(METAENGINE_Render_Target *target, unsigned int num_vertices,
                                float *vertices, METAENGINE_Color color,
                                METAENGINE_Render_bool close_loop);

/*! Renders a colored filled polygon.  The vertices are expected to define a convex polygon.
 * \param target The destination render target
 * \param num_vertices Number of vertices (x and y pairs)
 * \param vertices An array of vertex positions stored as interlaced x and y coords, e.g. {x1, y1, x2, y2, ...}
 * \param color The color of the shape to render
 */
void METAENGINE_Render_PolygonFilled(METAENGINE_Render_Target *target, unsigned int num_vertices,
                                     float *vertices, METAENGINE_Color color);

// End of Shapes
/*! @} */

/*! \ingroup ShaderInterface
 *  @{ */

/*! Creates a new, empty shader program.  You will need to compile shaders, attach them to the program, then link the program.
 * \see METAENGINE_Render_AttachShader
 * \see METAENGINE_Render_LinkShaderProgram
 */
UInt32 METAENGINE_Render_CreateShaderProgram(void);

/*! Deletes a shader program. */
void METAENGINE_Render_FreeShaderProgram(UInt32 program_object);

/*! Compiles shader source and returns the new shader object. */
UInt32 METAENGINE_Render_CompileShader(METAENGINE_Render_ShaderEnum shader_type,
                                       const char *shader_source);

/*! Creates and links a shader program with the given shader objects. */
UInt32 METAENGINE_Render_LinkShaders(UInt32 shader_object1, UInt32 shader_object2);

/*! Creates and links a shader program with the given shader objects. */
UInt32 METAENGINE_Render_LinkManyShaders(UInt32 *shader_objects, int count);

/*! Deletes a shader object. */
void METAENGINE_Render_FreeShader(UInt32 shader_object);

/*! Attaches a shader object to a shader program for future linking. */
void METAENGINE_Render_AttachShader(UInt32 program_object, UInt32 shader_object);

/*! Detaches a shader object from a shader program. */
void METAENGINE_Render_DetachShader(UInt32 program_object, UInt32 shader_object);

/*! Links a shader program with any attached shader objects. */
METAENGINE_Render_bool METAENGINE_Render_LinkShaderProgram(UInt32 program_object);

/*! \return The current shader program */
UInt32 METAENGINE_Render_GetCurrentShaderProgram(void);

/*! Returns 1 if the given shader program is a default shader for the current context, 0 otherwise. */
METAENGINE_Render_bool METAENGINE_Render_IsDefaultShaderProgram(UInt32 program_object);

/*! Activates the given shader program.  Passing NULL for 'block' will disable the built-in shader variables for custom shaders until a METAENGINE_Render_ShaderBlock is set again. */
void METAENGINE_Render_ActivateShaderProgram(UInt32 program_object,
                                             METAENGINE_Render_ShaderBlock *block);

/*! Deactivates the current shader program (activates program 0). */
void METAENGINE_Render_DeactivateShaderProgram(void);

/*! Returns the last shader log message. */
const char *METAENGINE_Render_GetShaderMessage(void);

/*! Returns an integer representing the location of the specified attribute shader variable. */
int METAENGINE_Render_GetAttributeLocation(UInt32 program_object, const char *attrib_name);

/*! Returns a filled METAENGINE_Render_AttributeFormat object. */
METAENGINE_Render_AttributeFormat METAENGINE_Render_MakeAttributeFormat(
        int num_elems_per_vertex, METAENGINE_Render_TypeEnum type, METAENGINE_Render_bool normalize,
        int stride_bytes, int offset_bytes);

/*! Returns a filled METAENGINE_Render_Attribute object. */
METAENGINE_Render_Attribute METAENGINE_Render_MakeAttribute(
        int location, void *values, METAENGINE_Render_AttributeFormat format);

/*! Returns an integer representing the location of the specified uniform shader variable. */
int METAENGINE_Render_GetUniformLocation(UInt32 program_object, const char *uniform_name);

/*! Loads the given shader program's built-in attribute and uniform locations. */
METAENGINE_Render_ShaderBlock METAENGINE_Render_LoadShaderBlock(UInt32 program_object,
                                                                const char *position_name,
                                                                const char *texcoord_name,
                                                                const char *color_name,
                                                                const char *modelViewMatrix_name);

/*! Sets the current shader block to use the given attribute and uniform locations. */
void METAENGINE_Render_SetShaderBlock(METAENGINE_Render_ShaderBlock block);

/*! Gets the shader block for the current shader. */
METAENGINE_Render_ShaderBlock METAENGINE_Render_GetShaderBlock(void);

/*! Sets the given image unit to the given image so that a custom shader can sample multiple textures.
    \param image The source image/texture.  Pass NULL to disable the image unit.
    \param location The uniform location of a texture sampler
    \param image_unit The index of the texture unit to set.  0 is the first unit, which is used by SDL_gpu's blitting functions.  1 would be the second unit. */
void METAENGINE_Render_SetShaderImage(METAENGINE_Render_Image *image, int location, int image_unit);

/*! Fills "values" with the value of the uniform shader variable at the given location. */
void METAENGINE_Render_GetUniformiv(UInt32 program_object, int location, int *values);

/*! Sets the value of the integer uniform shader variable at the given location.
    This is equivalent to calling METAENGINE_Render_SetUniformiv(location, 1, 1, &value). */
void METAENGINE_Render_SetUniformi(int location, int value);

/*! Sets the value of the integer uniform shader variable at the given location. */
void METAENGINE_Render_SetUniformiv(int location, int num_elements_per_value, int num_values,
                                    int *values);

/*! Fills "values" with the value of the uniform shader variable at the given location. */
void METAENGINE_Render_GetUniformuiv(UInt32 program_object, int location, unsigned int *values);

/*! Sets the value of the unsigned integer uniform shader variable at the given location.
    This is equivalent to calling METAENGINE_Render_SetUniformuiv(location, 1, 1, &value). */
void METAENGINE_Render_SetUniformui(int location, unsigned int value);

/*! Sets the value of the unsigned integer uniform shader variable at the given location. */
void METAENGINE_Render_SetUniformuiv(int location, int num_elements_per_value, int num_values,
                                     unsigned int *values);

/*! Fills "values" with the value of the uniform shader variable at the given location. */
void METAENGINE_Render_GetUniformfv(UInt32 program_object, int location, float *values);

/*! Sets the value of the floating point uniform shader variable at the given location.
    This is equivalent to calling METAENGINE_Render_SetUniformfv(location, 1, 1, &value). */
void METAENGINE_Render_SetUniformf(int location, float value);

/*! Sets the value of the floating point uniform shader variable at the given location. */
void METAENGINE_Render_SetUniformfv(int location, int num_elements_per_value, int num_values,
                                    float *values);

/*! Fills "values" with the value of the uniform shader variable at the given location.  The results are identical to calling METAENGINE_Render_GetUniformfv().  Matrices are gotten in column-major order. */
void METAENGINE_Render_GetUniformMatrixfv(UInt32 program_object, int location, float *values);

/*! Sets the value of the matrix uniform shader variable at the given location.  The size of the matrices sent is specified by num_rows and num_columns.  Rows and columns must be between 2 and 4. */
void METAENGINE_Render_SetUniformMatrixfv(int location, int num_matrices, int num_rows,
                                          int num_columns, METAENGINE_Render_bool transpose,
                                          float *values);

/*! Sets a constant-value shader attribute that will be used for each rendered vertex. */
void METAENGINE_Render_SetAttributef(int location, float value);

/*! Sets a constant-value shader attribute that will be used for each rendered vertex. */
void METAENGINE_Render_SetAttributei(int location, int value);

/*! Sets a constant-value shader attribute that will be used for each rendered vertex. */
void METAENGINE_Render_SetAttributeui(int location, unsigned int value);

/*! Sets a constant-value shader attribute that will be used for each rendered vertex. */
void METAENGINE_Render_SetAttributefv(int location, int num_elements, float *value);

/*! Sets a constant-value shader attribute that will be used for each rendered vertex. */
void METAENGINE_Render_SetAttributeiv(int location, int num_elements, int *value);

/*! Sets a constant-value shader attribute that will be used for each rendered vertex. */
void METAENGINE_Render_SetAttributeuiv(int location, int num_elements, unsigned int *value);

/*! Enables a shader attribute and sets its source data. */
void METAENGINE_Render_SetAttributeSource(int num_values, METAENGINE_Render_Attribute source);

// End of ShaderInterface
/*! @} */

class Drawing {
public:
    static void drawText(std::string name, std::string text, uint8_t x, uint8_t y,
                         ImVec4 col = {1.0f, 1.0f, 1.0f, 1.0f});
    static void drawTextEx(std::string name, uint8_t x, uint8_t y, std::function<void()> func);
    static b2Vec2 rotate_point(float cx, float cy, float angle, b2Vec2 p);
    static void drawPolygon(METAENGINE_Render_Target *renderer, METAENGINE_Color col, b2Vec2 *verts,
                            int x, int y, float scale, int count, float angle, float cx, float cy);
    static uint32 darkenColor(uint32 col, float brightness);
};

#define METAENGINE_Render_GLT_NULL 0
#define METAENGINE_Render_GLT_NULL_HANDLE 0

#define METAENGINE_Render_GLT_LEFT 0
#define METAENGINE_Render_GLT_TOP 0

#define METAENGINE_Render_GLT_CENTER 1

#define METAENGINE_Render_GLT_RIGHT 2
#define METAENGINE_Render_GLT_BOTTOM 2

static GLboolean METAENGINE_Render_GLT_Initialized = GL_FALSE;

typedef struct METAENGINE_Render_GLTtext METAENGINE_Render_GLTtext;

GLboolean METAENGINE_Render_GLT_Init(void);
void METAENGINE_Render_GLT_Terminate(void);

METAENGINE_Render_GLTtext *METAENGINE_Render_GLT_CreateText(void);
void METAENGINE_Render_GLT_DeleteText(METAENGINE_Render_GLTtext *text);
#define METAENGINE_Render_GLT_DestroyText METAENGINE_Render_GLT_DeleteText

bool METAENGINE_Render_GLT_SetText(METAENGINE_Render_GLTtext *text, const char *string);
const char *METAENGINE_Render_GLT_GetText(METAENGINE_Render_GLTtext *text);

void METAENGINE_Render_GLT_Viewport(GLsizei width, GLsizei height);

void METAENGINE_Render_GLT_BeginDraw();
void METAENGINE_Render_GLT_EndDraw();

void METAENGINE_Render_GLT_DrawText(METAENGINE_Render_GLTtext *text, const GLfloat mvp[16]);

void METAENGINE_Render_GLT_DrawText2D(METAENGINE_Render_GLTtext *text, GLfloat x, GLfloat y,
                                      GLfloat scale);
void METAENGINE_Render_GLT_DrawText2DAligned(METAENGINE_Render_GLTtext *text, GLfloat x, GLfloat y,
                                             GLfloat scale, int horizontalAlignment,
                                             int verticalAlignment);

void METAENGINE_Render_GLT_DrawText3D(METAENGINE_Render_GLTtext *text, GLfloat x, GLfloat y,
                                      GLfloat z, GLfloat scale, GLfloat view[16],
                                      GLfloat projection[16]);

void METAENGINE_Render_GLT_Color(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void METAENGINE_Render_GLT_GetColor(GLfloat *r, GLfloat *g, GLfloat *b, GLfloat *a);

GLfloat METAENGINE_Render_GLT_GetLineHeight(GLfloat scale);

GLfloat METAENGINE_Render_GLT_GetTextWidth(const METAENGINE_Render_GLTtext *text, GLfloat scale);
GLfloat METAENGINE_Render_GLT_GetTextHeight(const METAENGINE_Render_GLTtext *text, GLfloat scale);

GLboolean METAENGINE_Render_GLT_IsCharacterSupported(const char c);
GLint METAENGINE_Render_GLT_CountSupportedCharacters(const char *str);

GLboolean METAENGINE_Render_GLT_IsCharacterDrawable(const char c);
GLint METAENGINE_Render_GLT_CountDrawableCharacters(const char *str);

GLint METAENGINE_Render_GLT_CountNewLines(const char *str);

// --------------------------------------------------------------------




// --------------------------------------------------------------------

// Internal API for managing window mappings
void METAENGINE_Render_AddWindowMapping(METAENGINE_Render_Target *target);
void METAENGINE_Render_RemoveWindowMapping(UInt32 windowID);
void METAENGINE_Render_RemoveWindowMappingByTarget(METAENGINE_Render_Target *target);

/*! Private implementation of renderer members. */
typedef struct METAENGINE_Render_RendererImpl
{
    /*! \see METAENGINE_Render_Init()
	 *  \see METAENGINE_Render_InitRenderer()
	 *  \see METAENGINE_Render_InitRendererByID()
	 */
    METAENGINE_Render_Target *(*Init)(METAENGINE_Render_Renderer *renderer,
                                      METAENGINE_Render_RendererID renderer_request, UInt16 w,
                                      UInt16 h, METAENGINE_Render_WindowFlagEnum SDL_flags);

    /*! \see METAENGINE_Render_CreateTargetFromWindow
     * The extra parameter is used internally to reuse/reinit a target. */
    METAENGINE_Render_Target *(*CreateTargetFromWindow)(METAENGINE_Render_Renderer *renderer,
                                                        UInt32 windowID,
                                                        METAENGINE_Render_Target *target);

    /*! \see METAENGINE_Render_SetActiveTarget() */
    METAENGINE_Render_bool (*SetActiveTarget)(METAENGINE_Render_Renderer *renderer,
                                              METAENGINE_Render_Target *target);

    /*! \see METAENGINE_Render_CreateAliasTarget() */
    METAENGINE_Render_Target *(*CreateAliasTarget)(METAENGINE_Render_Renderer *renderer,
                                                   METAENGINE_Render_Target *target);

    /*! \see METAENGINE_Render_MakeCurrent */
    void (*MakeCurrent)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                        UInt32 windowID);

    /*! Sets up this renderer to act as the current renderer.  Called automatically by METAENGINE_Render_SetCurrentRenderer(). */
    void (*SetAsCurrent)(METAENGINE_Render_Renderer *renderer);

    /*! \see METAENGINE_Render_ResetRendererState() */
    void (*ResetRendererState)(METAENGINE_Render_Renderer *renderer);

    /*! \see METAENGINE_Render_AddDepthBuffer() */
    METAENGINE_Render_bool (*AddDepthBuffer)(METAENGINE_Render_Renderer *renderer,
                                             METAENGINE_Render_Target *target);

    /*! \see METAENGINE_Render_SetWindowResolution() */
    METAENGINE_Render_bool (*SetWindowResolution)(METAENGINE_Render_Renderer *renderer, UInt16 w,
                                                  UInt16 h);

    /*! \see METAENGINE_Render_SetVirtualResolution() */
    void (*SetVirtualResolution)(METAENGINE_Render_Renderer *renderer,
                                 METAENGINE_Render_Target *target, UInt16 w, UInt16 h);

    /*! \see METAENGINE_Render_UnsetVirtualResolution() */
    void (*UnsetVirtualResolution)(METAENGINE_Render_Renderer *renderer,
                                   METAENGINE_Render_Target *target);

    /*! Clean up the renderer state. */
    void (*Quit)(METAENGINE_Render_Renderer *renderer);

    /*! \see METAENGINE_Render_SetFullscreen() */
    METAENGINE_Render_bool (*SetFullscreen)(METAENGINE_Render_Renderer *renderer,
                                            METAENGINE_Render_bool enable_fullscreen,
                                            METAENGINE_Render_bool use_desktop_resolution);

    /*! \see METAENGINE_Render_SetCamera() */
    METAENGINE_Render_Camera (*SetCamera)(METAENGINE_Render_Renderer *renderer,
                                          METAENGINE_Render_Target *target,
                                          METAENGINE_Render_Camera *cam);

    /*! \see METAENGINE_Render_CreateImage() */
    METAENGINE_Render_Image *(*CreateImage)(METAENGINE_Render_Renderer *renderer, UInt16 w,
                                            UInt16 h, METAENGINE_Render_FormatEnum format);

    /*! \see METAENGINE_Render_CreateImageUsingTexture() */
    METAENGINE_Render_Image *(*CreateImageUsingTexture)(METAENGINE_Render_Renderer *renderer,
                                                        METAENGINE_Render_TextureHandle handle,
                                                        METAENGINE_Render_bool take_ownership);

    /*! \see METAENGINE_Render_CreateAliasImage() */
    METAENGINE_Render_Image *(*CreateAliasImage)(METAENGINE_Render_Renderer *renderer,
                                                 METAENGINE_Render_Image *image);

    /*! \see METAENGINE_Render_CopyImage() */
    METAENGINE_Render_Image *(*CopyImage)(METAENGINE_Render_Renderer *renderer,
                                          METAENGINE_Render_Image *image);

    /*! \see METAENGINE_Render_UpdateImage */
    void (*UpdateImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image,
                        const METAENGINE_Render_Rect *image_rect, void *surface,
                        const METAENGINE_Render_Rect *surface_rect);

    /*! \see METAENGINE_Render_UpdateImageBytes */
    void (*UpdateImageBytes)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image,
                             const METAENGINE_Render_Rect *image_rect, const unsigned char *bytes,
                             int bytes_per_row);

    /*! \see METAENGINE_Render_ReplaceImage */
    METAENGINE_Render_bool (*ReplaceImage)(METAENGINE_Render_Renderer *renderer,
                                           METAENGINE_Render_Image *image, void *surface,
                                           const METAENGINE_Render_Rect *surface_rect);

    /*! \see METAENGINE_Render_CopyImageFromSurface() */
    METAENGINE_Render_Image *(*CopyImageFromSurface)(METAENGINE_Render_Renderer *renderer,
                                                     void *surface,
                                                     const METAENGINE_Render_Rect *surface_rect);

    /*! \see METAENGINE_Render_CopyImageFromTarget() */
    METAENGINE_Render_Image *(*CopyImageFromTarget)(METAENGINE_Render_Renderer *renderer,
                                                    METAENGINE_Render_Target *target);

    /*! \see METAENGINE_Render_CopySurfaceFromTarget() */
    void *(*CopySurfaceFromTarget)(METAENGINE_Render_Renderer *renderer,
                                   METAENGINE_Render_Target *target);

    /*! \see METAENGINE_Render_CopySurfaceFromImage() */
    void *(*CopySurfaceFromImage)(METAENGINE_Render_Renderer *renderer,
                                  METAENGINE_Render_Image *image);

    /*! \see METAENGINE_Render_FreeImage() */
    void (*FreeImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);

    /*! \see METAENGINE_Render_GetTarget() */
    METAENGINE_Render_Target *(*GetTarget)(METAENGINE_Render_Renderer *renderer,
                                           METAENGINE_Render_Image *image);

    /*! \see METAENGINE_Render_FreeTarget() */
    void (*FreeTarget)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);

    /*! \see METAENGINE_Render_Blit() */
    void (*Blit)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image,
                 METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x,
                 float y);

    /*! \see METAENGINE_Render_BlitRotate() */
    void (*BlitRotate)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image,
                       METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x,
                       float y, float degrees);

    /*! \see METAENGINE_Render_BlitScale() */
    void (*BlitScale)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image,
                      METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target, float x,
                      float y, float scaleX, float scaleY);

    /*! \see METAENGINE_Render_BlitTransform */
    void (*BlitTransform)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image,
                          METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target,
                          float x, float y, float degrees, float scaleX, float scaleY);

    /*! \see METAENGINE_Render_BlitTransformX() */
    void (*BlitTransformX)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image,
                           METAENGINE_Render_Rect *src_rect, METAENGINE_Render_Target *target,
                           float x, float y, float pivot_x, float pivot_y, float degrees,
                           float scaleX, float scaleY);

    /*! \see METAENGINE_Render_PrimitiveBatchV() */
    void (*PrimitiveBatchV)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image,
                            METAENGINE_Render_Target *target,
                            METAENGINE_Render_PrimitiveEnum primitive_type,
                            unsigned short num_vertices, void *values, unsigned int num_indices,
                            unsigned short *indices, METAENGINE_Render_BatchFlagEnum flags);

    /*! \see METAENGINE_Render_GenerateMipmaps() */
    void (*GenerateMipmaps)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image);

    /*! \see METAENGINE_Render_SetClip() */
    METAENGINE_Render_Rect (*SetClip)(METAENGINE_Render_Renderer *renderer,
                                      METAENGINE_Render_Target *target, Int16 x, Int16 y, UInt16 w,
                                      UInt16 h);

    /*! \see METAENGINE_Render_UnsetClip() */
    void (*UnsetClip)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);

    /*! \see METAENGINE_Render_GetPixel() */
    METAENGINE_Color (*GetPixel)(METAENGINE_Render_Renderer *renderer,
                                 METAENGINE_Render_Target *target, Int16 x, Int16 y);

    /*! \see METAENGINE_Render_SetImageFilter() */
    void (*SetImageFilter)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image,
                           METAENGINE_Render_FilterEnum filter);

    /*! \see METAENGINE_Render_SetWrapMode() */
    void (*SetWrapMode)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image,
                        METAENGINE_Render_WrapEnum wrap_mode_x,
                        METAENGINE_Render_WrapEnum wrap_mode_y);

    /*! \see METAENGINE_Render_GetTextureHandle() */
    METAENGINE_Render_TextureHandle (*GetTextureHandle)(METAENGINE_Render_Renderer *renderer,
                                                        METAENGINE_Render_Image *image);

    /*! \see METAENGINE_Render_ClearRGBA() */
    void (*ClearRGBA)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                      UInt8 r, UInt8 g, UInt8 b, UInt8 a);
    /*! \see METAENGINE_Render_FlushBlitBuffer() */
    void (*FlushBlitBuffer)(METAENGINE_Render_Renderer *renderer);
    /*! \see METAENGINE_Render_Flip() */
    void (*Flip)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target);

    /*! \see METAENGINE_Render_CreateShaderProgram() */
    UInt32 (*CreateShaderProgram)(METAENGINE_Render_Renderer *renderer);

    /*! \see METAENGINE_Render_FreeShaderProgram() */
    void (*FreeShaderProgram)(METAENGINE_Render_Renderer *renderer, UInt32 program_object);

    UInt32 (*CompileShaderInternal)(METAENGINE_Render_Renderer *renderer,
                                    METAENGINE_Render_ShaderEnum shader_type,
                                    const char *shader_source);

    /*! \see METAENGINE_Render_CompileShader() */
    UInt32 (*CompileShader)(METAENGINE_Render_Renderer *renderer,
                            METAENGINE_Render_ShaderEnum shader_type, const char *shader_source);

    /*! \see METAENGINE_Render_FreeShader() */
    void (*FreeShader)(METAENGINE_Render_Renderer *renderer, UInt32 shader_object);

    /*! \see METAENGINE_Render_AttachShader() */
    void (*AttachShader)(METAENGINE_Render_Renderer *renderer, UInt32 program_object,
                         UInt32 shader_object);

    /*! \see METAENGINE_Render_DetachShader() */
    void (*DetachShader)(METAENGINE_Render_Renderer *renderer, UInt32 program_object,
                         UInt32 shader_object);

    /*! \see METAENGINE_Render_LinkShaderProgram() */
    METAENGINE_Render_bool (*LinkShaderProgram)(METAENGINE_Render_Renderer *renderer,
                                                UInt32 program_object);

    /*! \see METAENGINE_Render_ActivateShaderProgram() */
    void (*ActivateShaderProgram)(METAENGINE_Render_Renderer *renderer, UInt32 program_object,
                                  METAENGINE_Render_ShaderBlock *block);

    /*! \see METAENGINE_Render_DeactivateShaderProgram() */
    void (*DeactivateShaderProgram)(METAENGINE_Render_Renderer *renderer);

    /*! \see METAENGINE_Render_GetShaderMessage() */
    const char *(*GetShaderMessage)(METAENGINE_Render_Renderer *renderer);

    /*! \see METAENGINE_Render_GetAttribLocation() */
    int (*GetAttributeLocation)(METAENGINE_Render_Renderer *renderer, UInt32 program_object,
                                const char *attrib_name);

    /*! \see METAENGINE_Render_GetUniformLocation() */
    int (*GetUniformLocation)(METAENGINE_Render_Renderer *renderer, UInt32 program_object,
                              const char *uniform_name);

    /*! \see METAENGINE_Render_LoadShaderBlock() */
    METAENGINE_Render_ShaderBlock (*LoadShaderBlock)(
            METAENGINE_Render_Renderer *renderer, UInt32 program_object, const char *position_name,
            const char *texcoord_name, const char *color_name, const char *modelViewMatrix_name);

    /*! \see METAENGINE_Render_SetShaderBlock() */
    void (*SetShaderBlock)(METAENGINE_Render_Renderer *renderer,
                           METAENGINE_Render_ShaderBlock block);

    /*! \see METAENGINE_Render_SetShaderImage() */
    void (*SetShaderImage)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Image *image,
                           int location, int image_unit);

    /*! \see METAENGINE_Render_GetUniformiv() */
    void (*GetUniformiv)(METAENGINE_Render_Renderer *renderer, UInt32 program_object, int location,
                         int *values);

    /*! \see METAENGINE_Render_SetUniformi() */
    void (*SetUniformi)(METAENGINE_Render_Renderer *renderer, int location, int value);

    /*! \see METAENGINE_Render_SetUniformiv() */
    void (*SetUniformiv)(METAENGINE_Render_Renderer *renderer, int location,
                         int num_elements_per_value, int num_values, int *values);

    /*! \see METAENGINE_Render_GetUniformuiv() */
    void (*GetUniformuiv)(METAENGINE_Render_Renderer *renderer, UInt32 program_object, int location,
                          unsigned int *values);

    /*! \see METAENGINE_Render_SetUniformui() */
    void (*SetUniformui)(METAENGINE_Render_Renderer *renderer, int location, unsigned int value);

    /*! \see METAENGINE_Render_SetUniformuiv() */
    void (*SetUniformuiv)(METAENGINE_Render_Renderer *renderer, int location,
                          int num_elements_per_value, int num_values, unsigned int *values);

    /*! \see METAENGINE_Render_GetUniformfv() */
    void (*GetUniformfv)(METAENGINE_Render_Renderer *renderer, UInt32 program_object, int location,
                         float *values);

    /*! \see METAENGINE_Render_SetUniformf() */
    void (*SetUniformf)(METAENGINE_Render_Renderer *renderer, int location, float value);

    /*! \see METAENGINE_Render_SetUniformfv() */
    void (*SetUniformfv)(METAENGINE_Render_Renderer *renderer, int location,
                         int num_elements_per_value, int num_values, float *values);

    /*! \see METAENGINE_Render_SetUniformMatrixfv() */
    void (*SetUniformMatrixfv)(METAENGINE_Render_Renderer *renderer, int location, int num_matrices,
                               int num_rows, int num_columns, METAENGINE_Render_bool transpose,
                               float *values);

    /*! \see METAENGINE_Render_SetAttributef() */
    void (*SetAttributef)(METAENGINE_Render_Renderer *renderer, int location, float value);

    /*! \see METAENGINE_Render_SetAttributei() */
    void (*SetAttributei)(METAENGINE_Render_Renderer *renderer, int location, int value);

    /*! \see METAENGINE_Render_SetAttributeui() */
    void (*SetAttributeui)(METAENGINE_Render_Renderer *renderer, int location, unsigned int value);

    /*! \see METAENGINE_Render_SetAttributefv() */
    void (*SetAttributefv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements,
                           float *value);

    /*! \see METAENGINE_Render_SetAttributeiv() */
    void (*SetAttributeiv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements,
                           int *value);

    /*! \see METAENGINE_Render_SetAttributeuiv() */
    void (*SetAttributeuiv)(METAENGINE_Render_Renderer *renderer, int location, int num_elements,
                            unsigned int *value);

    /*! \see METAENGINE_Render_SetAttributeSource() */
    void (*SetAttributeSource)(METAENGINE_Render_Renderer *renderer, int num_values,
                               METAENGINE_Render_Attribute source);

    // Shapes

    /*! \see METAENGINE_Render_SetLineThickness() */
    float (*SetLineThickness)(METAENGINE_Render_Renderer *renderer, float thickness);

    /*! \see METAENGINE_Render_GetLineThickness() */
    float (*GetLineThickness)(METAENGINE_Render_Renderer *renderer);

    /*! \see METAENGINE_Render_Pixel() */
    void (*Pixel)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x,
                  float y, METAENGINE_Color color);

    /*! \see METAENGINE_Render_Line() */
    void (*Line)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1,
                 float y1, float x2, float y2, METAENGINE_Color color);

    /*! \see METAENGINE_Render_Arc() */
    void (*Arc)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x,
                float y, float radius, float start_angle, float end_angle, METAENGINE_Color color);

    /*! \see METAENGINE_Render_ArcFilled() */
    void (*ArcFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                      float x, float y, float radius, float start_angle, float end_angle,
                      METAENGINE_Color color);

    /*! \see METAENGINE_Render_Circle() */
    void (*Circle)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x,
                   float y, float radius, METAENGINE_Color color);

    /*! \see METAENGINE_Render_CircleFilled() */
    void (*CircleFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                         float x, float y, float radius, METAENGINE_Color color);

    /*! \see METAENGINE_Render_Ellipse() */
    void (*Ellipse)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x,
                    float y, float rx, float ry, float degrees, METAENGINE_Color color);

    /*! \see METAENGINE_Render_EllipseFilled() */
    void (*EllipseFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                          float x, float y, float rx, float ry, float degrees,
                          METAENGINE_Color color);

    /*! \see METAENGINE_Render_Sector() */
    void (*Sector)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x,
                   float y, float inner_radius, float outer_radius, float start_angle,
                   float end_angle, METAENGINE_Color color);

    /*! \see METAENGINE_Render_SectorFilled() */
    void (*SectorFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                         float x, float y, float inner_radius, float outer_radius,
                         float start_angle, float end_angle, METAENGINE_Color color);

    /*! \see METAENGINE_Render_Tri() */
    void (*Tri)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target, float x1,
                float y1, float x2, float y2, float x3, float y3, METAENGINE_Color color);

    /*! \see METAENGINE_Render_TriFilled() */
    void (*TriFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                      float x1, float y1, float x2, float y2, float x3, float y3,
                      METAENGINE_Color color);

    /*! \see METAENGINE_Render_Rectangle() */
    void (*Rectangle)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                      float x1, float y1, float x2, float y2, METAENGINE_Color color);

    /*! \see METAENGINE_Render_RectangleFilled() */
    void (*RectangleFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                            float x1, float y1, float x2, float y2, METAENGINE_Color color);

    /*! \see METAENGINE_Render_RectangleRound() */
    void (*RectangleRound)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                           float x1, float y1, float x2, float y2, float radius,
                           METAENGINE_Color color);

    /*! \see METAENGINE_Render_RectangleRoundFilled() */
    void (*RectangleRoundFilled)(METAENGINE_Render_Renderer *renderer,
                                 METAENGINE_Render_Target *target, float x1, float y1, float x2,
                                 float y2, float radius, METAENGINE_Color color);

    /*! \see METAENGINE_Render_Polygon() */
    void (*Polygon)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                    unsigned int num_vertices, float *vertices, METAENGINE_Color color);

    /*! \see METAENGINE_Render_Polyline() */
    void (*Polyline)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                     unsigned int num_vertices, float *vertices, METAENGINE_Color color,
                     METAENGINE_Render_bool close_loop);

    /*! \see METAENGINE_Render_PolygonFilled() */
    void (*PolygonFilled)(METAENGINE_Render_Renderer *renderer, METAENGINE_Render_Target *target,
                          unsigned int num_vertices, float *vertices, METAENGINE_Color color);

} METAENGINE_Render_RendererImpl;

#endif
