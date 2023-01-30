#ifndef _METADOT_RENDER_CORE_H_
#define _METADOT_RENDER_CORE_H_

#include "stdarg.h"
#include "stddef.h"
#include "stdint.h"

#define R_invalid_index (-1)

typedef ptrdiff_t R_int;
typedef int R_bool;

#include "core/macros.h"

#pragma region internal flags and macros

#define R_GET_PIXEL(surface, x, y) *((U32 *)((U8 *)surface->pixels + ((y)*surface->pitch) + ((x) * sizeof(U32))))

#define R_assert(x)

#ifndef R_extern
#ifdef __cplusplus
#define R_extern extern "C"
#else
#define R_extern extern
#endif
#endif

#if defined(METADOT_COMPILER_MSVC)
#define R_dll_import __declspec(dllimport)
#define R_dll_export __declspec(dllexport)
#define R_dll_private static
#else
#if defined(METADOT_COMPILER_GCC) || defined(METADOT_COMPILER_CLANG)
#define R_dll_import __attribute__((visibility("default")))
#define R_dll_export __attribute__((visibility("default")))
#define R_dll_private __attribute__((visibility("hidden")))
#else
#define R_dll_import
#define R_dll_export
#define R_dll_private static
#endif
#endif

#ifndef R_public
#define R_public R_extern
#endif

#ifndef R_internal
#define R_internal static
#endif

// Used to make constant literals work even in C++ mode
#ifdef __cplusplus
#define R_lit(type) type
#else
#define R_lit(type) (type)
#endif

#define R_concat_impl(a, b) a##b
#define R_concat(a, b) R_concat_impl(a, b)

/* This macro is used to name variables in custom macros such as custom for-each-loops */
#define R_macro_var(X) R_concat(R_macro_gen_var, R_concat(X, __LINE__))
#pragma endregion

#pragma region source location
typedef struct R_source_location {
    const char *file_name;
    const char *proc_name;
    R_int line_in_file;
} R_source_location;

#define R_current_source_location (R_lit(R_source_location){__FILE__, __FUNCTION__, __LINE__})
#pragma endregion

#pragma region allocator

#define R_default_allocator (R_lit(R_allocator){0, R_libc_allocator_wrapper})
#define R_calloc(allocator, size) (R_calloc_wrapper((allocator), 1, size))
#define R_alloc(allocator, size) ((allocator).allocator_proc(&(allocator), R_current_source_location, R_allocator_mode_alloc, (R_lit(R_allocator_args){0, (size), 0})))
#define R_free(allocator, ptr) ((allocator).allocator_proc(&(allocator), R_current_source_location, R_allocator_mode_free, (R_lit(R_allocator_args){(ptr), 0, 0})))
#define R_realloc(allocator, ptr, new_size, old_size) \
    ((allocator).allocator_proc(&(allocator), R_current_source_location, R_allocator_mode_realloc, (R_lit(R_allocator_args){(ptr), (new_size), (old_size)})))

typedef enum R_allocator_mode {
    R_allocator_mode_unknown = 0,
    R_allocator_mode_alloc,
    R_allocator_mode_free,
    R_allocator_mode_realloc,
} R_allocator_mode;

typedef struct R_allocator_args {
    /*
     * In case of R_allocator_mode_alloc this argument can be ignored.
     * In case of R_allocator_mode_realloc this argument is the pointer to the buffer that must be reallocated.
     * In case of R_allocator_mode_free this argument is the pointer that needs to be freed.
     */
    void *pointer_to_free_or_realloc;

    /*
     * In case of R_allocator_mode_alloc this is the new size that needs to be allocated.
     * In case of R_allocator_mode_realloc this is the new size that the buffer should have.
     * In case of R_allocator_mode_free this argument can be ignored.
     */
    R_int size_to_allocate_or_reallocate;

    /*
     * In case of R_allocator_mode_alloc this argument can be ignored.
     * In case of R_allocator_mode_realloc this is the old size of the buffer.
     * In case of R_allocator_mode_free this argument can be ignored.
     */
    R_int old_size;
} R_allocator_args;

struct R_allocator;

typedef void *(R_allocator_proc)(struct R_allocator *this_allocator, R_source_location source_location, R_allocator_mode mode, R_allocator_args args);

typedef struct R_allocator {
    void *user_data;
    R_allocator_proc *allocator_proc;
} R_allocator;

R_public void *R_calloc_wrapper(R_allocator allocator, R_int amount, R_int size);
R_public void *R_default_realloc(R_allocator allocator, void *source, int old_size, int new_size);
R_public void *R_libc_allocator_wrapper(R_allocator *this_allocator, R_source_location source_location, R_allocator_mode mode, R_allocator_args args);

R_public METADOT_THREADLOCAL R_allocator R__global_allocator_for_dependencies;
#define R_set_global_dependencies_allocator(allocator) R__global_allocator_for_dependencies = (allocator)
#pragma endregion

#pragma region io
#define R_file_size(io, filename) ((io).file_size_proc((io).user_data, filename))
#define R_read_file(io, filename, dst, dst_size) ((io).read_file_proc((io).user_data, filename, dst, dst_size))
#define R_default_io (R_lit(R_io_callbacks){0, R_libc_get_file_size, R_libc_load_file_into_buffer})

typedef struct R_io_callbacks {
    void *user_data;
    R_int (*file_size_proc)(void *user_data, const char *filename);
    R_bool (*read_file_proc)(void *user_data, const char *filename, void *dst,
                             R_int dst_size);  // Returns true if operation was successful
} R_io_callbacks;

R_public R_int R_libc_get_file_size(void *user_data, const char *filename);
R_public R_bool R_libc_load_file_into_buffer(void *user_data, const char *filename, void *dst, R_int dst_size);
#pragma endregion

#pragma region error
#define R_make_recorded_error(error_type) (R_lit(R_recorded_error){R_current_source_location, error_type})

typedef enum R_error_type {
    R_no_error,
    R_bad_argument,
    R_bad_alloc,
    R_bad_io,
    R_bad_buffer_size,
    R_bad_format,
    R_limit_reached,
    R_stbi_failed,
    R_stbtt_failed,
    R_unsupported,
} R_error_type;

typedef struct R_recorded_error {
    R_source_location reported_source_location;
    R_error_type error_type;
} R_recorded_error;

R_public R_recorded_error R_get_last_recorded_error();

R_public METADOT_THREADLOCAL R_recorded_error R__last_error;
#pragma endregion

#pragma region rng
#define R_default_rand_proc (R_libc_rand_wrapper)

typedef R_int (*R_rand_proc)(R_int min, R_int max);

R_public R_int R_libc_rand_wrapper(R_int min, R_int max);
#pragma endregion

#pragma region min max
R_internal inline int R_min_i(R_int a, R_int b) { return ((a) < (b) ? (a) : (b)); }
R_internal inline int R_max_i(R_int a, R_int b) { return ((a) > (b) ? (a) : (b)); }

R_internal inline int R_min_f(float a, float b) { return ((a) < (b) ? (a) : (b)); }
R_internal inline int R_max_f(float a, float b) { return ((a) > (b) ? (a) : (b)); }
#pragma endregion

#pragma region String

#define RF_INVALID_CODEPOINT '?'

typedef uint32_t R_rune;
typedef uint64_t R_utf8_char;

typedef struct R_utf8_stats {
    R_int bytes_processed;
    R_int invalid_bytes;
    R_int valid_rune_count;
    R_int total_rune_count;
} R_utf8_stats;

typedef struct R_decoded_rune {
    R_rune codepoint;
    R_int bytes_processed;
    R_bool valid;
} R_decoded_rune;

typedef struct R_decoded_string {
    R_rune *codepoints;
    R_int size;
    R_int invalid_bytes_count;
    R_bool valid;
} R_decoded_string;

typedef struct R_str {
    char *data;
    R_int size;
} R_str;

typedef struct R_strbuf {
    char *data;
    R_int size;
    R_int capacity;
    R_allocator allocator;
    R_bool valid;
} R_strbuf;

#pragma region unicode
R_public R_decoded_rune R_decode_utf8_char(const char *src, R_int size);

R_public R_utf8_stats R_count_utf8_chars(const char *src, R_int size);

R_public R_utf8_stats R_count_utf8_chars_til(const char *src, R_int size, R_int n);

R_public R_decoded_string R_decode_utf8_to_buffer(const char *src, R_int size, R_rune *dst, R_int dst_size);

R_public R_decoded_string R_decode_utf8(const char *src, R_int size, R_allocator allocator);
#pragma endregion

#pragma region ascii
R_int R_str_to_int(R_str src);
float R_str_to_float(R_str src);

int R_to_digit(char c);
char R_to_upper(char c);
char R_to_lower(char c);

R_bool R_is_ascii(char c);
R_bool R_is_lower(char c);
R_bool R_is_upper(char c);
R_bool R_is_alpha(char c);
R_bool R_is_digit(char c);
R_bool R_is_alnum(char c);
R_bool R_is_space(char c);
#pragma endregion

#pragma region strbuf
R_public R_strbuf R_strbuf_make_ex(R_int initial_amount, R_allocator allocator);

R_public R_strbuf R_strbuf_clone_ex(R_strbuf buf, R_allocator allocator);

R_public R_str R_strbuf_to_str(R_strbuf src);

R_public R_int R_strbuf_remaining_capacity(const R_strbuf *this_buf);

R_public void R_strbuf_reserve(R_strbuf *this_buf, R_int size);

R_public void R_strbuf_ensure_capacity_for(R_strbuf *this_buf, R_int size);

R_public void R_strbuf_append(R_strbuf *this_buf, R_str it);

R_public void R_strbuf_prepend(R_strbuf *this_buf, R_str it);

R_public void R_strbuf_insert_utf8(R_strbuf *this_buf, R_str str_to_insert, R_int insert_at);

R_public void R_strbuf_insert_b(R_strbuf *this_buf, R_str str_to_insert, R_int insert_at);

R_public void R_strbuf_remove_range_utf8(R_strbuf *this_buf, R_int begin, R_int end);

R_public void R_strbuf_remove_range_b(R_strbuf *this_buf, R_int begin, R_int end);

R_public void R_strbuf_free(R_strbuf *this_buf);
#pragma endregion

#pragma region str
R_public R_bool R_str_valid(R_str src);

R_public R_int R_str_len(R_str src);

R_public R_str R_cstr(const char *src);

R_public R_str R_str_advance_b(R_str src, R_int amount);

R_public R_str R_str_eat_spaces(R_str src);

R_public R_rune R_str_get_rune(R_str src, R_int n);

R_public R_utf8_char R_str_get_utf8_char(R_str src, R_int n);

R_public R_str R_str_sub_utf8(R_str, R_int begin, R_int end);

R_public R_str R_str_sub_b(R_str, R_int begin, R_int end);

R_public int R_str_cmp(R_str a, R_str b);

R_public R_bool R_str_match_prefix(R_str, R_str);

R_public R_bool R_str_match_suffix(R_str, R_str);

R_public R_bool R_str_match(R_str, R_str);

R_public R_int R_str_find_first(R_str haystack, R_str needle);

R_public R_int R_str_find_last(R_str haystack, R_str needle);

R_public R_bool R_str_contains(R_str, R_str);

R_public R_utf8_char R_rune_to_utf8_char(R_rune src);

R_public R_str R_str_pop_first_split(R_str *src, R_str split_by);

R_public R_str R_str_pop_last_split(R_str *src, R_str split_by);

#define R_for_str_split(iter, src, split_by)                          \
    R_str R_macro_var(src_) = src;                                    \
    R_str iter = R_str_pop_first_split(&R_macro_var(src_), split_by); \
    R_str R_macro_var(split_by_) = split_by;                          \
    for (; R_str_valid(R_macro_var(src_)); iter = R_str_pop_first_split(&R_macro_var(src_), R_macro_var(split_by_)))
#pragma endregion

#pragma endregion String

#pragma region Math

#define R_pi (3.14159265358979323846f)
#define R_deg2rad (R_pi / 180.0f)
#define R_rad2deg (180.0f / R_pi)

typedef struct R_sizei {
    int width;
    int height;
} R_sizei;

typedef struct R_sizef {
    float width;
    float height;
} R_sizef;

typedef struct R_vec2 {
    float x;
    float y;
} R_vec2;

typedef struct R_vec3 {
    float x;
    float y;
    float z;
} R_vec3;

typedef struct R_vec4 {
    float x;
    float y;
    float z;
    float w;
} R_vec4, R_quaternion;

// The matrix is OpenGL style 4x4 - right handed, column major
typedef struct R_mat {
    float m0, m4, m8, m12;
    float m1, m5, m9, m13;
    float m2, m6, m10, m14;
    float m3, m7, m11, m15;
} R_mat;

typedef struct R_float16 {
    float v[16];
} R_float16;

typedef struct R_rec {
    float x;
    float y;
    float width;
    float height;
} R_rec;

typedef struct R_ray {
    R_vec3 position;   // position (origin)
    R_vec3 direction;  // direction
} R_ray;

typedef struct R_ray_hit_info {
    R_bool hit;       // Did the ray hit something?
    float distance;   // Distance to nearest hit
    R_vec3 position;  // Position of nearest hit
    R_vec3 normal;    // Surface normal of hit
} R_ray_hit_info;

typedef struct R_bounding_box {
    R_vec3 min;  // Minimum vertex box-corner
    R_vec3 max;  // Maximum vertex box-corner
} R_bounding_box;

#pragma region misc
R_public float R_next_pot(float it);
R_public R_vec2 R_center_to_object(R_sizef center_this,
                                   R_rec to_this);          // Returns the position of an object such that it will be centered to a rectangle
R_public float R_clamp(float value, float min, float max);  // Clamp float value
R_public float R_lerp(float start, float end,
                      float amount);  // Calculate linear interpolation between two floats
#pragma endregion

#pragma region vec and matrix math

R_public R_vec2 R_vec2_add(R_vec2 v1, R_vec2 v2);         // Add two vectors (v1 + v2)
R_public R_vec2 R_vec2_sub(R_vec2 v1, R_vec2 v2);         // Subtract two vectors (v1 - v2)
R_public float R_vec2_len(R_vec2 v);                      // Calculate vector length
R_public float R_vec2_dot_product(R_vec2 v1, R_vec2 v2);  // Calculate two vectors dot product
R_public float R_vec2_distance(R_vec2 v1, R_vec2 v2);     // Calculate distance between two vectors
R_public float R_vec2_angle(R_vec2 v1, R_vec2 v2);        // Calculate angle from two vectors in X-axis
R_public R_vec2 R_vec2_scale(R_vec2 v, float scale);      // Scale vector (multiply by value)
R_public R_vec2 R_vec2_mul_v(R_vec2 v1, R_vec2 v2);       // Multiply vector by vector
R_public R_vec2 R_vec2_negate(R_vec2 v);                  // Negate vector
R_public R_vec2 R_vec2_div(R_vec2 v, float div);          // Divide vector by a float value
R_public R_vec2 R_vec2_div_v(R_vec2 v1, R_vec2 v2);       // Divide vector by vector
R_public R_vec2 R_vec2_normalize(R_vec2 v);               // Normalize provided vector
R_public R_vec2 R_vec2_lerp(R_vec2 v1, R_vec2 v2,
                            float amount);  // Calculate linear interpolation between two vectors

R_public R_vec3 R_vec3_add(R_vec3 v1, R_vec3 v2);    // Add two vectors
R_public R_vec3 R_vec3_sub(R_vec3 v1, R_vec3 v2);    // Subtract two vectors
R_public R_vec3 R_vec3_mul(R_vec3 v, float scalar);  // Multiply vector by scalar
R_public R_vec3 R_vec3_mul_v(R_vec3 v1, R_vec3 v2);  // Multiply vector by vector
R_public R_vec3 R_vec3_cross_product(R_vec3 v1,
                                     R_vec3 v2);          // Calculate two vectors cross product
R_public R_vec3 R_vec3_perpendicular(R_vec3 v);           // Calculate one vector perpendicular vector
R_public float R_vec3_len(R_vec3 v);                      // Calculate vector length
R_public float R_vec3_dot_product(R_vec3 v1, R_vec3 v2);  // Calculate two vectors dot product
R_public float R_vec3_distance(R_vec3 v1, R_vec3 v2);     // Calculate distance between two vectors
R_public R_vec3 R_vec3_scale(R_vec3 v, float scale);      // Scale provided vector
R_public R_vec3 R_vec3_negate(R_vec3 v);                  // Negate provided vector (invert direction)
R_public R_vec3 R_vec3_div(R_vec3 v, float div);          // Divide vector by a float value
R_public R_vec3 R_vec3_div_v(R_vec3 v1, R_vec3 v2);       // Divide vector by vector
R_public R_vec3 R_vec3_normalize(R_vec3 v);               // Normalize provided vector
R_public void R_vec3_ortho_normalize(R_vec3 *v1,
                                     R_vec3 *v2);                       // Orthonormalize provided vectors. Makes vectors normalized and orthogonal to each other. Gram-Schmidt function implementation
R_public R_vec3 R_vec3_transform(R_vec3 v, R_mat mat);                  // Transforms a R_vec3 by a given R_mat
R_public R_vec3 R_vec3_rotate_by_quaternion(R_vec3 v, R_quaternion q);  // R_transform a vector by quaternion rotation
R_public R_vec3 R_vec3_lerp(R_vec3 v1, R_vec3 v2,
                            float amount);                // Calculate linear interpolation between two vectors
R_public R_vec3 R_vec3_reflect(R_vec3 v, R_vec3 normal);  // Calculate reflected vector to normal
R_public R_vec3 R_vec3_min(R_vec3 v1,
                           R_vec3 v2);  // Return min value for each pair of components
R_public R_vec3 R_vec3_max(R_vec3 v1,
                           R_vec3 v2);  // Return max value for each pair of components
R_public R_vec3 R_vec3_barycenter(R_vec3 p, R_vec3 a, R_vec3 b,
                                  R_vec3 c);  // Compute barycenter coordinates (u, v, w) for point p with respect to triangle (a, b, c) NOTE: Assumes P is on the plane of the triangle

R_public float R_mat_determinant(R_mat mat);                // Compute matrix determinant
R_public float R_mat_trace(R_mat mat);                      // Returns the trace of the matrix (sum of the values along the diagonal)
R_public R_mat R_mat_transpose(R_mat mat);                  // Transposes provided matrix
R_public R_mat R_mat_invert(R_mat mat);                     // Invert provided matrix
R_public R_mat R_mat_normalize(R_mat mat);                  // Normalize provided matrix
R_public R_mat R_mat_identity(void);                        // Returns identity matrix
R_public R_mat R_mat_add(R_mat left, R_mat right);          // Add two matrices
R_public R_mat R_mat_sub(R_mat left, R_mat right);          // Subtract two matrices (left - right)
R_public R_mat R_mat_translate(float x, float y, float z);  // Returns translation matrix
R_public R_mat R_mat_rotate(R_vec3 axis,
                            float angle);               // Create rotation matrix from axis and angle. NOTE: Angle should be provided in radians
R_public R_mat R_mat_rotate_xyz(R_vec3 ang);            // Returns xyz-rotation matrix (angles in radians)
R_public R_mat R_mat_rotate_x(float angle);             // Returns x-rotation matrix (angle in radians)
R_public R_mat R_mat_rotate_y(float angle);             // Returns y-rotation matrix (angle in radians)
R_public R_mat R_mat_rotate_z(float angle);             // Returns z-rotation matrix (angle in radians)
R_public R_mat R_mat_scale(float x, float y, float z);  // Returns scaling matrix
R_public R_mat R_mat_mul(R_mat left,
                         R_mat right);  // Returns two matrix multiplication. NOTE: When multiplying matrices... the order matters!
R_public R_mat R_mat_frustum(double left, double right, double bottom, double top, double near_val,
                             double far_val);  // Returns perspective GL_PROJECTION matrix
R_public R_mat R_mat_perspective(double fovy, double aspect, double near_val,
                                 double far_val);  // Returns perspective GL_PROJECTION matrix. NOTE: Angle should be provided in radians
R_public R_mat R_mat_ortho(double left, double right, double bottom, double top, double near_val,
                           double far_val);  // Returns orthographic GL_PROJECTION matrix
R_public R_mat R_mat_look_at(R_vec3 eye, R_vec3 target,
                             R_vec3 up);         // Returns camera look-at matrix (view matrix)
R_public R_float16 R_mat_to_float16(R_mat mat);  // Returns the matrix as an array of 16 floats

R_public R_quaternion R_quaternion_identity(void);             // Returns identity quaternion
R_public float R_quaternion_len(R_quaternion q);               // Computes the length of a quaternion
R_public R_quaternion R_quaternion_normalize(R_quaternion q);  // Normalize provided quaternion
R_public R_quaternion R_quaternion_invert(R_quaternion q);     // Invert provided quaternion
R_public R_quaternion R_quaternion_mul(R_quaternion q1,
                                       R_quaternion q2);  // Calculate two quaternion multiplication
R_public R_quaternion R_quaternion_lerp(R_quaternion q1, R_quaternion q2,
                                        float amount);  // Calculate linear interpolation between two quaternions
R_public R_quaternion R_quaternion_nlerp(R_quaternion q1, R_quaternion q2,
                                         float amount);  // Calculate slerp-optimized interpolation between two quaternions
R_public R_quaternion R_quaternion_slerp(R_quaternion q1, R_quaternion q2,
                                         float amount);  // Calculates spherical linear interpolation between two quaternions
R_public R_quaternion R_quaternion_from_vec3_to_vec3(R_vec3 from,
                                                     R_vec3 to);  // Calculate quaternion based on the rotation from one vector to another
R_public R_quaternion R_quaternion_from_mat(R_mat mat);           // Returns a quaternion for a given rotation matrix
R_public R_mat R_quaternion_to_mat(R_quaternion q);               // Returns a matrix for a given quaternion
R_public R_quaternion R_quaternion_from_axis_angle(R_vec3 axis,
                                                   float angle);  // Returns rotation quaternion for an angle and axis. NOTE: angle must be provided in radians
R_public void R_quaternion_to_axis_angle(R_quaternion q, R_vec3 *outAxis,
                                         float *outAngle);                          // Returns the rotation angle and axis for a given quaternion
R_public R_quaternion R_quaternion_from_euler(float roll, float pitch, float yaw);  // Returns he quaternion equivalent to Euler angles
R_public R_vec3 R_quaternion_to_euler(R_quaternion q);  // Return the Euler angles equivalent to quaternion (roll, pitch, yaw). NOTE: Angles are returned in a R_vec3 struct in degrees
R_public R_quaternion R_quaternion_transform(R_quaternion q, R_mat mat);  // R_transform a quaternion given a transformation matrix

#pragma endregion

#pragma region collision detection

R_public R_bool R_rec_match(R_rec a, R_rec b);
R_public R_bool R_check_collision_recs(R_rec rec1,
                                       R_rec rec2);  // Check collision between two rectangles
R_public R_bool R_check_collision_circles(R_vec2 center1, float radius1, R_vec2 center2,
                                          float radius2);                              // Check collision between two circles
R_public R_bool R_check_collision_circle_rec(R_vec2 center, float radius, R_rec rec);  // Check collision between circle and rectangle
R_public R_bool R_check_collision_point_rec(R_vec2 point,
                                            R_rec rec);  // Check if point is inside rectangle
R_public R_bool R_check_collision_point_circle(R_vec2 point, R_vec2 center,
                                               float radius);  // Check if point is inside circle
R_public R_bool R_check_collision_point_triangle(R_vec2 point, R_vec2 p1, R_vec2 p2,
                                                 R_vec2 p3);  // Check if point is inside a triangle

R_public R_rec R_get_collision_rec(R_rec rec1, R_rec rec2);  // Get collision rectangle for two rectangles collision

R_public R_bool R_check_collision_spheres(R_vec3 center_a, float radius_a, R_vec3 center_b,
                                          float radius_b);                          // Detect collision between two spheres
R_public R_bool R_check_collision_boxes(R_bounding_box box1, R_bounding_box box2);  // Detect collision between two bounding boxes
R_public R_bool R_check_collision_box_sphere(R_bounding_box box, R_vec3 center,
                                             float radius);                            // Detect collision between box and sphere
R_public R_bool R_check_collision_ray_sphere(R_ray ray, R_vec3 center, float radius);  // Detect collision between ray and sphere
R_public R_bool R_check_collision_ray_sphere_ex(R_ray ray, R_vec3 center, float radius,
                                                R_vec3 *collision_point);  // Detect collision between ray and sphere, returns collision point
R_public R_bool R_check_collision_ray_box(R_ray ray, R_bounding_box box);  // Detect collision between ray and box

R_public R_ray_hit_info R_collision_ray_triangle(R_ray ray, R_vec3 p1, R_vec3 p2,
                                                 R_vec3 p3);  // Get collision info between ray and triangle
R_public R_ray_hit_info R_collision_ray_ground(R_ray ray,
                                               float ground_height);  // Get collision info between ray and ground plane (Y-normal plane)

#pragma endregion

#pragma region base64

typedef struct R_base64_output {
    int size;
    unsigned char *buffer;
} R_base64_output;

R_public int R_get_size_base64(const unsigned char *input);
R_public R_base64_output R_decode_base64(const unsigned char *input, R_allocator allocator);

#pragma endregion

#pragma endregion Math

#pragma region ARRAY

typedef struct R_arr_header {
    R_int size;
    R_int capacity;
    R_allocator allocator;
} R_arr_header;

#define R_arr(T) T *
#define R_arr_internals(arr) ((R_arr_header *)((arr) - sizeof(R_arr_header)))
#define R_arr_size(arr) (R_arr_internals(arr)->size)
#define R_arr_capacity(arr) (R_arr_internals(arr)->capacity)
#define R_arr_allocator(arr) (R_arr_internals(arr)->allocator)
#define R_arr_begin(arr) (arr)
#define R_arr_end(arr) ((arr) + R_arr_internals(arr)->size)
#define R_arr_first(arr) ((arr)[0])
#define R_arr_last(arr) ((arr)[R_arr_size(arr) - 1])
#define R_arr_is_valid_index(arr, i) (((i) >= 0) && ((i) < R_arr_size(arr)))
#define R_arr_has_space_for(arr, n) (R_arr_size(arr) < (R_arr_capacity(arr) - n))
#define R_arr_add(arr, item) ((arr = R_arr_ensure_capacity((arr), R_arr_has_space_for(arr, 1) ? 1 : ceilf(R_arr_size(arr) * 1.5f)), (arr)[R_arr_size++] = (item) : 0))
#define R_arr_remove_unordered(arr, i) ((arr)[i] = (arr)[R_arr_size(arr)--])
#define R_arr_remove_ordered(arr, i) (R_arr_remove_ordered_impl(R_arr_internals(arr), sizeof(arr[0]), i))
#define R_arr_ensure_capacity(arr, cap) (R_arr_ensure_capacity_impl(R_arr_internals(arr), sizeof(arr[0]), cap))

R_public R_arr(void) R_arr_ensure_capacity_impl(R_arr_header *header, R_int size_of_t, R_int cap);

// R_public R_arr(void)

#pragma endregion ARRAY

#endif
