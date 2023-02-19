
#ifndef METAENGINE_MATH_H
#define METAENGINE_MATH_H

#include "core/core.h"

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
        metadot_vec2 pos;
        metadot_vec2 rect;
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

#ifdef __cplusplus
    inline float &operator[](const int &Index) { return Elements[Index]; }
#endif
} metadot_vec4;

typedef union metadot_mat4 {
    float Elements[4][4];

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

#ifndef METAENGINE_MATH_PREFIX
#define METAENGINE_MATH_PREFIX(name) METAENGINE_MATH_##name
#endif

static_inline metadot_vec2 METAENGINE_MATH_PREFIX(MultiplyVec2)(metadot_vec2 Left, metadot_vec2 Right) {

    metadot_vec2 Result;

    Result.X = Left.X * Right.X;
    Result.Y = Left.Y * Right.Y;

    return (Result);
}

static_inline metadot_vec2 METAENGINE_MATH_PREFIX(MultiplyVec2f)(metadot_vec2 Left, float Right) {

    metadot_vec2 Result;

    Result.X = Left.X * Right;
    Result.Y = Left.Y * Right;

    return (Result);
}

#ifdef __cplusplus

inline metadot_vec2 operator*(metadot_vec2 Left, metadot_vec2 Right) {

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(MultiplyVec2)(Left, Right);

    return (Result);
}

inline metadot_vec2 operator*(metadot_vec2 Left, float Right) {

    metadot_vec2 Result = METAENGINE_MATH_PREFIX(MultiplyVec2f)(Left, Right);

    return (Result);
}

#endif

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif