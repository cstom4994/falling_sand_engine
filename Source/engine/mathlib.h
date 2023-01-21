
#ifndef METAENGINE_MATH_H
#define METAENGINE_MATH_H

#include "core/core.h"

#ifndef COVERAGE
#define COVERAGE(a, b)
#endif

#ifndef ASSERT_COVERED
#define ASSERT_COVERED(a)
#endif

// let's figure out if SSE is really available (unless disabled anyway)
// (it isn't on non-x86/x86_64 platforms or even x86 without explicit SSE support)
// => only use "#ifdef METAENGINE_MATH__USE_SSE" to check for SSE support below this block!
#ifndef METAENGINE_MATH_NO_SSE

#ifdef _MSC_VER
/* MSVC supports SSE in amd64 mode or _M_IX86_FP >= 1 (2 means SSE2) */
#if defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1)
#define METAENGINE_MATH__USE_SSE 1
#endif
#else          /* not MSVC, probably GCC, clang, icc or something that doesn't support SSE anyway */
#ifdef __SSE__ /* they #define __SSE__ if it's supported */
#define METAENGINE_MATH__USE_SSE 1
#endif /*  __SSE__ */
#endif /* not _MSC_VER */

#endif /* #ifndef METAENGINE_MATH_NO_SSE */

#ifdef METAENGINE_MATH__USE_SSE
#include <xmmintrin.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4201)
#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#if (defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ < 8)) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wgnu-anonymous-struct"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
#define METAENGINE_MATH_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#define METAENGINE_MATH_DEPRECATED(msg) __declspec(deprecated(msg))
#else
#define METAENGINE_MATH_DEPRECATED(msg)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(METAENGINE_MATH_SINF) || !defined(METAENGINE_MATH_COSF) || !defined(METAENGINE_MATH_TANF) || !defined(METAENGINE_MATH_SQRTF) || !defined(METAENGINE_MATH_EXPF) || \
        !defined(METAENGINE_MATH_LOGF) || !defined(METAENGINE_MATH_ACOSF) || !defined(METAENGINE_MATH_ATANF) || !defined(METAENGINE_MATH_ATAN2F)
#include <math.h>
#endif

#ifndef METAENGINE_MATH_SINF
#define METAENGINE_MATH_SINF sinf
#endif

#ifndef METAENGINE_MATH_COSF
#define METAENGINE_MATH_COSF cosf
#endif

#ifndef METAENGINE_MATH_TANF
#define METAENGINE_MATH_TANF tanf
#endif

#ifndef METAENGINE_MATH_SQRTF
#define METAENGINE_MATH_SQRTF sqrtf
#endif

#ifndef METAENGINE_MATH_EXPF
#define METAENGINE_MATH_EXPF expf
#endif

#ifndef METAENGINE_MATH_LOGF
#define METAENGINE_MATH_LOGF logf
#endif

#ifndef METAENGINE_MATH_ACOSF
#define METAENGINE_MATH_ACOSF acosf
#endif

#ifndef METAENGINE_MATH_ATANF
#define METAENGINE_MATH_ATANF atanf
#endif

#ifndef METAENGINE_MATH_ATAN2F
#define METAENGINE_MATH_ATAN2F atan2f
#endif

#define METAENGINE_MATH_PI32 3.14159265359f
#define METAENGINE_MATH_PI 3.14159265358979323846

#define METAENGINE_MATH_MIN(a, b) ((a) > (b) ? (b) : (a))
#define METAENGINE_MATH_MAX(a, b) ((a) < (b) ? (b) : (a))
#define METAENGINE_MATH_ABS(a) ((a) > 0 ? (a) : -(a))
#define METAENGINE_MATH_MOD(a, m) (((a) % (m)) >= 0 ? ((a) % (m)) : (((a) % (m)) + (m)))
#define METAENGINE_MATH_SQUARE(x) ((x) * (x))

#ifndef METAENGINE_MATH_PREFIX
#define METAENGINE_MATH_PREFIX(name) METAENGINE_MATH_##name
#endif

typedef union metadot_vec2 {
    struct {
        float X, Y;
    };

    struct {
        float U, V;
    };

    struct {
        float Left, Right;
    };

    struct {
        float Width, Height;
    };

    float Elements[2];

#ifdef __cplusplus
    inline float &operator[](const int &Index) { return Elements[Index]; }
#endif
} metadot_vec2;

typedef union metadot_vec3 {
    struct {
        float X, Y, Z;
    };

    struct {
        float U, V, W;
    };

    struct {
        float R, G, B;
    };

    struct {
        metadot_vec2 XY;
        float Ignored0_;
    };

    struct {
        float Ignored1_;
        metadot_vec2 YZ;
    };

    struct {
        metadot_vec2 UV;
        float Ignored2_;
    };

    struct {
        float Ignored3_;
        metadot_vec2 VW;
    };

    float Elements[3];

#ifdef __cplusplus
    inline float &operator[](const int &Index) { return Elements[Index]; }
#endif
} metadot_vec3;

typedef union metadot_vec4 {
    struct {
        union {
            metadot_vec3 XYZ;
            struct {
                float X, Y, Z;
            };
        };

        float W;
    };
    struct {
        union {
            metadot_vec3 RGB;
            struct {
                float R, G, B;
            };
        };

        float A;
    };

    struct {
        metadot_vec2 XY;
        float Ignored0_;
        float Ignored1_;
    };

    struct {
        float Ignored2_;
        metadot_vec2 YZ;
        float Ignored3_;
    };

    struct {
        float Ignored4_;
        float Ignored5_;
        metadot_vec2 ZW;
    };

    float Elements[4];

#ifdef METAENGINE_MATH__USE_SSE
    __m128 InternalElementsSSE;
#endif

#ifdef __cplusplus
    inline float &operator[](const int &Index) { return Elements[Index]; }
#endif
} metadot_vec4;

typedef union metadot_mat4 {
    float Elements[4][4];

#ifdef METAENGINE_MATH__USE_SSE
    __m128 Columns[4];

    METAENGINE_MATH_DEPRECATED("Our matrices are column-major, so this was named incorrectly. Use Columns instead.")
    __m128 Rows[4];
#endif

#ifdef __cplusplus
    inline metadot_vec4 operator[](const int &Index) {
        metadot_vec4 Result;
        float *Column = Elements[Index];

        Result.Elements[0] = Column[0];
        Result.Elements[1] = Column[1];
        Result.Elements[2] = Column[2];
        Result.Elements[3] = Column[3];

        return Result;
    }
#endif
} metadot_mat4;

typedef union metadot_quaternion {
    struct {
        union {
            metadot_vec3 XYZ;
            struct {
                float X, Y, Z;
            };
        };

        float W;
    };

    float Elements[4];

#ifdef METAENGINE_MATH__USE_SSE
    __m128 InternalElementsSSE;
#endif
} metadot_quaternion;

typedef struct U16Point {
    U16 x;
    U16 y;
} U16Point;

typedef struct metadot_rect {
    float x, y;
    float w, h;
} metadot_rect;

typedef signed int metadot_bool;

typedef metadot_vec2 metadot_v2;
typedef metadot_vec3 metadot_v3;
typedef metadot_vec4 metadot_v4;
typedef metadot_mat4 metadot_m4;

#define VECTOR3_ZERO \
    (metadot_vec3) { 0.0f, 0.0f, 0.0f }
#define VECTOR3_FORWARD \
    (metadot_vec3) { 1.0f, 0.0f, 0.0f }
#define VECTOR3_UP \
    (metadot_vec3) { 0.0f, 0.0f, 1.0f }
#define VECTOR3_DOWN \
    (metadot_vec3) { 0.0f, 0.0f, -1.0f }
#define VECTOR3_LEFT \
    (metadot_vec3) { 0.0f, 1.0f, 0.0f }

#define INT_INFINITY 0x3f3f3f3f

// #define sign(x) (x > 0 ? 1 : (x < 0 ? -1 : 0))
#define UTIL_clamp(x, m, M) (x < m ? m : (x > M ? M : x))
#define FRAC0(x) (x - floorf(x))
#define FRAC1(x) (1 - x + floorf(x))

#define UTIL_cross(u, v) \
    (metadot_vec3) { (u).y *(v).z - (u).z *(v).y, (u).z *(v).x - (u).x *(v).z, (u).x *(v).y - (u).y *(v).x }
#define UTIL_dot(u, v) ((u).X * (v).X + (u).Y * (v).Y + (u).Z * (v).Z)
// #define norm(v) sqrt(dot(v, v))// norm = length of  vector

/*
 * Floating-point math functions
 */

COVERAGE(METAENGINE_MATH_SinF, 1)
static_inline float METAENGINE_MATH_PREFIX(SinF)(float Radians) {
    ASSERT_COVERED(METAENGINE_MATH_SinF);

    float Result = METAENGINE_MATH_SINF(Radians);

    return (Result);
}

COVERAGE(METAENGINE_MATH_CosF, 1)
static_inline float METAENGINE_MATH_PREFIX(CosF)(float Radians) {
    ASSERT_COVERED(METAENGINE_MATH_CosF);

    float Result = METAENGINE_MATH_COSF(Radians);

    return (Result);
}

COVERAGE(METAENGINE_MATH_TanF, 1)
static_inline float METAENGINE_MATH_PREFIX(TanF)(float Radians) {
    ASSERT_COVERED(METAENGINE_MATH_TanF);

    float Result = METAENGINE_MATH_TANF(Radians);

    return (Result);
}

COVERAGE(METAENGINE_MATH_ACosF, 1)
static_inline float METAENGINE_MATH_PREFIX(ACosF)(float Radians) {
    ASSERT_COVERED(METAENGINE_MATH_ACosF);

    float Result = METAENGINE_MATH_ACOSF(Radians);

    return (Result);
}

COVERAGE(METAENGINE_MATH_ATanF, 1)
static_inline float METAENGINE_MATH_PREFIX(ATanF)(float Radians) {
    ASSERT_COVERED(METAENGINE_MATH_ATanF);

    float Result = METAENGINE_MATH_ATANF(Radians);

    return (Result);
}

COVERAGE(METAENGINE_MATH_ATan2F, 1)
static_inline float METAENGINE_MATH_PREFIX(ATan2F)(float Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_ATan2F);

    float Result = METAENGINE_MATH_ATAN2F(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_ExpF, 1)
static_inline float METAENGINE_MATH_PREFIX(ExpF)(float Float) {
    ASSERT_COVERED(METAENGINE_MATH_ExpF);

    float Result = METAENGINE_MATH_EXPF(Float);

    return (Result);
}

COVERAGE(METAENGINE_MATH_LogF, 1)
static_inline float METAENGINE_MATH_PREFIX(LogF)(float Float) {
    ASSERT_COVERED(METAENGINE_MATH_LogF);

    float Result = METAENGINE_MATH_LOGF(Float);

    return (Result);
}

COVERAGE(METAENGINE_MATH_SquareRootF, 1)
static_inline float METAENGINE_MATH_PREFIX(SquareRootF)(float Float) {
    ASSERT_COVERED(METAENGINE_MATH_SquareRootF);

    float Result;

#ifdef METAENGINE_MATH__USE_SSE
    __m128 In = _mm_set_ss(Float);
    __m128 Out = _mm_sqrt_ss(In);
    Result = _mm_cvtss_f32(Out);
#else
    Result = METAENGINE_MATH_SQRTF(Float);
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_RSquareRootF, 1)
static_inline float METAENGINE_MATH_PREFIX(RSquareRootF)(float Float) {
    ASSERT_COVERED(METAENGINE_MATH_RSquareRootF);

    float Result;

#ifdef METAENGINE_MATH__USE_SSE
    __m128 In = _mm_set_ss(Float);
    __m128 Out = _mm_rsqrt_ss(In);
    Result = _mm_cvtss_f32(Out);
#else
    Result = 1.0f / METAENGINE_MATH_PREFIX(SquareRootF)(Float);
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_Power, 2)
static_inline float METAENGINE_MATH_PREFIX(Power)(float Base, int Exponent) {
    ASSERT_COVERED(METAENGINE_MATH_Power);

    float Result = 1.0f;
    float Mul = Exponent < 0 ? 1.f / Base : Base;
    int X = Exponent < 0 ? -Exponent : Exponent;
    while (X) {
        if (X & 1) {
            ASSERT_COVERED(METAENGINE_MATH_Power);

            Result *= Mul;
        }

        Mul *= Mul;
        X >>= 1;
    }

    return (Result);
}

COVERAGE(METAENGINE_MATH_PowerF, 1)
static_inline float METAENGINE_MATH_PREFIX(PowerF)(float Base, float Exponent) {
    ASSERT_COVERED(METAENGINE_MATH_PowerF);

    float Result = METAENGINE_MATH_EXPF(Exponent * METAENGINE_MATH_LOGF(Base));

    return (Result);
}

/*
 * Utility functions
 */

COVERAGE(METAENGINE_MATH_ToRadians, 1)
static_inline float METAENGINE_MATH_PREFIX(ToRadians)(float Degrees) {
    ASSERT_COVERED(METAENGINE_MATH_ToRadians);

    float Result = Degrees * (METAENGINE_MATH_PI32 / 180.0f);

    return (Result);
}

COVERAGE(METAENGINE_MATH_Lerp, 1)
static_inline float METAENGINE_MATH_PREFIX(Lerp)(float A, float Time, float B) {
    ASSERT_COVERED(METAENGINE_MATH_Lerp);

    float Result = (1.0f - Time) * A + Time * B;

    return (Result);
}

COVERAGE(METAENGINE_MATH_Clamp, 1)
static_inline float METAENGINE_MATH_PREFIX(Clamp)(float Min, float Value, float Max) {
    ASSERT_COVERED(METAENGINE_MATH_Clamp);

    float Result = Value;

    if (Result < Min) {
        Result = Min;
    }

    if (Result > Max) {
        Result = Max;
    }

    return (Result);
}

/*
 * Vector initialization
 */

COVERAGE(METAENGINE_MATH_Vec2, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(Vec2)(float X, float Y) {
    ASSERT_COVERED(METAENGINE_MATH_Vec2);

    metadot_vec2 Result;

    Result.X = X;
    Result.Y = Y;

    return (Result);
}

COVERAGE(METAENGINE_MATH_Vec2i, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(Vec2i)(int X, int Y) {
    ASSERT_COVERED(METAENGINE_MATH_Vec2i);

    metadot_vec2 Result;

    Result.X = (float)X;
    Result.Y = (float)Y;

    return (Result);
}

COVERAGE(METAENGINE_MATH_Vec3, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(Vec3)(float X, float Y, float Z) {
    ASSERT_COVERED(METAENGINE_MATH_Vec3);

    metadot_vec3 Result;

    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;

    return (Result);
}

COVERAGE(METAENGINE_MATH_Vec3i, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(Vec3i)(int X, int Y, int Z) {
    ASSERT_COVERED(METAENGINE_MATH_Vec3i);

    metadot_vec3 Result;

    Result.X = (float)X;
    Result.Y = (float)Y;
    Result.Z = (float)Z;

    return (Result);
}

COVERAGE(METAENGINE_MATH_Vec4, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(Vec4)(float X, float Y, float Z, float W) {
    ASSERT_COVERED(METAENGINE_MATH_Vec4);

    metadot_vec4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = _mm_setr_ps(X, Y, Z, W);
#else
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    Result.W = W;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_Vec4i, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(Vec4i)(int X, int Y, int Z, int W) {
    ASSERT_COVERED(METAENGINE_MATH_Vec4i);

    metadot_vec4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = _mm_setr_ps((float)X, (float)Y, (float)Z, (float)W);
#else
    Result.X = (float)X;
    Result.Y = (float)Y;
    Result.Z = (float)Z;
    Result.W = (float)W;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_Vec4v, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(Vec4v)(metadot_vec3 Vector, float W) {
    ASSERT_COVERED(METAENGINE_MATH_Vec4v);

    metadot_vec4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = _mm_setr_ps(Vector.X, Vector.Y, Vector.Z, W);
#else
    Result.XYZ = Vector;
    Result.W = W;
#endif

    return (Result);
}

/*
 * Binary vector operations
 */

COVERAGE(METAENGINE_MATH_AddVec2, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(AddVec2)(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddVec2);

    metadot_vec2 Result;

    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddVec3, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(AddVec3)(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddVec3);

    metadot_vec3 Result;

    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddVec4, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(AddVec4)(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddVec4);

    metadot_vec4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = _mm_add_ps(Left.InternalElementsSSE, Right.InternalElementsSSE);
#else
    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;
    Result.W = Left.W + Right.W;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractVec2, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(SubtractVec2)(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractVec2);

    metadot_vec2 Result;

    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractVec3, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(SubtractVec3)(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractVec3);

    metadot_vec3 Result;

    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    Result.Z = Left.Z - Right.Z;

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractVec4, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(SubtractVec4)(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractVec4);

    metadot_vec4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = _mm_sub_ps(Left.InternalElementsSSE, Right.InternalElementsSSE);
#else
    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    Result.Z = Left.Z - Right.Z;
    Result.W = Left.W - Right.W;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec2, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(MultiplyVec2)(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec2);

    metadot_vec2 Result;

    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec2f, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(MultiplyVec2f)(metadot_vec2 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec2f);

    metadot_vec2 Result;

    Result.X = Left.X * Right;
    Result.Y = Left.Y * Right;

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec3, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(MultiplyVec3)(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec3);

    metadot_vec3 Result;

    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;
    Result.Z = Left.Z * Right.Z;

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec3f, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(MultiplyVec3f)(metadot_vec3 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec3f);

    metadot_vec3 Result;

    Result.X = Left.X * Right;
    Result.Y = Left.Y * Right;
    Result.Z = Left.Z * Right;

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec4, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(MultiplyVec4)(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec4);

    metadot_vec4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = _mm_mul_ps(Left.InternalElementsSSE, Right.InternalElementsSSE);
#else
    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;
    Result.Z = Left.Z * Right.Z;
    Result.W = Left.W * Right.W;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec4f, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(MultiplyVec4f)(metadot_vec4 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec4f);

    metadot_vec4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    __m128 Scalar = _mm_set1_ps(Right);
    Result.InternalElementsSSE = _mm_mul_ps(Left.InternalElementsSSE, Scalar);
#else
    Result.X = Left.X * Right;
    Result.Y = Left.Y * Right;
    Result.Z = Left.Z * Right;
    Result.W = Left.W * Right;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec2, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(DivideVec2)(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec2);

    metadot_vec2 Result;

    Result.X = Left.X / Right.X;
    Result.Y = Left.Y / Right.Y;

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec2f, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(DivideVec2f)(metadot_vec2 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec2f);

    metadot_vec2 Result;

    Result.X = Left.X / Right;
    Result.Y = Left.Y / Right;

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec3, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(DivideVec3)(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec3);

    metadot_vec3 Result;

    Result.X = Left.X / Right.X;
    Result.Y = Left.Y / Right.Y;
    Result.Z = Left.Z / Right.Z;

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec3f, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(DivideVec3f)(metadot_vec3 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec3f);

    metadot_vec3 Result;

    Result.X = Left.X / Right;
    Result.Y = Left.Y / Right;
    Result.Z = Left.Z / Right;

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec4, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(DivideVec4)(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec4);

    metadot_vec4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = _mm_div_ps(Left.InternalElementsSSE, Right.InternalElementsSSE);
#else
    Result.X = Left.X / Right.X;
    Result.Y = Left.Y / Right.Y;
    Result.Z = Left.Z / Right.Z;
    Result.W = Left.W / Right.W;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec4f, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(DivideVec4f)(metadot_vec4 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec4f);

    metadot_vec4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    __m128 Scalar = _mm_set1_ps(Right);
    Result.InternalElementsSSE = _mm_div_ps(Left.InternalElementsSSE, Scalar);
#else
    Result.X = Left.X / Right;
    Result.Y = Left.Y / Right;
    Result.Z = Left.Z / Right;
    Result.W = Left.W / Right;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_EqualsVec2, 1)
static_inline metadot_bool METAENGINE_MATH_PREFIX(EqualsVec2)(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_EqualsVec2);

    metadot_bool Result = (Left.X == Right.X && Left.Y == Right.Y);

    return (Result);
}

COVERAGE(METAENGINE_MATH_EqualsVec3, 1)
static_inline metadot_bool METAENGINE_MATH_PREFIX(EqualsVec3)(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_EqualsVec3);

    metadot_bool Result = (Left.X == Right.X && Left.Y == Right.Y && Left.Z == Right.Z);

    return (Result);
}

COVERAGE(METAENGINE_MATH_EqualsVec4, 1)
static_inline metadot_bool METAENGINE_MATH_PREFIX(EqualsVec4)(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_EqualsVec4);

    metadot_bool Result = (Left.X == Right.X && Left.Y == Right.Y && Left.Z == Right.Z && Left.W == Right.W);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DotVec2, 1)
static_inline float METAENGINE_MATH_PREFIX(DotVec2)(metadot_vec2 VecOne, metadot_vec2 VecTwo) {
    ASSERT_COVERED(METAENGINE_MATH_DotVec2);

    float Result = (VecOne.X * VecTwo.X) + (VecOne.Y * VecTwo.Y);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DotVec3, 1)
static_inline float METAENGINE_MATH_PREFIX(DotVec3)(metadot_vec3 VecOne, metadot_vec3 VecTwo) {
    ASSERT_COVERED(METAENGINE_MATH_DotVec3);

    float Result = (VecOne.X * VecTwo.X) + (VecOne.Y * VecTwo.Y) + (VecOne.Z * VecTwo.Z);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DotVec4, 1)
static_inline float METAENGINE_MATH_PREFIX(DotVec4)(metadot_vec4 VecOne, metadot_vec4 VecTwo) {
    ASSERT_COVERED(METAENGINE_MATH_DotVec4);

    float Result;

    // NOTE(zak): IN the future if we wanna check what version SSE is support
    // we can use _mm_dp_ps (4.3) but for now we will use the old way.
    // Or a r = _mm_mul_ps(v1, v2), r = _mm_hadd_ps(r, r), r = _mm_hadd_ps(r, r) for SSE3
#ifdef METAENGINE_MATH__USE_SSE
    __m128 SSEResultOne = _mm_mul_ps(VecOne.InternalElementsSSE, VecTwo.InternalElementsSSE);
    __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    _mm_store_ss(&Result, SSEResultOne);
#else
    Result = (VecOne.X * VecTwo.X) + (VecOne.Y * VecTwo.Y) + (VecOne.Z * VecTwo.Z) + (VecOne.W * VecTwo.W);
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_Cross, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(Cross)(metadot_vec3 VecOne, metadot_vec3 VecTwo) {
    ASSERT_COVERED(METAENGINE_MATH_Cross);

    metadot_vec3 Result;

    Result.X = (VecOne.Y * VecTwo.Z) - (VecOne.Z * VecTwo.Y);
    Result.Y = (VecOne.Z * VecTwo.X) - (VecOne.X * VecTwo.Z);
    Result.Z = (VecOne.X * VecTwo.Y) - (VecOne.Y * VecTwo.X);

    return (Result);
}

/*
 * Unary vector operations
 */

COVERAGE(METAENGINE_MATH_LengthSquaredVec2, 1)
static_inline float METAENGINE_MATH_PREFIX(LengthSquaredVec2)(metadot_vec2 A) {
    ASSERT_COVERED(METAENGINE_MATH_LengthSquaredVec2);

    float Result = METAENGINE_MATH_PREFIX(DotVec2)(A, A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_LengthSquaredVec3, 1)
static_inline float METAENGINE_MATH_PREFIX(LengthSquaredVec3)(metadot_vec3 A) {
    ASSERT_COVERED(METAENGINE_MATH_LengthSquaredVec3);

    float Result = METAENGINE_MATH_PREFIX(DotVec3)(A, A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_LengthSquaredVec4, 1)
static_inline float METAENGINE_MATH_PREFIX(LengthSquaredVec4)(metadot_vec4 A) {
    ASSERT_COVERED(METAENGINE_MATH_LengthSquaredVec4);

    float Result = METAENGINE_MATH_PREFIX(DotVec4)(A, A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_LengthVec2, 1)
static_inline float METAENGINE_MATH_PREFIX(LengthVec2)(metadot_vec2 A) {
    ASSERT_COVERED(METAENGINE_MATH_LengthVec2);

    float Result = METAENGINE_MATH_PREFIX(SquareRootF)(METAENGINE_MATH_PREFIX(LengthSquaredVec2)(A));

    return (Result);
}

COVERAGE(METAENGINE_MATH_LengthVec3, 1)
static_inline float METAENGINE_MATH_PREFIX(LengthVec3)(metadot_vec3 A) {
    ASSERT_COVERED(METAENGINE_MATH_LengthVec3);

    float Result = METAENGINE_MATH_PREFIX(SquareRootF)(METAENGINE_MATH_PREFIX(LengthSquaredVec3)(A));

    return (Result);
}

COVERAGE(METAENGINE_MATH_LengthVec4, 1)
static_inline float METAENGINE_MATH_PREFIX(LengthVec4)(metadot_vec4 A) {
    ASSERT_COVERED(METAENGINE_MATH_LengthVec4);

    float Result = METAENGINE_MATH_PREFIX(SquareRootF)(METAENGINE_MATH_PREFIX(LengthSquaredVec4)(A));

    return (Result);
}

COVERAGE(METAENGINE_MATH_NormalizeVec2, 2)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(NormalizeVec2)(metadot_vec2 A) {
    ASSERT_COVERED(METAENGINE_MATH_NormalizeVec2);

    metadot_vec2 Result = {0};

    float VectorLength = METAENGINE_MATH_PREFIX(LengthVec2)(A);

    /* NOTE(kiljacken): We need a zero check to not divide-by-zero */
    if (VectorLength != 0.0f) {
        ASSERT_COVERED(METAENGINE_MATH_NormalizeVec2);

        Result.X = A.X * (1.0f / VectorLength);
        Result.Y = A.Y * (1.0f / VectorLength);
    }

    return (Result);
}

COVERAGE(METAENGINE_MATH_NormalizeVec3, 2)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(NormalizeVec3)(metadot_vec3 A) {
    ASSERT_COVERED(METAENGINE_MATH_NormalizeVec3);

    metadot_vec3 Result = {0};

    float VectorLength = METAENGINE_MATH_PREFIX(LengthVec3)(A);

    /* NOTE(kiljacken): We need a zero check to not divide-by-zero */
    if (VectorLength != 0.0f) {
        ASSERT_COVERED(METAENGINE_MATH_NormalizeVec3);

        Result.X = A.X * (1.0f / VectorLength);
        Result.Y = A.Y * (1.0f / VectorLength);
        Result.Z = A.Z * (1.0f / VectorLength);
    }

    return (Result);
}

COVERAGE(METAENGINE_MATH_NormalizeVec4, 2)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(NormalizeVec4)(metadot_vec4 A) {
    ASSERT_COVERED(METAENGINE_MATH_NormalizeVec4);

    metadot_vec4 Result = {0};

    float VectorLength = METAENGINE_MATH_PREFIX(LengthVec4)(A);

    /* NOTE(kiljacken): We need a zero check to not divide-by-zero */
    if (VectorLength != 0.0f) {
        ASSERT_COVERED(METAENGINE_MATH_NormalizeVec4);

        float Multiplier = 1.0f / VectorLength;

#ifdef METAENGINE_MATH__USE_SSE
        __m128 SSEMultiplier = _mm_set1_ps(Multiplier);
        Result.InternalElementsSSE = _mm_mul_ps(A.InternalElementsSSE, SSEMultiplier);
#else
        Result.X = A.X * Multiplier;
        Result.Y = A.Y * Multiplier;
        Result.Z = A.Z * Multiplier;
        Result.W = A.W * Multiplier;
#endif
    }

    return (Result);
}

COVERAGE(METAENGINE_MATH_FastNormalizeVec2, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(FastNormalizeVec2)(metadot_vec2 A) {
    ASSERT_COVERED(METAENGINE_MATH_FastNormalizeVec2);

    return METAENGINE_MATH_PREFIX(MultiplyVec2f)(A, METAENGINE_MATH_PREFIX(RSquareRootF)(METAENGINE_MATH_PREFIX(DotVec2)(A, A)));
}

COVERAGE(METAENGINE_MATH_FastNormalizeVec3, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(FastNormalizeVec3)(metadot_vec3 A) {
    ASSERT_COVERED(METAENGINE_MATH_FastNormalizeVec3);

    return METAENGINE_MATH_PREFIX(MultiplyVec3f)(A, METAENGINE_MATH_PREFIX(RSquareRootF)(METAENGINE_MATH_PREFIX(DotVec3)(A, A)));
}

COVERAGE(METAENGINE_MATH_FastNormalizeVec4, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(FastNormalizeVec4)(metadot_vec4 A) {
    ASSERT_COVERED(METAENGINE_MATH_FastNormalizeVec4);

    return METAENGINE_MATH_PREFIX(MultiplyVec4f)(A, METAENGINE_MATH_PREFIX(RSquareRootF)(METAENGINE_MATH_PREFIX(DotVec4)(A, A)));
}

/*
 * SSE stuff
 */

#ifdef METAENGINE_MATH__USE_SSE
COVERAGE(METAENGINE_MATH_LinearCombineSSE, 1)
static_inline __m128 METAENGINE_MATH_PREFIX(LinearCombineSSE)(__m128 Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_LinearCombineSSE);

    __m128 Result;
    Result = _mm_mul_ps(_mm_shuffle_ps(Left, Left, 0x00), Right.Columns[0]);
    Result = _mm_add_ps(Result, _mm_mul_ps(_mm_shuffle_ps(Left, Left, 0x55), Right.Columns[1]));
    Result = _mm_add_ps(Result, _mm_mul_ps(_mm_shuffle_ps(Left, Left, 0xaa), Right.Columns[2]));
    Result = _mm_add_ps(Result, _mm_mul_ps(_mm_shuffle_ps(Left, Left, 0xff), Right.Columns[3]));

    return (Result);
}
#endif

/*
 * Matrix functions
 */

COVERAGE(METAENGINE_MATH_Mat4, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Mat4)(void) {
    ASSERT_COVERED(METAENGINE_MATH_Mat4);

    metadot_mat4 Result = {0};

    return (Result);
}

COVERAGE(METAENGINE_MATH_Mat4d, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Mat4d)(float Diagonal) {
    ASSERT_COVERED(METAENGINE_MATH_Mat4d);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(Mat4)();

    Result.Elements[0][0] = Diagonal;
    Result.Elements[1][1] = Diagonal;
    Result.Elements[2][2] = Diagonal;
    Result.Elements[3][3] = Diagonal;

    return (Result);
}

COVERAGE(METAENGINE_MATH_Transpose, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Transpose)(metadot_mat4 Matrix) {
    ASSERT_COVERED(METAENGINE_MATH_Transpose);

    metadot_mat4 Result = Matrix;

#ifdef METAENGINE_MATH__USE_SSE
    _MM_TRANSPOSE4_PS(Result.Columns[0], Result.Columns[1], Result.Columns[2], Result.Columns[3]);
#else
    int Columns;
    for (Columns = 0; Columns < 4; ++Columns) {
        int Rows;
        for (Rows = 0; Rows < 4; ++Rows) {
            Result.Elements[Rows][Columns] = Matrix.Elements[Columns][Rows];
        }
    }
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddMat4, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(AddMat4)(metadot_mat4 Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddMat4);

    metadot_mat4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.Columns[0] = _mm_add_ps(Left.Columns[0], Right.Columns[0]);
    Result.Columns[1] = _mm_add_ps(Left.Columns[1], Right.Columns[1]);
    Result.Columns[2] = _mm_add_ps(Left.Columns[2], Right.Columns[2]);
    Result.Columns[3] = _mm_add_ps(Left.Columns[3], Right.Columns[3]);
#else
    int Columns;
    for (Columns = 0; Columns < 4; ++Columns) {
        int Rows;
        for (Rows = 0; Rows < 4; ++Rows) {
            Result.Elements[Columns][Rows] = Left.Elements[Columns][Rows] + Right.Elements[Columns][Rows];
        }
    }
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractMat4, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(SubtractMat4)(metadot_mat4 Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractMat4);

    metadot_mat4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.Columns[0] = _mm_sub_ps(Left.Columns[0], Right.Columns[0]);
    Result.Columns[1] = _mm_sub_ps(Left.Columns[1], Right.Columns[1]);
    Result.Columns[2] = _mm_sub_ps(Left.Columns[2], Right.Columns[2]);
    Result.Columns[3] = _mm_sub_ps(Left.Columns[3], Right.Columns[3]);
#else
    int Columns;
    for (Columns = 0; Columns < 4; ++Columns) {
        int Rows;
        for (Rows = 0; Rows < 4; ++Rows) {
            Result.Elements[Columns][Rows] = Left.Elements[Columns][Rows] - Right.Elements[Columns][Rows];
        }
    }
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyMat4, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(MultiplyMat4)(metadot_mat4 Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyMat4);

    metadot_mat4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.Columns[0] = METAENGINE_MATH_PREFIX(LinearCombineSSE)(Right.Columns[0], Left);
    Result.Columns[1] = METAENGINE_MATH_PREFIX(LinearCombineSSE)(Right.Columns[1], Left);
    Result.Columns[2] = METAENGINE_MATH_PREFIX(LinearCombineSSE)(Right.Columns[2], Left);
    Result.Columns[3] = METAENGINE_MATH_PREFIX(LinearCombineSSE)(Right.Columns[3], Left);
#else
    int Columns;
    for (Columns = 0; Columns < 4; ++Columns) {
        int Rows;
        for (Rows = 0; Rows < 4; ++Rows) {
            float Sum = 0;
            int CurrentMatrice;
            for (CurrentMatrice = 0; CurrentMatrice < 4; ++CurrentMatrice) {
                Sum += Left.Elements[CurrentMatrice][Rows] * Right.Elements[Columns][CurrentMatrice];
            }

            Result.Elements[Columns][Rows] = Sum;
        }
    }
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyMat4f, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(MultiplyMat4f)(metadot_mat4 Matrix, float Scalar) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyMat4f);

    metadot_mat4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    __m128 SSEScalar = _mm_set1_ps(Scalar);
    Result.Columns[0] = _mm_mul_ps(Matrix.Columns[0], SSEScalar);
    Result.Columns[1] = _mm_mul_ps(Matrix.Columns[1], SSEScalar);
    Result.Columns[2] = _mm_mul_ps(Matrix.Columns[2], SSEScalar);
    Result.Columns[3] = _mm_mul_ps(Matrix.Columns[3], SSEScalar);
#else
    int Columns;
    for (Columns = 0; Columns < 4; ++Columns) {
        int Rows;
        for (Rows = 0; Rows < 4; ++Rows) {
            Result.Elements[Columns][Rows] = Matrix.Elements[Columns][Rows] * Scalar;
        }
    }
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyMat4ByVec4, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(MultiplyMat4ByVec4)(metadot_mat4 Matrix, metadot_vec4 Vector) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyMat4ByVec4);

    metadot_vec4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = METAENGINE_MATH_PREFIX(LinearCombineSSE)(Vector.InternalElementsSSE, Matrix);
#else
    int Columns, Rows;
    for (Rows = 0; Rows < 4; ++Rows) {
        float Sum = 0;
        for (Columns = 0; Columns < 4; ++Columns) {
            Sum += Matrix.Elements[Columns][Rows] * Vector.Elements[Columns];
        }

        Result.Elements[Rows] = Sum;
    }
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideMat4f, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(DivideMat4f)(metadot_mat4 Matrix, float Scalar) {
    ASSERT_COVERED(METAENGINE_MATH_DivideMat4f);

    metadot_mat4 Result;

#ifdef METAENGINE_MATH__USE_SSE
    __m128 SSEScalar = _mm_set1_ps(Scalar);
    Result.Columns[0] = _mm_div_ps(Matrix.Columns[0], SSEScalar);
    Result.Columns[1] = _mm_div_ps(Matrix.Columns[1], SSEScalar);
    Result.Columns[2] = _mm_div_ps(Matrix.Columns[2], SSEScalar);
    Result.Columns[3] = _mm_div_ps(Matrix.Columns[3], SSEScalar);
#else
    int Columns;
    for (Columns = 0; Columns < 4; ++Columns) {
        int Rows;
        for (Rows = 0; Rows < 4; ++Rows) {
            Result.Elements[Columns][Rows] = Matrix.Elements[Columns][Rows] / Scalar;
        }
    }
#endif

    return (Result);
}

/*
 * Common graphics transformations
 */

COVERAGE(METAENGINE_MATH_Orthographic, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Orthographic)(float Left, float Right, float Bottom, float Top, float Near, float Far) {
    ASSERT_COVERED(METAENGINE_MATH_Orthographic);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(Mat4)();

    Result.Elements[0][0] = 2.0f / (Right - Left);
    Result.Elements[1][1] = 2.0f / (Top - Bottom);
    Result.Elements[2][2] = 2.0f / (Near - Far);
    Result.Elements[3][3] = 1.0f;

    Result.Elements[3][0] = (Left + Right) / (Left - Right);
    Result.Elements[3][1] = (Bottom + Top) / (Bottom - Top);
    Result.Elements[3][2] = (Far + Near) / (Near - Far);

    return (Result);
}

COVERAGE(METAENGINE_MATH_Perspective, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Perspective)(float FOV, float AspectRatio, float Near, float Far) {
    ASSERT_COVERED(METAENGINE_MATH_Perspective);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(Mat4)();

    // See https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml

    float Cotangent = 1.0f / METAENGINE_MATH_PREFIX(TanF)(FOV * (METAENGINE_MATH_PI32 / 360.0f));

    Result.Elements[0][0] = Cotangent / AspectRatio;
    Result.Elements[1][1] = Cotangent;
    Result.Elements[2][3] = -1.0f;
    Result.Elements[2][2] = (Near + Far) / (Near - Far);
    Result.Elements[3][2] = (2.0f * Near * Far) / (Near - Far);
    Result.Elements[3][3] = 0.0f;

    return (Result);
}

COVERAGE(METAENGINE_MATH_Translate, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Translate)(metadot_vec3 Translation) {
    ASSERT_COVERED(METAENGINE_MATH_Translate);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(Mat4d)(1.0f);

    Result.Elements[3][0] = Translation.X;
    Result.Elements[3][1] = Translation.Y;
    Result.Elements[3][2] = Translation.Z;

    return (Result);
}

COVERAGE(METAENGINE_MATH_Rotate, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Rotate)(float Angle, metadot_vec3 Axis) {
    ASSERT_COVERED(METAENGINE_MATH_Rotate);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(Mat4d)(1.0f);

    Axis = METAENGINE_MATH_PREFIX(NormalizeVec3)(Axis);

    float SinTheta = METAENGINE_MATH_PREFIX(SinF)(METAENGINE_MATH_PREFIX(ToRadians)(Angle));
    float CosTheta = METAENGINE_MATH_PREFIX(CosF)(METAENGINE_MATH_PREFIX(ToRadians)(Angle));
    float CosValue = 1.0f - CosTheta;

    Result.Elements[0][0] = (Axis.X * Axis.X * CosValue) + CosTheta;
    Result.Elements[0][1] = (Axis.X * Axis.Y * CosValue) + (Axis.Z * SinTheta);
    Result.Elements[0][2] = (Axis.X * Axis.Z * CosValue) - (Axis.Y * SinTheta);

    Result.Elements[1][0] = (Axis.Y * Axis.X * CosValue) - (Axis.Z * SinTheta);
    Result.Elements[1][1] = (Axis.Y * Axis.Y * CosValue) + CosTheta;
    Result.Elements[1][2] = (Axis.Y * Axis.Z * CosValue) + (Axis.X * SinTheta);

    Result.Elements[2][0] = (Axis.Z * Axis.X * CosValue) + (Axis.Y * SinTheta);
    Result.Elements[2][1] = (Axis.Z * Axis.Y * CosValue) - (Axis.X * SinTheta);
    Result.Elements[2][2] = (Axis.Z * Axis.Z * CosValue) + CosTheta;

    return (Result);
}

COVERAGE(METAENGINE_MATH_Scale, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Scale)(metadot_vec3 Scale) {
    ASSERT_COVERED(METAENGINE_MATH_Scale);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(Mat4d)(1.0f);

    Result.Elements[0][0] = Scale.X;
    Result.Elements[1][1] = Scale.Y;
    Result.Elements[2][2] = Scale.Z;

    return (Result);
}

COVERAGE(METAENGINE_MATH_LookAt, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(LookAt)(metadot_vec3 Eye, metadot_vec3 Center, metadot_vec3 Up) {
    ASSERT_COVERED(METAENGINE_MATH_LookAt);

    metadot_mat4 Result;

    metadot_vec3 F = METAENGINE_MATH_PREFIX(NormalizeVec3)(METAENGINE_MATH_PREFIX(SubtractVec3)(Center, Eye));
    metadot_vec3 S = METAENGINE_MATH_PREFIX(NormalizeVec3)(METAENGINE_MATH_PREFIX(Cross)(F, Up));
    metadot_vec3 U = METAENGINE_MATH_PREFIX(Cross)(S, F);

    Result.Elements[0][0] = S.X;
    Result.Elements[0][1] = U.X;
    Result.Elements[0][2] = -F.X;
    Result.Elements[0][3] = 0.0f;

    Result.Elements[1][0] = S.Y;
    Result.Elements[1][1] = U.Y;
    Result.Elements[1][2] = -F.Y;
    Result.Elements[1][3] = 0.0f;

    Result.Elements[2][0] = S.Z;
    Result.Elements[2][1] = U.Z;
    Result.Elements[2][2] = -F.Z;
    Result.Elements[2][3] = 0.0f;

    Result.Elements[3][0] = -METAENGINE_MATH_PREFIX(DotVec3)(S, Eye);
    Result.Elements[3][1] = -METAENGINE_MATH_PREFIX(DotVec3)(U, Eye);
    Result.Elements[3][2] = METAENGINE_MATH_PREFIX(DotVec3)(F, Eye);
    Result.Elements[3][3] = 1.0f;

    return (Result);
}

/*
 * Quaternion operations
 */

COVERAGE(METAENGINE_MATH_Quaternion, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(Quaternion)(float X, float Y, float Z, float W) {
    ASSERT_COVERED(METAENGINE_MATH_Quaternion);

    metadot_quaternion Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = _mm_setr_ps(X, Y, Z, W);
#else
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    Result.W = W;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_QuaternionV4, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(QuaternionV4)(metadot_vec4 Vector) {
    ASSERT_COVERED(METAENGINE_MATH_QuaternionV4);

    metadot_quaternion Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = Vector.InternalElementsSSE;
#else
    Result.X = Vector.X;
    Result.Y = Vector.Y;
    Result.Z = Vector.Z;
    Result.W = Vector.W;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddQuaternion, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(AddQuaternion)(metadot_quaternion Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddQuaternion);

    metadot_quaternion Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = _mm_add_ps(Left.InternalElementsSSE, Right.InternalElementsSSE);
#else

    Result.X = Left.X + Right.X;
    Result.Y = Left.Y + Right.Y;
    Result.Z = Left.Z + Right.Z;
    Result.W = Left.W + Right.W;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractQuaternion, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(SubtractQuaternion)(metadot_quaternion Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractQuaternion);

    metadot_quaternion Result;

#ifdef METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = _mm_sub_ps(Left.InternalElementsSSE, Right.InternalElementsSSE);
#else

    Result.X = Left.X - Right.X;
    Result.Y = Left.Y - Right.Y;
    Result.Z = Left.Z - Right.Z;
    Result.W = Left.W - Right.W;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyQuaternion, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(MultiplyQuaternion)(metadot_quaternion Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyQuaternion);

    metadot_quaternion Result;

#ifdef METAENGINE_MATH__USE_SSE
    __m128 SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.InternalElementsSSE, Left.InternalElementsSSE, _MM_SHUFFLE(0, 0, 0, 0)), _mm_setr_ps(0.f, -0.f, 0.f, -0.f));
    __m128 SSEResultTwo = _mm_shuffle_ps(Right.InternalElementsSSE, Right.InternalElementsSSE, _MM_SHUFFLE(0, 1, 2, 3));
    __m128 SSEResultThree = _mm_mul_ps(SSEResultTwo, SSEResultOne);

    SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.InternalElementsSSE, Left.InternalElementsSSE, _MM_SHUFFLE(1, 1, 1, 1)), _mm_setr_ps(0.f, 0.f, -0.f, -0.f));
    SSEResultTwo = _mm_shuffle_ps(Right.InternalElementsSSE, Right.InternalElementsSSE, _MM_SHUFFLE(1, 0, 3, 2));
    SSEResultThree = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

    SSEResultOne = _mm_xor_ps(_mm_shuffle_ps(Left.InternalElementsSSE, Left.InternalElementsSSE, _MM_SHUFFLE(2, 2, 2, 2)), _mm_setr_ps(-0.f, 0.f, 0.f, -0.f));
    SSEResultTwo = _mm_shuffle_ps(Right.InternalElementsSSE, Right.InternalElementsSSE, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultThree = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));

    SSEResultOne = _mm_shuffle_ps(Left.InternalElementsSSE, Left.InternalElementsSSE, _MM_SHUFFLE(3, 3, 3, 3));
    SSEResultTwo = _mm_shuffle_ps(Right.InternalElementsSSE, Right.InternalElementsSSE, _MM_SHUFFLE(3, 2, 1, 0));
    Result.InternalElementsSSE = _mm_add_ps(SSEResultThree, _mm_mul_ps(SSEResultTwo, SSEResultOne));
#else
    Result.X = (Left.X * Right.W) + (Left.Y * Right.Z) - (Left.Z * Right.Y) + (Left.W * Right.X);
    Result.Y = (-Left.X * Right.Z) + (Left.Y * Right.W) + (Left.Z * Right.X) + (Left.W * Right.Y);
    Result.Z = (Left.X * Right.Y) - (Left.Y * Right.X) + (Left.Z * Right.W) + (Left.W * Right.Z);
    Result.W = (-Left.X * Right.X) - (Left.Y * Right.Y) - (Left.Z * Right.Z) + (Left.W * Right.W);
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyQuaternionF, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(MultiplyQuaternionF)(metadot_quaternion Left, float Multiplicative) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyQuaternionF);

    metadot_quaternion Result;

#ifdef METAENGINE_MATH__USE_SSE
    __m128 Scalar = _mm_set1_ps(Multiplicative);
    Result.InternalElementsSSE = _mm_mul_ps(Left.InternalElementsSSE, Scalar);
#else
    Result.X = Left.X * Multiplicative;
    Result.Y = Left.Y * Multiplicative;
    Result.Z = Left.Z * Multiplicative;
    Result.W = Left.W * Multiplicative;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideQuaternionF, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(DivideQuaternionF)(metadot_quaternion Left, float Dividend) {
    ASSERT_COVERED(METAENGINE_MATH_DivideQuaternionF);

    metadot_quaternion Result;

#ifdef METAENGINE_MATH__USE_SSE
    __m128 Scalar = _mm_set1_ps(Dividend);
    Result.InternalElementsSSE = _mm_div_ps(Left.InternalElementsSSE, Scalar);
#else
    Result.X = Left.X / Dividend;
    Result.Y = Left.Y / Dividend;
    Result.Z = Left.Z / Dividend;
    Result.W = Left.W / Dividend;
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_DotQuaternion, 1)
static_inline float METAENGINE_MATH_PREFIX(DotQuaternion)(metadot_quaternion Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_DotQuaternion);

    float Result;

#ifdef METAENGINE_MATH__USE_SSE
    __m128 SSEResultOne = _mm_mul_ps(Left.InternalElementsSSE, Right.InternalElementsSSE);
    __m128 SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(2, 3, 0, 1));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    SSEResultTwo = _mm_shuffle_ps(SSEResultOne, SSEResultOne, _MM_SHUFFLE(0, 1, 2, 3));
    SSEResultOne = _mm_add_ps(SSEResultOne, SSEResultTwo);
    _mm_store_ss(&Result, SSEResultOne);
#else
    Result = (Left.X * Right.X) + (Left.Y * Right.Y) + (Left.Z * Right.Z) + (Left.W * Right.W);
#endif

    return (Result);
}

COVERAGE(METAENGINE_MATH_InverseQuaternion, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(InverseQuaternion)(metadot_quaternion Left) {
    ASSERT_COVERED(METAENGINE_MATH_InverseQuaternion);

    metadot_quaternion Result;

    Result.X = -Left.X;
    Result.Y = -Left.Y;
    Result.Z = -Left.Z;
    Result.W = Left.W;

    Result = METAENGINE_MATH_PREFIX(DivideQuaternionF)(Result, (METAENGINE_MATH_PREFIX(DotQuaternion)(Left, Left)));

    return (Result);
}

COVERAGE(METAENGINE_MATH_NormalizeQuaternion, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(NormalizeQuaternion)(metadot_quaternion Left) {
    ASSERT_COVERED(METAENGINE_MATH_NormalizeQuaternion);

    metadot_quaternion Result;

    float Length = METAENGINE_MATH_PREFIX(SquareRootF)(METAENGINE_MATH_PREFIX(DotQuaternion)(Left, Left));
    Result = METAENGINE_MATH_PREFIX(DivideQuaternionF)(Left, Length);

    return (Result);
}

COVERAGE(METAENGINE_MATH_NLerp, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(NLerp)(metadot_quaternion Left, float Time, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_NLerp);

    metadot_quaternion Result;

#ifdef METAENGINE_MATH__USE_SSE
    __m128 ScalarLeft = _mm_set1_ps(1.0f - Time);
    __m128 ScalarRight = _mm_set1_ps(Time);
    __m128 SSEResultOne = _mm_mul_ps(Left.InternalElementsSSE, ScalarLeft);
    __m128 SSEResultTwo = _mm_mul_ps(Right.InternalElementsSSE, ScalarRight);
    Result.InternalElementsSSE = _mm_add_ps(SSEResultOne, SSEResultTwo);
#else
    Result.X = METAENGINE_MATH_PREFIX(Lerp)(Left.X, Time, Right.X);
    Result.Y = METAENGINE_MATH_PREFIX(Lerp)(Left.Y, Time, Right.Y);
    Result.Z = METAENGINE_MATH_PREFIX(Lerp)(Left.Z, Time, Right.Z);
    Result.W = METAENGINE_MATH_PREFIX(Lerp)(Left.W, Time, Right.W);
#endif
    Result = METAENGINE_MATH_PREFIX(NormalizeQuaternion)(Result);

    return (Result);
}

COVERAGE(METAENGINE_MATH_Slerp, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(Slerp)(metadot_quaternion Left, float Time, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_Slerp);

    metadot_quaternion Result;
    metadot_quaternion QuaternionLeft;
    metadot_quaternion QuaternionRight;

    float Cos_Theta = METAENGINE_MATH_PREFIX(DotQuaternion)(Left, Right);
    float Angle = METAENGINE_MATH_PREFIX(ACosF)(Cos_Theta);

    float S1 = METAENGINE_MATH_PREFIX(SinF)((1.0f - Time) * Angle);
    float S2 = METAENGINE_MATH_PREFIX(SinF)(Time * Angle);
    float Is = 1.0f / METAENGINE_MATH_PREFIX(SinF)(Angle);

    QuaternionLeft = METAENGINE_MATH_PREFIX(MultiplyQuaternionF)(Left, S1);
    QuaternionRight = METAENGINE_MATH_PREFIX(MultiplyQuaternionF)(Right, S2);

    Result = METAENGINE_MATH_PREFIX(AddQuaternion)(QuaternionLeft, QuaternionRight);
    Result = METAENGINE_MATH_PREFIX(MultiplyQuaternionF)(Result, Is);

    return (Result);
}

COVERAGE(METAENGINE_MATH_QuaternionToMat4, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(QuaternionToMat4)(metadot_quaternion Left) {
    ASSERT_COVERED(METAENGINE_MATH_QuaternionToMat4);

    metadot_mat4 Result;

    metadot_quaternion NormalizedQuaternion = METAENGINE_MATH_PREFIX(NormalizeQuaternion)(Left);

    float XX, YY, ZZ, XY, XZ, YZ, WX, WY, WZ;

    XX = NormalizedQuaternion.X * NormalizedQuaternion.X;
    YY = NormalizedQuaternion.Y * NormalizedQuaternion.Y;
    ZZ = NormalizedQuaternion.Z * NormalizedQuaternion.Z;
    XY = NormalizedQuaternion.X * NormalizedQuaternion.Y;
    XZ = NormalizedQuaternion.X * NormalizedQuaternion.Z;
    YZ = NormalizedQuaternion.Y * NormalizedQuaternion.Z;
    WX = NormalizedQuaternion.W * NormalizedQuaternion.X;
    WY = NormalizedQuaternion.W * NormalizedQuaternion.Y;
    WZ = NormalizedQuaternion.W * NormalizedQuaternion.Z;

    Result.Elements[0][0] = 1.0f - 2.0f * (YY + ZZ);
    Result.Elements[0][1] = 2.0f * (XY + WZ);
    Result.Elements[0][2] = 2.0f * (XZ - WY);
    Result.Elements[0][3] = 0.0f;

    Result.Elements[1][0] = 2.0f * (XY - WZ);
    Result.Elements[1][1] = 1.0f - 2.0f * (XX + ZZ);
    Result.Elements[1][2] = 2.0f * (YZ + WX);
    Result.Elements[1][3] = 0.0f;

    Result.Elements[2][0] = 2.0f * (XZ + WY);
    Result.Elements[2][1] = 2.0f * (YZ - WX);
    Result.Elements[2][2] = 1.0f - 2.0f * (XX + YY);
    Result.Elements[2][3] = 0.0f;

    Result.Elements[3][0] = 0.0f;
    Result.Elements[3][1] = 0.0f;
    Result.Elements[3][2] = 0.0f;
    Result.Elements[3][3] = 1.0f;

    return (Result);
}

// This method taken from Mike Day at Insomniac Games.
// https://d3cw3dd2w32x2b.cloudfront.net/wp-content/uploads/2015/01/matrix-to-quat.pdf
//
// Note that as mentioned at the top of the paper, the paper assumes the matrix
// would be *post*-multiplied to a vector to rotate it, meaning the matrix is
// the transpose of what we're dealing with. But, because our matrices are
// stored in column-major order, the indices *appear* to match the paper.
//
// For example, m12 in the paper is row 1, column 2. We need to transpose it to
// row 2, column 1. But, because the column comes first when referencing
// elements, it looks like M.Elements[1][2].
//
// Don't be confused! Or if you must be confused, at least trust this
// comment. :)
COVERAGE(METAENGINE_MATH_Mat4ToQuaternion, 4)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(Mat4ToQuaternion)(metadot_mat4 M) {
    float T;
    metadot_quaternion Q;

    if (M.Elements[2][2] < 0.0f) {
        if (M.Elements[0][0] > M.Elements[1][1]) {
            ASSERT_COVERED(METAENGINE_MATH_Mat4ToQuaternion);

            T = 1 + M.Elements[0][0] - M.Elements[1][1] - M.Elements[2][2];
            Q = METAENGINE_MATH_PREFIX(Quaternion)(T, M.Elements[0][1] + M.Elements[1][0], M.Elements[2][0] + M.Elements[0][2], M.Elements[1][2] - M.Elements[2][1]);
        } else {
            ASSERT_COVERED(METAENGINE_MATH_Mat4ToQuaternion);

            T = 1 - M.Elements[0][0] + M.Elements[1][1] - M.Elements[2][2];
            Q = METAENGINE_MATH_PREFIX(Quaternion)(M.Elements[0][1] + M.Elements[1][0], T, M.Elements[1][2] + M.Elements[2][1], M.Elements[2][0] - M.Elements[0][2]);
        }
    } else {
        if (M.Elements[0][0] < -M.Elements[1][1]) {
            ASSERT_COVERED(METAENGINE_MATH_Mat4ToQuaternion);

            T = 1 - M.Elements[0][0] - M.Elements[1][1] + M.Elements[2][2];
            Q = METAENGINE_MATH_PREFIX(Quaternion)(M.Elements[2][0] + M.Elements[0][2], M.Elements[1][2] + M.Elements[2][1], T, M.Elements[0][1] - M.Elements[1][0]);
        } else {
            ASSERT_COVERED(METAENGINE_MATH_Mat4ToQuaternion);

            T = 1 + M.Elements[0][0] + M.Elements[1][1] + M.Elements[2][2];
            Q = METAENGINE_MATH_PREFIX(Quaternion)(M.Elements[1][2] - M.Elements[2][1], M.Elements[2][0] - M.Elements[0][2], M.Elements[0][1] - M.Elements[1][0], T);
        }
    }

    Q = METAENGINE_MATH_PREFIX(MultiplyQuaternionF)(Q, 0.5f / METAENGINE_MATH_PREFIX(SquareRootF)(T));

    return Q;
}

COVERAGE(METAENGINE_MATH_QuaternionFromAxisAngle, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(QuaternionFromAxisAngle)(metadot_vec3 Axis, float AngleOfRotation) {
    ASSERT_COVERED(METAENGINE_MATH_QuaternionFromAxisAngle);

    metadot_quaternion Result;

    metadot_vec3 AxisNormalized = METAENGINE_MATH_PREFIX(NormalizeVec3)(Axis);
    float SineOfRotation = METAENGINE_MATH_PREFIX(SinF)(AngleOfRotation / 2.0f);

    Result.XYZ = METAENGINE_MATH_PREFIX(MultiplyVec3f)(AxisNormalized, SineOfRotation);
    Result.W = METAENGINE_MATH_PREFIX(CosF)(AngleOfRotation / 2.0f);

    return (Result);
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

COVERAGE(METAENGINE_MATH_LengthVec2CPP, 1)
static_inline float METAENGINE_MATH_PREFIX(Length)(metadot_vec2 A) {
    ASSERT_COVERED(METAENGINE_MATH_LengthVec2CPP);

    float Result = METAENGINE_MATH_PREFIX(LengthVec2)(A);
    return (Result);
}

COVERAGE(METAENGINE_MATH_LengthVec3CPP, 1)
static_inline float METAENGINE_MATH_PREFIX(Length)(metadot_vec3 A) {
    ASSERT_COVERED(METAENGINE_MATH_LengthVec3CPP);

    float Result = METAENGINE_MATH_PREFIX(LengthVec3)(A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_LengthVec4CPP, 1)
static_inline float METAENGINE_MATH_PREFIX(Length)(metadot_vec4 A) {
    ASSERT_COVERED(METAENGINE_MATH_LengthVec4CPP);

    float Result = METAENGINE_MATH_PREFIX(LengthVec4)(A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_LengthSquaredVec2CPP, 1)
static_inline float METAENGINE_MATH_PREFIX(LengthSquared)(metadot_vec2 A) {
    ASSERT_COVERED(METAENGINE_MATH_LengthSquaredVec2CPP);

    float Result = METAENGINE_MATH_PREFIX(LengthSquaredVec2)(A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_LengthSquaredVec3CPP, 1)
static_inline float METAENGINE_MATH_PREFIX(LengthSquared)(metadot_vec3 A) {
    ASSERT_COVERED(METAENGINE_MATH_LengthSquaredVec3CPP);

    float Result = METAENGINE_MATH_PREFIX(LengthSquaredVec3)(A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_LengthSquaredVec4CPP, 1)
static_inline float METAENGINE_MATH_PREFIX(LengthSquared)(metadot_vec4 A) {
    ASSERT_COVERED(METAENGINE_MATH_LengthSquaredVec4CPP);

    float Result = METAENGINE_MATH_PREFIX(LengthSquaredVec4)(A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_NormalizeVec2CPP, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(Normalize)(metadot_vec2 A) {
    ASSERT_COVERED(METAENGINE_MATH_NormalizeVec2CPP);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(NormalizeVec2)(A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_NormalizeVec3CPP, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(Normalize)(metadot_vec3 A) {
    ASSERT_COVERED(METAENGINE_MATH_NormalizeVec3CPP);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(NormalizeVec3)(A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_NormalizeVec4CPP, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(Normalize)(metadot_vec4 A) {
    ASSERT_COVERED(METAENGINE_MATH_NormalizeVec4CPP);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(NormalizeVec4)(A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_FastNormalizeVec2CPP, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(FastNormalize)(metadot_vec2 A) {
    ASSERT_COVERED(METAENGINE_MATH_FastNormalizeVec2CPP);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(FastNormalizeVec2)(A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_FastNormalizeVec3CPP, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(FastNormalize)(metadot_vec3 A) {
    ASSERT_COVERED(METAENGINE_MATH_FastNormalizeVec3CPP);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(FastNormalizeVec3)(A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_FastNormalizeVec4CPP, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(FastNormalize)(metadot_vec4 A) {
    ASSERT_COVERED(METAENGINE_MATH_FastNormalizeVec4CPP);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(FastNormalizeVec4)(A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_NormalizeQuaternionCPP, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(Normalize)(metadot_quaternion A) {
    ASSERT_COVERED(METAENGINE_MATH_NormalizeQuaternionCPP);

    metadot_quaternion Result = METAENGINE_MATH_PREFIX(NormalizeQuaternion)(A);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DotVec2CPP, 1)
static_inline float METAENGINE_MATH_PREFIX(Dot)(metadot_vec2 VecOne, metadot_vec2 VecTwo) {
    ASSERT_COVERED(METAENGINE_MATH_DotVec2CPP);

    float Result = METAENGINE_MATH_PREFIX(DotVec2)(VecOne, VecTwo);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DotVec3CPP, 1)
static_inline float METAENGINE_MATH_PREFIX(Dot)(metadot_vec3 VecOne, metadot_vec3 VecTwo) {
    ASSERT_COVERED(METAENGINE_MATH_DotVec3CPP);

    float Result = METAENGINE_MATH_PREFIX(DotVec3)(VecOne, VecTwo);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DotVec4CPP, 1)
static_inline float METAENGINE_MATH_PREFIX(Dot)(metadot_vec4 VecOne, metadot_vec4 VecTwo) {
    ASSERT_COVERED(METAENGINE_MATH_DotVec4CPP);

    float Result = METAENGINE_MATH_PREFIX(DotVec4)(VecOne, VecTwo);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DotQuaternionCPP, 1)
static_inline float METAENGINE_MATH_PREFIX(Dot)(metadot_quaternion QuatOne, metadot_quaternion QuatTwo) {
    ASSERT_COVERED(METAENGINE_MATH_DotQuaternionCPP);

    float Result = METAENGINE_MATH_PREFIX(DotQuaternion)(QuatOne, QuatTwo);

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddVec2CPP, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(Add)(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddVec2CPP);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(AddVec2)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddVec3CPP, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(Add)(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddVec3CPP);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(AddVec3)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddVec4CPP, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(Add)(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddVec4CPP);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(AddVec4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddMat4CPP, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Add)(metadot_mat4 Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddMat4CPP);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(AddMat4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddQuaternionCPP, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(Add)(metadot_quaternion Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddQuaternionCPP);

    metadot_quaternion Result = METAENGINE_MATH_PREFIX(AddQuaternion)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractVec2CPP, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(Subtract)(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractVec2CPP);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(SubtractVec2)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractVec3CPP, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(Subtract)(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractVec3CPP);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(SubtractVec3)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractVec4CPP, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(Subtract)(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractVec4CPP);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(SubtractVec4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractMat4CPP, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Subtract)(metadot_mat4 Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractMat4CPP);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(SubtractMat4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractQuaternionCPP, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(Subtract)(metadot_quaternion Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractQuaternionCPP);

    metadot_quaternion Result = METAENGINE_MATH_PREFIX(SubtractQuaternion)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec2CPP, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(Multiply)(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec2CPP);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(MultiplyVec2)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec2fCPP, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(Multiply)(metadot_vec2 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec2fCPP);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(MultiplyVec2f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec3CPP, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(Multiply)(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec3CPP);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(MultiplyVec3)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec3fCPP, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(Multiply)(metadot_vec3 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec3fCPP);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(MultiplyVec3f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec4CPP, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(Multiply)(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec4CPP);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(MultiplyVec4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec4fCPP, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(Multiply)(metadot_vec4 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec4fCPP);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(MultiplyVec4f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyMat4CPP, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Multiply)(metadot_mat4 Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyMat4CPP);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(MultiplyMat4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyMat4fCPP, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Multiply)(metadot_mat4 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyMat4fCPP);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(MultiplyMat4f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyMat4ByVec4CPP, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(Multiply)(metadot_mat4 Matrix, metadot_vec4 Vector) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyMat4ByVec4CPP);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(MultiplyMat4ByVec4)(Matrix, Vector);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyQuaternionCPP, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(Multiply)(metadot_quaternion Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyQuaternionCPP);

    metadot_quaternion Result = METAENGINE_MATH_PREFIX(MultiplyQuaternion)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyQuaternionFCPP, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(Multiply)(metadot_quaternion Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyQuaternionFCPP);

    metadot_quaternion Result = METAENGINE_MATH_PREFIX(MultiplyQuaternionF)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec2CPP, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(Divide)(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec2CPP);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(DivideVec2)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec2fCPP, 1)
static_inline metadot_vec2 METAENGINE_MATH_PREFIX(Divide)(metadot_vec2 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec2fCPP);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(DivideVec2f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec3CPP, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(Divide)(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec3CPP);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(DivideVec3)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec3fCPP, 1)
static_inline metadot_vec3 METAENGINE_MATH_PREFIX(Divide)(metadot_vec3 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec3fCPP);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(DivideVec3f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec4CPP, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(Divide)(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec4CPP);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(DivideVec4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec4fCPP, 1)
static_inline metadot_vec4 METAENGINE_MATH_PREFIX(Divide)(metadot_vec4 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec4fCPP);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(DivideVec4f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideMat4fCPP, 1)
static_inline metadot_mat4 METAENGINE_MATH_PREFIX(Divide)(metadot_mat4 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideMat4fCPP);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(DivideMat4f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideQuaternionFCPP, 1)
static_inline metadot_quaternion METAENGINE_MATH_PREFIX(Divide)(metadot_quaternion Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideQuaternionFCPP);

    metadot_quaternion Result = METAENGINE_MATH_PREFIX(DivideQuaternionF)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_EqualsVec2CPP, 1)
static_inline metadot_bool METAENGINE_MATH_PREFIX(Equals)(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_EqualsVec2CPP);

    metadot_bool Result = METAENGINE_MATH_PREFIX(EqualsVec2)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_EqualsVec3CPP, 1)
static_inline metadot_bool METAENGINE_MATH_PREFIX(Equals)(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_EqualsVec3CPP);

    metadot_bool Result = METAENGINE_MATH_PREFIX(EqualsVec3)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_EqualsVec4CPP, 1)
static_inline metadot_bool METAENGINE_MATH_PREFIX(Equals)(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_EqualsVec4CPP);

    metadot_bool Result = METAENGINE_MATH_PREFIX(EqualsVec4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddVec2Op, 1)
static_inline metadot_vec2 operator+(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddVec2Op);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(AddVec2)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddVec3Op, 1)
static_inline metadot_vec3 operator+(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddVec3Op);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(AddVec3)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddVec4Op, 1)
static_inline metadot_vec4 operator+(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddVec4Op);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(AddVec4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddMat4Op, 1)
static_inline metadot_mat4 operator+(metadot_mat4 Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddMat4Op);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(AddMat4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddQuaternionOp, 1)
static_inline metadot_quaternion operator+(metadot_quaternion Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddQuaternionOp);

    metadot_quaternion Result = METAENGINE_MATH_PREFIX(AddQuaternion)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractVec2Op, 1)
static_inline metadot_vec2 operator-(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractVec2Op);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(SubtractVec2)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractVec3Op, 1)
static_inline metadot_vec3 operator-(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractVec3Op);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(SubtractVec3)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractVec4Op, 1)
static_inline metadot_vec4 operator-(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractVec4Op);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(SubtractVec4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractMat4Op, 1)
static_inline metadot_mat4 operator-(metadot_mat4 Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractMat4Op);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(SubtractMat4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_SubtractQuaternionOp, 1)
static_inline metadot_quaternion operator-(metadot_quaternion Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractQuaternionOp);

    metadot_quaternion Result = METAENGINE_MATH_PREFIX(SubtractQuaternion)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec2Op, 1)
static_inline metadot_vec2 operator*(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec2Op);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(MultiplyVec2)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec3Op, 1)
static_inline metadot_vec3 operator*(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec3Op);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(MultiplyVec3)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec4Op, 1)
static_inline metadot_vec4 operator*(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec4Op);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(MultiplyVec4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyMat4Op, 1)
static_inline metadot_mat4 operator*(metadot_mat4 Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyMat4Op);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(MultiplyMat4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyQuaternionOp, 1)
static_inline metadot_quaternion operator*(metadot_quaternion Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyQuaternionOp);

    metadot_quaternion Result = METAENGINE_MATH_PREFIX(MultiplyQuaternion)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec2fOp, 1)
static_inline metadot_vec2 operator*(metadot_vec2 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec2fOp);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(MultiplyVec2f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec3fOp, 1)
static_inline metadot_vec3 operator*(metadot_vec3 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec3fOp);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(MultiplyVec3f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec4fOp, 1)
static_inline metadot_vec4 operator*(metadot_vec4 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec4fOp);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(MultiplyVec4f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyMat4fOp, 1)
static_inline metadot_mat4 operator*(metadot_mat4 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyMat4fOp);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(MultiplyMat4f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyQuaternionFOp, 1)
static_inline metadot_quaternion operator*(metadot_quaternion Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyQuaternionFOp);

    metadot_quaternion Result = METAENGINE_MATH_PREFIX(MultiplyQuaternionF)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec2fOpLeft, 1)
static_inline metadot_vec2 operator*(float Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec2fOpLeft);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(MultiplyVec2f)(Right, Left);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec3fOpLeft, 1)
static_inline metadot_vec3 operator*(float Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec3fOpLeft);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(MultiplyVec3f)(Right, Left);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyVec4fOpLeft, 1)
static_inline metadot_vec4 operator*(float Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec4fOpLeft);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(MultiplyVec4f)(Right, Left);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyMat4fOpLeft, 1)
static_inline metadot_mat4 operator*(float Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyMat4fOpLeft);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(MultiplyMat4f)(Right, Left);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyQuaternionFOpLeft, 1)
static_inline metadot_quaternion operator*(float Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyQuaternionFOpLeft);

    metadot_quaternion Result = METAENGINE_MATH_PREFIX(MultiplyQuaternionF)(Right, Left);

    return (Result);
}

COVERAGE(METAENGINE_MATH_MultiplyMat4ByVec4Op, 1)
static_inline metadot_vec4 operator*(metadot_mat4 Matrix, metadot_vec4 Vector) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyMat4ByVec4Op);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(MultiplyMat4ByVec4)(Matrix, Vector);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec2Op, 1)
static_inline metadot_vec2 operator/(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec2Op);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(DivideVec2)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec3Op, 1)
static_inline metadot_vec3 operator/(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec3Op);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(DivideVec3)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec4Op, 1)
static_inline metadot_vec4 operator/(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec4Op);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(DivideVec4)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec2fOp, 1)
static_inline metadot_vec2 operator/(metadot_vec2 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec2fOp);

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(DivideVec2f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec3fOp, 1)
static_inline metadot_vec3 operator/(metadot_vec3 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec3fOp);

    metadot_vec3 Result = METAENGINE_MATH_PREFIX(DivideVec3f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideVec4fOp, 1)
static_inline metadot_vec4 operator/(metadot_vec4 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec4fOp);

    metadot_vec4 Result = METAENGINE_MATH_PREFIX(DivideVec4f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideMat4fOp, 1)
static_inline metadot_mat4 operator/(metadot_mat4 Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideMat4fOp);

    metadot_mat4 Result = METAENGINE_MATH_PREFIX(DivideMat4f)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_DivideQuaternionFOp, 1)
static_inline metadot_quaternion operator/(metadot_quaternion Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideQuaternionFOp);

    metadot_quaternion Result = METAENGINE_MATH_PREFIX(DivideQuaternionF)(Left, Right);

    return (Result);
}

COVERAGE(METAENGINE_MATH_AddVec2Assign, 1)
static_inline metadot_vec2 &operator+=(metadot_vec2 &Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddVec2Assign);

    return (Left = Left + Right);
}

COVERAGE(METAENGINE_MATH_AddVec3Assign, 1)
static_inline metadot_vec3 &operator+=(metadot_vec3 &Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddVec3Assign);

    return (Left = Left + Right);
}

COVERAGE(METAENGINE_MATH_AddVec4Assign, 1)
static_inline metadot_vec4 &operator+=(metadot_vec4 &Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddVec4Assign);

    return (Left = Left + Right);
}

COVERAGE(METAENGINE_MATH_AddMat4Assign, 1)
static_inline metadot_mat4 &operator+=(metadot_mat4 &Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddMat4Assign);

    return (Left = Left + Right);
}

COVERAGE(METAENGINE_MATH_AddQuaternionAssign, 1)
static_inline metadot_quaternion &operator+=(metadot_quaternion &Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_AddQuaternionAssign);

    return (Left = Left + Right);
}

COVERAGE(METAENGINE_MATH_SubtractVec2Assign, 1)
static_inline metadot_vec2 &operator-=(metadot_vec2 &Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractVec2Assign);

    return (Left = Left - Right);
}

COVERAGE(METAENGINE_MATH_SubtractVec3Assign, 1)
static_inline metadot_vec3 &operator-=(metadot_vec3 &Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractVec3Assign);

    return (Left = Left - Right);
}

COVERAGE(METAENGINE_MATH_SubtractVec4Assign, 1)
static_inline metadot_vec4 &operator-=(metadot_vec4 &Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractVec4Assign);

    return (Left = Left - Right);
}

COVERAGE(METAENGINE_MATH_SubtractMat4Assign, 1)
static_inline metadot_mat4 &operator-=(metadot_mat4 &Left, metadot_mat4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractMat4Assign);

    return (Left = Left - Right);
}

COVERAGE(METAENGINE_MATH_SubtractQuaternionAssign, 1)
static_inline metadot_quaternion &operator-=(metadot_quaternion &Left, metadot_quaternion Right) {
    ASSERT_COVERED(METAENGINE_MATH_SubtractQuaternionAssign);

    return (Left = Left - Right);
}

COVERAGE(METAENGINE_MATH_MultiplyVec2Assign, 1)
static_inline metadot_vec2 &operator*=(metadot_vec2 &Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec2Assign);

    return (Left = Left * Right);
}

COVERAGE(METAENGINE_MATH_MultiplyVec3Assign, 1)
static_inline metadot_vec3 &operator*=(metadot_vec3 &Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec3Assign);

    return (Left = Left * Right);
}

COVERAGE(METAENGINE_MATH_MultiplyVec4Assign, 1)
static_inline metadot_vec4 &operator*=(metadot_vec4 &Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec4Assign);

    return (Left = Left * Right);
}

COVERAGE(METAENGINE_MATH_MultiplyVec2fAssign, 1)
static_inline metadot_vec2 &operator*=(metadot_vec2 &Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec2fAssign);

    return (Left = Left * Right);
}

COVERAGE(METAENGINE_MATH_MultiplyVec3fAssign, 1)
static_inline metadot_vec3 &operator*=(metadot_vec3 &Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec3fAssign);

    return (Left = Left * Right);
}

COVERAGE(METAENGINE_MATH_MultiplyVec4fAssign, 1)
static_inline metadot_vec4 &operator*=(metadot_vec4 &Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyVec4fAssign);

    return (Left = Left * Right);
}

COVERAGE(METAENGINE_MATH_MultiplyMat4fAssign, 1)
static_inline metadot_mat4 &operator*=(metadot_mat4 &Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyMat4fAssign);

    return (Left = Left * Right);
}

COVERAGE(METAENGINE_MATH_MultiplyQuaternionFAssign, 1)
static_inline metadot_quaternion &operator*=(metadot_quaternion &Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_MultiplyQuaternionFAssign);

    return (Left = Left * Right);
}

COVERAGE(METAENGINE_MATH_DivideVec2Assign, 1)
static_inline metadot_vec2 &operator/=(metadot_vec2 &Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec2Assign);

    return (Left = Left / Right);
}

COVERAGE(METAENGINE_MATH_DivideVec3Assign, 1)
static_inline metadot_vec3 &operator/=(metadot_vec3 &Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec3Assign);

    return (Left = Left / Right);
}

COVERAGE(METAENGINE_MATH_DivideVec4Assign, 1)
static_inline metadot_vec4 &operator/=(metadot_vec4 &Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec4Assign);

    return (Left = Left / Right);
}

COVERAGE(METAENGINE_MATH_DivideVec2fAssign, 1)
static_inline metadot_vec2 &operator/=(metadot_vec2 &Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec2fAssign);

    return (Left = Left / Right);
}

COVERAGE(METAENGINE_MATH_DivideVec3fAssign, 1)
static_inline metadot_vec3 &operator/=(metadot_vec3 &Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec3fAssign);

    return (Left = Left / Right);
}

COVERAGE(METAENGINE_MATH_DivideVec4fAssign, 1)
static_inline metadot_vec4 &operator/=(metadot_vec4 &Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideVec4fAssign);

    return (Left = Left / Right);
}

COVERAGE(METAENGINE_MATH_DivideMat4fAssign, 1)
static_inline metadot_mat4 &operator/=(metadot_mat4 &Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideMat4fAssign);

    return (Left = Left / Right);
}

COVERAGE(METAENGINE_MATH_DivideQuaternionFAssign, 1)
static_inline metadot_quaternion &operator/=(metadot_quaternion &Left, float Right) {
    ASSERT_COVERED(METAENGINE_MATH_DivideQuaternionFAssign);

    return (Left = Left / Right);
}

COVERAGE(METAENGINE_MATH_EqualsVec2Op, 1)
static_inline metadot_bool operator==(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_EqualsVec2Op);

    return METAENGINE_MATH_PREFIX(EqualsVec2)(Left, Right);
}

COVERAGE(METAENGINE_MATH_EqualsVec3Op, 1)
static_inline metadot_bool operator==(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_EqualsVec3Op);

    return METAENGINE_MATH_PREFIX(EqualsVec3)(Left, Right);
}

COVERAGE(METAENGINE_MATH_EqualsVec4Op, 1)
static_inline metadot_bool operator==(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_EqualsVec4Op);

    return METAENGINE_MATH_PREFIX(EqualsVec4)(Left, Right);
}

COVERAGE(METAENGINE_MATH_EqualsVec2OpNot, 1)
static_inline metadot_bool operator!=(metadot_vec2 Left, metadot_vec2 Right) {
    ASSERT_COVERED(METAENGINE_MATH_EqualsVec2OpNot);

    return !METAENGINE_MATH_PREFIX(EqualsVec2)(Left, Right);
}

COVERAGE(METAENGINE_MATH_EqualsVec3OpNot, 1)
static_inline metadot_bool operator!=(metadot_vec3 Left, metadot_vec3 Right) {
    ASSERT_COVERED(METAENGINE_MATH_EqualsVec3OpNot);

    return !METAENGINE_MATH_PREFIX(EqualsVec3)(Left, Right);
}

COVERAGE(METAENGINE_MATH_EqualsVec4OpNot, 1)
static_inline metadot_bool operator!=(metadot_vec4 Left, metadot_vec4 Right) {
    ASSERT_COVERED(METAENGINE_MATH_EqualsVec4OpNot);

    return !METAENGINE_MATH_PREFIX(EqualsVec4)(Left, Right);
}

COVERAGE(METAENGINE_MATH_UnaryMinusVec2, 1)
static_inline metadot_vec2 operator-(metadot_vec2 In) {
    ASSERT_COVERED(METAENGINE_MATH_UnaryMinusVec2);

    metadot_vec2 Result;
    Result.X = -In.X;
    Result.Y = -In.Y;
    return (Result);
}

COVERAGE(METAENGINE_MATH_UnaryMinusVec3, 1)
static_inline metadot_vec3 operator-(metadot_vec3 In) {
    ASSERT_COVERED(METAENGINE_MATH_UnaryMinusVec3);

    metadot_vec3 Result;
    Result.X = -In.X;
    Result.Y = -In.Y;
    Result.Z = -In.Z;
    return (Result);
}

COVERAGE(METAENGINE_MATH_UnaryMinusVec4, 1)
static_inline metadot_vec4 operator-(metadot_vec4 In) {
    ASSERT_COVERED(METAENGINE_MATH_UnaryMinusVec4);

    metadot_vec4 Result;
#if METAENGINE_MATH__USE_SSE
    Result.InternalElementsSSE = _mm_xor_ps(In.InternalElementsSSE, _mm_set1_ps(-0.0f));
#else
    Result.X = -In.X;
    Result.Y = -In.Y;
    Result.Z = -In.Z;
    Result.W = -In.W;
#endif
    return (Result);
}

#endif /* __cplusplus */

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif