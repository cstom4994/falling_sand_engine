#ifndef _METADOT_RENDER_BACKEND_SOKOL_H_
#define _METADOT_RENDER_BACKEND_SOKOL_H_

#if defined(METADOT_SOKOL)

#define RF_DEFAULT_GFX_BACKEND_INIT_DATA (NULL)

typedef float R_gfx_vertex_data_type;
typedef float R_gfx_texcoord_data_type;
typedef unsigned char R_gfx_color_data_type;
#if defined(METADOT_BACKEND_GL)
typedef unsigned int R_gfx_vertex_index_data_type;
#else
typedef unsigned short R_gfx_vertex_index_data_type;
#endif

#define RF_GFX_VERTEX_COMPONENT_COUNT (3 * 4)  // 3 float by vertex, 4 vertex by quad
#define RF_GFX_TEXCOORD_COMPONENT_COUNT (2 * 4)// 2 float by texcoord, 4 texcoord by quad
#define RF_GFX_COLOR_COMPONENT_COUNT (4 * 4)   // 4 float by color, 4 colors by quad
#define RF_GFX_VERTEX_INDEX_COMPONENT_COUNT (6)// 6 int by quad (indices)

// Dynamic vertex buffers (position + texcoords + colors + indices arrays)
typedef struct R_vertex_buffer
{
    int elements_count;// Number of elements in the buffer (QUADS)
    int v_counter;     // Vertex position counter to process (and draw) from full buffer
    int tc_counter;    // Vertex texcoord counter to process (and draw) from full buffer
    int c_counter;     // Vertex color counter to process (and draw) from full buffer

    unsigned int vao_id;   // OpenGL Vertex Array Object id
    unsigned int vbo_id[4];// OpenGL Vertex Buffer Objects id (4 types of vertex data)

    R_gfx_vertex_data_type
            *vertices;// Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
    R_gfx_texcoord_data_type *
            texcoords;// Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
    R_gfx_color_data_type
            *colors;// Vertex colors (RGBA - 4 components per vertex) (shader-location = 3)
    R_gfx_vertex_index_data_type
            *indices;// Vertex indices (in case vertex data comes indexed) (6 indices per quad)
} R_vertex_buffer;

typedef struct R_draw_call
{
    R_drawing_mode mode;// Drawing mode: RF_LINES, RF_TRIANGLES, RF_QUADS
    int vertex_count;    // Number of vertex of the draw
    int vertex_alignment;// Number of vertex required for index alignment (LINES, TRIANGLES)
    //unsigned int vao_id;   // Vertex array id to be used on the draw
    //unsigned int shaderId; // R_shader id to be used on the draw
    unsigned int texture_id;// R_texture id to be used on the draw
    // TODO: Support additional texture units?

    //R_mat projection;     // Projection matrix for this draw
    //R_mat modelview;      // Modelview matrix for this draw
} R_draw_call;

typedef struct R_gfx_backend_data
{
    struct
    {
        R_bool tex_comp_dxt_supported;          // DDS texture compression support
        R_bool tex_comp_etc1_supported;         // ETC1 texture compression support
        R_bool tex_comp_etc2_supported;         // ETC2/EAC texture compression support
        R_bool tex_comp_pvrt_supported;         // PVR texture compression support
        R_bool tex_comp_astc_supported;         // ASTC texture compression support
        R_bool tex_npot_supported;              // NPOT textures full support
        R_bool tex_float_supported;             // float textures support (32 bit per channel)
        R_bool tex_depth_supported;             // Depth textures supported
        int max_depth_bits;                      // Maximum bits for depth component
        R_bool tex_mirror_clamp_supported;      // Clamp mirror wrap mode supported
        R_bool tex_anisotropic_filter_supported;// Anisotropic texture filtering support
        float max_anisotropic_level;   // Maximum anisotropy level supported (minimum is 2.0f)
        R_bool debug_marker_supported;// Debug marker support
    } extensions;
} R_gfx_backend_data;

#endif

#endif