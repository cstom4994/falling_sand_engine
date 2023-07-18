// Copyright(c) 2023, KaoruXun All rights reserved.

#ifndef ME_MATH_BASE_HPP
#define ME_MATH_BASE_HPP

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#include <cassert>
#include <list>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

#include "engine/core/basic_types.h"
#include "engine/core/macros.hpp"

// if you wish NOT to use SSE3 SIMD intrinsics, simply change the
// #define to 0
#define MATH_USE_SSE 1
#if MATH_USE_SSE
#include <pmmintrin.h>
#include <xmmintrin.h>
#endif

#ifdef ME_PLATFORM_WIN32
#undef far
#undef near
#endif

//----------------------------------------------------------------------//
// STRUCT DEFINITIONS:

// a 2-dimensional vector of floats
typedef union MEvec2 {
    float v[2];
    struct {
        float x, y;
    };
    struct {
        float w, h;
    };

    MEvec2(){};
    MEvec2(float _c) { x = _c, y = _c; };
    MEvec2(float _x, float _y) { x = _x, y = _y; };

    inline float &operator[](size_t i) { return v[i]; };
} MEvec2;

// a 3-dimensional vector of floats
typedef union MEvec3 {
    float v[3];
    struct {
        float x, y, z;
    };
    struct {
        float w, h, d;
    };
    struct {
        float r, g, b;
    };

    MEvec3(){};
    MEvec3(float _x, float _y, float _z) { x = _x, y = _y, z = _z; };
    MEvec3(float _c) { x = _c, y = _c, z = _c; };
    MEvec3(MEvec2 _xy, float _z) { x = _xy.x, y = _xy.y, z = _z; };
    MEvec3(float _x, MEvec3 _yz) { x = _x, y = _yz.x, z = _yz.y; };

    inline float &operator[](size_t i) { return v[i]; };
} MEvec3;

// a 4-dimensional vector of floats
typedef union MEvec4 {
    float v[4];
    struct {
        float x, y, z, w;
    };
    struct {
        float r, g, b, a;
    };

#if MATH_USE_SSE

    __m128 packed;

#endif

    MEvec4(){};
    MEvec4(float _x, float _y, float _z, float _w) { x = _x, y = _y, z = _z, w = _w; };
    MEvec4(float _c) { x = _c, y = _c, z = _c, w = _c; };
    MEvec4(MEvec3 _xyz, float _w) { x = _xyz.x, y = _xyz.y, z = _xyz.z, w = _w; };
    MEvec4(float _x, MEvec3 _yzw) { x = _x, y = _yzw.x, z = _yzw.y, w = _yzw.z; };
    MEvec4(MEvec2 _xy, MEvec2 _zw) { x = _xy.x, y = _xy.y, z = _zw.x, w = _zw.y; };

    inline float &operator[](size_t i) { return v[i]; };
} MEvec4;

//-----------------------------//
// matrices are column-major

typedef union MEmat3 {
    float m[3][3];
    MEvec3 v[3];
    MEmat3(){};
    MEmat3(float _a11, float _a12, float _a13, float _a21, float _a22, float _a23, float _a31, float _a32, float _a33) {
        m[0][0] = _a11;
        m[0][1] = _a12;
        m[0][2] = _a13;
        m[1][0] = _a21;
        m[1][1] = _a22;
        m[1][2] = _a23;
        m[2][0] = _a31;
        m[2][1] = _a32;
        m[2][2] = _a33;
    };
    inline MEvec3 &operator[](size_t i) { return v[i]; };
} MEmat3;

typedef union MEmat4 {
    float m[4][4];

#if MATH_USE_SSE

    __m128 packed[4];  // array of columns

#endif

    MEvec4 v[4];
    MEmat4(){};
    MEmat4(float _c) {
        m[0][0] = _c;
        m[0][1] = _c;
        m[0][2] = _c;
        m[0][3] = _c;
        m[1][0] = _c;
        m[1][1] = _c;
        m[1][2] = _c;
        m[1][3] = _c;
        m[2][0] = _c;
        m[2][1] = _c;
        m[2][2] = _c;
        m[2][3] = _c;
        m[3][0] = _c;
        m[3][1] = _c;
        m[3][2] = _c;
        m[3][3] = _c;
    };
    MEmat4(float _a11, float _a12, float _a13, float _a14, float _a21, float _a22, float _a23, float _a24, float _a31, float _a32, float _a33, float _a34, float _a41, float _a42, float _a43,
           float _a44) {
        m[0][0] = _a11;
        m[0][1] = _a12;
        m[0][2] = _a13;
        m[0][3] = _a14;
        m[1][0] = _a21;
        m[1][1] = _a22;
        m[1][2] = _a23;
        m[1][3] = _a24;
        m[2][0] = _a31;
        m[2][1] = _a32;
        m[2][2] = _a33;
        m[2][3] = _a34;
        m[3][0] = _a41;
        m[3][1] = _a42;
        m[3][2] = _a43;
        m[3][3] = _a44;
    };
    inline MEvec4 &operator[](size_t i) { return v[i]; };
} MEmat4;

//-----------------------------//

typedef union MEquaternion {
    float q[4];
    struct {
        float x, y, z, w;
    };

#if MATH_USE_SSE

    __m128 packed;

#endif

    MEquaternion(){};
    MEquaternion(float _x, float _y, float _z, float _w) { x = _x, y = _y, z = _z, w = _w; };
    MEquaternion(float _c) { x = _c, y = _c, z = _c, w = _c; };
    MEquaternion(MEvec3 _xyz, float _w) { x = _xyz.x, y = _xyz.y, z = _xyz.z, w = _w; };
    MEquaternion(float _x, MEvec3 _yzw) { x = _x, y = _yzw.x, z = _yzw.y, w = _yzw.z; };
    MEquaternion(MEvec2 _xy, MEvec2 _zw) { x = _xy.x, y = _xy.y, z = _zw.x, w = _zw.y; };

    inline float operator[](size_t i) { return q[i]; };
} MEquaternion;

// typedefs:
typedef MEvec2 MEvec2;
typedef MEvec3 MEvec3;
typedef MEvec4 MEvec4;
typedef MEmat3 MEmat3;
typedef MEmat4 MEmat4;
typedef MEquaternion MEquaternion;

typedef struct MErect {
    float x, y;
    float w, h;
} ME_rect;

typedef signed int ME_bool;

#define VECTOR3_ZERO \
    MEvec3 { 0.0f, 0.0f, 0.0f }
#define VECTOR3_FORWARD \
    MEvec3 { 1.0f, 0.0f, 0.0f }
#define VECTOR3_UP \
    MEvec3 { 0.0f, 0.0f, 1.0f }
#define VECTOR3_DOWN \
    MEvec3 { 0.0f, 0.0f, -1.0f }
#define VECTOR3_LEFT \
    MEvec3 { 0.0f, 1.0f, 0.0f }

#define INT_INFINITY 0x3f3f3f3f

// #define sign(x) (x > 0 ? 1 : (x < 0 ? -1 : 0))
#define UTIL_clamp(x, m, M) (x < m ? m : (x > M ? M : x))
#define FRAC0(x) (x - floorf(x))
#define FRAC1(x) (1 - x + floorf(x))

#define UTIL_cross(u, v) \
    (MEvec3) { (u).y *(v).z - (u).z *(v).y, (u).z *(v).x - (u).x *(v).z, (u).x *(v).y - (u).y *(v).x }
#define UTIL_dot(u, v) ((u).x * (v).x + (u).y * (v).y + (u).z * (v).z)
// #define norm(v) sqrt(dot(v, v))// norm = length of  vector

template <class T>
constexpr T pi = T(3.1415926535897932385L);

#define MATH_INLINE static inline

// if you wish to set your own function prefix or remove it entirely,
// simply change this macro:
#define MATH_PREFIX(name) ME_##name

// if you wish to not use any of the CRT functions, you must #define your
// own versions of the below functions and #include the appropriate header
#include <math.h>

#define MATH_SQRTF sqrtf
#define MATH_SINF sinf
#define MATH_COSF cosf
#define MATH_TANF tanf
#define MATH_ACOSF acosf

static const double PI = 3.14159265358979323846;
static const f32 FloatEpsilon = FLT_EPSILON;
static const f32 HalfPI = PI * 0.5f;
static const f32 TAU = PI * 2.0f;

template <typename T>
static inline f32 degreesToRadians(const T degrees) {
    return (static_cast<f32>(degrees) * PI / 180.0f);
}

template <typename T, int Size>
static inline int arrayLength(const T (&)[Size]) {
    return Size;
}

//----------------------------------------------------------------------//
// HELPER FUNCS:

#define MATH_MIN(x, y) ((x) < (y) ? (x) : (y))
#define MATH_MAX(x, y) ((x) > (y) ? (x) : (y))
#define MATH_ABS(x) ((x) > 0 ? (x) : -(x))

MATH_INLINE float MATH_PREFIX(rad_to_deg)(float rad) { return rad * 57.2957795131f; }

MATH_INLINE float MATH_PREFIX(deg_to_rad)(float deg) { return deg * 0.01745329251f; }

#if MATH_USE_SSE

MATH_INLINE __m128 MATH_PREFIX(mat4_mult_column_sse)(__m128 c1, MEmat4 m2) {
    __m128 result;

    result = _mm_mul_ps(_mm_shuffle_ps(c1, c1, _MM_SHUFFLE(0, 0, 0, 0)), m2.packed[0]);
    result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(c1, c1, _MM_SHUFFLE(1, 1, 1, 1)), m2.packed[1]));
    result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(c1, c1, _MM_SHUFFLE(2, 2, 2, 2)), m2.packed[2]));
    result = _mm_add_ps(result, _mm_mul_ps(_mm_shuffle_ps(c1, c1, _MM_SHUFFLE(3, 3, 3, 3)), m2.packed[3]));

    return result;
}

#endif

//----------------------------------------------------------------------//
// VECTOR FUNCTIONS:

// addition:

MATH_INLINE MEvec2 MATH_PREFIX(vec2_add)(MEvec2 v1, MEvec2 v2) {
    MEvec2 result;

    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;

    return result;
}

MATH_INLINE MEvec3 MATH_PREFIX(vec3_add)(MEvec3 v1, MEvec3 v2) {
    MEvec3 result;

    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;

    return result;
}

MATH_INLINE MEvec4 MATH_PREFIX(vec4_add)(MEvec4 v1, MEvec4 v2) {
    MEvec4 result;

#if MATH_USE_SSE

    result.packed = _mm_add_ps(v1.packed, v2.packed);

#else

    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;
    result.w = v1.w + v2.w;

#endif

    return result;
}

// subtraction:

MATH_INLINE MEvec2 MATH_PREFIX(vec2_sub)(MEvec2 v1, MEvec2 v2) {
    MEvec2 result;

    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;

    return result;
}

MATH_INLINE MEvec3 MATH_PREFIX(vec3_sub)(MEvec3 v1, MEvec3 v2) {
    MEvec3 result;

    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;

    return result;
}

MATH_INLINE MEvec4 MATH_PREFIX(vec4_sub)(MEvec4 v1, MEvec4 v2) {
    MEvec4 result;

#if MATH_USE_SSE

    result.packed = _mm_sub_ps(v1.packed, v2.packed);

#else

    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    result.w = v1.w - v2.w;

#endif

    return result;
}

// multiplication:

MATH_INLINE MEvec2 MATH_PREFIX(vec2_mult)(MEvec2 v1, MEvec2 v2) {
    MEvec2 result;

    result.x = v1.x * v2.x;
    result.y = v1.y * v2.y;

    return result;
}

MATH_INLINE MEvec3 MATH_PREFIX(vec3_mult)(MEvec3 v1, MEvec3 v2) {
    MEvec3 result;

    result.x = v1.x * v2.x;
    result.y = v1.y * v2.y;
    result.z = v1.z * v2.z;

    return result;
}

MATH_INLINE MEvec4 MATH_PREFIX(vec4_mult)(MEvec4 v1, MEvec4 v2) {
    MEvec4 result;

#if MATH_USE_SSE

    result.packed = _mm_mul_ps(v1.packed, v2.packed);

#else

    result.x = v1.x * v2.x;
    result.y = v1.y * v2.y;
    result.z = v1.z * v2.z;
    result.w = v1.w * v2.w;

#endif

    return result;
}

// division:

MATH_INLINE MEvec2 MATH_PREFIX(vec2_div)(MEvec2 v1, MEvec2 v2) {
    MEvec2 result;

    result.x = v1.x / v2.x;
    result.y = v1.y / v2.y;

    return result;
}

MATH_INLINE MEvec3 MATH_PREFIX(vec3_div)(MEvec3 v1, MEvec3 v2) {
    MEvec3 result;

    result.x = v1.x / v2.x;
    result.y = v1.y / v2.y;
    result.z = v1.z / v2.z;

    return result;
}

MATH_INLINE MEvec4 MATH_PREFIX(vec4_div)(MEvec4 v1, MEvec4 v2) {
    MEvec4 result;

#if MATH_USE_SSE

    result.packed = _mm_div_ps(v1.packed, v2.packed);

#else

    result.x = v1.x / v2.x;
    result.y = v1.y / v2.y;
    result.z = v1.z / v2.z;
    result.w = v1.w / v2.w;

#endif

    return result;
}

// scalar multiplication:

MATH_INLINE MEvec2 MATH_PREFIX(vec2_scale)(MEvec2 v, float s) {
    MEvec2 result;

    result.x = v.x * s;
    result.y = v.y * s;

    return result;
}

MATH_INLINE MEvec3 MATH_PREFIX(vec3_scale)(MEvec3 v, float s) {
    MEvec3 result;

    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;

    return result;
}

MATH_INLINE MEvec4 MATH_PREFIX(vec4_scale)(MEvec4 v, float s) {
    MEvec4 result;

#if MATH_USE_SSE

    __m128 scale = _mm_set1_ps(s);
    result.packed = _mm_mul_ps(v.packed, scale);

#else

    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    result.w = v.w * s;

#endif

    return result;
}

// dot product:

MATH_INLINE float MATH_PREFIX(vec2_dot)(MEvec2 v1, MEvec2 v2) {
    float result;

    result = v1.x * v2.x + v1.y * v2.y;

    return result;
}

MATH_INLINE float MATH_PREFIX(vec3_dot)(MEvec3 v1, MEvec3 v2) {
    float result;

    result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;

    return result;
}

MATH_INLINE float MATH_PREFIX(vec4_dot)(MEvec4 v1, MEvec4 v2) {
    float result;

#if MATH_USE_SSE

    __m128 r = _mm_mul_ps(v1.packed, v2.packed);
    r = _mm_hadd_ps(r, r);
    r = _mm_hadd_ps(r, r);
    _mm_store_ss(&result, r);

#else

    result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;

#endif

    return result;
}

// cross product

MATH_INLINE MEvec3 MATH_PREFIX(vec3_cross)(MEvec3 v1, MEvec3 v2) {
    MEvec3 result;

    result.x = (v1.y * v2.z) - (v1.z * v2.y);
    result.y = (v1.z * v2.x) - (v1.x * v2.z);
    result.z = (v1.x * v2.y) - (v1.y * v2.x);

    return result;
}

// length:

MATH_INLINE float MATH_PREFIX(vec2_length)(MEvec2 v) {
    float result;

    result = MATH_SQRTF(MATH_PREFIX(vec2_dot)(v, v));

    return result;
}

MATH_INLINE float MATH_PREFIX(vec3_length)(MEvec3 v) {
    float result;

    result = MATH_SQRTF(MATH_PREFIX(vec3_dot)(v, v));

    return result;
}

MATH_INLINE float MATH_PREFIX(vec4_length)(MEvec4 v) {
    float result;

    result = MATH_SQRTF(MATH_PREFIX(vec4_dot)(v, v));

    return result;
}

// normalize:

MATH_INLINE MEvec2 MATH_PREFIX(vec2_normalize)(MEvec2 v) {
    MEvec2 result = {0};

    float invLen = MATH_PREFIX(vec2_length)(v);
    if (invLen != 0.0f) {
        invLen = 1.0f / invLen;
        result.x = v.x * invLen;
        result.y = v.y * invLen;
    }

    return result;
}

MATH_INLINE MEvec3 MATH_PREFIX(vec3_normalize)(MEvec3 v) {
    MEvec3 result = {0};

    float invLen = MATH_PREFIX(vec3_length)(v);
    if (invLen != 0.0f) {
        invLen = 1.0f / invLen;
        result.x = v.x * invLen;
        result.y = v.y * invLen;
        result.z = v.z * invLen;
    }

    return result;
}

MATH_INLINE MEvec4 MATH_PREFIX(vec4_normalize)(MEvec4 v) {
    MEvec4 result = {0};

    float len = MATH_PREFIX(vec4_length)(v);
    if (len != 0.0f) {
#if MATH_USE_SSE

        __m128 scale = _mm_set1_ps(len);
        result.packed = _mm_div_ps(v.packed, scale);

#else

        float invLen = 1.0f / len;

        result.x = v.x * invLen;
        result.y = v.y * invLen;
        result.z = v.z * invLen;
        result.w = v.w * invLen;

#endif
    }

    return result;
}

// distance:

MATH_INLINE float MATH_PREFIX(vec2_distance)(MEvec2 v1, MEvec2 v2) {
    float result;

    MEvec2 to = MATH_PREFIX(vec2_sub)(v1, v2);
    result = MATH_PREFIX(vec2_length)(to);

    return result;
}

MATH_INLINE float MATH_PREFIX(vec3_distance)(MEvec3 v1, MEvec3 v2) {
    float result;

    MEvec3 to = MATH_PREFIX(vec3_sub)(v1, v2);
    result = MATH_PREFIX(vec3_length)(to);

    return result;
}

MATH_INLINE float MATH_PREFIX(vec4_distance)(MEvec4 v1, MEvec4 v2) {
    float result;

    MEvec4 to = MATH_PREFIX(vec4_sub)(v1, v2);
    result = MATH_PREFIX(vec4_length)(to);

    return result;
}

// equality:

MATH_INLINE bool MATH_PREFIX(vec2_equals)(MEvec2 v1, MEvec2 v2) {
    bool result;

    result = (v1.x == v2.x) && (v1.y == v2.y);

    return result;
}

MATH_INLINE bool MATH_PREFIX(vec3_equals)(MEvec3 v1, MEvec3 v2) {
    bool result;

    result = (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z);

    return result;
}

MATH_INLINE bool MATH_PREFIX(vec4_equals)(MEvec4 v1, MEvec4 v2) {
    bool result;

    // TODO: there are SIMD instructions for floating point equality, find a way to get a single bool from them
    result = (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z) && (v1.w == v2.w);

    return result;
}

// min:

MATH_INLINE MEvec2 MATH_PREFIX(vec2_min)(MEvec2 v1, MEvec2 v2) {
    MEvec2 result;

    result.x = MATH_MIN(v1.x, v2.x);
    result.y = MATH_MIN(v1.y, v2.y);

    return result;
}

MATH_INLINE MEvec3 MATH_PREFIX(vec3_min)(MEvec3 v1, MEvec3 v2) {
    MEvec3 result;

    result.x = MATH_MIN(v1.x, v2.x);
    result.y = MATH_MIN(v1.y, v2.y);
    result.z = MATH_MIN(v1.z, v2.z);

    return result;
}

MATH_INLINE MEvec4 MATH_PREFIX(vec4_min)(MEvec4 v1, MEvec4 v2) {
    MEvec4 result;

#if MATH_USE_SSE

    result.packed = _mm_min_ps(v1.packed, v2.packed);

#else

    result.x = MATH_MIN(v1.x, v2.x);
    result.y = MATH_MIN(v1.y, v2.y);
    result.z = MATH_MIN(v1.z, v2.z);
    result.w = MATH_MIN(v1.w, v2.w);

#endif

    return result;
}

// max:

MATH_INLINE MEvec2 MATH_PREFIX(vec2_max)(MEvec2 v1, MEvec2 v2) {
    MEvec2 result;

    result.x = MATH_MAX(v1.x, v2.x);
    result.y = MATH_MAX(v1.y, v2.y);

    return result;
}

MATH_INLINE MEvec3 MATH_PREFIX(vec3_max)(MEvec3 v1, MEvec3 v2) {
    MEvec3 result;

    result.x = MATH_MAX(v1.x, v2.x);
    result.y = MATH_MAX(v1.y, v2.y);
    result.z = MATH_MAX(v1.z, v2.z);

    return result;
}

MATH_INLINE MEvec4 MATH_PREFIX(vec4_max)(MEvec4 v1, MEvec4 v2) {
    MEvec4 result;

#if MATH_USE_SSE

    result.packed = _mm_max_ps(v1.packed, v2.packed);

#else

    result.x = MATH_MAX(v1.x, v2.x);
    result.y = MATH_MAX(v1.y, v2.y);
    result.z = MATH_MAX(v1.z, v2.z);
    result.w = MATH_MAX(v1.w, v2.w);

#endif

    return result;
}

//----------------------------------------------------------------------//
// MATRIX FUNCTIONS:

// initialization:

MATH_INLINE MEmat3 MATH_PREFIX(mat3_identity)() {
    MEmat3 result = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(mat4_identity)() {
    MEmat4 result = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    return result;
}

// addition:

MATH_INLINE MEmat3 MATH_PREFIX(mat3_add)(MEmat3 m1, MEmat3 m2) {
    MEmat3 result;

    result.m[0][0] = m1.m[0][0] + m2.m[0][0];
    result.m[0][1] = m1.m[0][1] + m2.m[0][1];
    result.m[0][2] = m1.m[0][2] + m2.m[0][2];
    result.m[1][0] = m1.m[1][0] + m2.m[1][0];
    result.m[1][1] = m1.m[1][1] + m2.m[1][1];
    result.m[1][2] = m1.m[1][2] + m2.m[1][2];
    result.m[2][0] = m1.m[2][0] + m2.m[2][0];
    result.m[2][1] = m1.m[2][1] + m2.m[2][1];
    result.m[2][2] = m1.m[2][2] + m2.m[2][2];

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(mat4_add)(MEmat4 m1, MEmat4 m2) {
    MEmat4 result;

#if MATH_USE_SSE

    result.packed[0] = _mm_add_ps(m1.packed[0], m2.packed[0]);
    result.packed[1] = _mm_add_ps(m1.packed[1], m2.packed[1]);
    result.packed[2] = _mm_add_ps(m1.packed[2], m2.packed[2]);
    result.packed[3] = _mm_add_ps(m1.packed[3], m2.packed[3]);

#else

    result.m[0][0] = m1.m[0][0] + m2.m[0][0];
    result.m[0][1] = m1.m[0][1] + m2.m[0][1];
    result.m[0][2] = m1.m[0][2] + m2.m[0][2];
    result.m[0][3] = m1.m[0][3] + m2.m[0][3];
    result.m[1][0] = m1.m[1][0] + m2.m[1][0];
    result.m[1][1] = m1.m[1][1] + m2.m[1][1];
    result.m[1][2] = m1.m[1][2] + m2.m[1][2];
    result.m[1][3] = m1.m[1][3] + m2.m[1][3];
    result.m[2][0] = m1.m[2][0] + m2.m[2][0];
    result.m[2][1] = m1.m[2][1] + m2.m[2][1];
    result.m[2][2] = m1.m[2][2] + m2.m[2][2];
    result.m[2][3] = m1.m[2][3] + m2.m[2][3];
    result.m[3][0] = m1.m[3][0] + m2.m[3][0];
    result.m[3][1] = m1.m[3][1] + m2.m[3][1];
    result.m[3][2] = m1.m[3][2] + m2.m[3][2];
    result.m[3][3] = m1.m[3][3] + m2.m[3][3];

#endif

    return result;
}

// subtraction:

MATH_INLINE MEmat3 MATH_PREFIX(mat3_sub)(MEmat3 m1, MEmat3 m2) {
    MEmat3 result;

    result.m[0][0] = m1.m[0][0] - m2.m[0][0];
    result.m[0][1] = m1.m[0][1] - m2.m[0][1];
    result.m[0][2] = m1.m[0][2] - m2.m[0][2];
    result.m[1][0] = m1.m[1][0] - m2.m[1][0];
    result.m[1][1] = m1.m[1][1] - m2.m[1][1];
    result.m[1][2] = m1.m[1][2] - m2.m[1][2];
    result.m[2][0] = m1.m[2][0] - m2.m[2][0];
    result.m[2][1] = m1.m[2][1] - m2.m[2][1];
    result.m[2][2] = m1.m[2][2] - m2.m[2][2];

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(mat4_sub)(MEmat4 m1, MEmat4 m2) {
    MEmat4 result;

#if MATH_USE_SSE

    result.packed[0] = _mm_sub_ps(m1.packed[0], m2.packed[0]);
    result.packed[1] = _mm_sub_ps(m1.packed[1], m2.packed[1]);
    result.packed[2] = _mm_sub_ps(m1.packed[2], m2.packed[2]);
    result.packed[3] = _mm_sub_ps(m1.packed[3], m2.packed[3]);

#else

    result.m[0][0] = m1.m[0][0] - m2.m[0][0];
    result.m[0][1] = m1.m[0][1] - m2.m[0][1];
    result.m[0][2] = m1.m[0][2] - m2.m[0][2];
    result.m[0][3] = m1.m[0][3] - m2.m[0][3];
    result.m[1][0] = m1.m[1][0] - m2.m[1][0];
    result.m[1][1] = m1.m[1][1] - m2.m[1][1];
    result.m[1][2] = m1.m[1][2] - m2.m[1][2];
    result.m[1][3] = m1.m[1][3] - m2.m[1][3];
    result.m[2][0] = m1.m[2][0] - m2.m[2][0];
    result.m[2][1] = m1.m[2][1] - m2.m[2][1];
    result.m[2][2] = m1.m[2][2] - m2.m[2][2];
    result.m[2][3] = m1.m[2][3] - m2.m[2][3];
    result.m[3][0] = m1.m[3][0] - m2.m[3][0];
    result.m[3][1] = m1.m[3][1] - m2.m[3][1];
    result.m[3][2] = m1.m[3][2] - m2.m[3][2];
    result.m[3][3] = m1.m[3][3] - m2.m[3][3];

#endif

    return result;
}

// multiplication:

MATH_INLINE MEmat3 MATH_PREFIX(mat3_mult)(MEmat3 m1, MEmat3 m2) {
    MEmat3 result;

    result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[1][0] * m2.m[0][1] + m1.m[2][0] * m2.m[0][2];
    result.m[0][1] = m1.m[0][1] * m2.m[0][0] + m1.m[1][1] * m2.m[0][1] + m1.m[2][1] * m2.m[0][2];
    result.m[0][2] = m1.m[0][2] * m2.m[0][0] + m1.m[1][2] * m2.m[0][1] + m1.m[2][2] * m2.m[0][2];
    result.m[1][0] = m1.m[0][0] * m2.m[1][0] + m1.m[1][0] * m2.m[1][1] + m1.m[2][0] * m2.m[1][2];
    result.m[1][1] = m1.m[0][1] * m2.m[1][0] + m1.m[1][1] * m2.m[1][1] + m1.m[2][1] * m2.m[1][2];
    result.m[1][2] = m1.m[0][2] * m2.m[1][0] + m1.m[1][2] * m2.m[1][1] + m1.m[2][2] * m2.m[1][2];
    result.m[2][0] = m1.m[0][0] * m2.m[2][0] + m1.m[1][0] * m2.m[2][1] + m1.m[2][0] * m2.m[2][2];
    result.m[2][1] = m1.m[0][1] * m2.m[2][0] + m1.m[1][1] * m2.m[2][1] + m1.m[2][1] * m2.m[2][2];
    result.m[2][2] = m1.m[0][2] * m2.m[2][0] + m1.m[1][2] * m2.m[2][1] + m1.m[2][2] * m2.m[2][2];

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(mat4_mult)(MEmat4 m1, MEmat4 m2) {
    MEmat4 result;

#if MATH_USE_SSE

    result.packed[0] = MATH_PREFIX(mat4_mult_column_sse)(m2.packed[0], m1);
    result.packed[1] = MATH_PREFIX(mat4_mult_column_sse)(m2.packed[1], m1);
    result.packed[2] = MATH_PREFIX(mat4_mult_column_sse)(m2.packed[2], m1);
    result.packed[3] = MATH_PREFIX(mat4_mult_column_sse)(m2.packed[3], m1);

#else

    result.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[1][0] * m2.m[0][1] + m1.m[2][0] * m2.m[0][2] + m1.m[3][0] * m2.m[0][3];
    result.m[0][1] = m1.m[0][1] * m2.m[0][0] + m1.m[1][1] * m2.m[0][1] + m1.m[2][1] * m2.m[0][2] + m1.m[3][1] * m2.m[0][3];
    result.m[0][2] = m1.m[0][2] * m2.m[0][0] + m1.m[1][2] * m2.m[0][1] + m1.m[2][2] * m2.m[0][2] + m1.m[3][2] * m2.m[0][3];
    result.m[0][3] = m1.m[0][3] * m2.m[0][0] + m1.m[1][3] * m2.m[0][1] + m1.m[2][3] * m2.m[0][2] + m1.m[3][3] * m2.m[0][3];
    result.m[1][0] = m1.m[0][0] * m2.m[1][0] + m1.m[1][0] * m2.m[1][1] + m1.m[2][0] * m2.m[1][2] + m1.m[3][0] * m2.m[1][3];
    result.m[1][1] = m1.m[0][1] * m2.m[1][0] + m1.m[1][1] * m2.m[1][1] + m1.m[2][1] * m2.m[1][2] + m1.m[3][1] * m2.m[1][3];
    result.m[1][2] = m1.m[0][2] * m2.m[1][0] + m1.m[1][2] * m2.m[1][1] + m1.m[2][2] * m2.m[1][2] + m1.m[3][2] * m2.m[1][3];
    result.m[1][3] = m1.m[0][3] * m2.m[1][0] + m1.m[1][3] * m2.m[1][1] + m1.m[2][3] * m2.m[1][2] + m1.m[3][3] * m2.m[1][3];
    result.m[2][0] = m1.m[0][0] * m2.m[2][0] + m1.m[1][0] * m2.m[2][1] + m1.m[2][0] * m2.m[2][2] + m1.m[3][0] * m2.m[2][3];
    result.m[2][1] = m1.m[0][1] * m2.m[2][0] + m1.m[1][1] * m2.m[2][1] + m1.m[2][1] * m2.m[2][2] + m1.m[3][1] * m2.m[2][3];
    result.m[2][2] = m1.m[0][2] * m2.m[2][0] + m1.m[1][2] * m2.m[2][1] + m1.m[2][2] * m2.m[2][2] + m1.m[3][2] * m2.m[2][3];
    result.m[2][3] = m1.m[0][3] * m2.m[2][0] + m1.m[1][3] * m2.m[2][1] + m1.m[2][3] * m2.m[2][2] + m1.m[3][3] * m2.m[2][3];
    result.m[3][0] = m1.m[0][0] * m2.m[3][0] + m1.m[1][0] * m2.m[3][1] + m1.m[2][0] * m2.m[3][2] + m1.m[3][0] * m2.m[3][3];
    result.m[3][1] = m1.m[0][1] * m2.m[3][0] + m1.m[1][1] * m2.m[3][1] + m1.m[2][1] * m2.m[3][2] + m1.m[3][1] * m2.m[3][3];
    result.m[3][2] = m1.m[0][2] * m2.m[3][0] + m1.m[1][2] * m2.m[3][1] + m1.m[2][2] * m2.m[3][2] + m1.m[3][2] * m2.m[3][3];
    result.m[3][3] = m1.m[0][3] * m2.m[3][0] + m1.m[1][3] * m2.m[3][1] + m1.m[2][3] * m2.m[3][2] + m1.m[3][3] * m2.m[3][3];

#endif

    return result;
}

MATH_INLINE MEvec3 MATH_PREFIX(mat3_mult_vec3)(MEmat3 m, MEvec3 v) {
    MEvec3 result;

    result.x = m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z;
    result.y = m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z;
    result.z = m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z;

    return result;
}

MATH_INLINE MEvec4 MATH_PREFIX(mat4_mult_vec4)(MEmat4 m, MEvec4 v) {
    MEvec4 result;

#if MATH_USE_SSE

    result.packed = MATH_PREFIX(mat4_mult_column_sse)(v.packed, m);

#else

    result.x = m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w;
    result.y = m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w;
    result.z = m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w;
    result.w = m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w;

#endif

    return result;
}

// transpose:

MATH_INLINE MEmat3 MATH_PREFIX(mat3_transpose)(MEmat3 m) {
    MEmat3 result;

    result.m[0][0] = m.m[0][0];
    result.m[0][1] = m.m[1][0];
    result.m[0][2] = m.m[2][0];
    result.m[1][0] = m.m[0][1];
    result.m[1][1] = m.m[1][1];
    result.m[1][2] = m.m[2][1];
    result.m[2][0] = m.m[0][2];
    result.m[2][1] = m.m[1][2];
    result.m[2][2] = m.m[2][2];

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(mat4_transpose)(MEmat4 m) {
    MEmat4 result = m;

#if MATH_USE_SSE

    _MM_TRANSPOSE4_PS(result.packed[0], result.packed[1], result.packed[2], result.packed[3]);

#else

    result.m[0][0] = m.m[0][0];
    result.m[0][1] = m.m[1][0];
    result.m[0][2] = m.m[2][0];
    result.m[0][3] = m.m[3][0];
    result.m[1][0] = m.m[0][1];
    result.m[1][1] = m.m[1][1];
    result.m[1][2] = m.m[2][1];
    result.m[1][3] = m.m[3][1];
    result.m[2][0] = m.m[0][2];
    result.m[2][1] = m.m[1][2];
    result.m[2][2] = m.m[2][2];
    result.m[2][3] = m.m[3][2];
    result.m[3][0] = m.m[0][3];
    result.m[3][1] = m.m[1][3];
    result.m[3][2] = m.m[2][3];
    result.m[3][3] = m.m[3][3];

#endif

    return result;
}

// inverse:

MATH_INLINE MEmat3 MATH_PREFIX(mat3_inv)(MEmat3 m) {
    MEmat3 result;

    float det;
    float a = m.m[0][0], b = m.m[0][1], c = m.m[0][2], d = m.m[1][0], e = m.m[1][1], f = m.m[1][2], g = m.m[2][0], h = m.m[2][1], i = m.m[2][2];

    result.m[0][0] = e * i - f * h;
    result.m[0][1] = -(b * i - h * c);
    result.m[0][2] = b * f - e * c;
    result.m[1][0] = -(d * i - g * f);
    result.m[1][1] = a * i - c * g;
    result.m[1][2] = -(a * f - d * c);
    result.m[2][0] = d * h - g * e;
    result.m[2][1] = -(a * h - g * b);
    result.m[2][2] = a * e - b * d;

    det = 1.0f / (a * result.m[0][0] + b * result.m[1][0] + c * result.m[2][0]);

    result.m[0][0] *= det;
    result.m[0][1] *= det;
    result.m[0][2] *= det;
    result.m[1][0] *= det;
    result.m[1][1] *= det;
    result.m[1][2] *= det;
    result.m[2][0] *= det;
    result.m[2][1] *= det;
    result.m[2][2] *= det;

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(mat4_inv)(MEmat4 mat) {
    // TODO: this function is not SIMD optimized, figure out how to do it

    MEmat4 result;

    float tmp[6];
    float det;
    float a = mat.m[0][0], b = mat.m[0][1], c = mat.m[0][2], d = mat.m[0][3], e = mat.m[1][0], f = mat.m[1][1], g = mat.m[1][2], h = mat.m[1][3], i = mat.m[2][0], j = mat.m[2][1], k = mat.m[2][2],
          l = mat.m[2][3], m = mat.m[3][0], n = mat.m[3][1], o = mat.m[3][2], p = mat.m[3][3];

    tmp[0] = k * p - o * l;
    tmp[1] = j * p - n * l;
    tmp[2] = j * o - n * k;
    tmp[3] = i * p - m * l;
    tmp[4] = i * o - m * k;
    tmp[5] = i * n - m * j;

    result.m[0][0] = f * tmp[0] - g * tmp[1] + h * tmp[2];
    result.m[1][0] = -(e * tmp[0] - g * tmp[3] + h * tmp[4]);
    result.m[2][0] = e * tmp[1] - f * tmp[3] + h * tmp[5];
    result.m[3][0] = -(e * tmp[2] - f * tmp[4] + g * tmp[5]);

    result.m[0][1] = -(b * tmp[0] - c * tmp[1] + d * tmp[2]);
    result.m[1][1] = a * tmp[0] - c * tmp[3] + d * tmp[4];
    result.m[2][1] = -(a * tmp[1] - b * tmp[3] + d * tmp[5]);
    result.m[3][1] = a * tmp[2] - b * tmp[4] + c * tmp[5];

    tmp[0] = g * p - o * h;
    tmp[1] = f * p - n * h;
    tmp[2] = f * o - n * g;
    tmp[3] = e * p - m * h;
    tmp[4] = e * o - m * g;
    tmp[5] = e * n - m * f;

    result.m[0][2] = b * tmp[0] - c * tmp[1] + d * tmp[2];
    result.m[1][2] = -(a * tmp[0] - c * tmp[3] + d * tmp[4]);
    result.m[2][2] = a * tmp[1] - b * tmp[3] + d * tmp[5];
    result.m[3][2] = -(a * tmp[2] - b * tmp[4] + c * tmp[5]);

    tmp[0] = g * l - k * h;
    tmp[1] = f * l - j * h;
    tmp[2] = f * k - j * g;
    tmp[3] = e * l - i * h;
    tmp[4] = e * k - i * g;
    tmp[5] = e * j - i * f;

    result.m[0][3] = -(b * tmp[0] - c * tmp[1] + d * tmp[2]);
    result.m[1][3] = a * tmp[0] - c * tmp[3] + d * tmp[4];
    result.m[2][3] = -(a * tmp[1] - b * tmp[3] + d * tmp[5]);
    result.m[3][3] = a * tmp[2] - b * tmp[4] + c * tmp[5];

    det = 1.0f / (a * result.m[0][0] + b * result.m[1][0] + c * result.m[2][0] + d * result.m[3][0]);

#if MATH_USE_SSE

    __m128 scale = _mm_set1_ps(det);
    result.packed[0] = _mm_mul_ps(result.packed[0], scale);
    result.packed[1] = _mm_mul_ps(result.packed[1], scale);
    result.packed[2] = _mm_mul_ps(result.packed[2], scale);
    result.packed[3] = _mm_mul_ps(result.packed[3], scale);

#else

    result.m[0][0] = result.m[0][0] * det;
    result.m[0][1] = result.m[0][1] * det;
    result.m[0][2] = result.m[0][2] * det;
    result.m[0][3] = result.m[0][3] * det;
    result.m[1][0] = result.m[1][0] * det;
    result.m[1][1] = result.m[1][1] * det;
    result.m[1][2] = result.m[1][2] * det;
    result.m[1][3] = result.m[1][3] * det;
    result.m[2][0] = result.m[2][0] * det;
    result.m[2][1] = result.m[2][1] * det;
    result.m[2][2] = result.m[2][2] * det;
    result.m[2][3] = result.m[2][3] * det;
    result.m[3][0] = result.m[3][0] * det;
    result.m[3][1] = result.m[3][1] * det;
    result.m[3][2] = result.m[3][2] * det;
    result.m[3][3] = result.m[3][3] * det;

#endif

    return result;
}

// translation:

MATH_INLINE MEmat3 MATH_PREFIX(mat3_translate)(MEvec2 t) {
    MEmat3 result = MATH_PREFIX(mat3_identity)();

    result.m[2][0] = t.x;
    result.m[2][1] = t.y;

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(mat4_translate)(MEvec3 t) {
    MEmat4 result = MATH_PREFIX(mat4_identity)();

    result.m[3][0] = t.x;
    result.m[3][1] = t.y;
    result.m[3][2] = t.z;

    return result;
}

// scaling:

MATH_INLINE MEmat3 MATH_PREFIX(mat3_scale)(MEvec2 s) {
    MEmat3 result = MATH_PREFIX(mat3_identity)();

    result.m[0][0] = s.x;
    result.m[1][1] = s.y;

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(mat4_scale)(MEvec3 s) {
    MEmat4 result = MATH_PREFIX(mat4_identity)();

    result.m[0][0] = s.x;
    result.m[1][1] = s.y;
    result.m[2][2] = s.z;

    return result;
}

// rotation:

MATH_INLINE MEmat3 MATH_PREFIX(mat3_rotate)(float angle) {
    MEmat3 result = MATH_PREFIX(mat3_identity)();

    float radians = MATH_PREFIX(deg_to_rad)(angle);
    float sine = MATH_SINF(radians);
    float cosine = MATH_COSF(radians);

    result.m[0][0] = cosine;
    result.m[1][0] = sine;
    result.m[0][1] = -sine;
    result.m[1][1] = cosine;

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(mat4_rotate)(MEvec3 axis, float angle) {
    MEmat4 result = MATH_PREFIX(mat4_identity)();

    axis = MATH_PREFIX(vec3_normalize)(axis);

    float radians = MATH_PREFIX(deg_to_rad)(angle);
    float sine = MATH_SINF(radians);
    float cosine = MATH_COSF(radians);
    float cosine2 = 1.0f - cosine;

    result.m[0][0] = axis.x * axis.x * cosine2 + cosine;
    result.m[0][1] = axis.x * axis.y * cosine2 + axis.z * sine;
    result.m[0][2] = axis.x * axis.z * cosine2 - axis.y * sine;
    result.m[1][0] = axis.y * axis.x * cosine2 - axis.z * sine;
    result.m[1][1] = axis.y * axis.y * cosine2 + cosine;
    result.m[1][2] = axis.y * axis.z * cosine2 + axis.x * sine;
    result.m[2][0] = axis.z * axis.x * cosine2 + axis.y * sine;
    result.m[2][1] = axis.z * axis.y * cosine2 - axis.x * sine;
    result.m[2][2] = axis.z * axis.z * cosine2 + cosine;

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(mat4_rotate_euler)(MEvec3 angles) {
    MEmat4 result = MATH_PREFIX(mat4_identity)();

    MEvec3 radians;
    radians.x = MATH_PREFIX(deg_to_rad)(angles.x);
    radians.y = MATH_PREFIX(deg_to_rad)(angles.y);
    radians.z = MATH_PREFIX(deg_to_rad)(angles.z);

    float sinX = MATH_SINF(radians.x);
    float cosX = MATH_COSF(radians.x);
    float sinY = MATH_SINF(radians.y);
    float cosY = MATH_COSF(radians.y);
    float sinZ = MATH_SINF(radians.z);
    float cosZ = MATH_COSF(radians.z);

    result.m[0][0] = cosY * cosZ;
    result.m[0][1] = cosY * sinZ;
    result.m[0][2] = -sinY;
    result.m[1][0] = sinX * sinY * cosZ - cosX * sinZ;
    result.m[1][1] = sinX * sinY * sinZ + cosX * cosZ;
    result.m[1][2] = sinX * cosY;
    result.m[2][0] = cosX * sinY * cosZ + sinX * sinZ;
    result.m[2][1] = cosX * sinY * sinZ - sinX * cosZ;
    result.m[2][2] = cosX * cosY;

    return result;
}

// to mat3:

MATH_INLINE MEmat3 MATH_PREFIX(mat4_top_left)(MEmat4 m) {
    MEmat3 result;

    result.m[0][0] = m.m[0][0];
    result.m[0][1] = m.m[0][1];
    result.m[0][2] = m.m[0][2];
    result.m[1][0] = m.m[1][0];
    result.m[1][1] = m.m[1][1];
    result.m[1][2] = m.m[1][2];
    result.m[2][0] = m.m[2][0];
    result.m[2][1] = m.m[2][1];
    result.m[2][2] = m.m[2][2];

    return result;
}

// projection:

MATH_INLINE MEmat4 MATH_PREFIX(mat4_perspective)(float fov, float aspect, float near, float far) {
    MEmat4 result = {0};

    float scale = MATH_TANF(MATH_PREFIX(deg_to_rad)(fov * 0.5f)) * near;

    float right = aspect * scale;
    float left = -right;
    float top = scale;
    float bot = -top;

    result.m[0][0] = near / right;
    result.m[1][1] = near / top;
    result.m[2][2] = -(far + near) / (far - near);
    result.m[3][2] = -2.0f * far * near / (far - near);
    result.m[2][3] = -1.0f;

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(mat4_orthographic)(float left, float right, float bot, float top, float near, float far) {
    MEmat4 result = MATH_PREFIX(mat4_identity)();

    result.m[0][0] = 2.0f / (right - left);
    result.m[1][1] = 2.0f / (top - bot);
    result.m[2][2] = 2.0f / (near - far);

    result.m[3][0] = (left + right) / (left - right);
    result.m[3][1] = (bot + top) / (bot - top);
    result.m[3][2] = (near + far) / (near - far);

    return result;
}

// view matrix:

MATH_INLINE MEmat4 MATH_PREFIX(mat4_look)(MEvec3 pos, MEvec3 dir, MEvec3 up) {
    MEmat4 result;

    MEvec3 r = MATH_PREFIX(vec3_normalize)(MATH_PREFIX(vec3_cross)(up, dir));
    MEvec3 u = MATH_PREFIX(vec3_cross)(dir, r);

    MEmat4 RUD = MATH_PREFIX(mat4_identity)();
    RUD.m[0][0] = r.x;
    RUD.m[1][0] = r.y;
    RUD.m[2][0] = r.z;
    RUD.m[0][1] = u.x;
    RUD.m[1][1] = u.y;
    RUD.m[2][1] = u.z;
    RUD.m[0][2] = dir.x;
    RUD.m[1][2] = dir.y;
    RUD.m[2][2] = dir.z;

    MEvec3 oppPos = {-pos.x, -pos.y, -pos.z};
    result = MATH_PREFIX(mat4_mult)(RUD, MATH_PREFIX(mat4_translate)(oppPos));

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(mat4_lookat)(MEvec3 pos, MEvec3 target, MEvec3 up) {
    MEmat4 result;

    MEvec3 dir = MATH_PREFIX(vec3_normalize)(MATH_PREFIX(vec3_sub)(pos, target));
    result = MATH_PREFIX(mat4_look)(pos, dir, up);

    return result;
}

//----------------------------------------------------------------------//
// QUATERNION FUNCTIONS:

MATH_INLINE MEquaternion MATH_PREFIX(quaternion_identity)() {
    MEquaternion result;

    result.x = 0.0f;
    result.y = 0.0f;
    result.z = 0.0f;
    result.w = 1.0f;

    return result;
}

MATH_INLINE MEquaternion MATH_PREFIX(quaternion_add)(MEquaternion q1, MEquaternion q2) {
    MEquaternion result;

#if MATH_USE_SSE

    result.packed = _mm_add_ps(q1.packed, q2.packed);

#else

    result.x = q1.x + q2.x;
    result.y = q1.y + q2.y;
    result.z = q1.z + q2.z;
    result.w = q1.w + q2.w;

#endif

    return result;
}

MATH_INLINE MEquaternion MATH_PREFIX(quaternion_sub)(MEquaternion q1, MEquaternion q2) {
    MEquaternion result;

#if MATH_USE_SSE

    result.packed = _mm_sub_ps(q1.packed, q2.packed);

#else

    result.x = q1.x - q2.x;
    result.y = q1.y - q2.y;
    result.z = q1.z - q2.z;
    result.w = q1.w - q2.w;

#endif

    return result;
}

MATH_INLINE MEquaternion MATH_PREFIX(quaternion_mult)(MEquaternion q1, MEquaternion q2) {
    MEquaternion result;

#if MATH_USE_SSE

    __m128 temp1;
    __m128 temp2;

    temp1 = _mm_shuffle_ps(q1.packed, q1.packed, _MM_SHUFFLE(3, 3, 3, 3));
    temp2 = q2.packed;
    result.packed = _mm_mul_ps(temp1, temp2);

    temp1 = _mm_xor_ps(_mm_shuffle_ps(q1.packed, q1.packed, _MM_SHUFFLE(0, 0, 0, 0)), _mm_setr_ps(0.0f, -0.0f, 0.0f, -0.0f));
    temp2 = _mm_shuffle_ps(q2.packed, q2.packed, _MM_SHUFFLE(0, 1, 2, 3));
    result.packed = _mm_add_ps(result.packed, _mm_mul_ps(temp1, temp2));

    temp1 = _mm_xor_ps(_mm_shuffle_ps(q1.packed, q1.packed, _MM_SHUFFLE(1, 1, 1, 1)), _mm_setr_ps(0.0f, 0.0f, -0.0f, -0.0f));
    temp2 = _mm_shuffle_ps(q2.packed, q2.packed, _MM_SHUFFLE(1, 0, 3, 2));
    result.packed = _mm_add_ps(result.packed, _mm_mul_ps(temp1, temp2));

    temp1 = _mm_xor_ps(_mm_shuffle_ps(q1.packed, q1.packed, _MM_SHUFFLE(2, 2, 2, 2)), _mm_setr_ps(-0.0f, 0.0f, 0.0f, -0.0f));
    temp2 = _mm_shuffle_ps(q2.packed, q2.packed, _MM_SHUFFLE(2, 3, 0, 1));
    result.packed = _mm_add_ps(result.packed, _mm_mul_ps(temp1, temp2));

#else

    result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
    result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
    result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;
    result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;

#endif

    return result;
}

MATH_INLINE MEquaternion MATH_PREFIX(quaternion_scale)(MEquaternion q, float s) {
    MEquaternion result;

#if MATH_USE_SSE

    __m128 scale = _mm_set1_ps(s);
    result.packed = _mm_mul_ps(q.packed, scale);

#else

    result.x = q.x * s;
    result.y = q.y * s;
    result.z = q.z * s;
    result.w = q.w * s;

#endif

    return result;
}

MATH_INLINE float MATH_PREFIX(quaternion_dot)(MEquaternion q1, MEquaternion q2) {
    float result;

#if MATH_USE_SSE

    __m128 r = _mm_mul_ps(q1.packed, q2.packed);
    r = _mm_hadd_ps(r, r);
    r = _mm_hadd_ps(r, r);
    _mm_store_ss(&result, r);

#else

    result = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

#endif

    return result;
}

MATH_INLINE float MATH_PREFIX(quaternion_length)(MEquaternion q) {
    float result;

    result = MATH_SQRTF(MATH_PREFIX(quaternion_dot)(q, q));

    return result;
}

MATH_INLINE MEquaternion MATH_PREFIX(quaternion_normalize)(MEquaternion q) {
    MEquaternion result = {0};

    float len = MATH_PREFIX(quaternion_length)(q);
    if (len != 0.0f) {
#if MATH_USE_SSE

        __m128 scale = _mm_set1_ps(len);
        result.packed = _mm_div_ps(q.packed, scale);

#else

        float invLen = 1.0f / len;

        result.x = q.x * invLen;
        result.y = q.y * invLen;
        result.z = q.z * invLen;
        result.w = q.w * invLen;

#endif
    }

    return result;
}

MATH_INLINE MEquaternion MATH_PREFIX(quaternion_conjugate)(MEquaternion q) {
    MEquaternion result;

    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = q.w;

    return result;
}

MATH_INLINE MEquaternion MATH_PREFIX(quaternion_inv)(MEquaternion q) {
    MEquaternion result;

    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = q.w;

#if MATH_USE_SSE

    __m128 scale = _mm_set1_ps(MATH_PREFIX(quaternion_dot)(q, q));
    _mm_div_ps(result.packed, scale);

#else

    float invLen2 = 1.0f / MATH_PREFIX(quaternion_dot)(q, q);

    result.x *= invLen2;
    result.y *= invLen2;
    result.z *= invLen2;
    result.w *= invLen2;

#endif

    return result;
}

MATH_INLINE MEquaternion MATH_PREFIX(quaternion_slerp)(MEquaternion q1, MEquaternion q2, float a) {
    MEquaternion result;

    float cosine = MATH_PREFIX(quaternion_dot)(q1, q2);
    float angle = MATH_ACOSF(cosine);

    float sine1 = MATH_SINF((1.0f - a) * angle);
    float sine2 = MATH_SINF(a * angle);
    float invSine = 1.0f / MATH_SINF(angle);

    q1 = MATH_PREFIX(quaternion_scale)(q1, sine1);
    q2 = MATH_PREFIX(quaternion_scale)(q2, sine2);

    result = MATH_PREFIX(quaternion_add)(q1, q2);
    result = MATH_PREFIX(quaternion_scale)(result, invSine);

    return result;
}

MATH_INLINE MEquaternion MATH_PREFIX(quaternion_from_axis_angle)(MEvec3 axis, float angle) {
    MEquaternion result;

    float radians = MATH_PREFIX(deg_to_rad)(angle * 0.5f);
    axis = MATH_PREFIX(vec3_normalize)(axis);
    float sine = MATH_SINF(radians);

    result.x = axis.x * sine;
    result.y = axis.y * sine;
    result.z = axis.z * sine;
    result.w = MATH_COSF(radians);

    return result;
}

MATH_INLINE MEquaternion MATH_PREFIX(quaternion_from_euler)(MEvec3 angles) {
    MEquaternion result;

    MEvec3 radians;
    radians.x = MATH_PREFIX(deg_to_rad)(angles.x * 0.5f);
    radians.y = MATH_PREFIX(deg_to_rad)(angles.y * 0.5f);
    radians.z = MATH_PREFIX(deg_to_rad)(angles.z * 0.5f);

    float sinx = MATH_SINF(radians.x);
    float cosx = MATH_COSF(radians.x);
    float siny = MATH_SINF(radians.y);
    float cosy = MATH_COSF(radians.y);
    float sinz = MATH_SINF(radians.z);
    float cosz = MATH_COSF(radians.z);

#if MATH_USE_SSE

    __m128 packedx = _mm_setr_ps(sinx, cosx, cosx, cosx);
    __m128 packedy = _mm_setr_ps(cosy, siny, cosy, cosy);
    __m128 packedz = _mm_setr_ps(cosz, cosz, sinz, cosz);

    result.packed = _mm_mul_ps(_mm_mul_ps(packedx, packedy), packedz);

    packedx = _mm_shuffle_ps(packedx, packedx, _MM_SHUFFLE(0, 0, 0, 1));
    packedy = _mm_shuffle_ps(packedy, packedy, _MM_SHUFFLE(1, 1, 0, 1));
    packedz = _mm_shuffle_ps(packedz, packedz, _MM_SHUFFLE(2, 0, 2, 2));

    result.packed = _mm_addsub_ps(result.packed, _mm_mul_ps(_mm_mul_ps(packedx, packedy), packedz));

#else

    result.x = sinx * cosy * cosz - cosx * siny * sinz;
    result.y = cosx * siny * cosz + sinx * cosy * sinz;
    result.z = cosx * cosy * sinz - sinx * siny * cosz;
    result.w = cosx * cosy * cosz + sinx * siny * sinz;

#endif

    return result;
}

MATH_INLINE MEmat4 MATH_PREFIX(quaternion_to_mat4)(MEquaternion q) {
    MEmat4 result = MATH_PREFIX(mat4_identity)();

    float x2 = q.x + q.x;
    float y2 = q.y + q.y;
    float z2 = q.z + q.z;
    float xx2 = q.x * x2;
    float xy2 = q.x * y2;
    float xz2 = q.x * z2;
    float yy2 = q.y * y2;
    float yz2 = q.y * z2;
    float zz2 = q.z * z2;
    float sx2 = q.w * x2;
    float sy2 = q.w * y2;
    float sz2 = q.w * z2;

    result.m[0][0] = 1.0f - (yy2 + zz2);
    result.m[0][1] = xy2 - sz2;
    result.m[0][2] = xz2 + sy2;
    result.m[1][0] = xy2 + sz2;
    result.m[1][1] = 1.0f - (xx2 + zz2);
    result.m[1][2] = yz2 - sx2;
    result.m[2][0] = xz2 - sy2;
    result.m[2][1] = yz2 + sx2;
    result.m[2][2] = 1.0f - (xx2 + yy2);

    return result;
}

#ifdef ME_PLATFORM_WIN32
#define far
#define near
#endif

#endif
