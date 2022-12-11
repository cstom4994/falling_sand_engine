#include "Engine/Renderer/Render.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma region error

R_thread_local R_recorded_error R__last_error;

R_public R_recorded_error R_get_last_recorded_error() { return R__last_error; }

#pragma endregion

#pragma region allocator

R_thread_local R_allocator R__global_allocator_for_dependencies;

R_public void *R_calloc_wrapper(R_allocator allocator, R_int amount, R_int size) {
    void *ptr = R_alloc(allocator, amount * size);
    memset(ptr, 0, amount * size);
    return ptr;
}

R_public void *R_default_realloc(R_allocator allocator, void *source, int old_size, int new_size) {
    void *new_alloc = R_alloc(allocator, new_size);
    if (new_alloc && source && old_size) { memcpy(new_alloc, source, old_size); }
    if (source) { R_free(allocator, source); }
    return new_alloc;
}

R_public void *R_libc_allocator_wrapper(struct R_allocator *this_allocator,
                                        R_source_location source_location, R_allocator_mode mode,
                                        R_allocator_args args) {
    R_assert(this_allocator);
    (void) this_allocator;
    (void) source_location;

    void *result = 0;

    switch (mode) {
        case R_allocator_mode_alloc:
            result = malloc(args.size_to_allocate_or_reallocate);
            break;

        case R_allocator_mode_free:
            free(args.pointer_to_free_or_realloc);
            break;

        case R_allocator_mode_realloc:
            result = realloc(args.pointer_to_free_or_realloc, args.size_to_allocate_or_reallocate);
            break;

        default:
            break;
    }

    return result;
}

#pragma endregion

#pragma region io

R_public R_int R_libc_get_file_size(void *user_data, const char *filename) {
    ((void) user_data);

    FILE *file = fopen(filename, "rb");

    fseek(file, 0L, SEEK_END);
    int size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    fclose(file);

    return size;
}

R_public R_bool R_libc_load_file_into_buffer(void *user_data, const char *filename, void *dst,
                                             R_int dst_size) {
    ((void) user_data);
    R_bool result = 0;

    FILE *file = fopen(filename, "rb");
    if (file != NULL) {
        fseek(file, 0L, SEEK_END);
        int file_size = ftell(file);
        fseek(file, 0L, SEEK_SET);

        if (dst_size >= file_size) {
            int read_size = fread(dst, 1, file_size, file);
            int no_error = ferror(file) == 0;
            if (no_error && read_size == file_size) { result = 1; }
        }
        // else log_error buffer is not big enough
    }
    // else log error could not open file

    fclose(file);

    return result;
}

#pragma endregion

#pragma region logger

R_internal R_log_type R__log_filter;
R_internal R_logger R__logger;

R_public void R_set_logger(R_logger logger) { R__logger = logger; }
R_public void R_set_logger_filter(R_log_type filter) { R__log_filter = filter; }

R_public R_logger R_get_logger() { return R__logger; }
R_public R_log_type R_get_log_filter() { return R__log_filter; }

R_public void R__internal_log(R_source_location source_location, R_log_type log_type,
                              const char *msg, ...) {
    if (!(log_type & R__log_filter)) return;

    va_list args;

    va_start(args, msg);

    R_error_type error_type = R_no_error;

    // If the log type is an error then the error type must be the first arg
    if (log_type == R_log_type_error) { error_type = va_arg(args, R_error_type); }

    if (R__logger.log_proc) {
        R__logger.log_proc(&R__logger, source_location, log_type, msg, error_type, args);
    }

    va_end(args);
}

R_public const char *R_log_type_string(R_log_type log_type) {
    switch (log_type) {
        case R_log_type_none:
            return "NONE";
        case R_log_type_debug:
            return "DEBUG";
        case R_log_type_info:
            return "INFO";
        case R_log_type_warning:
            return "WARNING";
        case R_log_type_error:
            return "ERROR";
        default:
            return "METADOT_RENDER_LOG_TYPE_UNKNOWN";
    }
}

R_public void R_libc_printf_logger(struct R_logger *logger, R_source_location source_location,
                                   R_log_type log_type, const char *msg, R_error_type error_type,
                                   va_list args) {
    ((void) logger);// unused
    printf("[MetaDot Renderer %s]: ", R_log_type_string(log_type));
    vprintf(msg, args);
    printf("\n");
}

#pragma endregion

#pragma region misc

R_public float R_next_pot(float it) { return powf(2, ceilf(logf(it) / logf(2))); }

R_public R_vec2 R_center_to_object(R_sizef center_this, R_rec to_this) {
    R_vec2 result = {to_this.x + to_this.width / 2 - center_this.width / 2,
                     to_this.y + to_this.height / 2 - center_this.height / 2};
    return result;
}

R_public float R_clamp(float value, float min, float max) {
    const float res = value < min ? min : value;
    return res > max ? max : res;
}

R_public float R_lerp(float start, float end, float amount) {
    return start + amount * (end - start);
}

#pragma endregion

#pragma region base64

R_public int R_get_size_base64(const unsigned char *input) {
    int size = 0;

    for (R_int i = 0; input[4 * i] != 0; i++) {
        if (input[4 * i + 3] == '=') {
            if (input[4 * i + 2] == '=') size += 1;
            else
                size += 2;
        } else
            size += 3;
    }

    return size;
}

R_public R_base64_output R_decode_base64(const unsigned char *input, R_allocator allocator) {
    static const unsigned char R_base64_table[] = {
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            0,  62, 0,  0,  0,  63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  0,  0,
            0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
            19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  0,  0,  26, 27, 28, 29, 30, 31, 32, 33,
            34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

    R_base64_output result;
    result.size = R_get_size_base64(input);
    result.buffer = R_alloc(allocator, result.size);

    for (R_int i = 0; i < result.size / 3; i++) {
        unsigned char a = R_base64_table[(int) input[4 * i + 0]];
        unsigned char b = R_base64_table[(int) input[4 * i + 1]];
        unsigned char c = R_base64_table[(int) input[4 * i + 2]];
        unsigned char d = R_base64_table[(int) input[4 * i + 3]];

        result.buffer[3 * i + 0] = (a << 2) | (b >> 4);
        result.buffer[3 * i + 1] = (b << 4) | (c >> 2);
        result.buffer[3 * i + 2] = (c << 6) | d;
    }

    int n = result.size / 3;

    if (result.size % 3 == 1) {
        unsigned char a = R_base64_table[(int) input[4 * n + 0]];
        unsigned char b = R_base64_table[(int) input[4 * n + 1]];

        result.buffer[result.size - 1] = (a << 2) | (b >> 4);
    } else if (result.size % 3 == 2) {
        unsigned char a = R_base64_table[(int) input[4 * n + 0]];
        unsigned char b = R_base64_table[(int) input[4 * n + 1]];
        unsigned char c = R_base64_table[(int) input[4 * n + 2]];

        result.buffer[result.size - 2] = (a << 2) | (b >> 4);
        result.buffer[result.size - 1] = (b << 4) | (c >> 2);
    }

    return result;
}

#pragma endregion

#pragma region vec and matrix math

// Add two vectors (v1 + v2)
R_public R_vec2 R_vec2_add(R_vec2 v1, R_vec2 v2) {
    R_vec2 result = {v1.x + v2.x, v1.y + v2.y};
    return result;
}

// Subtract two vectors (v1 - v2)
R_public R_vec2 R_vec2_sub(R_vec2 v1, R_vec2 v2) {
    R_vec2 result = {v1.x - v2.x, v1.y - v2.y};
    return result;
}

// Calculate vector length
R_public float R_vec2_len(R_vec2 v) {
    float result = sqrt((v.x * v.x) + (v.y * v.y));
    return result;
}

// Calculate two vectors dot product
R_public float R_vec2_dot_product(R_vec2 v1, R_vec2 v2) {
    float result = (v1.x * v2.x + v1.y * v2.y);
    return result;
}

// Calculate distance between two vectors
R_public float R_vec2_distance(R_vec2 v1, R_vec2 v2) {
    float result = sqrt((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y));
    return result;
}

// Calculate angle from two vectors in X-axis
R_public float R_vec2_angle(R_vec2 v1, R_vec2 v2) {
    float result = atan2f(v2.y - v1.y, v2.x - v1.x) * (180.0f / R_pi);
    if (result < 0) result += 360.0f;
    return result;
}

// Scale vector (multiply by value)
R_public R_vec2 R_vec2_scale(R_vec2 v, float scale) {
    R_vec2 result = {v.x * scale, v.y * scale};
    return result;
}

// Multiply vector by vector
R_public R_vec2 R_vec2_mul_v(R_vec2 v1, R_vec2 v2) {
    R_vec2 result = {v1.x * v2.x, v1.y * v2.y};
    return result;
}

// Negate vector
R_public R_vec2 R_vec2_negate(R_vec2 v) {
    R_vec2 result = {-v.x, -v.y};
    return result;
}

// Divide vector by a float value
R_public R_vec2 R_vec2_div(R_vec2 v, float div) {
    R_vec2 result = {v.x / div, v.y / div};
    return result;
}

// Divide vector by vector
R_public R_vec2 R_vec2_div_v(R_vec2 v1, R_vec2 v2) {
    R_vec2 result = {v1.x / v2.x, v1.y / v2.y};
    return result;
}

// Normalize provided vector
R_public R_vec2 R_vec2_normalize(R_vec2 v) {
    R_vec2 result = R_vec2_div(v, R_vec2_len(v));
    return result;
}

// Calculate linear interpolation between two vectors
R_public R_vec2 R_vec2_lerp(R_vec2 v1, R_vec2 v2, float amount) {
    R_vec2 result = {0};

    result.x = v1.x + amount * (v2.x - v1.x);
    result.y = v1.y + amount * (v2.y - v1.y);

    return result;
}

// Add two vectors
R_public R_vec3 R_vec3_add(R_vec3 v1, R_vec3 v2) {
    R_vec3 result = {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z};
    return result;
}

// Subtract two vectors
R_public R_vec3 R_vec3_sub(R_vec3 v1, R_vec3 v2) {
    R_vec3 result = {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z};
    return result;
}

// Multiply vector by scalar
R_public R_vec3 R_vec3_mul(R_vec3 v, float scalar) {
    R_vec3 result = {v.x * scalar, v.y * scalar, v.z * scalar};
    return result;
}

// Multiply vector by vector
R_public R_vec3 R_vec3_mul_v(R_vec3 v1, R_vec3 v2) {
    R_vec3 result = {v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
    return result;
}

// Calculate two vectors cross product
R_public R_vec3 R_vec3_cross_product(R_vec3 v1, R_vec3 v2) {
    R_vec3 result = {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
                     v1.x * v2.y - v1.y * v2.x};
    return result;
}

// Calculate one vector perpendicular vector
R_public R_vec3 R_vec3_perpendicular(R_vec3 v) {
    R_vec3 result = {0};

    float min = (float) fabs(v.x);
    R_vec3 cardinalAxis = {1.0f, 0.0f, 0.0f};

    if (fabs(v.y) < min) {
        min = (float) fabs(v.y);
        R_vec3 tmp = {0.0f, 1.0f, 0.0f};
        cardinalAxis = tmp;
    }

    if (fabs(v.z) < min) {
        R_vec3 tmp = {0.0f, 0.0f, 1.0f};
        cardinalAxis = tmp;
    }

    result = R_vec3_cross_product(v, cardinalAxis);

    return result;
}

// Calculate vector length
R_public float R_vec3_len(R_vec3 v) {
    float result = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return result;
}

// Calculate two vectors dot product
R_public float R_vec3_dot_product(R_vec3 v1, R_vec3 v2) {
    float result = (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
    return result;
}

// Calculate distance between two vectors
R_public float R_vec3_distance(R_vec3 v1, R_vec3 v2) {
    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    float dz = v2.z - v1.z;
    float result = sqrt(dx * dx + dy * dy + dz * dz);
    return result;
}

// Scale provided vector
R_public R_vec3 R_vec3_scale(R_vec3 v, float scale) {
    R_vec3 result = {v.x * scale, v.y * scale, v.z * scale};
    return result;
}

// Negate provided vector (invert direction)
R_public R_vec3 R_vec3_negate(R_vec3 v) {
    R_vec3 result = {-v.x, -v.y, -v.z};
    return result;
}

// Divide vector by a float value
R_public R_vec3 R_vec3_div(R_vec3 v, float div) {
    R_vec3 result = {v.x / div, v.y / div, v.z / div};
    return result;
}

// Divide vector by vector
R_public R_vec3 R_vec3_div_v(R_vec3 v1, R_vec3 v2) {
    R_vec3 result = {v1.x / v2.x, v1.y / v2.y, v1.z / v2.z};
    return result;
}

// Normalize provided vector
R_public R_vec3 R_vec3_normalize(R_vec3 v) {
    R_vec3 result = v;

    float length, ilength;
    length = R_vec3_len(v);
    if (length == 0.0f) length = 1.0f;
    ilength = 1.0f / length;

    result.x *= ilength;
    result.y *= ilength;
    result.z *= ilength;

    return result;
}

// Orthonormalize provided vectors
// Makes vectors normalized and orthogonal to each other
// Gram-Schmidt function implementation
R_public void R_vec3_ortho_normalize(R_vec3 *v1, R_vec3 *v2) {
    *v1 = R_vec3_normalize(*v1);
    R_vec3 vn = R_vec3_cross_product(*v1, *v2);
    vn = R_vec3_normalize(vn);
    *v2 = R_vec3_cross_product(vn, *v1);
}

// Transforms a R_vec3 by a given R_mat
R_public R_vec3 R_vec3_transform(R_vec3 v, R_mat mat) {
    R_vec3 result = {0};
    float x = v.x;
    float y = v.y;
    float z = v.z;

    result.x = mat.m0 * x + mat.m4 * y + mat.m8 * z + mat.m12;
    result.y = mat.m1 * x + mat.m5 * y + mat.m9 * z + mat.m13;
    result.z = mat.m2 * x + mat.m6 * y + mat.m10 * z + mat.m14;

    return result;
}

// R_transform a vector by quaternion rotation
R_public R_vec3 R_vec3_rotate_by_quaternion(R_vec3 v, R_quaternion q) {
    R_vec3 result = {0};

    result.x = v.x * (q.x * q.x + q.w * q.w - q.y * q.y - q.z * q.z) +
               v.y * (2 * q.x * q.y - 2 * q.w * q.z) + v.z * (2 * q.x * q.z + 2 * q.w * q.y);
    result.y = v.x * (2 * q.w * q.z + 2 * q.x * q.y) +
               v.y * (q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z) +
               v.z * (-2 * q.w * q.x + 2 * q.y * q.z);
    result.z = v.x * (-2 * q.w * q.y + 2 * q.x * q.z) + v.y * (2 * q.w * q.x + 2 * q.y * q.z) +
               v.z * (q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z);

    return result;
}

// Calculate linear interpolation between two vectors
R_public R_vec3 R_vec3_lerp(R_vec3 v1, R_vec3 v2, float amount) {
    R_vec3 result = {0};

    result.x = v1.x + amount * (v2.x - v1.x);
    result.y = v1.y + amount * (v2.y - v1.y);
    result.z = v1.z + amount * (v2.z - v1.z);

    return result;
}

// Calculate reflected vector to normal
R_public R_vec3 R_vec3_reflect(R_vec3 v, R_vec3 normal) {
    // I is the original vector
    // N is the normal of the incident plane
    // R = I - (2*N*( DotProduct[ I,N] ))

    R_vec3 result = {0};

    float dotProduct = R_vec3_dot_product(v, normal);

    result.x = v.x - (2.0f * normal.x) * dotProduct;
    result.y = v.y - (2.0f * normal.y) * dotProduct;
    result.z = v.z - (2.0f * normal.z) * dotProduct;

    return result;
}

// Return min value for each pair of components
R_public R_vec3 R_vec3_min(R_vec3 v1, R_vec3 v2) {
    R_vec3 result = {0};

    result.x = fmin(v1.x, v2.x);
    result.y = fmin(v1.y, v2.y);
    result.z = fmin(v1.z, v2.z);

    return result;
}

// Return max value for each pair of components
R_public R_vec3 R_vec3_max(R_vec3 v1, R_vec3 v2) {
    R_vec3 result = {0};

    result.x = fmax(v1.x, v2.x);
    result.y = fmax(v1.y, v2.y);
    result.z = fmax(v1.z, v2.z);

    return result;
}

// Compute barycenter coordinates (u, v, w) for point p with respect to triangle (a, b, c)
// NOTE: Assumes P is on the plane of the triangle
R_public R_vec3 R_vec3_barycenter(R_vec3 p, R_vec3 a, R_vec3 b, R_vec3 c) {
    //Vector v0 = b - a, v1 = c - a, v2 = p - a;

    R_vec3 v0 = R_vec3_sub(b, a);
    R_vec3 v1 = R_vec3_sub(c, a);
    R_vec3 v2 = R_vec3_sub(p, a);
    float d00 = R_vec3_dot_product(v0, v0);
    float d01 = R_vec3_dot_product(v0, v1);
    float d11 = R_vec3_dot_product(v1, v1);
    float d20 = R_vec3_dot_product(v2, v0);
    float d21 = R_vec3_dot_product(v2, v1);

    float denom = d00 * d11 - d01 * d01;

    R_vec3 result = {0};

    result.y = (d11 * d20 - d01 * d21) / denom;
    result.z = (d00 * d21 - d01 * d20) / denom;
    result.x = 1.0f - (result.z + result.y);

    return result;
}

// Compute matrix determinant
R_public float R_mat_determinant(R_mat mat) {
    float result = 0.0;

    // Cache the matrix values (speed optimization)
    float a00 = mat.m0, a01 = mat.m1, a02 = mat.m2, a03 = mat.m3;
    float a10 = mat.m4, a11 = mat.m5, a12 = mat.m6, a13 = mat.m7;
    float a20 = mat.m8, a21 = mat.m9, a22 = mat.m10, a23 = mat.m11;
    float a30 = mat.m12, a31 = mat.m13, a32 = mat.m14, a33 = mat.m15;

    result = a30 * a21 * a12 * a03 - a20 * a31 * a12 * a03 - a30 * a11 * a22 * a03 +
             a10 * a31 * a22 * a03 + a20 * a11 * a32 * a03 - a10 * a21 * a32 * a03 -
             a30 * a21 * a02 * a13 + a20 * a31 * a02 * a13 + a30 * a01 * a22 * a13 -
             a00 * a31 * a22 * a13 - a20 * a01 * a32 * a13 + a00 * a21 * a32 * a13 +
             a30 * a11 * a02 * a23 - a10 * a31 * a02 * a23 - a30 * a01 * a12 * a23 +
             a00 * a31 * a12 * a23 + a10 * a01 * a32 * a23 - a00 * a11 * a32 * a23 -
             a20 * a11 * a02 * a33 + a10 * a21 * a02 * a33 + a20 * a01 * a12 * a33 -
             a00 * a21 * a12 * a33 - a10 * a01 * a22 * a33 + a00 * a11 * a22 * a33;

    return result;
}

// Returns the trace of the matrix (sum of the values along the diagonal)
R_public float R_mat_trace(R_mat mat) {
    float result = (mat.m0 + mat.m5 + mat.m10 + mat.m15);
    return result;
}

// Transposes provided matrix
R_public R_mat R_mat_transpose(R_mat mat) {
    R_mat result = {0};

    result.m0 = mat.m0;
    result.m1 = mat.m4;
    result.m2 = mat.m8;
    result.m3 = mat.m12;
    result.m4 = mat.m1;
    result.m5 = mat.m5;
    result.m6 = mat.m9;
    result.m7 = mat.m13;
    result.m8 = mat.m2;
    result.m9 = mat.m6;
    result.m10 = mat.m10;
    result.m11 = mat.m14;
    result.m12 = mat.m3;
    result.m13 = mat.m7;
    result.m14 = mat.m11;
    result.m15 = mat.m15;

    return result;
}

// Invert provided matrix
R_public R_mat R_mat_invert(R_mat mat) {
    R_mat result = {0};

    // Cache the matrix values (speed optimization)
    float a00 = mat.m0, a01 = mat.m1, a02 = mat.m2, a03 = mat.m3;
    float a10 = mat.m4, a11 = mat.m5, a12 = mat.m6, a13 = mat.m7;
    float a20 = mat.m8, a21 = mat.m9, a22 = mat.m10, a23 = mat.m11;
    float a30 = mat.m12, a31 = mat.m13, a32 = mat.m14, a33 = mat.m15;

    float b00 = a00 * a11 - a01 * a10;
    float b01 = a00 * a12 - a02 * a10;
    float b02 = a00 * a13 - a03 * a10;
    float b03 = a01 * a12 - a02 * a11;
    float b04 = a01 * a13 - a03 * a11;
    float b05 = a02 * a13 - a03 * a12;
    float b06 = a20 * a31 - a21 * a30;
    float b07 = a20 * a32 - a22 * a30;
    float b08 = a20 * a33 - a23 * a30;
    float b09 = a21 * a32 - a22 * a31;
    float b10 = a21 * a33 - a23 * a31;
    float b11 = a22 * a33 - a23 * a32;

    // Calculate the invert determinant (inlined to avoid double-caching)
    float invDet = 1.0f / (b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06);

    result.m0 = (a11 * b11 - a12 * b10 + a13 * b09) * invDet;
    result.m1 = (-a01 * b11 + a02 * b10 - a03 * b09) * invDet;
    result.m2 = (a31 * b05 - a32 * b04 + a33 * b03) * invDet;
    result.m3 = (-a21 * b05 + a22 * b04 - a23 * b03) * invDet;
    result.m4 = (-a10 * b11 + a12 * b08 - a13 * b07) * invDet;
    result.m5 = (a00 * b11 - a02 * b08 + a03 * b07) * invDet;
    result.m6 = (-a30 * b05 + a32 * b02 - a33 * b01) * invDet;
    result.m7 = (a20 * b05 - a22 * b02 + a23 * b01) * invDet;
    result.m8 = (a10 * b10 - a11 * b08 + a13 * b06) * invDet;
    result.m9 = (-a00 * b10 + a01 * b08 - a03 * b06) * invDet;
    result.m10 = (a30 * b04 - a31 * b02 + a33 * b00) * invDet;
    result.m11 = (-a20 * b04 + a21 * b02 - a23 * b00) * invDet;
    result.m12 = (-a10 * b09 + a11 * b07 - a12 * b06) * invDet;
    result.m13 = (a00 * b09 - a01 * b07 + a02 * b06) * invDet;
    result.m14 = (-a30 * b03 + a31 * b01 - a32 * b00) * invDet;
    result.m15 = (a20 * b03 - a21 * b01 + a22 * b00) * invDet;

    return result;
}

// Normalize provided matrix
R_public R_mat R_mat_normalize(R_mat mat) {
    R_mat result = {0};

    float det = R_mat_determinant(mat);

    result.m0 = mat.m0 / det;
    result.m1 = mat.m1 / det;
    result.m2 = mat.m2 / det;
    result.m3 = mat.m3 / det;
    result.m4 = mat.m4 / det;
    result.m5 = mat.m5 / det;
    result.m6 = mat.m6 / det;
    result.m7 = mat.m7 / det;
    result.m8 = mat.m8 / det;
    result.m9 = mat.m9 / det;
    result.m10 = mat.m10 / det;
    result.m11 = mat.m11 / det;
    result.m12 = mat.m12 / det;
    result.m13 = mat.m13 / det;
    result.m14 = mat.m14 / det;
    result.m15 = mat.m15 / det;

    return result;
}

// Returns identity matrix
R_public R_mat R_mat_identity(void) {
    R_mat result = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    return result;
}

// Add two matrices
R_public R_mat R_mat_add(R_mat left, R_mat right) {
    R_mat result = R_mat_identity();

    result.m0 = left.m0 + right.m0;
    result.m1 = left.m1 + right.m1;
    result.m2 = left.m2 + right.m2;
    result.m3 = left.m3 + right.m3;
    result.m4 = left.m4 + right.m4;
    result.m5 = left.m5 + right.m5;
    result.m6 = left.m6 + right.m6;
    result.m7 = left.m7 + right.m7;
    result.m8 = left.m8 + right.m8;
    result.m9 = left.m9 + right.m9;
    result.m10 = left.m10 + right.m10;
    result.m11 = left.m11 + right.m11;
    result.m12 = left.m12 + right.m12;
    result.m13 = left.m13 + right.m13;
    result.m14 = left.m14 + right.m14;
    result.m15 = left.m15 + right.m15;

    return result;
}

// Subtract two matrices (left - right)
R_public R_mat R_mat_sub(R_mat left, R_mat right) {
    R_mat result = R_mat_identity();

    result.m0 = left.m0 - right.m0;
    result.m1 = left.m1 - right.m1;
    result.m2 = left.m2 - right.m2;
    result.m3 = left.m3 - right.m3;
    result.m4 = left.m4 - right.m4;
    result.m5 = left.m5 - right.m5;
    result.m6 = left.m6 - right.m6;
    result.m7 = left.m7 - right.m7;
    result.m8 = left.m8 - right.m8;
    result.m9 = left.m9 - right.m9;
    result.m10 = left.m10 - right.m10;
    result.m11 = left.m11 - right.m11;
    result.m12 = left.m12 - right.m12;
    result.m13 = left.m13 - right.m13;
    result.m14 = left.m14 - right.m14;
    result.m15 = left.m15 - right.m15;

    return result;
}

// Returns translation matrix
R_public R_mat R_mat_translate(float x, float y, float z) {
    R_mat result = {1.0f, 0.0f, 0.0f, x, 0.0f, 1.0f, 0.0f, y,
                    0.0f, 0.0f, 1.0f, z, 0.0f, 0.0f, 0.0f, 1.0f};

    return result;
}

// Create rotation matrix from axis and angle
// NOTE: Angle should be provided in radians
R_public R_mat R_mat_rotate(R_vec3 axis, float angle) {
    R_mat result = {0};

    float x = axis.x, y = axis.y, z = axis.z;

    float length = sqrt(x * x + y * y + z * z);

    if ((length != 1.0f) && (length != 0.0f)) {
        length = 1.0f / length;
        x *= length;
        y *= length;
        z *= length;
    }

    float sinres = sinf(angle);
    float cosres = cosf(angle);
    float t = 1.0f - cosres;

    result.m0 = x * x * t + cosres;
    result.m1 = y * x * t + z * sinres;
    result.m2 = z * x * t - y * sinres;
    result.m3 = 0.0f;

    result.m4 = x * y * t - z * sinres;
    result.m5 = y * y * t + cosres;
    result.m6 = z * y * t + x * sinres;
    result.m7 = 0.0f;

    result.m8 = x * z * t + y * sinres;
    result.m9 = y * z * t - x * sinres;
    result.m10 = z * z * t + cosres;
    result.m11 = 0.0f;

    result.m12 = 0.0f;
    result.m13 = 0.0f;
    result.m14 = 0.0f;
    result.m15 = 1.0f;

    return result;
}

// Returns xyz-rotation matrix (angles in radians)
R_public R_mat R_mat_rotate_xyz(R_vec3 ang) {
    R_mat result = R_mat_identity();

    float cosz = cosf(-ang.z);
    float sinz = sinf(-ang.z);
    float cosy = cosf(-ang.y);
    float siny = sinf(-ang.y);
    float cosx = cosf(-ang.x);
    float sinx = sinf(-ang.x);

    result.m0 = cosz * cosy;
    result.m4 = (cosz * siny * sinx) - (sinz * cosx);
    result.m8 = (cosz * siny * cosx) + (sinz * sinx);

    result.m1 = sinz * cosy;
    result.m5 = (sinz * siny * sinx) + (cosz * cosx);
    result.m9 = (sinz * siny * cosx) - (cosz * sinx);

    result.m2 = -siny;
    result.m6 = cosy * sinx;
    result.m10 = cosy * cosx;

    return result;
}

// Returns x-rotation matrix (angle in radians)
R_public R_mat R_mat_rotate_x(float angle) {
    R_mat result = R_mat_identity();

    float cosres = cosf(angle);
    float sinres = sinf(angle);

    result.m5 = cosres;
    result.m6 = -sinres;
    result.m9 = sinres;
    result.m10 = cosres;

    return result;
}

// Returns y-rotation matrix (angle in radians)
R_public R_mat R_mat_rotate_y(float angle) {
    R_mat result = R_mat_identity();

    float cosres = cosf(angle);
    float sinres = sinf(angle);

    result.m0 = cosres;
    result.m2 = sinres;
    result.m8 = -sinres;
    result.m10 = cosres;

    return result;
}

// Returns z-rotation matrix (angle in radians)
R_public R_mat R_mat_rotate_z(float angle) {
    R_mat result = R_mat_identity();

    float cosres = cosf(angle);
    float sinres = sinf(angle);

    result.m0 = cosres;
    result.m1 = -sinres;
    result.m4 = sinres;
    result.m5 = cosres;

    return result;
}

// Returns scaling matrix
R_public R_mat R_mat_scale(float x, float y, float z) {
    R_mat result = {x,    0.0f, 0.0f, 0.0f, 0.0f, y,    0.0f, 0.0f,
                    0.0f, 0.0f, z,    0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    return result;
}

// Returns two matrix multiplication
// NOTE: When multiplying matrices... the order matters!
R_public R_mat R_mat_mul(R_mat left, R_mat right) {
    R_mat result = {0};

    result.m0 = left.m0 * right.m0 + left.m1 * right.m4 + left.m2 * right.m8 + left.m3 * right.m12;
    result.m1 = left.m0 * right.m1 + left.m1 * right.m5 + left.m2 * right.m9 + left.m3 * right.m13;
    result.m2 = left.m0 * right.m2 + left.m1 * right.m6 + left.m2 * right.m10 + left.m3 * right.m14;
    result.m3 = left.m0 * right.m3 + left.m1 * right.m7 + left.m2 * right.m11 + left.m3 * right.m15;
    result.m4 = left.m4 * right.m0 + left.m5 * right.m4 + left.m6 * right.m8 + left.m7 * right.m12;
    result.m5 = left.m4 * right.m1 + left.m5 * right.m5 + left.m6 * right.m9 + left.m7 * right.m13;
    result.m6 = left.m4 * right.m2 + left.m5 * right.m6 + left.m6 * right.m10 + left.m7 * right.m14;
    result.m7 = left.m4 * right.m3 + left.m5 * right.m7 + left.m6 * right.m11 + left.m7 * right.m15;
    result.m8 =
            left.m8 * right.m0 + left.m9 * right.m4 + left.m10 * right.m8 + left.m11 * right.m12;
    result.m9 =
            left.m8 * right.m1 + left.m9 * right.m5 + left.m10 * right.m9 + left.m11 * right.m13;
    result.m10 =
            left.m8 * right.m2 + left.m9 * right.m6 + left.m10 * right.m10 + left.m11 * right.m14;
    result.m11 =
            left.m8 * right.m3 + left.m9 * right.m7 + left.m10 * right.m11 + left.m11 * right.m15;
    result.m12 =
            left.m12 * right.m0 + left.m13 * right.m4 + left.m14 * right.m8 + left.m15 * right.m12;
    result.m13 =
            left.m12 * right.m1 + left.m13 * right.m5 + left.m14 * right.m9 + left.m15 * right.m13;
    result.m14 =
            left.m12 * right.m2 + left.m13 * right.m6 + left.m14 * right.m10 + left.m15 * right.m14;
    result.m15 =
            left.m12 * right.m3 + left.m13 * right.m7 + left.m14 * right.m11 + left.m15 * right.m15;

    return result;
}

// Returns perspective GL_PROJECTION matrix
R_public R_mat R_mat_frustum(double left, double right, double bottom, double top, double near_val,
                             double far_val) {
    R_mat result = {0};

    float rl = (float) (right - left);
    float tb = (float) (top - bottom);
    float fn = (float) (far_val - near_val);

    result.m0 = ((float) near_val * 2.0f) / rl;
    result.m1 = 0.0f;
    result.m2 = 0.0f;
    result.m3 = 0.0f;

    result.m4 = 0.0f;
    result.m5 = ((float) near_val * 2.0f) / tb;
    result.m6 = 0.0f;
    result.m7 = 0.0f;

    result.m8 = ((float) right + (float) left) / rl;
    result.m9 = ((float) top + (float) bottom) / tb;
    result.m10 = -((float) far_val + (float) near_val) / fn;
    result.m11 = -1.0f;

    result.m12 = 0.0f;
    result.m13 = 0.0f;
    result.m14 = -((float) far_val * (float) near_val * 2.0f) / fn;
    result.m15 = 0.0f;

    return result;
}

// Returns perspective GL_PROJECTION matrix
// NOTE: Angle should be provided in radians
R_public R_mat R_mat_perspective(double fovy, double aspect, double near_val, double far_val) {
    double top = near_val * tan(fovy * 0.5);
    double right = top * aspect;
    R_mat result = R_mat_frustum(-right, right, -top, top, near_val, far_val);

    return result;
}

// Returns orthographic GL_PROJECTION matrix
R_public R_mat R_mat_ortho(double left, double right, double bottom, double top, double near_val,
                           double far_val) {
    R_mat result = {0};

    float rl = (float) (right - left);
    float tb = (float) (top - bottom);
    float fn = (float) (far_val - near_val);

    result.m0 = 2.0f / rl;
    result.m1 = 0.0f;
    result.m2 = 0.0f;
    result.m3 = 0.0f;
    result.m4 = 0.0f;
    result.m5 = 2.0f / tb;
    result.m6 = 0.0f;
    result.m7 = 0.0f;
    result.m8 = 0.0f;
    result.m9 = 0.0f;
    result.m10 = -2.0f / fn;
    result.m11 = 0.0f;
    result.m12 = -((float) left + (float) right) / rl;
    result.m13 = -((float) top + (float) bottom) / tb;
    result.m14 = -((float) far_val + (float) near_val) / fn;
    result.m15 = 1.0f;

    return result;
}

// Returns camera look-at matrix (view matrix)
R_public R_mat R_mat_look_at(R_vec3 eye, R_vec3 target, R_vec3 up) {
    R_mat result = {0};

    R_vec3 z = R_vec3_sub(eye, target);
    z = R_vec3_normalize(z);
    R_vec3 x = R_vec3_cross_product(up, z);
    x = R_vec3_normalize(x);
    R_vec3 y = R_vec3_cross_product(z, x);
    y = R_vec3_normalize(y);

    result.m0 = x.x;
    result.m1 = x.y;
    result.m2 = x.z;
    result.m3 = 0.0f;
    result.m4 = y.x;
    result.m5 = y.y;
    result.m6 = y.z;
    result.m7 = 0.0f;
    result.m8 = z.x;
    result.m9 = z.y;
    result.m10 = z.z;
    result.m11 = 0.0f;
    result.m12 = eye.x;
    result.m13 = eye.y;
    result.m14 = eye.z;
    result.m15 = 1.0f;

    result = R_mat_invert(result);

    return result;
}

R_public R_float16 R_mat_to_float16(R_mat mat) {
    R_float16 buffer = {0};

    buffer.v[0] = mat.m0;
    buffer.v[1] = mat.m1;
    buffer.v[2] = mat.m2;
    buffer.v[3] = mat.m3;
    buffer.v[4] = mat.m4;
    buffer.v[5] = mat.m5;
    buffer.v[6] = mat.m6;
    buffer.v[7] = mat.m7;
    buffer.v[8] = mat.m8;
    buffer.v[9] = mat.m9;
    buffer.v[10] = mat.m10;
    buffer.v[11] = mat.m11;
    buffer.v[12] = mat.m12;
    buffer.v[13] = mat.m13;
    buffer.v[14] = mat.m14;
    buffer.v[15] = mat.m15;

    return buffer;
}

// Returns identity quaternion
R_public R_quaternion R_quaternion_identity(void) {
    R_quaternion result = {0.0f, 0.0f, 0.0f, 1.0f};
    return result;
}

// Computes the length of a quaternion
R_public float R_quaternion_len(R_quaternion q) {
    float result = (float) sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    return result;
}

// Normalize provided quaternion
R_public R_quaternion R_quaternion_normalize(R_quaternion q) {
    R_quaternion result = {0};

    float length, ilength;
    length = R_quaternion_len(q);
    if (length == 0.0f) length = 1.0f;
    ilength = 1.0f / length;

    result.x = q.x * ilength;
    result.y = q.y * ilength;
    result.z = q.z * ilength;
    result.w = q.w * ilength;

    return result;
}

// Invert provided quaternion
R_public R_quaternion R_quaternion_invert(R_quaternion q) {
    R_quaternion result = q;
    float length = R_quaternion_len(q);
    float lengthSq = length * length;

    if (lengthSq != 0.0) {
        float i = 1.0f / lengthSq;

        result.x *= -i;
        result.y *= -i;
        result.z *= -i;
        result.w *= i;
    }

    return result;
}

// Calculate two quaternion multiplication
R_public R_quaternion R_quaternion_mul(R_quaternion q1, R_quaternion q2) {
    R_quaternion result = {0};

    float qax = q1.x, qay = q1.y, qaz = q1.z, qaw = q1.w;
    float qbx = q2.x, qby = q2.y, qbz = q2.z, qbw = q2.w;

    result.x = qax * qbw + qaw * qbx + qay * qbz - qaz * qby;
    result.y = qay * qbw + qaw * qby + qaz * qbx - qax * qbz;
    result.z = qaz * qbw + qaw * qbz + qax * qby - qay * qbx;
    result.w = qaw * qbw - qax * qbx - qay * qby - qaz * qbz;

    return result;
}

// Calculate linear interpolation between two quaternions
R_public R_quaternion R_quaternion_lerp(R_quaternion q1, R_quaternion q2, float amount) {
    R_quaternion result = {0};

    result.x = q1.x + amount * (q2.x - q1.x);
    result.y = q1.y + amount * (q2.y - q1.y);
    result.z = q1.z + amount * (q2.z - q1.z);
    result.w = q1.w + amount * (q2.w - q1.w);

    return result;
}

// Calculate slerp-optimized interpolation between two quaternions
R_public R_quaternion R_quaternion_nlerp(R_quaternion q1, R_quaternion q2, float amount) {
    R_quaternion result = R_quaternion_lerp(q1, q2, amount);
    result = R_quaternion_normalize(result);

    return result;
}

// Calculates spherical linear interpolation between two quaternions
R_public R_quaternion R_quaternion_slerp(R_quaternion q1, R_quaternion q2, float amount) {
    R_quaternion result = {0};

    float cosHalfTheta = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

    if (fabs(cosHalfTheta) >= 1.0f) result = q1;
    else if (cosHalfTheta > 0.95f)
        result = R_quaternion_nlerp(q1, q2, amount);
    else {
        float halfTheta = (float) acos(cosHalfTheta);
        float sinHalfTheta = (float) sqrt(1.0f - cosHalfTheta * cosHalfTheta);

        if (fabs(sinHalfTheta) < 0.001f) {
            result.x = (q1.x * 0.5f + q2.x * 0.5f);
            result.y = (q1.y * 0.5f + q2.y * 0.5f);
            result.z = (q1.z * 0.5f + q2.z * 0.5f);
            result.w = (q1.w * 0.5f + q2.w * 0.5f);
        } else {
            float ratioA = sinf((1 - amount) * halfTheta) / sinHalfTheta;
            float ratioB = sinf(amount * halfTheta) / sinHalfTheta;

            result.x = (q1.x * ratioA + q2.x * ratioB);
            result.y = (q1.y * ratioA + q2.y * ratioB);
            result.z = (q1.z * ratioA + q2.z * ratioB);
            result.w = (q1.w * ratioA + q2.w * ratioB);
        }
    }

    return result;
}

// Calculate quaternion based on the rotation from one vector to another
R_public R_quaternion R_quaternion_from_vector3_to_vector3(R_vec3 from, R_vec3 to) {
    R_quaternion result = {0};

    float cos2Theta = R_vec3_dot_product(from, to);
    R_vec3 cross = R_vec3_cross_product(from, to);

    result.x = cross.x;
    result.y = cross.y;
    result.z = cross.y;
    result.w = 1.0f + cos2Theta;// NOTE: Added QuaternioIdentity()

    // Normalize to essentially nlerp the original and identity to 0.5
    result = R_quaternion_normalize(result);

    // Above lines are equivalent to:
    //R_quaternion result = R_quaternion_nlerp(q, R_quaternion_identity(), 0.5f);

    return result;
}

// Returns a quaternion for a given rotation matrix
R_public R_quaternion R_quaternion_from_matrix(R_mat mat) {
    R_quaternion result = {0};

    float trace = R_mat_trace(mat);

    if (trace > 0.0f) {
        float s = (float) sqrt(trace + 1) * 2.0f;
        float invS = 1.0f / s;

        result.w = s * 0.25f;
        result.x = (mat.m6 - mat.m9) * invS;
        result.y = (mat.m8 - mat.m2) * invS;
        result.z = (mat.m1 - mat.m4) * invS;
    } else {
        float m00 = mat.m0, m11 = mat.m5, m22 = mat.m10;

        if (m00 > m11 && m00 > m22) {
            float s = (float) sqrt(1.0f + m00 - m11 - m22) * 2.0f;
            float invS = 1.0f / s;

            result.w = (mat.m6 - mat.m9) * invS;
            result.x = s * 0.25f;
            result.y = (mat.m4 + mat.m1) * invS;
            result.z = (mat.m8 + mat.m2) * invS;
        } else if (m11 > m22) {
            float s = (float) sqrt(1.0f + m11 - m00 - m22) * 2.0f;
            float invS = 1.0f / s;

            result.w = (mat.m8 - mat.m2) * invS;
            result.x = (mat.m4 + mat.m1) * invS;
            result.y = s * 0.25f;
            result.z = (mat.m9 + mat.m6) * invS;
        } else {
            float s = (float) sqrt(1.0f + m22 - m00 - m11) * 2.0f;
            float invS = 1.0f / s;

            result.w = (mat.m1 - mat.m4) * invS;
            result.x = (mat.m8 + mat.m2) * invS;
            result.y = (mat.m9 + mat.m6) * invS;
            result.z = s * 0.25f;
        }
    }

    return result;
}

// Returns a matrix for a given quaternion
R_public R_mat R_quaternion_to_matrix(R_quaternion q) {
    R_mat result = {0};

    float x = q.x, y = q.y, z = q.z, w = q.w;

    float x2 = x + x;
    float y2 = y + y;
    float z2 = z + z;

    float length = R_quaternion_len(q);
    float lengthSquared = length * length;

    float xx = x * x2 / lengthSquared;
    float xy = x * y2 / lengthSquared;
    float xz = x * z2 / lengthSquared;

    float yy = y * y2 / lengthSquared;
    float yz = y * z2 / lengthSquared;
    float zz = z * z2 / lengthSquared;

    float wx = w * x2 / lengthSquared;
    float wy = w * y2 / lengthSquared;
    float wz = w * z2 / lengthSquared;

    result.m0 = 1.0f - (yy + zz);
    result.m1 = xy - wz;
    result.m2 = xz + wy;
    result.m3 = 0.0f;
    result.m4 = xy + wz;
    result.m5 = 1.0f - (xx + zz);
    result.m6 = yz - wx;
    result.m7 = 0.0f;
    result.m8 = xz - wy;
    result.m9 = yz + wx;
    result.m10 = 1.0f - (xx + yy);
    result.m11 = 0.0f;
    result.m12 = 0.0f;
    result.m13 = 0.0f;
    result.m14 = 0.0f;
    result.m15 = 1.0f;

    return result;
}

// Returns rotation quaternion for an angle and axis
// NOTE: angle must be provided in radians
R_public R_quaternion R_quaternion_from_axis_angle(R_vec3 axis, float angle) {
    R_quaternion result = {0.0f, 0.0f, 0.0f, 1.0f};

    if (R_vec3_len(axis) != 0.0f) angle *= 0.5f;

    axis = R_vec3_normalize(axis);

    float sinres = sinf(angle);
    float cosres = cosf(angle);

    result.x = axis.x * sinres;
    result.y = axis.y * sinres;
    result.z = axis.z * sinres;
    result.w = cosres;

    result = R_quaternion_normalize(result);

    return result;
}

// Returns the rotation angle and axis for a given quaternion
R_public void R_quaternion_to_axis_angle(R_quaternion q, R_vec3 *outAxis, float *outAngle) {
    if (fabs(q.w) > 1.0f) q = R_quaternion_normalize(q);

    R_vec3 resAxis = {0.0f, 0.0f, 0.0f};
    float resAngle = 0.0f;

    resAngle = 2.0f * (float) acos(q.w);
    float den = (float) sqrt(1.0f - q.w * q.w);

    if (den > 0.0001f) {
        resAxis.x = q.x / den;
        resAxis.y = q.y / den;
        resAxis.z = q.z / den;
    } else {
        // This occurs when the angle is zero.
        // Not a problem: just set an arbitrary normalized axis.
        resAxis.x = 1.0f;
    }

    *outAxis = resAxis;
    *outAngle = resAngle;
}

// Returns he quaternion equivalent to Euler angles
R_public R_quaternion R_quaternion_from_euler(float roll, float pitch, float yaw) {
    R_quaternion q = {0};

    float x0 = cosf(roll * 0.5f);
    float x1 = sinf(roll * 0.5f);
    float y0 = cosf(pitch * 0.5f);
    float y1 = sinf(pitch * 0.5f);
    float z0 = cosf(yaw * 0.5f);
    float z1 = sinf(yaw * 0.5f);

    q.x = x1 * y0 * z0 - x0 * y1 * z1;
    q.y = x0 * y1 * z0 + x1 * y0 * z1;
    q.z = x0 * y0 * z1 - x1 * y1 * z0;
    q.w = x0 * y0 * z0 + x1 * y1 * z1;

    return q;
}

// Return the Euler angles equivalent to quaternion (roll, pitch, yaw)
// NOTE: Angles are returned in a R_vec3 struct in degrees
R_public R_vec3 R_quaternion_to_euler(R_quaternion q) {
    R_vec3 result = {0};

    // roll (x-axis rotation)
    float x0 = 2.0f * (q.w * q.x + q.y * q.z);
    float x1 = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    result.x = atan2f(x0, x1) * R_rad2deg;

    // pitch (y-axis rotation)
    float y0 = 2.0f * (q.w * q.y - q.z * q.x);
    y0 = y0 > 1.0f ? 1.0f : y0;
    y0 = y0 < -1.0f ? -1.0f : y0;
    result.y = sinf(y0) * R_rad2deg;

    // yaw (z-axis rotation)
    float z0 = 2.0f * (q.w * q.z + q.x * q.y);
    float z1 = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    result.z = atan2f(z0, z1) * R_rad2deg;

    return result;
}

// R_transform a quaternion given a transformation matrix
R_public R_quaternion R_quaternion_transform(R_quaternion q, R_mat mat) {
    R_quaternion result = {0};

    result.x = mat.m0 * q.x + mat.m4 * q.y + mat.m8 * q.z + mat.m12 * q.w;
    result.y = mat.m1 * q.x + mat.m5 * q.y + mat.m9 * q.z + mat.m13 * q.w;
    result.z = mat.m2 * q.x + mat.m6 * q.y + mat.m10 * q.z + mat.m14 * q.w;
    result.w = mat.m3 * q.x + mat.m7 * q.y + mat.m11 * q.z + mat.m15 * q.w;

    return result;
}

#pragma endregion

#pragma region collision detection

// Check if point is inside rectangle
R_bool R_check_collision_point_rec(R_vec2 point, R_rec rec) {
    R_bool collision = 0;

    if ((point.x >= rec.x) && (point.x <= (rec.x + rec.width)) && (point.y >= rec.y) &&
        (point.y <= (rec.y + rec.height)))
        collision = 1;

    return collision;
}

// Check if point is inside circle
R_bool R_check_collision_point_circle(R_vec2 point, R_vec2 center, float radius) {
    return R_check_collision_circles(point, 0, center, radius);
}

// Check if point is inside a triangle defined by three points (p1, p2, p3)
R_bool R_check_collision_point_triangle(R_vec2 point, R_vec2 p1, R_vec2 p2, R_vec2 p3) {
    R_bool collision = 0;

    float alpha = ((p2.y - p3.y) * (point.x - p3.x) + (p3.x - p2.x) * (point.y - p3.y)) /
                  ((p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y));

    float beta = ((p3.y - p1.y) * (point.x - p3.x) + (p1.x - p3.x) * (point.y - p3.y)) /
                 ((p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y));

    float gamma = 1.0f - alpha - beta;

    if ((alpha > 0) && (beta > 0) & (gamma > 0)) collision = 1;

    return collision;
}

// Check collision between two rectangles
R_bool R_check_collision_recs(R_rec rec1, R_rec rec2) {
    R_bool collision = 0;

    if ((rec1.x < (rec2.x + rec2.width) && (rec1.x + rec1.width) > rec2.x) &&
        (rec1.y < (rec2.y + rec2.height) && (rec1.y + rec1.height) > rec2.y))
        collision = 1;

    return collision;
}

// Check collision between two circles
R_bool R_check_collision_circles(R_vec2 center1, float radius1, R_vec2 center2, float radius2) {
    R_bool collision = 0;

    float dx = center2.x - center1.x;// X distance between centers
    float dy = center2.y - center1.y;// Y distance between centers

    float distance = sqrt(dx * dx + dy * dy);// Distance between centers

    if (distance <= (radius1 + radius2)) collision = 1;

    return collision;
}

// Check collision between circle and rectangle
// NOTE: Reviewed version to take into account corner limit case
R_bool R_check_collision_circle_rec(R_vec2 center, float radius, R_rec rec) {
    int recCenterX = (int) (rec.x + rec.width / 2.0f);
    int recCenterY = (int) (rec.y + rec.height / 2.0f);

    float dx = (float) fabs(center.x - recCenterX);
    float dy = (float) fabs(center.y - recCenterY);

    if (dx > (rec.width / 2.0f + radius)) { return 0; }
    if (dy > (rec.height / 2.0f + radius)) { return 0; }

    if (dx <= (rec.width / 2.0f)) { return 1; }
    if (dy <= (rec.height / 2.0f)) { return 1; }

    float cornerDistanceSq = (dx - rec.width / 2.0f) * (dx - rec.width / 2.0f) +
                             (dy - rec.height / 2.0f) * (dy - rec.height / 2.0f);

    return (cornerDistanceSq <= (radius * radius));
}

// Get collision rectangle for two rectangles collision
R_rec R_get_collision_rec(R_rec rec1, R_rec rec2) {
    R_rec retRec = {0, 0, 0, 0};

    if (R_check_collision_recs(rec1, rec2)) {
        float dxx = (float) fabs(rec1.x - rec2.x);
        float dyy = (float) fabs(rec1.y - rec2.y);

        if (rec1.x <= rec2.x) {
            if (rec1.y <= rec2.y) {
                retRec.x = rec2.x;
                retRec.y = rec2.y;
                retRec.width = rec1.width - dxx;
                retRec.height = rec1.height - dyy;
            } else {
                retRec.x = rec2.x;
                retRec.y = rec1.y;
                retRec.width = rec1.width - dxx;
                retRec.height = rec2.height - dyy;
            }
        } else {
            if (rec1.y <= rec2.y) {
                retRec.x = rec1.x;
                retRec.y = rec2.y;
                retRec.width = rec2.width - dxx;
                retRec.height = rec1.height - dyy;
            } else {
                retRec.x = rec1.x;
                retRec.y = rec1.y;
                retRec.width = rec2.width - dxx;
                retRec.height = rec2.height - dyy;
            }
        }

        if (rec1.width > rec2.width) {
            if (retRec.width >= rec2.width) retRec.width = rec2.width;
        } else {
            if (retRec.width >= rec1.width) retRec.width = rec1.width;
        }

        if (rec1.height > rec2.height) {
            if (retRec.height >= rec2.height) retRec.height = rec2.height;
        } else {
            if (retRec.height >= rec1.height) retRec.height = rec1.height;
        }
    }

    return retRec;
}

// Detect collision between two spheres
R_public R_bool R_check_collision_spheres(R_vec3 center_a, float radius_a, R_vec3 center_b,
                                          float radius_b) {
    R_bool collision = 0;

    // Simple way to check for collision, just checking distance between two points
    // Unfortunately, R_sqrtf() is a costly operation, so we avoid it with following solution
    /*
float dx = centerA.x - centerB.x;      // X distance between centers
float dy = centerA.y - centerB.y;      // Y distance between centers
float dz = centerA.z - centerB.z;      // Z distance between centers

float distance = R_sqrtf(dx*dx + dy*dy + dz*dz);  // Distance between centers

if (distance <= (radiusA + radiusB)) collision = 1;
*/
    // Check for distances squared to avoid R_sqrtf()
    if (R_vec3_dot_product(R_vec3_sub(center_b, center_a), R_vec3_sub(center_b, center_a)) <=
        (radius_a + radius_b) * (radius_a + radius_b))
        collision = 1;

    return collision;
}

// Detect collision between two boxes. Note: Boxes are defined by two points minimum and maximum
R_public R_bool R_check_collision_boxes(R_bounding_box box1, R_bounding_box box2) {
    R_bool collision = 1;

    if ((box1.max.x >= box2.min.x) && (box1.min.x <= box2.max.x)) {
        if ((box1.max.y < box2.min.y) || (box1.min.y > box2.max.y)) collision = 0;
        if ((box1.max.z < box2.min.z) || (box1.min.z > box2.max.z)) collision = 0;
    } else
        collision = 0;

    return collision;
}

// Detect collision between box and sphere
R_public R_bool R_check_collision_box_sphere(R_bounding_box box, R_vec3 center, float radius) {
    R_bool collision = 0;

    float dmin = 0;

    if (center.x < box.min.x) dmin += powf(center.x - box.min.x, 2);
    else if (center.x > box.max.x)
        dmin += powf(center.x - box.max.x, 2);

    if (center.y < box.min.y) dmin += powf(center.y - box.min.y, 2);
    else if (center.y > box.max.y)
        dmin += powf(center.y - box.max.y, 2);

    if (center.z < box.min.z) dmin += powf(center.z - box.min.z, 2);
    else if (center.z > box.max.z)
        dmin += powf(center.z - box.max.z, 2);

    if (dmin <= (radius * radius)) collision = 1;

    return collision;
}

// Detect collision between ray and sphere
R_public R_bool R_check_collision_ray_sphere(R_ray ray, R_vec3 center, float radius) {
    R_bool collision = 0;

    R_vec3 ray_sphere_pos = R_vec3_sub(center, ray.position);
    float distance = R_vec3_len(ray_sphere_pos);
    float vector = R_vec3_dot_product(ray_sphere_pos, ray.direction);
    float d = radius * radius - (distance * distance - vector * vector);

    if (d >= 0.0f) collision = 1;

    return collision;
}

// Detect collision between ray and sphere with extended parameters and collision point detection
R_public R_bool R_check_collision_ray_sphere_ex(R_ray ray, R_vec3 center, float radius,
                                                R_vec3 *collision_point) {
    R_bool collision = 0;

    R_vec3 ray_sphere_pos = R_vec3_sub(center, ray.position);
    float distance = R_vec3_len(ray_sphere_pos);
    float vector = R_vec3_dot_product(ray_sphere_pos, ray.direction);
    float d = radius * radius - (distance * distance - vector * vector);

    if (d >= 0.0f) collision = 1;

    // Check if ray origin is inside the sphere to calculate the correct collision point
    float collision_distance = 0;

    if (distance < radius) collision_distance = vector + sqrt(d);
    else
        collision_distance = vector - sqrt(d);

    // Calculate collision point
    R_vec3 c_point = R_vec3_add(ray.position, R_vec3_scale(ray.direction, collision_distance));

    collision_point->x = c_point.x;
    collision_point->y = c_point.y;
    collision_point->z = c_point.z;

    return collision;
}

// Detect collision between ray and bounding box
R_public R_bool R_check_collision_ray_box(R_ray ray, R_bounding_box box) {
    R_bool collision = 0;

    float t[8];
    t[0] = (box.min.x - ray.position.x) / ray.direction.x;
    t[1] = (box.max.x - ray.position.x) / ray.direction.x;
    t[2] = (box.min.y - ray.position.y) / ray.direction.y;
    t[3] = (box.max.y - ray.position.y) / ray.direction.y;
    t[4] = (box.min.z - ray.position.z) / ray.direction.z;
    t[5] = (box.max.z - ray.position.z) / ray.direction.z;
    t[6] = (float) fmax(fmax(fmin(t[0], t[1]), fmin(t[2], t[3])), fmin(t[4], t[5]));
    t[7] = (float) fmin(fmin(fmax(t[0], t[1]), fmax(t[2], t[3])), fmax(t[4], t[5]));

    collision = !(t[7] < 0 || t[6] > t[7]);

    return collision;
}

// Get collision info between ray and triangle. Note: Based on https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
R_public R_ray_hit_info R_collision_ray_triangle(R_ray ray, R_vec3 p1, R_vec3 p2, R_vec3 p3) {
    R_ray_hit_info result = {0};
    R_vec3 edge1 = {0};
    R_vec3 edge2 = {0};
    R_vec3 p = {0};
    R_vec3 q = {0};
    R_vec3 tv = {0};
    float det = 0;
    float inv_det = 0;
    float u = 0;
    float v = 0;
    float t = 0;
    double epsilon = 0.000001;// Just a small number

    // Find vectors for two edges sharing V1
    edge1 = R_vec3_sub(p2, p1);
    edge2 = R_vec3_sub(p3, p1);

    // Begin calculating determinant - also used to calculate u parameter
    p = R_vec3_cross_product(ray.direction, edge2);

    // If determinant is near zero, ray lies in plane of triangle or ray is parallel to plane of triangle
    det = R_vec3_dot_product(edge1, p);

    // Avoid culling!
    if ((det > -epsilon) && (det < epsilon)) return result;

    inv_det = 1.0f / det;

    // Calculate distance from V1 to ray origin
    tv = R_vec3_sub(ray.position, p1);

    // Calculate u parameter and test bound
    u = R_vec3_dot_product(tv, p) * inv_det;

    // The intersection lies outside of the triangle
    if ((u < 0.0f) || (u > 1.0f)) return result;

    // Prepare to test v parameter
    q = R_vec3_cross_product(tv, edge1);

    // Calculate V parameter and test bound
    v = R_vec3_dot_product(ray.direction, q) * inv_det;

    // The intersection lies outside of the triangle
    if ((v < 0.0f) || ((u + v) > 1.0f)) return result;

    t = R_vec3_dot_product(edge2, q) * inv_det;

    if (t > epsilon) {
        // R_ray hit, get hit point and normal
        result.hit = 1;
        result.distance = t;
        result.hit = 1;
        result.normal = R_vec3_normalize(R_vec3_cross_product(edge1, edge2));
        result.position = R_vec3_add(ray.position, R_vec3_scale(ray.direction, t));
    }

    return result;
}

// Get collision info between ray and ground plane (Y-normal plane)
R_public R_ray_hit_info R_collision_ray_ground(R_ray ray, float ground_height) {
    R_ray_hit_info result = {0};
    double epsilon = 0.000001;// Just a small number

    if (fabs(ray.direction.y) > epsilon) {
        float distance = (ray.position.y - ground_height) / -ray.direction.y;

        if (distance >= 0.0) {
            result.hit = 1;
            result.distance = distance;
            result.normal = (R_vec3){0.0, 1.0, 0.0};
            result.position = R_vec3_add(ray.position, R_vec3_scale(ray.direction, distance));
        }
    }

    return result;
}

#pragma endregion

#pragma region unicode
/*
   Returns next codepoint in a UTF8 encoded text, scanning until '\0' is found or the length is exhausted
   When a invalid UTF8 R_byte is encountered we exit as soon as possible and a '?'(0x3f) codepoint is returned
   Total number of bytes processed are returned as a parameter
   NOTE: the standard says U+FFFD should be returned in case of errors
   but that character is not supported by the default font in raylib
   TODO: optimize this code for speed!!
*/
R_public R_decoded_rune R_decode_utf8_char(const char *src, R_int size) {
    /*
    UTF8 specs from https://www.ietf.org/rfc/rfc3629.txt
    Char. number range  |        UTF-8 byte sequence
      (hexadecimal)     |              (binary)
    --------------------+---------------------------------------------
    0000 0000-0000 007F | 0xxxxxxx
    0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    */

    if (size < 1) { return (R_decoded_rune){RF_INVALID_CODEPOINT}; }

    // The first UTF8 byte
    const int byte = (unsigned char) (src[0]);

    if (byte <= 0x7f) {
        // Only one byte (ASCII range x00-7F)
        const int code = src[0];

        // Codepoints after U+10ffff are invalid
        const int valid = code > 0x10ffff;

        return (R_decoded_rune){valid ? RF_INVALID_CODEPOINT : code, .bytes_processed = 1,
                                .valid = valid};
    } else if ((byte & 0xe0) == 0xc0) {
        if (size < 2) {
            return (R_decoded_rune){
                    RF_INVALID_CODEPOINT,
                    .bytes_processed = 1,
            };
        }

        // Two bytes
        // [0]xC2-DF    [1]UTF8-tail(x80-BF)
        const unsigned char byte1 = src[1];

        // Check for unexpected sequence
        if ((byte1 == '\0') || ((byte1 >> 6) != 2)) {
            return (R_decoded_rune){RF_INVALID_CODEPOINT, .bytes_processed = 2};
        }

        if ((byte >= 0xc2) && (byte <= 0xdf)) {
            const int code = ((byte & 0x1f) << 6) | (byte1 & 0x3f);

            // Codepoints after U+10ffff are invalid
            const int valid = code > 0x10ffff;

            return (R_decoded_rune){valid ? RF_INVALID_CODEPOINT : code, .bytes_processed = 2,
                                    .valid = valid};
        }
    } else if ((byte & 0xf0) == 0xe0) {
        if (size < 2) { return (R_decoded_rune){RF_INVALID_CODEPOINT, .bytes_processed = 1}; }

        // Three bytes
        const unsigned char byte1 = src[1];

        // Check for unexpected sequence
        if ((byte1 == '\0') || (size < 3) || ((byte1 >> 6) != 2)) {
            return (R_decoded_rune){RF_INVALID_CODEPOINT, .bytes_processed = 2};
        }

        const unsigned char byte2 = src[2];

        // Check for unexpected sequence
        if ((byte2 == '\0') || ((byte2 >> 6) != 2)) {
            return (R_decoded_rune){RF_INVALID_CODEPOINT, .bytes_processed = 3};
        }

        /*
            [0]xE0    [1]xA0-BF       [2]UTF8-tail(x80-BF)
            [0]xE1-EC [1]UTF8-tail    [2]UTF8-tail(x80-BF)
            [0]xED    [1]x80-9F       [2]UTF8-tail(x80-BF)
            [0]xEE-EF [1]UTF8-tail    [2]UTF8-tail(x80-BF)
        */
        if (((byte == 0xe0) && !((byte1 >= 0xa0) && (byte1 <= 0xbf))) ||
            ((byte == 0xed) && !((byte1 >= 0x80) && (byte1 <= 0x9f)))) {
            return (R_decoded_rune){RF_INVALID_CODEPOINT, .bytes_processed = 2};
        }

        if ((byte >= 0xe0) && (byte <= 0xef)) {
            const int code = ((byte & 0xf) << 12) | ((byte1 & 0x3f) << 6) | (byte2 & 0x3f);

            // Codepoints after U+10ffff are invalid
            const int valid = code > 0x10ffff;
            return (R_decoded_rune){valid ? RF_INVALID_CODEPOINT : code, .bytes_processed = 3,
                                    .valid = valid};
        }
    } else if ((byte & 0xf8) == 0xf0) {
        // Four bytes
        if (byte > 0xf4 || size < 2) {
            return (R_decoded_rune){RF_INVALID_CODEPOINT, .bytes_processed = 1};
        }

        const unsigned char byte1 = src[1];

        // Check for unexpected sequence
        if ((byte1 == '\0') || (size < 3) || ((byte1 >> 6) != 2)) {
            return (R_decoded_rune){RF_INVALID_CODEPOINT, .bytes_processed = 2};
        }

        const unsigned char byte2 = src[2];

        // Check for unexpected sequence
        if ((byte2 == '\0') || (size < 4) || ((byte2 >> 6) != 2)) {
            return (R_decoded_rune){RF_INVALID_CODEPOINT, .bytes_processed = 3};
        }

        const unsigned char byte3 = src[3];

        // Check for unexpected sequence
        if ((byte3 == '\0') || ((byte3 >> 6) != 2)) {
            return (R_decoded_rune){RF_INVALID_CODEPOINT, .bytes_processed = 4};
        }

        /*
            [0]xF0       [1]x90-BF       [2]UTF8-tail  [3]UTF8-tail
            [0]xF1-F3    [1]UTF8-tail    [2]UTF8-tail  [3]UTF8-tail
            [0]xF4       [1]x80-8F       [2]UTF8-tail  [3]UTF8-tail
        */

        // Check for unexpected sequence
        if (((byte == 0xf0) && !((byte1 >= 0x90) && (byte1 <= 0xbf))) ||
            ((byte == 0xf4) && !((byte1 >= 0x80) && (byte1 <= 0x8f)))) {
            return (R_decoded_rune){RF_INVALID_CODEPOINT, .bytes_processed = 2};
        }

        if (byte >= 0xf0) {
            const int code = ((byte & 0x7) << 18) | ((byte1 & 0x3f) << 12) | ((byte2 & 0x3f) << 6) |
                             (byte3 & 0x3f);

            // Codepoints after U+10ffff are invalid
            const int valid = code > 0x10ffff;
            return (R_decoded_rune){valid ? RF_INVALID_CODEPOINT : code, .bytes_processed = 4,
                                    .valid = valid};
        }
    }

    return (R_decoded_rune){.codepoint = RF_INVALID_CODEPOINT, .bytes_processed = 1};
}

R_public R_utf8_stats R_count_utf8_chars(const char *src, R_int size) {
    R_utf8_stats result = {0};

    if (src && size > 0) {
        while (size > 0) {
            R_decoded_rune decoded_rune = R_decode_utf8_char(src, size);

            src += decoded_rune.bytes_processed;
            size -= decoded_rune.bytes_processed;

            result.bytes_processed += decoded_rune.bytes_processed;
            result.invalid_bytes += decoded_rune.valid ? 0 : decoded_rune.bytes_processed;
            result.valid_rune_count += decoded_rune.valid ? 1 : 0;
            result.total_rune_count += 1;
        }
    }

    return result;
}

R_public R_utf8_stats R_count_utf8_chars_til(const char *src, R_int size, R_int n) {
    R_utf8_stats result = {0};

    if (src && size > 0) {
        while (size > 0 && n > 0) {
            R_decoded_rune decoded_rune = R_decode_utf8_char(src, size);

            src += decoded_rune.bytes_processed;
            size -= decoded_rune.bytes_processed;
            n -= 1;

            result.bytes_processed += decoded_rune.bytes_processed;
            result.invalid_bytes += decoded_rune.valid ? 0 : decoded_rune.bytes_processed;
            result.valid_rune_count += decoded_rune.valid ? 1 : 0;
            result.total_rune_count += 1;
        }
    }

    return result;
}

R_public R_decoded_string R_decode_utf8_to_buffer(const char *src, R_int size, R_rune *dst,
                                                  R_int dst_size) {
    R_decoded_string result = {0};

    result.codepoints = dst;

    if (src && size > 0 && dst && dst_size > 0) {
        int dst_i = 0;
        int invalid_bytes = 0;

        while (size > 0 && dst_i < dst_size) {
            R_decoded_rune decoding_result = R_decode_utf8_char(src, size);

            // Count the invalid bytes
            if (!decoding_result.valid) { invalid_bytes += decoding_result.bytes_processed; }

            src += decoding_result.bytes_processed;
            size -= decoding_result.bytes_processed;

            dst[dst_i++] = decoding_result.codepoint;
        }

        result.size = dst_i;
        result.valid = 1;
        result.invalid_bytes_count = invalid_bytes;
    }

    return result;
}

R_public R_decoded_string R_decode_utf8(const char *src, R_int size, R_allocator allocator) {
    R_decoded_string result = {0};

    R_rune *dst = R_alloc(allocator, sizeof(R_rune) * size);

    result = R_decode_utf8_to_buffer(src, size, dst, size);

    return result;
}
#pragma endregion

#pragma region ascii
R_int R_str_to_int(R_str src) {
    R_int result = 0;
    R_int sign = 1;

    src = R_str_eat_spaces(src);

    if (R_str_valid(src) && src.data[0] == '-') {
        sign = -1;
        src = R_str_advance_b(src, 1);
    }

    while (R_str_valid(src) && R_is_digit(src.data[0])) {
        result *= 10;
        result += R_to_digit(src.data[0]);
        src = R_str_advance_b(src, 1);
    }

    result *= sign;

    return result;
}

float R_str_to_float(R_str src);

int R_to_digit(char c) {
    int result = c - '0';
    return result;
}

char R_to_upper(char c) {
    char result = c;
    if (R_is_upper(c)) { result = c + 'A' - 'a'; }
    return result;
}

char R_to_lower(char c) {
    char result = c;
    if (R_is_lower(c)) { result = c + 'a' - 'A'; }
    return result;
}

R_bool R_is_ascii(char c) {
    R_bool result = 0;
    return result;
}

R_bool R_is_lower(char c) {
    R_bool result = c >= 'a' && c <= 'z';
    return result;
}

R_bool R_is_upper(char c) {
    R_bool result = c >= 'A' && c <= 'Z';
    return result;
}

R_bool R_is_alpha(char c) {
    R_bool result = R_is_lower(c) || R_is_upper(c);
    return result;
}

R_bool R_is_digit(char c) {
    R_bool result = c >= '0' && c <= '9';
    return result;
}

R_bool R_is_alnum(char c) {
    R_bool result = R_is_alpha(c) && R_is_alnum(c);
    return result;
}

R_bool R_is_space(char c) {
    R_bool result = c == ' ' || c == '\t';
    return result;
}
#pragma endregion

#pragma region strbuf
R_public R_strbuf R_strbuf_make_ex(R_int initial_amount, R_allocator allocator) {
    R_strbuf result = {0};

    void *data = R_alloc(allocator, initial_amount);

    if (data) {
        result.data = data;
        result.capacity = initial_amount;
        result.allocator = allocator;
        result.valid = 1;
    }

    return result;
}

R_public R_strbuf R_strbuf_clone_ex(R_strbuf this_buf, R_allocator allocator) {
    R_strbuf result = {0};

    if (this_buf.valid) {
        result = R_strbuf_make_ex(this_buf.capacity, allocator);
        R_strbuf_append(&result, R_strbuf_to_str(this_buf));
    }

    return result;
}

R_public R_str R_strbuf_to_str(R_strbuf src) {
    R_str result = {0};

    if (src.valid) {
        result.data = src.data;
        result.size = src.size;
    }

    return result;
}

R_public R_int R_strbuf_remaining_capacity(const R_strbuf *this_buf) {
    R_int result = this_buf->capacity - this_buf->size;
    return result;
}

R_public void R_strbuf_reserve(R_strbuf *this_buf, R_int new_capacity) {
    if (new_capacity > this_buf->capacity) {
        char *new_buf =
                R_realloc(this_buf->allocator, this_buf->data, new_capacity, this_buf->capacity);
        if (new_buf) {
            this_buf->data = new_buf;
            this_buf->valid = 1;
        } else {
            this_buf->valid = 0;
        }
    }
}

R_public void R_strbuf_ensure_capacity_for(R_strbuf *this_buf, R_int size) {
    R_int remaining_capacity = R_strbuf_remaining_capacity(this_buf);
    if (remaining_capacity < size) {
        // We either increase the buffer to capacity * 2 or to the necessary size to fit the size plus one for the null terminator
        R_int amount_to_reserve = R_max_i(this_buf->capacity * 2,
                                          this_buf->capacity + (size - remaining_capacity) + 1);
        R_strbuf_reserve(this_buf, amount_to_reserve);
    }
}

R_public void R_strbuf_append(R_strbuf *this_buf, R_str it) {
    R_strbuf_ensure_capacity_for(this_buf, it.size);

    memcpy(this_buf->data + this_buf->size, it.data, it.size);
    this_buf->size += it.size;
    this_buf->data[this_buf->size] = 0;
}

R_public void R_strbuf_prepend(R_strbuf *this_buf, R_str it) {
    R_strbuf_ensure_capacity_for(this_buf, it.size);

    memmove(this_buf->data + it.size, this_buf->data, this_buf->size);
    memcpy(this_buf->data, it.data, it.size);
    this_buf->size += it.size;
    this_buf->data[this_buf->size] = 0;
}

R_public void R_strbuf_insert_utf8(R_strbuf *this_buf, R_str str_to_insert, R_int insert_at) {
    R_strbuf_ensure_capacity_for(this_buf, str_to_insert.size);

    // Iterate over utf8 until we find the byte to insert at
    R_int insertion_point =
            R_count_utf8_chars_til(this_buf->data, this_buf->size, insert_at).bytes_processed;

    if (insertion_point && insertion_point < this_buf->size) {
        // Move all bytes from the insertion point ahead by the size of the string we need to insert
        {
            char *dst = this_buf->data + insertion_point + str_to_insert.size;
            char *src = this_buf->data + insertion_point;
            R_int src_size = this_buf->size - insertion_point;
            memmove(dst, src, src_size);
        }

        // Copy the string to insert
        {
            char *dst = this_buf->data + insertion_point;
            memmove(dst, str_to_insert.data, str_to_insert.size);
        }

        this_buf->size += str_to_insert.size;
        this_buf->data[this_buf->size] = 0;
    }
}

R_public void R_strbuf_insert_b(R_strbuf *this_buf, R_str str_to_insert, R_int insert_at) {
    if (R_str_valid(str_to_insert) && insert_at > 0) {
        R_strbuf_ensure_capacity_for(this_buf, str_to_insert.size);

        // Move all bytes from the insertion point ahead by the size of the string we need to insert
        {
            char *dst = this_buf->data + insert_at + str_to_insert.size;
            char *src = this_buf->data + insert_at;
            R_int src_size = this_buf->size - insert_at;
            memmove(dst, src, src_size);
        }

        // Copy the string to insert
        {
            char *dst = this_buf->data + insert_at;
            memcpy(dst, str_to_insert.data, str_to_insert.size);
        }

        this_buf->size += str_to_insert.size;
        this_buf->data[this_buf->size] = 0;
    }
}

R_public void R_strbuf_remove_range(R_strbuf *, R_int begin, R_int end);

R_public void R_strbuf_remove_range_b(R_strbuf *, R_int begin, R_int end);

R_public void R_strbuf_free(R_strbuf *this_buf) {
    R_free(this_buf->allocator, this_buf->data);

    this_buf->size = 0;
    this_buf->capacity = 0;
}
#pragma endregion

#pragma region str
R_public R_bool R_str_valid(R_str src) { return src.size != 0 && src.data; }

R_public R_int R_str_len(R_str src) {
    R_utf8_stats stats = R_count_utf8_chars(src.data, src.size);

    return stats.total_rune_count;
}

R_public R_str R_cstr(const char *src) {
    R_int size = strlen(src);
    R_str result = {
            .data = (char *) src,
            .size = size,
    };

    return result;
}

R_public R_str R_str_advance_b(R_str src, R_int amount) {
    R_str result = {0};
    result = R_str_sub_b(src, amount, src.size);
    return result;
}

R_public R_str R_str_eat_spaces(R_str src) {
    while (R_str_valid(src) && R_is_space(src.data[0])) { src = R_str_advance_b(src, 1); }

    return src;
}

R_public R_rune R_str_get_rune(R_str src, R_int n) {
    R_str target = R_str_sub_utf8(src, n, 0);
    R_rune result = R_decode_utf8_char(target.data, target.size).codepoint;
    return result;
}

R_public R_utf8_char R_str_get_utf8_n(R_str src, R_int n) {
    R_utf8_char result = 0;
    R_str target = R_str_sub_utf8(src, n, 0);
    R_utf8_stats stats = R_count_utf8_chars_til(target.data, target.size, 1);
    if (stats.bytes_processed > 0 && stats.bytes_processed < 4) {
        memcpy(&result, src.data, stats.bytes_processed);
    }
    return result;
}

R_public R_str R_str_sub_utf8(R_str src, R_int begin, R_int end) {
    R_str result = {0};

    if (begin < 0) { begin = src.size + begin; }

    if (end <= 0) { end = src.size + end; }

    if (R_str_valid(src) && begin > 0 && begin < end && end <= src.size) {
        R_utf8_stats stats = {0};

        // Find the begin utf8 position
        stats = R_count_utf8_chars_til(src.data, src.size, begin);
        src.data += stats.bytes_processed;
        src.size -= stats.bytes_processed;

        // Find the end utf8 position
        stats = R_count_utf8_chars_til(src.data, src.size, end - begin);

        result.data = src.data;
        result.size = stats.bytes_processed;
    }

    return result;
}

R_public R_str R_str_sub_b(R_str src, R_int begin, R_int end) {
    R_str result = {0};

    if (begin < 0) { begin = src.size + begin; }

    if (end <= 0) { end = src.size + end; }

    if (R_str_valid(src) && begin > 0 && begin < end && end <= src.size) {
        result.data = src.data + begin;
        result.size = end - begin;
    }

    return result;
}

R_public int R_str_cmp(R_str a, R_str b) {
    int result = memcmp(a.data, b.data, R_min_i(a.size, b.size));
    return result;
}

R_public R_bool R_str_match(R_str a, R_str b) {
    if (a.size != b.size) return 0;
    int cmp = memcmp(a.data, b.data, a.size);
    return cmp == 0;
}

R_public R_bool R_str_match_prefix(R_str str, R_str prefix) {
    if (str.size < prefix.size) return 0;
    int cmp = memcmp(str.data, prefix.data, prefix.size);
    return cmp == 0;
}

R_public R_bool R_str_match_suffix(R_str str, R_str suffix) {
    if (str.size < suffix.size) return 0;
    R_int offset = str.size - suffix.size;
    int cmp = memcmp(str.data + offset, suffix.data, suffix.size);
    return cmp == 0;
}

R_public R_int R_str_find_first(R_str haystack, R_str needle) {
    R_int result = R_invalid_index;
    if (needle.size <= haystack.size) {
        R_int char_ct = 0;
        for (R_str sub = haystack; sub.size >= needle.size; sub = R_str_sub_b(sub, 1, 0)) {
            if (R_str_match_prefix(sub, needle)) {
                result = char_ct;
                break;
            } else {
                char_ct++;
            }
        }
    }
    return result;
}

R_public R_int R_str_find_last(R_str haystack, R_str needle) {
    R_int result = R_invalid_index;
    if (needle.size <= haystack.size) {
        R_int char_ct = haystack.size - 1;
        for (R_str sub = haystack; R_str_valid(sub); sub = R_str_sub_b(sub, 0, -1)) {
            if (R_str_match_suffix(sub, needle)) {
                result = char_ct;
                break;
            } else {
                char_ct--;
            }
        }
    }
    return result;
}

R_public R_bool R_str_contains(R_str haystack, R_str needle) {
    R_bool result = R_str_find_first(haystack, needle) != R_invalid_index;
    return result;
}

R_public R_str R_str_pop_first_split(R_str *src, R_str split_by) {
    R_str result = {0};

    R_int i = R_str_find_first(*src, split_by);
    if (i != R_invalid_index) {
        result.data = src->data;
        result.size = i;
        src->data += i + split_by.size;
        src->size -= i + split_by.size;
    } else {
        result = *src;
        *src = (R_str){0};
    }

    return result;
}

R_public R_str R_str_pop_last_split(R_str *src, R_str split_by) {
    R_str result = {0};

    R_int i = R_str_find_last(*src, split_by);
    if (i != R_invalid_index) {
        result.data = src->data + i;
        result.size = src->size - i;
        src->size -= src->size - i - split_by.size;
    } else {
        result = *src;
        *src = (R_str){0};
    }

    return result;
}

R_public R_utf8_char R_rune_to_utf8_char(R_rune src) {
    R_utf8_char result = 0;
    memcpy(&result, &src, sizeof(R_rune));
    return result;
}

R_public R_rune R_utf8_char_to_rune(R_utf8_char src) {
    R_rune result = 0;
    int len = strlen((char *) &src);
    R_decoded_rune r = R_decode_utf8_char((char *) &src, len);
    result = r.codepoint;
    return result;
}
#pragma endregion