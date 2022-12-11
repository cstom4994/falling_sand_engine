#ifndef _METADOT_GFX_H_
#define _METADOT_GFX_H_

#include "Engine/Renderer/Render.h"

// Camera projection modes
typedef enum R_camera_type {
    RF_CAMERA_PERSPECTIVE = 0,
    RF_CAMERA_ORTHOGRAPHIC
} R_camera_type;

typedef struct R_camera2d
{
    R_vec2 offset; // Camera offset (displacement from target)
    R_vec2 target; // Camera target (rotation and zoom origin)
    float rotation;// Camera rotation in degrees
    float zoom;    // Camera zoom (scaling), should be 1.0f by default
} R_camera2d;

typedef struct R_camera3d
{
    R_camera_type
            type;// Camera type, defines projection types: RF_CAMERA_PERSPECTIVE or RF_CAMERA_ORTHOGRAPHIC
    R_vec3 position;// Camera position
    R_vec3 target;  // Camera target it looks-at
    R_vec3 up;      // Camera up vector (rotation over its axis)
    float fovy;// Camera field-of-view apperture in Y (degrees) in perspective, used as near plane width in orthographic
} R_camera3d;

R_public R_vec3 R_unproject(R_vec3 source, R_mat proj,
                            R_mat view);// Get world coordinates from screen coordinates
R_public R_ray R_get_mouse_ray(R_sizei screen_size, R_vec2 mouse_position,
                               R_camera3d camera);      // Returns a ray trace from mouse position
R_public R_mat R_get_camera_matrix(R_camera3d camera);  // Get transform matrix for camera
R_public R_mat R_get_camera_matrix2d(R_camera2d camera);// Returns camera 2d transform matrix
R_public R_vec2 R_get_world_to_screen(
        R_sizei screen_size, R_vec3 position,
        R_camera3d camera);// Returns the screen space position from a 3d world space position
R_public R_vec2 R_get_world_to_screen2d(
        R_vec2 position,
        R_camera2d camera);// Returns the screen space position for a 2d camera world space position
R_public R_vec2 R_get_screen_to_world2d(
        R_vec2 position,
        R_camera2d camera);// Returns the world space position for a 2d camera screen space position

#pragma region builtin camera

// Camera system modes
typedef enum R_builtin_camera3d_mode {
    RF_CAMERA_CUSTOM = 0,
    RF_CAMERA_FREE,
    RF_CAMERA_ORBITAL,
    RF_CAMERA_FIRST_PERSON,
    RF_CAMERA_THIRD_PERSON
} R_builtin_camera3d_mode;

typedef struct R_camera3d_state
{
    R_vec2 camera_angle;         // R_camera3d angle in plane XZ
    float camera_target_distance;// R_camera3d distance from position to target
    float player_eyes_position;
    R_builtin_camera3d_mode camera_mode;// Current camera mode
    int swing_counter;                  // Used for 1st person swinging movement
    R_vec2 previous_mouse_position;
} R_camera3d_state;

typedef struct R_input_state_for_update_camera
{
    R_vec2 mouse_position;
    int mouse_wheel_move;                    // Mouse wheel movement Y
    R_bool is_camera_pan_control_key_down;   // Middle mouse button
    R_bool is_camera_alt_control_key_down;   // Left Alt Key
    R_bool is_camera_smooth_zoom_control_key;// Left Control Key
    R_bool direction_keys[6];                // 'W', 'S', 'D', 'A', 'E', 'Q'
} R_input_state_for_update_camera;

R_public void R_set_camera3d_mode(R_camera3d_state *state, R_camera3d camera,
                                  R_builtin_camera3d_mode mode);
R_public void R_update_camera3d(R_camera3d *camera, R_camera3d_state *state,
                                R_input_state_for_update_camera input_state);

#pragma endregion

#define R_lightgray (R_lit(R_color){200, 200, 200, 255})
#define R_gray (R_lit(R_color){130, 130, 130, 255})
#define R_dark_gray (R_lit(R_color){80, 80, 80, 255})
#define R_yellow (R_lit(R_color){253, 249, 0, 255})
#define R_gold (R_lit(R_color){255, 203, 0, 255})
#define R_orange (R_lit(R_color){255, 161, 0, 255})
#define R_pink (R_lit(R_color){255, 109, 194, 255})
#define R_red (R_lit(R_color){230, 41, 55, 255})
#define R_maroon (R_lit(R_color){190, 33, 55, 255})
#define R_green (R_lit(R_color){0, 228, 48, 255})
#define R_lime (R_lit(R_color){0, 158, 47, 255})
#define R_dark_green (R_lit(R_color){0, 117, 44, 255})
#define R_sky_blue (R_lit(R_color){102, 191, 255, 255})
#define R_blue (R_lit(R_color){0, 121, 241, 255})
#define R_dark_blue (R_lit(R_color){0, 82, 172, 255})
#define R_purple (R_lit(R_color){200, 122, 255, 255})
#define R_violet (R_lit(R_color){135, 60, 190, 255})
#define R_dark_purple (R_lit(R_color){112, 31, 126, 255})
#define R_beige (R_lit(R_color){211, 176, 131, 255})
#define R_brown (R_lit(R_color){127, 106, 79, 255})
#define R_dark_brown (R_lit(R_color){76, 63, 47, 255})

#define R_white (R_lit(R_color){255, 255, 255, 255})
#define R_black (R_lit(R_color){0, 0, 0, 255})
#define R_blank (R_lit(R_color){0, 0, 0, 0})
#define R_magenta (R_lit(R_color){255, 0, 255, 255})
#define R_raywhite (R_lit(R_color){245, 245, 245, 255})

#define R_default_key_color (R_magenta)

typedef enum R_pixel_format {
    R_pixel_format_grayscale = 1,                           // 8 bit per pixel (no alpha)
    R_pixel_format_gray_alpha,                              // 8 * 2 bpp (2 channels)
    R_pixel_format_r5g6b5,                                  // 16 bpp
    R_pixel_format_r8g8b8,                                  // 24 bpp
    R_pixel_format_r5g5b5a1,                                // 16 bpp (1 bit alpha)
    R_pixel_format_r4g4b4a4,                                // 16 bpp (4 bit alpha)
    R_pixel_format_r8g8b8a8,                                // 32 bpp
    R_pixel_format_r32,                                     // 32 bpp (1 channel - float)
    R_pixel_format_r32g32b32,                               // 32 * 3 bpp (3 channels - float)
    R_pixel_format_r32g32b32a32,                            // 32 * 4 bpp (4 channels - float)
    R_pixel_format_normalized = R_pixel_format_r32g32b32a32,// 32 * 4 bpp (4 channels - float)
    R_pixel_format_dxt1_rgb,                                // 4 bpp (no alpha)
    R_pixel_format_dxt1_rgba,                               // 4 bpp (1 bit alpha)
    R_pixel_format_dxt3_rgba,                               // 8 bpp
    R_pixel_format_dxt5_rgba,                               // 8 bpp
    R_pixel_format_etc1_rgb,                                // 4 bpp
    R_pixel_format_etc2_rgb,                                // 4 bpp
    R_pixel_format_etc2_eac_rgba,                           // 8 bpp
    R_pixel_format_pvrt_rgb,                                // 4 bpp
    R_pixel_format_prvt_rgba,                               // 4 bpp
    R_pixel_format_astc_4x4_rgba,                           // 8 bpp
    R_pixel_format_astc_8x8_rgba                            // 2 bpp
} R_pixel_format;

typedef enum R_pixel_format R_compressed_pixel_format;
typedef enum R_pixel_format R_uncompressed_pixel_format;

// R8G8B8A8 format
typedef struct R_color
{
    unsigned char r, g, b, a;
} R_color;

typedef struct R_palette
{
    R_color *colors;
    int count;
} R_palette;

#pragma region pixel format
R_public const char *R_pixel_format_string(R_pixel_format format);
R_public R_bool R_is_uncompressed_format(R_pixel_format format);
R_public R_bool R_is_compressed_format(R_pixel_format format);
R_public int R_bits_per_pixel(R_pixel_format format);
R_public int R_bytes_per_pixel(R_uncompressed_pixel_format format);
R_public int R_pixel_buffer_size(int width, int height, R_pixel_format format);

R_public R_bool R_format_pixels_to_normalized(const void *src, R_int src_size,
                                              R_uncompressed_pixel_format src_format, R_vec4 *dst,
                                              R_int dst_size);
R_public R_bool R_format_pixels_to_rgba32(const void *src, R_int src_size,
                                          R_uncompressed_pixel_format src_format, R_color *dst,
                                          R_int dst_size);
R_public R_bool R_format_pixels(const void *src, R_int src_size,
                                R_uncompressed_pixel_format src_format, void *dst, R_int dst_size,
                                R_uncompressed_pixel_format dst_format);

R_public R_vec4 R_format_one_pixel_to_normalized(const void *src,
                                                 R_uncompressed_pixel_format src_format);
R_public R_color R_format_one_pixel_to_rgba32(const void *src,
                                              R_uncompressed_pixel_format src_format);
R_public void R_format_one_pixel(const void *src, R_uncompressed_pixel_format src_format, void *dst,
                                 R_uncompressed_pixel_format dst_format);
#pragma endregion

#pragma region color
R_public R_bool R_color_match_rgb(
        R_color a,
        R_color b);// Returns true if the two colors have the same values for the rgb components
R_public R_bool R_color_match(R_color a,
                              R_color b);  // Returns true if the two colors have the same values
R_public int R_color_to_int(R_color color);// Returns hexadecimal value for a R_color
R_public R_vec4 R_color_normalize(R_color color);// Returns color normalized as float [0..1]
R_public R_color
R_color_from_normalized(R_vec4 normalized);// Returns color from normalized values [0..1]
R_public R_vec3 R_color_to_hsv(
        R_color color);// Returns HSV values for a R_color. Hue is returned as degrees [0..360]
R_public R_color R_color_from_hsv(
        R_vec3 hsv);// Returns a R_color from HSV values. R_color->HSV->R_color conversion will not yield exactly the same color due to rounding errors. Implementation reference: https://en.wikipedia.org/wiki/HSL_and_HSV#Alternative_HSV_conversion
R_public R_color R_color_from_int(int hex_value);// Returns a R_color struct from hexadecimal value
R_public R_color R_fade(R_color color,
                        float alpha);// R_color fade-in or fade-out, alpha goes from 0.0f to 1.0f
#pragma endregion

typedef enum R_desired_channels {
    RF_ANY_CHANNELS = 0,
    RF_1BYTE_GRAYSCALE = 1,
    RF_2BYTE_GRAY_ALPHA = 2,
    RF_3BYTE_R8G8B8 = 3,
    RF_4BYTE_R8G8B8A8 = 4,
} R_desired_channels;

typedef struct R_image
{
    void *data;           // image raw data
    int width;            // image base width
    int height;           // image base height
    R_pixel_format format;// Data format (R_pixel_format type)
    R_bool valid;         // True if the image is valid and can be used
} R_image;

typedef struct R_mipmaps_stats
{
    int possible_mip_counts;
    int mipmaps_buffer_size;
} R_mipmaps_stats;

typedef struct R_mipmaps_image
{
    union {
        R_image image;
        struct
        {
            void *data;           // image raw data
            int width;            // image base width
            int height;           // image base height
            R_pixel_format format;// Data format (R_pixel_format type)
            R_bool valid;
        };
    };

    int mipmaps;// Mipmap levels, 1 by default
} R_mipmaps_image;

typedef struct R_gif
{
    int frames_count;
    int *frame_delays;

    union {
        R_image image;

        struct
        {
            void *data;           // R_image raw data
            int width;            // R_image base width
            int height;           // R_image base height
            R_pixel_format format;// Data format (R_pixel_format type)
            R_bool valid;
        };
    };
} R_gif;

#pragma region extract image data functions
R_public int R_image_size(R_image image);
R_public int R_image_size_in_format(R_image image, R_pixel_format format);

R_public R_bool R_image_get_pixels_as_rgba32_to_buffer(R_image image, R_color *dst, R_int dst_size);
R_public R_bool R_image_get_pixels_as_normalized_to_buffer(R_image image, R_vec4 *dst,
                                                           R_int dst_size);

R_public R_color *R_image_pixels_to_rgba32(R_image image, R_allocator allocator);
R_public R_vec4 *R_image_compute_pixels_to_normalized(R_image image, R_allocator allocator);

R_public void R_image_extract_palette_to_buffer(R_image image, R_color *palette_dst,
                                                R_int palette_size);
R_public R_palette R_image_extract_palette(R_image image, R_int palette_size,
                                           R_allocator allocator);
R_public R_rec R_image_alpha_border(R_image image, float threshold);
#pragma endregion

#pragma region loading &unloading functions
R_public R_bool R_supports_image_file_type(const char *filename);

R_public R_image R_load_image_from_file_data_to_buffer(const void *src, R_int src_size, void *dst,
                                                       R_int dst_size, R_desired_channels channels,
                                                       R_allocator temp_allocator);
R_public R_image R_load_image_from_file_data(const void *src, R_int src_size,
                                             R_desired_channels channels, R_allocator allocator,
                                             R_allocator temp_allocator);

R_public R_image R_load_image_from_hdr_file_data_to_buffer(const void *src, R_int src_size,
                                                           void *dst, R_int dst_size,
                                                           R_desired_channels channels,
                                                           R_allocator temp_allocator);
R_public R_image R_load_image_from_hdr_file_data(const void *src, R_int src_size,
                                                 R_allocator allocator, R_allocator temp_allocator);

R_public R_image R_load_image_from_format_to_buffer(const void *src, R_int src_size, int src_width,
                                                    int src_height,
                                                    R_uncompressed_pixel_format src_format,
                                                    void *dst, R_int dst_size,
                                                    R_uncompressed_pixel_format dst_format);
R_public R_image R_load_image_from_file(const char *filename, R_allocator allocator,
                                        R_allocator temp_allocator, R_io_callbacks io);

R_public void R_unload_image(R_image image, R_allocator allocator);
#pragma endregion

#pragma region mipmaps
R_public int R_mipmaps_image_size(R_mipmaps_image image);
R_public R_mipmaps_stats R_compute_mipmaps_stats(R_image image, int desired_mipmaps_count);
R_public R_mipmaps_image R_image_gen_mipmaps_to_buffer(
        R_image image, int gen_mipmaps_count, void *dst, R_int dst_size,
        R_allocator
                temp_allocator);// Generate all mipmap levels for a provided image. image.data is scaled to include mipmap levels. Mipmaps format is the same as base image
R_public R_mipmaps_image R_image_gen_mipmaps(R_image image, int desired_mipmaps_count,
                                             R_allocator allocator, R_allocator temp_allocator);
R_public void R_unload_mipmaps_image(R_mipmaps_image image, R_allocator allocator);
#pragma endregion

#pragma region dds
R_public R_int R_get_dds_image_size(const void *src, R_int src_size);
R_public R_mipmaps_image R_load_dds_image_to_buffer(const void *src, R_int src_size, void *dst,
                                                    R_int dst_size);
R_public R_mipmaps_image R_load_dds_image(const void *src, R_int src_size, R_allocator allocator);
R_public R_mipmaps_image R_load_dds_image_from_file(const char *file, R_allocator allocator,
                                                    R_allocator temp_allocator, R_io_callbacks io);
#pragma endregion

#pragma region pkm
R_public R_int R_get_pkm_image_size(const void *src, R_int src_size);
R_public R_image R_load_pkm_image_to_buffer(const void *src, R_int src_size, void *dst,
                                            R_int dst_size);
R_public R_image R_load_pkm_image(const void *src, R_int src_size, R_allocator allocator);
R_public R_image R_load_pkm_image_from_file(const char *file, R_allocator allocator,
                                            R_allocator temp_allocator, R_io_callbacks io);
#pragma endregion

#pragma region ktx
R_public R_int R_get_ktx_image_size(const void *src, R_int src_size);
R_public R_mipmaps_image R_load_ktx_image_to_buffer(const void *src, R_int src_size, void *dst,
                                                    R_int dst_size);
R_public R_mipmaps_image R_load_ktx_image(const void *src, R_int src_size, R_allocator allocator);
R_public R_mipmaps_image R_load_ktx_image_from_file(const char *file, R_allocator allocator,
                                                    R_allocator temp_allocator, R_io_callbacks io);
#pragma endregion

#pragma region gif
R_public R_gif R_load_animated_gif(const void *data, R_int data_size, R_allocator allocator,
                                   R_allocator temp_allocator);
R_public R_gif R_load_animated_gif_file(const char *filename, R_allocator allocator,
                                        R_allocator temp_allocator, R_io_callbacks io);
R_public R_sizei R_gif_frame_size(R_gif gif);
R_public R_image R_get_frame_from_gif(R_gif gif, int frame);
R_public void R_unload_gif(R_gif gif, R_allocator allocator);
#pragma endregion

#pragma region image gen
R_public R_vec2 R_get_seed_for_cellular_image(int seeds_per_row, int tile_size, int i,
                                              R_rand_proc rand);

R_public R_image R_gen_image_color_to_buffer(int width, int height, R_color color, R_color *dst,
                                             R_int dst_size);
R_public R_image R_gen_image_color(int width, int height, R_color color, R_allocator allocator);
R_public R_image R_gen_image_gradient_v_to_buffer(int width, int height, R_color top,
                                                  R_color bottom, R_color *dst, R_int dst_size);
R_public R_image R_gen_image_gradient_v(int width, int height, R_color top, R_color bottom,
                                        R_allocator allocator);
R_public R_image R_gen_image_gradient_h_to_buffer(int width, int height, R_color left,
                                                  R_color right, R_color *dst, R_int dst_size);
R_public R_image R_gen_image_gradient_h(int width, int height, R_color left, R_color right,
                                        R_allocator allocator);
R_public R_image R_gen_image_gradient_radial_to_buffer(int width, int height, float density,
                                                       R_color inner, R_color outer, R_color *dst,
                                                       R_int dst_size);
R_public R_image R_gen_image_gradient_radial(int width, int height, float density, R_color inner,
                                             R_color outer, R_allocator allocator);
R_public R_image R_gen_image_checked_to_buffer(int width, int height, int checks_x, int checks_y,
                                               R_color col1, R_color col2, R_color *dst,
                                               R_int dst_size);
R_public R_image R_gen_image_checked(int width, int height, int checks_x, int checks_y,
                                     R_color col1, R_color col2, R_allocator allocator);
R_public R_image R_gen_image_white_noise_to_buffer(int width, int height, float factor,
                                                   R_rand_proc rand, R_color *dst, R_int dst_size);
R_public R_image R_gen_image_white_noise(int width, int height, float factor, R_rand_proc rand,
                                         R_allocator allocator);
R_public R_image R_gen_image_perlin_noise_to_buffer(int width, int height, int offset_x,
                                                    int offset_y, float scale, R_color *dst,
                                                    R_int dst_size);
R_public R_image R_gen_image_perlin_noise(int width, int height, int offset_x, int offset_y,
                                          float scale, R_allocator allocator);
R_public R_image R_gen_image_cellular_to_buffer(int width, int height, int tile_size,
                                                R_rand_proc rand, R_color *dst, R_int dst_size);
R_public R_image R_gen_image_cellular(int width, int height, int tile_size, R_rand_proc rand,
                                      R_allocator allocator);
#pragma endregion

#pragma region image manipulation
R_public R_image R_image_copy_to_buffer(R_image image, void *dst, R_int dst_size);
R_public R_image R_image_copy(R_image image, R_allocator allocator);

R_public R_image R_image_crop_to_buffer(R_image image, R_rec crop, void *dst, R_int dst_size,
                                        R_uncompressed_pixel_format dst_format);
R_public R_image R_image_crop(R_image image, R_rec crop, R_allocator allocator);

R_public R_image R_image_resize_to_buffer(R_image image, int new_width, int new_height, void *dst,
                                          R_int dst_size, R_allocator temp_allocator);
R_public R_image R_image_resize(R_image image, int new_width, int new_height, R_allocator allocator,
                                R_allocator temp_allocator);
R_public R_image R_image_resize_nn_to_buffer(R_image image, int new_width, int new_height,
                                             void *dst, R_int dst_size);
R_public R_image R_image_resize_nn(R_image image, int new_width, int new_height,
                                   R_allocator allocator);

R_public R_image R_image_format_to_buffer(R_image image, R_uncompressed_pixel_format dst_format,
                                          void *dst, R_int dst_size);
R_public R_image R_image_format(R_image image, R_uncompressed_pixel_format new_format,
                                R_allocator allocator);

R_public R_image R_image_alpha_mask_to_buffer(R_image image, R_image alpha_mask, void *dst,
                                              R_int dst_size);
R_public R_image R_image_alpha_clear(R_image image, R_color color, float threshold,
                                     R_allocator allocator, R_allocator temp_allocator);
R_public R_image R_image_alpha_premultiply(R_image image, R_allocator allocator,
                                           R_allocator temp_allocator);

R_public R_rec R_image_alpha_crop_rec(R_image image, float threshold);
R_public R_image R_image_alpha_crop(R_image image, float threshold, R_allocator allocator);

R_public R_image R_image_dither(R_image image, int r_bpp, int g_bpp, int b_bpp, int a_bpp,
                                R_allocator allocator, R_allocator temp_allocator);

R_public void R_image_flip_vertical_in_place(R_image *image);
R_public R_image R_image_flip_vertical_to_buffer(R_image image, void *dst, R_int dst_size);
R_public R_image R_image_flip_vertical(R_image image, R_allocator allocator);

R_public void R_image_flip_horizontal_in_place(R_image *image);
R_public R_image R_image_flip_horizontal_to_buffer(R_image image, void *dst, R_int dst_size);
R_public R_image R_image_flip_horizontal(R_image image, R_allocator allocator);

R_public R_image R_image_rotate_cw_to_buffer(R_image image, void *dst, R_int dst_size);
R_public R_image R_image_rotate_cw(R_image image);
R_public R_image R_image_rotate_ccw_to_buffer(R_image image, void *dst, R_int dst_size);
R_public R_image R_image_rotate_ccw(R_image image);

R_public R_image R_image_color_tint_to_buffer(R_image image, R_color color, void *dst,
                                              R_int dst_size);
R_public R_image R_image_color_tint(R_image image, R_color color);
R_public R_image R_image_color_invert_to_buffer(R_image image, void *dst, R_int dst_size);
R_public R_image R_image_color_invert(R_image image);
R_public R_image R_image_color_grayscale_to_buffer(R_image image, void *dst, R_int dst_size);
R_public R_image R_image_color_grayscale(R_image image);
R_public R_image R_image_color_contrast_to_buffer(R_image image, float contrast, void *dst,
                                                  R_int dst_size);
R_public R_image R_image_color_contrast(R_image image, int contrast);
R_public R_image R_image_color_brightness_to_buffer(R_image image, int brightness, void *dst,
                                                    R_int dst_size);
R_public R_image R_image_color_brightness(R_image image, int brightness);
R_public R_image R_image_color_replace_to_buffer(R_image image, R_color color, R_color replace,
                                                 void *dst, R_int dst_size);
R_public R_image R_image_color_replace(R_image image, R_color color, R_color replace);

R_public void R_image_draw(R_image *dst, R_image src, R_rec src_rec, R_rec dst_rec, R_color tint,
                           R_allocator temp_allocator);
R_public void R_image_draw_rectangle(R_image *dst, R_rec rec, R_color color,
                                     R_allocator temp_allocator);
R_public void R_image_draw_rectangle_lines(R_image *dst, R_rec rec, int thick, R_color color,
                                           R_allocator temp_allocator);
#pragma endregion

#pragma region ez
#ifdef METADOT_OLD

#pragma region extract image data functions
R_public R_color *R_image_pixels_to_rgba32_ez(R_image image);
R_public R_vec4 *R_image_compute_pixels_to_normalized_ez(R_image image);
R_public R_palette R_image_extract_palette_ez(R_image image, int palette_size);
#pragma endregion

#pragma region loading &unloading functions
R_public R_image R_load_image_from_file_data_ez(const void *src, int src_size);
R_public R_image R_load_image_from_hdr_file_data_ez(const void *src, int src_size);
R_public R_image R_load_image_from_file_ez(const char *filename);
R_public void R_unload_image_ez(R_image image);
#pragma endregion

#pragma region image manipulation
R_public R_image R_image_copy_ez(R_image image);

R_public R_image R_image_crop_ez(R_image image, R_rec crop);

R_public R_image R_image_resize_ez(R_image image, int new_width, int new_height);
R_public R_image R_image_resize_nn_ez(R_image image, int new_width, int new_height);

R_public R_image R_image_format_ez(R_image image, R_uncompressed_pixel_format new_format);

R_public R_image R_image_alpha_clear_ez(R_image image, R_color color, float threshold);
R_public R_image R_image_alpha_premultiply_ez(R_image image);
R_public R_image R_image_alpha_crop_ez(R_image image, float threshold);
R_public R_image R_image_dither_ez(R_image image, int r_bpp, int g_bpp, int b_bpp, int a_bpp);

R_public R_image R_image_flip_vertical_ez(R_image image);
R_public R_image R_image_flip_horizontal_ez(R_image image);

R_public R_vec2 R_get_seed_for_cellular_image_ez(int seeds_per_row, int tile_size, int i);

R_public R_image R_gen_image_color_ez(int width, int height, R_color color);
R_public R_image R_gen_image_gradient_v_ez(int width, int height, R_color top, R_color bottom);
R_public R_image R_gen_image_gradient_h_ez(int width, int height, R_color left, R_color right);
R_public R_image R_gen_image_gradient_radial_ez(int width, int height, float density, R_color inner,
                                                R_color outer);
R_public R_image R_gen_image_checked_ez(int width, int height, int checks_x, int checks_y,
                                        R_color col1, R_color col2);
R_public R_image R_gen_image_white_noise_ez(int width, int height, float factor);
R_public R_image R_gen_image_perlin_noise_ez(int width, int height, int offset_x, int offset_y,
                                             float scale);
R_public R_image R_gen_image_cellular_ez(int width, int height, int tile_size);
#pragma endregion

#pragma region mipmaps
R_public R_mipmaps_image R_image_gen_mipmaps_ez(R_image image, int gen_mipmaps_count);
R_public void R_unload_mipmaps_image_ez(R_mipmaps_image image);
#pragma endregion

#pragma region dds
R_public R_mipmaps_image R_load_dds_image_ez(const void *src, int src_size);
R_public R_mipmaps_image R_load_dds_image_from_file_ez(const char *file);
#pragma endregion

#pragma region pkm
R_public R_image R_load_pkm_image_ez(const void *src, int src_size);
R_public R_image R_load_pkm_image_from_file_ez(const char *file);
#pragma endregion

#pragma region ktx
R_public R_mipmaps_image R_load_ktx_image_ez(const void *src, int src_size);
R_public R_mipmaps_image R_load_ktx_image_from_file_ez(const char *file);
#pragma endregion

#pragma region gif
R_public R_gif R_load_animated_gif_ez(const void *data, int data_size);
R_public R_gif R_load_animated_gif_file_ez(const char *filename);
R_public void R_unload_gif_ez(R_gif gif);
#pragma endregion

#endif// METADOT_OLD
#pragma endregion

#pragma region backend selection

#if !defined(METADOT_BACKEND_GL) && !defined(METADOT_BACKEND_GL_ES3) &&                            \
        !defined(METADOT_BACKEND_METAL) && !defined(METADOT_BACKEND_DIRECTX)
#define METADOT_NO_GRAPHICS_BACKEND_SELECTED_BY_THE_USER (1)
#endif

// If no graphics backend was set, choose OpenGL33 on desktop and OpenGL ES3 on mobile
#if METADOT_NO_GRAPHICS_BACKEND_SELECTED_BY_THE_USER
#if defined(METADOT_PLATFORM_WINDOWS) || defined(METADOT_PLATFORM_LINUX) ||                        \
        defined(METADOT_PLATFORM_APPLE)
#define METADOT_BACKEND_GL (1)
#else// if on mobile
#define METADOT_BACKEND_GL_ES3 (1)
#endif
#endif


#pragma endregion

#pragma region constants

#ifndef RF_DEFAULT_BATCH_ELEMENTS_COUNT
#if defined(METADOT_BACKEND_GL_ES3) || defined(METADOT_BACKEND_METAL)
#define RF_DEFAULT_BATCH_ELEMENTS_COUNT (2048)
#else
#define RF_DEFAULT_BATCH_ELEMENTS_COUNT (8192)
#endif
#endif

#if !defined(RF_DEFAULT_BATCH_VERTEX_BUFFERS_COUNT)
#define RF_DEFAULT_BATCH_VERTEX_BUFFERS_COUNT                                                      \
    (1)// Max number of buffers for batching (multi-buffering)
#endif

#if !defined(RF_MAX_MATRIX_STACK_SIZE)
#define RF_MAX_MATRIX_STACK_SIZE (32)// Max size of R_mat R__ctx->gl_ctx.stack
#endif

#if !defined(RF_DEFAULT_BATCH_DRAW_CALLS_COUNT)
#define RF_DEFAULT_BATCH_DRAW_CALLS_COUNT                                                          \
    (256)// Max R__ctx->gl_ctx.draws by state changes (mode, texture)
#endif

// Shader and material limits
#if !defined(RF_MAX_SHADER_LOCATIONS)
#define RF_MAX_SHADER_LOCATIONS                                                                    \
    (32)// Maximum number of predefined locations stored in shader struct
#endif

#if !defined(RF_MAX_MATERIAL_MAPS)
#define RF_MAX_MATERIAL_MAPS (12)// Maximum number of texture maps stored in shader struct
#endif

#if !defined(RF_MAX_TEXT_BUFFER_LENGTH)
#define RF_MAX_TEXT_BUFFER_LENGTH                                                                  \
    (1024)// Size of internal R_internal buffers used on some functions:
#endif

#if !defined(RF_MAX_MESH_VBO)
#define RF_MAX_MESH_VBO (7)// Maximum number of vbo per mesh
#endif

// Default vertex attribute names on shader to set location points
#define RF_DEFAULT_ATTRIB_POSITION_NAME "vertex_position"   // shader-location = 0
#define RF_DEFAULT_ATTRIB_TEXCOORD_NAME "vertex_tex_coord"  // shader-location = 1
#define RF_DEFAULT_ATTRIB_NORMAL_NAME "vertex_normal"       // shader-location = 2
#define RF_DEFAULT_ATTRIB_COLOR_NAME "vertex_color"         // shader-location = 3
#define RF_DEFAULT_ATTRIB_TANGENT_NAME "vertex_tangent"     // shader-location = 4
#define RF_DEFAULT_ATTRIB_TEXCOORD2_NAME "vertex_tex_coord2"// shader-location = 5

#pragma endregion

// Matrix modes (equivalent to OpenGL)
typedef enum R_matrix_mode {
    RF_MODELVIEW = 0x1700, // GL_MODELVIEW
    RF_PROJECTION = 0x1701,// GL_PROJECTION
    RF_TEXTURE = 0x1702,   // GL_TEXTURE
} R_matrix_mode;

// Drawing modes (equivalent to OpenGL)
typedef enum R_drawing_mode {
    RF_LINES = 0x0001,    // GL_LINES
    RF_TRIANGLES = 0x0004,// GL_TRIANGLES
    RF_QUADS = 0x0007,    // GL_QUADS
} R_drawing_mode;

// Shader location point type
typedef enum R_shader_location_index {
    RF_LOC_VERTEX_POSITION = 0,
    RF_LOC_VERTEX_TEXCOORD01 = 1,
    RF_LOC_VERTEX_TEXCOORD02 = 2,
    RF_LOC_VERTEX_NORMAL = 3,
    RF_LOC_VERTEX_TANGENT = 4,
    RF_LOC_VERTEX_COLOR = 5,
    RF_LOC_MATRIX_MVP = 6,
    RF_LOC_MATRIX_MODEL = 7,
    RF_LOC_MATRIX_VIEW = 8,
    RF_LOC_MATRIX_PROJECTION = 9,
    RF_LOC_VECTOR_VIEW = 10,
    RF_LOC_COLOR_DIFFUSE = 11,
    RF_LOC_COLOR_SPECULAR = 12,
    RF_LOC_COLOR_AMBIENT = 13,

    // These 2 are intentionally the same
    RF_LOC_MAP_ALBEDO = 14,
    RF_LOC_MAP_DIFFUSE = 14,

    // These 2 are intentionally the same
    RF_LOC_MAP_METALNESS = 15,
    RF_LOC_MAP_SPECULAR = 15,

    RF_LOC_MAP_NORMAL = 16,
    RF_LOC_MAP_ROUGHNESS = 17,
    RF_LOC_MAP_OCCLUSION = 18,
    RF_LOC_MAP_EMISSION = 19,
    RF_LOC_MAP_HEIGHT = 20,
    RF_LOC_MAP_CUBEMAP = 21,
    RF_LOC_MAP_IRRADIANCE = 22,
    RF_LOC_MAP_PREFILTER = 23,
    RF_LOC_MAP_BRDF = 24,
} R_shader_location_index;

// R_shader uniform data types
typedef enum R_shader_uniform_data_type {
    RF_UNIFORM_FLOAT = 0,
    RF_UNIFORM_VEC2,
    RF_UNIFORM_VEC3,
    RF_UNIFORM_VEC4,
    RF_UNIFORM_INT,
    RF_UNIFORM_IVEC2,
    RF_UNIFORM_IVEC3,
    RF_UNIFORM_IVEC4,
    RF_UNIFORM_SAMPLER2D
} R_shader_uniform_data_type;

// R_texture parameters: filter mode
// NOTE 1: Filtering considers mipmaps if available in the texture
// NOTE 2: Filter is accordingly set for minification and magnification
typedef enum R_texture_filter_mode {
    RF_FILTER_POINT = 0,      // No filter, just pixel aproximation
    RF_FILTER_BILINEAR,       // Linear filtering
    RF_FILTER_TRILINEAR,      // Trilinear filtering (linear with mipmaps)
    RF_FILTER_ANISOTROPIC_4x, // Anisotropic filtering 4x
    RF_FILTER_ANISOTROPIC_8x, // Anisotropic filtering 8x
    RF_FILTER_ANISOTROPIC_16x,// Anisotropic filtering 16x
} R_texture_filter_mode;

// Cubemap layout type
typedef enum R_cubemap_layout_type {
    RF_CUBEMAP_AUTO_DETECT = 0,    // Automatically detect layout type
    RF_CUBEMAP_LINE_VERTICAL,      // Layout is defined by a vertical line with faces
    RF_CUBEMAP_LINE_HORIZONTAL,    // Layout is defined by an horizontal line with faces
    RF_CUBEMAP_CROSS_THREE_BY_FOUR,// Layout is defined by a 3x4 cross with cubemap faces
    RF_CUBEMAP_CROSS_FOUR_BY_TREE, // Layout is defined by a 4x3 cross with cubemap faces
    RF_CUBEMAP_PANORAMA            // Layout is defined by a panorama image (equirectangular map)
} R_cubemap_layout_type;

// R_texture parameters: wrap mode
typedef enum R_texture_wrap_mode {
    RF_WRAP_REPEAT = 0,   // Repeats texture in tiled mode
    RF_WRAP_CLAMP,        // Clamps texture to edge pixel in tiled mode
    RF_WRAP_MIRROR_REPEAT,// Mirrors and repeats the texture in tiled mode
    RF_WRAP_MIRROR_CLAMP  // Mirrors and clamps to border the texture in tiled mode
} R_texture_wrap_mode;

// R_color blending modes (pre-defined)
typedef enum R_blend_mode {
    RF_BLEND_ALPHA = 0,// Blend textures considering alpha (default)
    RF_BLEND_ADDITIVE, // Blend textures adding colors
    RF_BLEND_MULTIPLIED// Blend textures multiplying colors
} R_blend_mode;

typedef struct R_shader
{
    unsigned int id;                  // R_shader program id
    int locs[RF_MAX_SHADER_LOCATIONS];// R_shader locations array (RF_MAX_SHADER_LOCATIONS)
} R_shader;

typedef struct R_gfx_pixel_format
{
    unsigned int internal_format;
    unsigned int format;
    unsigned int type;
    R_bool valid;
} R_gfx_pixel_format;

typedef struct R_texture2d
{
    unsigned int id;      // OpenGL texture id
    int width;            // R_texture base width
    int height;           // R_texture base height
    int mipmaps;          // Mipmap levels, 1 by default
    R_pixel_format format;// Data format (R_pixel_format type)
    R_bool valid;
} R_texture2d, R_texture_cubemap;

typedef struct R_render_texture2d
{
    unsigned int id;    // OpenGL Framebuffer Object (FBO) id
    R_texture2d texture;// R_color buffer attachment texture
    R_texture2d depth;  // Depth buffer attachment texture
    int depth_texture;  // Track if depth attachment is a texture or renderbuffer
} R_render_texture2d;

struct R_vertex_buffer;
struct R_mesh;
struct R_material;
struct R_gfx_backend_data;

#pragma region shader
R_public R_shader R_gfx_load_shader(
        const char *vs_code,
        const char *
                fs_code);// Load shader from code strings. If shader string is NULL, using default vertex/fragment shaders
R_public void R_gfx_unload_shader(R_shader shader);// Unload shader from GPU memory (VRAM)
R_public int R_gfx_get_shader_location(R_shader shader,
                                       const char *uniform_name);// Get shader uniform location
R_public void R_gfx_set_shader_value(R_shader shader, int uniform_loc, const void *value,
                                     int uniform_name);// Set shader uniform value
R_public void R_gfx_set_shader_value_v(R_shader shader, int uniform_loc, const void *value,
                                       int uniform_name,
                                       int count);// Set shader uniform value vector
R_public void R_gfx_set_shader_value_matrix(R_shader shader, int uniform_loc,
                                            R_mat mat);// Set shader uniform value (matrix 4x4)
R_public void R_gfx_set_shader_value_texture(
        R_shader shader, int uniform_loc,
        R_texture2d texture);// Set shader uniform value for texture
#pragma endregion

#pragma region gfx api
R_public R_mat R_gfx_get_matrix_projection();// Return internal R__ctx->gl_ctx.projection matrix
R_public R_mat R_gfx_get_matrix_modelview(); // Return internal R__ctx->gl_ctx.modelview matrix
R_public void R_gfx_set_matrix_projection(
        R_mat proj);// Set a custom projection matrix (replaces internal R__ctx->gl_ctx.projection matrix)
R_public void R_gfx_set_matrix_modelview(
        R_mat view);// Set a custom R__ctx->gl_ctx.modelview matrix (replaces internal R__ctx->gl_ctx.modelview matrix)

R_public void R_gfx_blend_mode(
        R_blend_mode mode);// Choose the blending mode (alpha, additive, multiplied)
R_public void R_gfx_matrix_mode(R_matrix_mode mode);// Choose the current matrix to be transformed
R_public void R_gfx_push_matrix();                  // Push the current matrix to R_global_gl_stack
R_public void R_gfx_pop_matrix();   // Pop lattest inserted matrix from R_global_gl_stack
R_public void R_gfx_load_identity();// Reset current matrix to identity matrix
R_public void R_gfx_translatef(float x, float y,
                               float z);// Multiply the current matrix by a translation matrix
R_public void R_gfx_rotatef(float angleDeg, float x, float y,
                            float z);// Multiply the current matrix by a rotation matrix
R_public void R_gfx_scalef(float x, float y,
                           float z);          // Multiply the current matrix by a scaling matrix
R_public void R_gfx_mult_matrixf(float *matf);// Multiply the current matrix by another matrix
R_public void R_gfx_frustum(double left, double right, double bottom, double top, double znear,
                            double zfar);
R_public void R_gfx_ortho(double left, double right, double bottom, double top, double znear,
                          double zfar);
R_public void R_gfx_viewport(int x, int y, int width, int height);// Set the viewport area

// Functions Declaration - Vertex level operations
R_public void R_gfx_begin(R_drawing_mode mode);// Initialize drawing mode (how to organize vertex)
R_public void R_gfx_end();                     // Finish vertex providing
R_public void R_gfx_vertex2i(int x, int y);    // Define one vertex (position) - 2 int
R_public void R_gfx_vertex2f(float x, float y);// Define one vertex (position) - 2 float
R_public void R_gfx_vertex3f(float x, float y, float z);// Define one vertex (position) - 3 float
R_public void R_gfx_tex_coord2f(float x,
                                float y);// Define one vertex (texture coordinate) - 2 float
R_public void R_gfx_normal3f(float x, float y, float z);// Define one vertex (normal) - 3 float
R_public void R_gfx_color4ub(unsigned char r, unsigned char g, unsigned char b,
                             unsigned char a);// Define one vertex (color) - 4 unsigned char
R_public void R_gfx_color3f(float x, float y, float z);// Define one vertex (color) - 3 float
R_public void R_gfx_color4f(float x, float y, float z,
                            float w);// Define one vertex (color) - 4 float

R_public void R_gfx_enable_texture(unsigned int id);// Enable texture usage
R_public void R_gfx_disable_texture();              // Disable texture usage
R_public void R_gfx_set_texture_wrap(
        R_texture2d texture,
        R_texture_wrap_mode wrap_mode);// Set texture parameters (wrap mode/filter mode)
R_public void R_gfx_set_texture_filter(R_texture2d texture,
                                       R_texture_filter_mode filter_mode);// Set filter for texture
R_public void R_gfx_enable_render_texture(unsigned int id);// Enable render texture (fbo)
R_public void R_gfx_disable_render_texture(
        void);// Disable render texture (fbo), return to default framebuffer
R_public void R_gfx_enable_depth_test(void);                     // Enable depth test
R_public void R_gfx_disable_depth_test(void);                    // Disable depth test
R_public void R_gfx_enable_backface_culling(void);               // Enable backface culling
R_public void R_gfx_disable_backface_culling(void);              // Disable backface culling
R_public void R_gfx_enable_scissor_test(void);                   // Enable scissor test
R_public void R_gfx_disable_scissor_test(void);                  // Disable scissor test
R_public void R_gfx_scissor(int x, int y, int width, int height);// Scissor test
R_public void R_gfx_enable_wire_mode(void);                      // Enable wire mode
R_public void R_gfx_disable_wire_mode(void);                     // Disable wire mode
R_public void R_gfx_delete_textures(unsigned int id);            // Delete OpenGL texture from GPU
R_public void R_gfx_delete_render_textures(
        R_render_texture2d target);                // Delete render textures (fbo) from GPU
R_public void R_gfx_delete_shader(unsigned int id);// Delete OpenGL shader program from GPU
R_public void R_gfx_delete_vertex_arrays(
        unsigned int id);                           // Unload vertex data (VAO) from GPU memory
R_public void R_gfx_delete_buffers(unsigned int id);// Unload vertex data (VBO) from GPU memory
R_public void R_gfx_clear_color(unsigned char r, unsigned char g, unsigned char b,
                                unsigned char a);// Clear color buffer with color
R_public void R_gfx_clear_screen_buffers(void);  // Clear used screen buffers (color and depth)
R_public void R_gfx_update_buffer(int buffer_id, void *data,
                                  int data_size);// Update GPU buffer with new data
R_public unsigned int R_gfx_load_attrib_buffer(unsigned int vao_id, int shader_loc, void *buffer,
                                               int size,
                                               R_bool dynamic);// Load a new attributes buffer
R_public void R_gfx_init_vertex_buffer(struct R_vertex_buffer *vertex_buffer);

R_public void R_gfx_close();// De-inititialize rf gfx (buffers, shaders, textures)
R_public void R_gfx_draw(); // Update and draw default internal buffers

R_public R_bool
R_gfx_check_buffer_limit(int v_count);// Check internal buffer overflow for a given number of vertex
R_public void R_gfx_set_debug_marker(const char *text);// Set debug marker for analysis

// Textures data management
R_public unsigned int R_gfx_load_texture(void *data, int width, int height, R_pixel_format format,
                                         int mipmap_count);// Load texture in GPU
R_public unsigned int R_gfx_load_texture_depth(
        int width, int height, int bits,
        R_bool use_render_buffer);// Load depth texture/renderbuffer (to be attached to fbo)
R_public unsigned int R_gfx_load_texture_cubemap(void *data, int size,
                                                 R_pixel_format format);// Load texture cubemap
R_public void R_gfx_update_texture(unsigned int id, int width, int height, R_pixel_format format,
                                   const void *pixels,
                                   int pixels_size);// Update GPU texture with new data
R_public R_gfx_pixel_format
R_gfx_get_internal_texture_formats(R_pixel_format format);// Get OpenGL internal formats
R_public void R_gfx_unload_texture(unsigned int id);      // Unload texture from GPU memory

R_public void R_gfx_generate_mipmaps(
        R_texture2d *texture);// Generate mipmap data for selected texture
R_public R_image R_gfx_read_texture_pixels_to_buffer(R_texture2d texture, void *dst, int dst_size);
R_public R_image R_gfx_read_texture_pixels(R_texture2d texture, R_allocator allocator);
R_public void R_gfx_read_screen_pixels(R_color *dst, int width,
                                       int height);// Read screen pixel data (color buffer)

// Render texture management (fbo)
R_public R_render_texture2d R_gfx_load_render_texture(
        int width, int height, R_pixel_format format, int depth_bits,
        R_bool use_depth_texture);// Load a render texture (with color and depth attachments)
R_public void R_gfx_render_texture_attach(R_render_texture2d target, unsigned int id,
                                          int attach_type);// Attach texture/renderbuffer to an fbo
R_public R_bool
R_gfx_render_texture_complete(R_render_texture2d target);// Verify render texture is complete

// Vertex data management
R_public void R_gfx_load_mesh(
        struct R_mesh *mesh,
        R_bool dynamic);// Upload vertex data into GPU and provided VAO/VBO ids
R_public void R_gfx_update_mesh(
        struct R_mesh mesh, int buffer,
        int num);// Update vertex or index data on GPU (upload new data to one buffer)
R_public void R_gfx_update_mesh_at(struct R_mesh mesh, int buffer, int num,
                                   int index);// Update vertex or index data on GPU, at index
R_public void R_gfx_draw_mesh(struct R_mesh mesh, struct R_material material,
                              R_mat transform);     // Draw a 3d mesh with material and transform
R_public void R_gfx_unload_mesh(struct R_mesh mesh);// Unload mesh data from CPU and GPU
#pragma endregion

#pragma region gfx backends

#if defined(METADOT_BACKEND_GL) || defined(METADOT_BACKEND_GL_ES3)
#include "Engine/Renderer/Backends/BackendGL.h"
#endif

#if defined(METADOT_BACKEND_METAL) || defined(METADOT_BACKEND_DIRECTX)
#include "Engine/Renderer/Backends/BackendSokol.h"
#endif

#pragma endregion

// The render batch must come after the gfx backends since it depends on the definitions of the backend
#pragma region render batch
typedef struct R_render_batch
{
    R_int vertex_buffers_count;
    R_int current_buffer;
    struct R_vertex_buffer *vertex_buffers;

    R_int draw_calls_size;
    R_int draw_calls_counter;
    struct R_draw_call *draw_calls;
    float current_depth;// Current depth value for next draw

    R_bool valid;
} R_render_batch;

typedef struct R_one_element_vertex_buffer
{
    R_gfx_vertex_data_type vertices[1 * RF_GFX_VERTEX_COMPONENT_COUNT];
    R_gfx_texcoord_data_type texcoords[1 * RF_GFX_TEXCOORD_COMPONENT_COUNT];
    R_gfx_color_data_type colors[1 * RF_GFX_COLOR_COMPONENT_COUNT];
    R_gfx_vertex_index_data_type indices[1 * RF_GFX_VERTEX_INDEX_COMPONENT_COUNT];
} R_one_element_vertex_buffer;

typedef struct R_default_vertex_buffer
{
    R_gfx_vertex_data_type
            vertices[RF_DEFAULT_BATCH_ELEMENTS_COUNT * RF_GFX_VERTEX_COMPONENT_COUNT];
    R_gfx_texcoord_data_type
            texcoords[RF_DEFAULT_BATCH_ELEMENTS_COUNT * RF_GFX_TEXCOORD_COMPONENT_COUNT];
    R_gfx_color_data_type colors[RF_DEFAULT_BATCH_ELEMENTS_COUNT * RF_GFX_COLOR_COMPONENT_COUNT];
    R_gfx_vertex_index_data_type
            indices[RF_DEFAULT_BATCH_ELEMENTS_COUNT * RF_GFX_VERTEX_INDEX_COMPONENT_COUNT];
} R_default_vertex_buffer;

typedef struct R_default_render_batch
{
    R_vertex_buffer vertex_buffers[RF_DEFAULT_BATCH_VERTEX_BUFFERS_COUNT];
    R_draw_call draw_calls[RF_DEFAULT_BATCH_DRAW_CALLS_COUNT];
    R_default_vertex_buffer vertex_buffers_memory[RF_DEFAULT_BATCH_VERTEX_BUFFERS_COUNT];
} R_default_render_batch;

R_public R_render_batch R_create_custom_render_batch_from_buffers(R_vertex_buffer *vertex_buffers,
                                                                  R_int vertex_buffers_count,
                                                                  R_draw_call *draw_calls,
                                                                  R_int draw_calls_count);
R_public R_render_batch R_create_custom_render_batch(R_int vertex_buffers_count,
                                                     R_int draw_calls_count,
                                                     R_int vertex_buffer_elements_count,
                                                     R_allocator allocator);
R_public R_render_batch R_create_default_render_batch(R_allocator allocator);

R_public void R_set_active_render_batch(R_render_batch *batch);
R_public void R_unload_render_batch(R_render_batch batch, R_allocator allocator);
#pragma endregion

#define RF_SDF_CHAR_PADDING (4)
#define RF_SDF_ON_EDGE_VALUE (128)
#define RF_SDF_PIXEL_DIST_SCALE (64.0f)

#define RF_BITMAP_ALPHA_THRESHOLD (80)
#define RF_DEFAULT_FONT_SIZE (64)

#define RF_BUILTIN_FONT_CHARS                                                                      \
    {                                                                                              \
        ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', '0', '1', \
                '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '@', 'A',    \
                'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',    \
                'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_', '`', 'a',   \
                'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',    \
                'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',                   \
    }
#define RF_BUILTIN_FONT_FIRST_CHAR (32)
#define RF_BUILTIN_FONT_LAST_CHAR (126)
#define RF_BUILTIN_CODEPOINTS_COUNT                                                                \
    (96)// ASCII 32 up to 126 is 96 glyphs (note that the range is inclusive)
#define RF_BUILTIN_FONT_PADDING (2)

#define RF_GLYPH_NOT_FOUND (-1)

#define RF_BUILTIN_FONT_CHARS_COUNT (224)// Number of characters in the raylib font

typedef enum R_font_antialias {
    RF_FONT_ANTIALIAS = 0,// Default font generation, anti-aliased
    RF_FONT_NO_ANTIALIAS, // Bitmap font generation, no anti-aliasing
} R_font_antialias;

typedef struct R_glyph_info
{
    R_rec rec;    // Characters rectangles in texture
    int codepoint;// Character value (Unicode)
    int offset_x; // Character offset X when drawing
    int offset_y; // Character offset Y when drawing
    int advance_x;// Character advance position X
} R_glyph_info;

typedef struct R_ttf_font_info
{
    // Font details
    const void *ttf_data;
    int font_size;
    int largest_glyph_size;

    // Font metrics
    float scale_factor;
    int ascent;
    int descent;
    int line_gap;

    // Take directly from stb_truetype because we don't want to include it's header in our public API
    struct
    {
        void *userdata;
        unsigned char *data;
        int fontstart;
        int numGlyphs;
        int loca, head, glyf, hhea, hmtx, kern, gpos, svg;
        int index_map;
        int indexToLocFormat;

        struct
        {
            unsigned char *data;
            int cursor;
            int size;
        } cff, charstrings, gsubrs, subrs, fontdicts, fdselect;
    } internal_stb_font_info;

    R_bool valid;
} R_ttf_font_info;

typedef struct R_font
{
    int base_size;
    R_texture2d texture;
    R_glyph_info *glyphs;
    R_int glyphs_count;
    R_bool valid;
} R_font;

typedef int R_glyph_index;

#pragma region ttf font
R_public R_ttf_font_info R_parse_ttf_font(const void *ttf_data, R_int font_size);
R_public void R_compute_ttf_font_glyph_metrics(R_ttf_font_info *font_info, const int *codepoints,
                                               R_int codepoints_count, R_glyph_info *dst,
                                               R_int dst_count);
R_public int R_compute_ttf_font_atlas_width(int padding, R_glyph_info *glyph_metrics,
                                            R_int glyphs_count);
R_public R_image R_generate_ttf_font_atlas(R_ttf_font_info *font_info, int atlas_width, int padding,
                                           R_glyph_info *glyphs, R_int glyphs_count,
                                           R_font_antialias antialias, unsigned short *dst,
                                           R_int dst_count, R_allocator temp_allocator);
R_public R_font R_ttf_font_from_atlas(int font_size, R_image atlas, R_glyph_info *glyph_metrics,
                                      R_int glyphs_count);

R_public R_font R_load_ttf_font_from_data(const void *font_file_data, int font_size,
                                          R_font_antialias antialias, const int *chars,
                                          R_int char_count, R_allocator allocator,
                                          R_allocator temp_allocator);
R_public R_font R_load_ttf_font_from_file(const char *filename, int font_size,
                                          R_font_antialias antialias, R_allocator allocator,
                                          R_allocator temp_allocator, R_io_callbacks io);
#pragma endregion

#pragma region image font
R_public R_bool R_compute_glyph_metrics_from_image(R_image image, R_color key,
                                                   const int *codepoints, R_glyph_info *dst,
                                                   R_int codepoints_and_dst_count);
R_public R_font R_load_image_font_from_data(R_image image, R_glyph_info *glyphs,
                                            R_int glyphs_count);
R_public R_font R_load_image_font(R_image image, R_color key, R_allocator allocator);
R_public R_font R_load_image_font_from_file(const char *path, R_color key, R_allocator allocator,
                                            R_allocator temp_allocator, R_io_callbacks io);
#pragma endregion

#pragma region font utils
R_public void R_unload_font(R_font font, R_allocator allocator);
R_public R_glyph_index R_get_glyph_index(R_font font, int character);
R_public int R_font_height(R_font font, float font_size);

R_public R_sizef R_measure_text(R_font font, const char *text, float font_size,
                                float extra_spacing);
R_public R_sizef R_measure_text_rec(R_font font, const char *text, R_rec rec, float font_size,
                                    float extra_spacing, R_bool wrap);

R_public R_sizef R_measure_string(R_font font, const char *text, int len, float font_size,
                                  float extra_spacing);
R_public R_sizef R_measure_string_rec(R_font font, const char *text, int text_len, R_rec rec,
                                      float font_size, float extra_spacing, R_bool wrap);
#pragma endregion

typedef enum R_material_map_type {
    // These 2 are the same
    RF_MAP_ALBEDO = 0,
    RF_MAP_DIFFUSE = 0,

    // These 2 are the same
    RF_MAP_METALNESS = 1,
    RF_MAP_SPECULAR = 1,

    RF_MAP_NORMAL = 2,
    RF_MAP_ROUGHNESS = 3,
    RF_MAP_OCCLUSION = 4,
    RF_MAP_EMISSION = 5,
    RF_MAP_HEIGHT = 6,
    RF_MAP_CUBEMAP = 7,   // NOTE: Uses GL_TEXTURE_CUBE_MAP
    RF_MAP_IRRADIANCE = 8,// NOTE: Uses GL_TEXTURE_CUBE_MAP
    RF_MAP_PREFILTER = 9, // NOTE: Uses GL_TEXTURE_CUBE_MAP
    RF_MAP_BRDF = 10
} R_material_map_type;

typedef struct R_mesh
{
    int vertex_count;  // Number of vertices stored in arrays
    int triangle_count;// Number of triangles stored (indexed or not)

    // Default vertex data
    float *vertices;// Vertex position (XYZ - 3 components per vertex) (shader-location = 0)
    float *texcoords;// Vertex texture coordinates (UV - 2 components per vertex) (shader-location = 1)
    float *texcoords2;// Vertex second texture coordinates (useful for lightmaps) (shader-location = 5)
    float *normals;   // Vertex normals (XYZ - 3 components per vertex) (shader-location = 2)
    float *tangents;      // Vertex tangents (XYZW - 4 components per vertex) (shader-location = 4)
    unsigned char *colors;// Vertex colors (RGBA - 4 components per vertex) (shader-location = 3)
    unsigned short *indices;// Vertex indices (in case vertex data comes indexed)

    // Animation vertex data
    float *anim_vertices;// Animated vertex positions (after bones transformations)
    float *anim_normals; // Animated normals (after bones transformations)
    int *bone_ids;       // Vertex bone ids, up to 4 bones influence by vertex (skinning)
    float *bone_weights; // Vertex bone weight, up to 4 bones influence by vertex (skinning)

    // OpenGL identifiers
    unsigned int vao_id; // OpenGL Vertex Array Object id
    unsigned int *vbo_id;// OpenGL Vertex Buffer Objects id (default vertex data)
} R_mesh;

typedef struct R_material_map
{
    R_texture2d texture;// R_material map texture
    R_color color;      // R_material map color
    float value;        // R_material map value
} R_material_map;

typedef struct R_material
{
    R_shader shader;     // R_material shader
    R_material_map *maps;// R_material maps array (RF_MAX_MATERIAL_MAPS)
    float *params;       // R_material generic parameters (if required)
} R_material;

typedef struct R_transform
{
    R_vec3 translation;   // Translation
    R_quaternion rotation;// Rotation
    R_vec3 scale;         // Scale
} R_transform;

typedef struct R_bone_info
{
    char name[32];// Bone name
    R_int parent; // Bone parent
} R_bone_info;

typedef struct R_model
{
    R_mat transform; // Local transform matrix
    R_int mesh_count;// Number of meshes
    R_mesh *meshes;  // Meshes array

    R_int material_count; // Number of materials
    R_material *materials;// Materials array
    int *mesh_material;   // Mesh material number

    // Animation data
    R_int bone_count;      // Number of bones
    R_bone_info *bones;    // Bones information (skeleton)
    R_transform *bind_pose;// Bones base transformation (pose)
} R_model;

typedef struct R_model_animation
{
    R_int bone_count;         // Number of bones
    R_bone_info *bones;       // Bones information (skeleton)
    R_int frame_count;        // Number of animation frames
    R_transform **frame_poses;// Poses array by frame
} R_model_animation;

typedef struct R_model_animation_array
{
    R_int size;
    R_model_animation *anims;
} R_model_animation_array;

typedef struct R_materials_array
{
    R_int size;
    R_material *materials;
} R_materials_array;

R_public R_bounding_box R_mesh_bounding_box(R_mesh mesh);// Compute mesh bounding box limits
R_public void R_mesh_compute_tangents(R_mesh *mesh, R_allocator allocator,
                                      R_allocator temp_allocator);// Compute mesh tangents
R_public void R_mesh_compute_binormals(R_mesh *mesh);             // Compute mesh binormals
R_public void R_unload_mesh(R_mesh mesh,
                            R_allocator allocator);// Unload mesh from memory (RAM and/or VRAM)

R_public R_model R_load_model(const char *filename, R_allocator allocator,
                              R_allocator temp_allocator, R_io_callbacks io);
R_public R_model
R_load_model_from_obj(const char *filename, R_allocator allocator, R_allocator temp_allocator,
                      R_io_callbacks io);// Load model from files (meshes and materials)
R_public R_model
R_load_model_from_iqm(const char *filename, R_allocator allocator, R_allocator temp_allocator,
                      R_io_callbacks io);// Load model from files (meshes and materials)
R_public R_model
R_load_model_from_gltf(const char *filename, R_allocator allocator, R_allocator temp_allocator,
                       R_io_callbacks io);// Load model from files (meshes and materials)
R_public R_model R_load_model_from_mesh(
        R_mesh mesh,
        R_allocator
                allocator);// Load model from generated mesh. Note: The function takes ownership of the mesh in model.meshes[0]
R_public void R_unload_model(R_model model,
                             R_allocator allocator);// Unload model from memory (RAM and/or VRAM)

R_public R_materials_array
R_load_materials_from_mtl(const char *filename, R_allocator allocator,
                          R_io_callbacks io);// Load materials from model file
R_public void R_set_material_texture(
        R_material *material, R_material_map_type map_type,
        R_texture2d
                texture);// Set texture for a material map type (R_map_diffuse, R_map_specular...)
R_public void R_set_model_mesh_material(R_model *model, int mesh_id,
                                        int material_id);// Set material for a mesh
R_public void R_unload_material(R_material material,
                                R_allocator allocator);// Unload material from GPU memory (VRAM)

// Animations

R_public R_model_animation_array R_load_model_animations_from_iqm_file(const char *filename,
                                                                       R_allocator allocator,
                                                                       R_allocator temp_allocator,
                                                                       R_io_callbacks io);
R_public R_model_animation_array
R_load_model_animations_from_iqm(const unsigned char *data, int data_size, R_allocator allocator,
                                 R_allocator temp_allocator);// Load model animations from file
R_public void R_update_model_animation(R_model model, R_model_animation anim,
                                       int frame);// Update model animation pose
R_public R_bool R_is_model_animation_valid(
        R_model model, R_model_animation anim);// Check model animation skeleton match
R_public R_ray_hit_info
R_collision_ray_model(R_ray ray, struct R_model model);// Get collision info between ray and model
R_public void R_unload_model_animation(R_model_animation anim,
                                       R_allocator allocator);// Unload animation data

// mesh generation functions

R_public R_mesh R_gen_mesh_cube(float width, float height, float length, R_allocator allocator,
                                R_allocator temp_allocator);// Generate cuboid mesh
R_public R_mesh R_gen_mesh_poly(int sides, float radius, R_allocator allocator,
                                R_allocator temp_allocator);// Generate polygonal mesh
R_public R_mesh
R_gen_mesh_plane(float width, float length, int res_x, int res_z, R_allocator allocator,
                 R_allocator temp_allocator);// Generate plane mesh (with subdivisions)
R_public R_mesh
R_gen_mesh_sphere(float radius, int rings, int slices, R_allocator allocator,
                  R_allocator temp_allocator);// Generate sphere mesh (standard sphere)
R_public R_mesh
R_gen_mesh_hemi_sphere(float radius, int rings, int slices, R_allocator allocator,
                       R_allocator temp_allocator);// Generate half-sphere mesh (no bottom cap)
R_public R_mesh R_gen_mesh_cylinder(float radius, float height, int slices, R_allocator allocator,
                                    R_allocator temp_allocator);// Generate cylinder mesh
R_public R_mesh R_gen_mesh_torus(float radius, float size, int rad_seg, int sides,
                                 R_allocator allocator,
                                 R_allocator temp_allocator);// Generate torus mesh
R_public R_mesh R_gen_mesh_knot(float radius, float size, int rad_seg, int sides,
                                R_allocator allocator,
                                R_allocator temp_allocator);// Generate trefoil knot mesh
R_public R_mesh
R_gen_mesh_heightmap(R_image heightmap, R_vec3 size, R_allocator allocator,
                     R_allocator temp_allocator);// Generate heightmap mesh from image data
R_public R_mesh
R_gen_mesh_cubicmap(R_image cubicmap, R_vec3 cube_size, R_allocator allocator,
                    R_allocator temp_allocator);// Generate cubes-based map mesh from image data

typedef struct R_default_font
{
    unsigned short pixels[128 * 128];
    R_glyph_info chars[RF_BUILTIN_FONT_CHARS_COUNT];
    unsigned short chars_pixels[128 * 128];
} R_default_font;

typedef struct R_gfx_context
{
    int current_width;
    int current_height;

    int render_width;
    int render_height;

    int framebuffer_width;
    int framebuffer_height;

    R_mat screen_scaling;
    R_render_batch *current_batch;

    R_matrix_mode current_matrix_mode;
    R_mat *current_matrix;
    R_mat modelview;
    R_mat projection;
    R_mat transform;
    R_bool transform_matrix_required;
    R_mat stack[RF_MAX_MATRIX_STACK_SIZE];
    int stack_counter;

    unsigned int
            default_texture_id;// Default texture (1px white) useful for plain color polys (required by shader)
    unsigned int
            default_vertex_shader_id;// Default vertex shader id (used by default shader program)
    unsigned int
            default_frag_shader_id;// Default fragment shader Id (used by default shader program)

    R_shader default_shader;// Basic shader, support vertex color and diffuse texture
    R_shader current_shader;// Shader to be used on rendering (by default, default_shader)

    R_blend_mode blend_mode;// Track current blending mode

    R_texture2d tex_shapes;
    R_rec rec_tex_shapes;

    R_font default_font;
    R_default_font default_font_buffers;
    R_gfx_backend_data gfx_backend_data;

    R_logger logger;
    R_log_type logger_filter;
} R_gfx_context;

R_public void R_gfx_init(R_gfx_context *ctx, int screen_width, int screen_height,
                         R_gfx_backend_data *gfx_data);

R_public R_material R_load_default_material(
        R_allocator allocator);// Load default material (Supports: DIFFUSE, SPECULAR, NORMAL maps)
R_public R_shader R_load_default_shader();

R_public R_render_batch *
R_get_current_render_batch();// Return a pointer to the current render batch
R_public R_font
R_get_default_font();// Get the default font, useful to be used with extended parameters
R_public R_shader R_get_default_shader();    // Get default shader
R_public R_texture2d R_get_default_texture();// Get default internal texture (white texture)
R_public R_gfx_context *R_get_gfx_context(); // Get the context pointer
R_public R_image R_get_screen_data(
        R_color *dst,
        R_int dst_size);// Get pixel data from GPU frontbuffer and return an R_image (screenshot)

R_public void R_set_global_gfx_context_pointer(R_gfx_context *ctx);// Set the global context pointer
R_public void R_set_viewport(int width, int height);// Set viewport for a provided width and height
R_public void R_set_shapes_texture(R_texture2d texture,
                                   R_rec source);// Define default texture used to draw shapes

typedef struct R_model R_model;

typedef enum R_text_wrap_mode {
    RF_CHAR_WRAP,
    RF_WORD_WRAP,
} R_text_wrap_mode;

typedef enum R_ninepatch_type {
    RF_NPT_9PATCH = 0,      // Npatch defined by 3x3 tiles
    RF_NPT_3PATCH_VERTICAL, // Npatch defined by 1x3 tiles
    RF_NPT_3PATCH_HORIZONTAL// Npatch defined by 3x1 tiles
} R_ninepatch_type;

typedef struct R_npatch_info
{
    R_rec source_rec;// Region in the texture
    int left;        // left border offset
    int top;         // top border offset
    int right;       // right border offset
    int bottom;      // bottom border offset
    int type;        // layout of the n-patch: 3x3, 1x3 or 3x1
} R_npatch_info;

R_public R_vec2 R_center_to_screen(
        float w,
        float h);// Returns the position of an object such that it will be centered to the screen

R_public void R_clear(R_color color);// Set background color (framebuffer clear color)

R_public void R_begin();// Setup canvas (framebuffer) to start drawing
R_public void R_end();  // End canvas drawing and swap buffers (double buffering)

R_public void R_begin_2d(R_camera2d camera);// Initialize 2D mode with custom camera (2D)
R_public void R_end_2d();                   // Ends 2D mode with custom camera

R_public void R_begin_3d(R_camera3d camera);// Initializes 3D mode with custom camera (3D)
R_public void R_end_3d();// Ends 3D mode and returns to default 2D orthographic mode

R_public void R_begin_render_to_texture(
        R_render_texture2d target);     // Initializes render texture for drawing
R_public void R_end_render_to_texture();// Ends drawing to render texture

R_public void R_begin_scissor_mode(
        int x, int y, int width,
        int height);               // Begin scissor mode (define screen area for following drawing)
R_public void R_end_scissor_mode();// End scissor mode

R_public void R_begin_shader(R_shader shader);// Begin custom shader drawing
R_public void R_end_shader();                 // End custom shader drawing (use default shader)

R_public void R_begin_blend_mode(
        R_blend_mode mode);      // Begin blending mode (alpha, additive, multiplied)
R_public void R_end_blend_mode();// End blending mode (reset to default: alpha blending)

R_public void R_draw_pixel(int pos_x, int pos_y, R_color color);// Draw a pixel
R_public void R_draw_pixel_v(R_vec2 position, R_color color);   // Draw a pixel (Vector version)

R_public void R_draw_line(int startPosX, int startPosY, int endPosX, int endPosY,
                          R_color color);// Draw a line
R_public void R_draw_line_v(R_vec2 startPos, R_vec2 endPos,
                            R_color color);// Draw a line (Vector version)
R_public void R_draw_line_ex(R_vec2 startPos, R_vec2 endPos, float thick,
                             R_color color);// Draw a line defining thickness
R_public void R_draw_line_bezier(R_vec2 start_pos, R_vec2 end_pos, float thick,
                                 R_color color);// Draw a line using cubic-bezier curves in-out
R_public void R_draw_line_strip(R_vec2 *points, int num_points,
                                R_color color);// Draw lines sequence

R_public void R_draw_circle(int center_x, int center_y, float radius,
                            R_color color);// Draw a color-filled circle
R_public void R_draw_circle_v(R_vec2 center, float radius,
                              R_color color);// Draw a color-filled circle (Vector version)
R_public void R_draw_circle_sector(R_vec2 center, float radius, int start_angle, int end_angle,
                                   int segments, R_color color);// Draw a piece of a circle
R_public void R_draw_circle_sector_lines(R_vec2 center, float radius, int start_angle,
                                         int end_angle, int segments,
                                         R_color color);// Draw circle sector outline
R_public void R_draw_circle_gradient(int center_x, int center_y, float radius, R_color color1,
                                     R_color color2);// Draw a gradient-filled circle
R_public void R_draw_circle_lines(int center_x, int center_y, float radius,
                                  R_color color);// Draw circle outline

R_public void R_draw_ring(R_vec2 center, float inner_radius, float outer_radius, int start_angle,
                          int end_angle, int segments, R_color color);// Draw ring
R_public void R_draw_ring_lines(R_vec2 center, float inner_radius, float outer_radius,
                                int start_angle, int end_angle, int segments,
                                R_color color);// Draw ring outline

R_public void R_draw_rectangle(int posX, int posY, int width, int height,
                               R_color color);// Draw a color-filled rectangle
R_public void R_draw_rectangle_v(R_vec2 position, R_vec2 size,
                                 R_color color);// Draw a color-filled rectangle (Vector version)
R_public void R_draw_rectangle_rec(R_rec rec, R_color color);// Draw a color-filled rectangle
R_public void R_draw_rectangle_pro(
        R_rec rec, R_vec2 origin, float rotation,
        R_color color);// Draw a color-filled rectangle with pro parameters

R_public void R_draw_rectangle_gradient_v(
        int pos_x, int pos_y, int width, int height, R_color color1,
        R_color color2);// Draw a vertical-gradient-filled rectangle
R_public void R_draw_rectangle_gradient_h(
        int pos_x, int pos_y, int width, int height, R_color color1,
        R_color color2);// Draw a horizontal-gradient-filled rectangle
R_public void R_draw_rectangle_gradient(
        R_rec rec, R_color col1, R_color col2, R_color col3,
        R_color col4);// Draw a gradient-filled rectangle with custom vertex colors

R_public void R_draw_rectangle_outline(
        R_rec rec, int line_thick, R_color color);// Draw rectangle outline with extended parameters
R_public void R_draw_rectangle_rounded(R_rec rec, float roundness, int segments,
                                       R_color color);// Draw rectangle with rounded edges
R_public void R_draw_rectangle_rounded_lines(
        R_rec rec, float roundness, int segments, int line_thick,
        R_color color);// Draw rectangle with rounded edges outline

R_public void R_draw_triangle(
        R_vec2 v1, R_vec2 v2, R_vec2 v3,
        R_color color);// Draw a color-filled triangle (vertex in counter-clockwise order!)
R_public void R_draw_triangle_lines(
        R_vec2 v1, R_vec2 v2, R_vec2 v3,
        R_color color);// Draw triangle outline (vertex in counter-clockwise order!)
R_public void R_draw_triangle_fan(
        R_vec2 *points, int num_points,
        R_color color);// Draw a triangle fan defined by points (first vertex is the center)
R_public void R_draw_triangle_strip(R_vec2 *points, int points_count,
                                    R_color color);// Draw a triangle strip defined by points
R_public void R_draw_poly(R_vec2 center, int sides, float radius, float rotation,
                          R_color color);// Draw a regular polygon (Vector version)

// R_texture2d drawing functions

R_public void R_draw_texture(R_texture2d texture, int x, int y,
                             R_color tint);// Draw a R_texture2d with extended parameters
R_public void R_draw_texture_ex(R_texture2d texture, int x, int y, int w, int h, float rotation,
                                R_color tint);// Draw a R_texture2d with extended parameters
R_public void R_draw_texture_region(
        R_texture2d texture, R_rec source_rec, R_rec dest_rec, R_vec2 origin, float rotation,
        R_color tint);// Draw a part of a texture defined by a rectangle with 'pro' parameters
R_public void R_draw_texture_npatch(
        R_texture2d texture, R_npatch_info n_patch_info, R_rec dest_rec, R_vec2 origin,
        float rotation,
        R_color tint);// Draws a texture (or part of it) that stretches or shrinks nicely

// Text drawing functions

R_public void R_draw_string(const char *string, int string_len, int posX, int posY, int font_size,
                            R_color color);// Draw text (using default font)
R_public void R_draw_string_ex(R_font font, const char *string, int string_len, R_vec2 position,
                               float fontSize, float spacing,
                               R_color tint);// Draw text using font and additional parameters
R_public void R_draw_string_wrap(R_font font, const char *string, int string_len, R_vec2 position,
                                 float font_size, float spacing, R_color tint, float wrap_width,
                                 R_text_wrap_mode mode);// Draw text and wrap at a specific width
R_public void R_draw_string_rec(R_font font, const char *string, int string_len, R_rec rec,
                                float font_size, float spacing, R_text_wrap_mode wrap,
                                R_color tint);// Draw text using font inside rectangle limits

R_public void R_draw_text(const char *text, int posX, int posY, int font_size,
                          R_color color);// Draw text (using default font)
R_public void R_draw_text_ex(R_font font, const char *text, R_vec2 position, float fontSize,
                             float spacing,
                             R_color tint);// Draw text using font and additional parameters
R_public void R_draw_text_wrap(R_font font, const char *text, R_vec2 position, float font_size,
                               float spacing, R_color tint, float wrap_width,
                               R_text_wrap_mode mode);// Draw text and wrap at a specific width
R_public void R_draw_text_rec(R_font font, const char *text, R_rec rec, float font_size,
                              float spacing, R_text_wrap_mode wrap,
                              R_color tint);// Draw text using font inside rectangle limits

R_public void R_draw_line3d(R_vec3 start_pos, R_vec3 end_pos,
                            R_color color);// Draw a line in 3D world space
R_public void R_draw_circle3d(R_vec3 center, float radius, R_vec3 rotation_axis,
                              float rotation_angle,
                              R_color color);// Draw a circle in 3D world space
R_public void R_draw_cube(R_vec3 position, float width, float height, float length,
                          R_color color);// Draw cube
R_public void R_draw_cube_wires(R_vec3 position, float width, float height, float length,
                                R_color color);// Draw cube wires
R_public void R_draw_cube_texture(R_texture2d texture, R_vec3 position, float width, float height,
                                  float length, R_color color);             // Draw cube textured
R_public void R_draw_sphere(R_vec3 center_pos, float radius, R_color color);// Draw sphere
R_public void R_draw_sphere_ex(R_vec3 center_pos, float radius, int rings, int slices,
                               R_color color);// Draw sphere with extended parameters
R_public void R_draw_sphere_wires(R_vec3 center_pos, float radius, int rings, int slices,
                                  R_color color);// Draw sphere wires
R_public void R_draw_cylinder(R_vec3 position, float radius_top, float radius_bottom, float height,
                              int slices, R_color color);// Draw a cylinder/cone
R_public void R_draw_cylinder_wires(R_vec3 position, float radius_top, float radius_bottom,
                                    float height, int slices,
                                    R_color color);// Draw a cylinder/cone wires
R_public void R_draw_plane(R_vec3 center_pos, R_vec2 size, R_color color);// Draw a plane XZ
R_public void R_draw_ray(R_ray ray, R_color color);                       // Draw a ray line
R_public void R_draw_grid(int slices, float spacing);// Draw a grid (centered at (0, 0, 0))
R_public void R_draw_gizmo(R_vec3 position);         // Draw simple gizmo

// R_model drawing functions
R_public void R_draw_model(R_model model, R_vec3 position, float scale,
                           R_color tint);// Draw a model (with texture if set)
R_public void R_draw_model_ex(R_model model, R_vec3 position, R_vec3 rotation_axis,
                              float rotation_angle, R_vec3 scale,
                              R_color tint);// Draw a model with extended parameters
R_public void R_draw_model_wires(
        R_model model, R_vec3 position, R_vec3 rotation_axis, float rotation_angle, R_vec3 scale,
        R_color tint);// Draw a model wires (with texture if set) with extended parameters
R_public void R_draw_bounding_box(R_bounding_box box, R_color color);// Draw bounding box (wires)
R_public void R_draw_billboard(R_camera3d camera, R_texture2d texture, R_vec3 center, float size,
                               R_color tint);// Draw a billboard texture
R_public void R_draw_billboard_rec(R_camera3d camera, R_texture2d texture, R_rec source_rec,
                                   R_vec3 center, float size,
                                   R_color tint);// Draw a billboard texture defined by source_rec

// R_image draw

R_public R_texture2d
R_load_texture_from_file(const char *filename, R_allocator temp_allocator,
                         R_io_callbacks io);// Load texture from file into GPU memory (VRAM)
R_public R_texture2d R_load_texture_from_file_data(
        const void *data, R_int dst_size,
        R_allocator temp_allocator);// Load texture from an image file data using stb
R_public R_texture2d R_load_texture_from_image(R_image image);// Load texture from image data
R_public R_texture2d
R_load_texture_from_image_with_mipmaps(R_mipmaps_image image);// Load texture from image data
R_public R_texture_cubemap R_load_texture_cubemap_from_image(
        R_image image, R_cubemap_layout_type layout_type,
        R_allocator
                temp_allocator);// Load cubemap from image, multiple image cubemap layouts supported
R_public R_render_texture2d
R_load_render_texture(int width, int height);// Load texture for rendering (framebuffer)

R_public void R_update_texture(
        R_texture2d texture, const void *pixels,
        R_int pixels_size);// Update GPU texture with new data. Pixels data must match texture.format
R_public void R_gen_texture_mipmaps(R_texture2d *texture);// Generate GPU mipmaps for a texture
R_public void R_set_texture_filter(
        R_texture2d texture, R_texture_filter_mode filter_mode);// Set texture scaling filter mode
R_public void R_set_texture_wrap(R_texture2d texture,
                                 R_texture_wrap_mode wrap_mode);// Set texture wrapping mode
R_public void R_unload_texture(R_texture2d texture);// Unload texture from GPU memory (VRAM)
R_public void R_unload_render_texture(
        R_render_texture2d target);// Unload render texture from GPU memory (VRAM)

R_public R_texture2d R_gen_texture_cubemap(R_shader shader, R_texture2d sky_hdr,
                                           R_int size);// Generate cubemap texture from HDR texture
R_public R_texture2d
R_gen_texture_irradiance(R_shader shader, R_texture2d cubemap,
                         R_int size);// Generate irradiance texture using cubemap data
R_public R_texture2d
R_gen_texture_prefilter(R_shader shader, R_texture2d cubemap,
                        R_int size);// Generate prefilter texture using cubemap data
R_public R_texture2d R_gen_texture_brdf(R_shader shader,
                                        R_int size);// Generate BRDF texture using cubemap data.

#pragma region

#include "string.h"

#ifndef RF_MAX_FILEPATH_LEN
#define RF_MAX_FILEPATH_LEN (1024)
#endif

R_internal inline R_bool R_match_str_n(const char *a, int a_len, const char *b, int b_len) {
    return a_len == b_len && strncmp(a, b, a_len) == 0;
}

R_internal inline R_bool R_match_str_cstr(const char *a, int a_len, const char *b) {
    return R_match_str_n(a, a_len, b, strlen(b));
}

R_internal inline R_bool R_is_file_extension(const char *filename, const char *ext) {
    int filename_len = strlen(filename);
    int ext_len = strlen(ext);

    if (filename_len < ext_len) { return 0; }

    return R_match_str_n(filename + filename_len - ext_len, ext_len, ext, ext_len);
}

R_internal inline const char *R__str_find_last(const char *s, const char *charset) {
    const char *latest_match = NULL;
    for (; s = strpbrk(s, charset), s != NULL; latest_match = s++) {}
    return latest_match;
}

R_internal inline const char *R_get_directory_path_from_file_path(const char *file_path) {
    static R_thread_local char R_global_dir_path[RF_MAX_FILEPATH_LEN];

    const char *last_slash = NULL;
    memset(R_global_dir_path, 0, RF_MAX_FILEPATH_LEN);

    last_slash = R__str_find_last(file_path, "\\/");
    if (!last_slash) { return NULL; }

    // NOTE: Be careful, strncpy() is not safe, it does not care about '\0'
    strncpy(R_global_dir_path, file_path, strlen(file_path) - (strlen(last_slash) - 1));
    R_global_dir_path[strlen(file_path) - strlen(last_slash)] = '\0';// Add '\0' manually

    return R_global_dir_path;
}

R_internal R_gfx_context *R__global_gfx_context_ptr;

#define R_ctx (*R__global_gfx_context_ptr)
#define R_gfx (R_ctx.gfx_backend_data)
#define R_gl (R_gfx.gl)
#define R_batch (*(R_ctx.current_batch))

#pragma endregion

#endif