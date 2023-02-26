// Copyright(c) 2022-2023, KaoruXun All rights reserved.

// This source file may include
// Polypartition (MIT) by Ivan Fratric
// MarchingSquares(Tom Gibara) (MIT) by Juha Reunanen

#ifndef _METADOT_MATH_HPP_
#define _METADOT_MATH_HPP_

#include <cassert>
#include <list>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

#include "core/core.hpp"
#include "libs/imgui/imgui.h"

#ifdef METADOT_PLATFORM_WIN32
#undef far
#undef near
#endif

union vec2 {
    float v[2] = {};
    struct {
        float x, y;
    };
    struct {
        float w, h;
    };

    vec2(){};
    vec2(float _x, float _y) { x = _x, y = _y; };

    inline float &operator[](size_t i) { return v[i]; };
};

union vec3 {
    float v[3] = {};
    struct {
        float x, y, z;
    };
    struct {
        float w, h, d;
    };
    struct {
        float r, g, b;
    };

    vec3(){};
    vec3(float _x, float _y, float _z) { x = _x, y = _y, z = _z; };
    vec3(vec2 _xy, float _z) { x = _xy.x, y = _xy.y, z = _z; };
    vec3(float _x, vec3 _yz) { x = _x, y = _yz.x, z = _yz.y; };

    inline float &operator[](size_t i) { return v[i]; };
};

union vec4 {
    float v[4] = {};
    struct {
        float x, y, z, w;
    };
    struct {
        float r, g, b, a;
    };

    struct {
        vec2 pos;
        vec2 rect;
    };

    vec4(){};
    vec4(float _x, float _y, float _z, float _w) { x = _x, y = _y, z = _z, w = _w; };
    vec4(vec3 _xyz, float _w) { x = _xyz.x, y = _xyz.y, z = _xyz.z, w = _w; };
    vec4(float _x, vec3 _yzw) { x = _x, y = _yzw.x, z = _yzw.y, w = _yzw.z; };
    vec4(vec2 _xy, vec2 _zw) { x = _xy.x, y = _xy.y, z = _zw.x, w = _zw.y; };

    inline float &operator[](size_t i) { return v[i]; };
};

union mat3 {
    float m[3][3] = {};
    vec3 v[3];
    mat3(){};
    inline vec3 &operator[](size_t i) { return v[i]; };
};

union mat4 {
    float m[4][4] = {};
    vec4 v[4];
    mat4(){};
    inline vec4 &operator[](size_t i) { return v[i]; };
};

union quaternion {
    float q[4] = {};
    struct {
        float x, y, z, w;
    };

    quaternion(){};
    quaternion(float _x, float _y, float _z, float _w) { x = _x, y = _y, z = _z, w = _w; };
    quaternion(vec3 _xyz, float _w) { x = _xyz.x, y = _xyz.y, z = _xyz.z, w = _w; };
    quaternion(float _x, vec3 _yzw) { x = _x, y = _yzw.x, z = _yzw.y, w = _yzw.z; };
    quaternion(vec2 _xy, vec2 _zw) { x = _xy.x, y = _xy.y, z = _zw.x, w = _zw.y; };

    inline float operator[](size_t i) { return q[i]; };
};

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
    vec3 { 0.0f, 0.0f, 0.0f }
#define VECTOR3_FORWARD \
    vec3 { 1.0f, 0.0f, 0.0f }
#define VECTOR3_UP \
    vec3 { 0.0f, 0.0f, 1.0f }
#define VECTOR3_DOWN \
    vec3 { 0.0f, 0.0f, -1.0f }
#define VECTOR3_LEFT \
    vec3 { 0.0f, 1.0f, 0.0f }

#define INT_INFINITY 0x3f3f3f3f

// #define sign(x) (x > 0 ? 1 : (x < 0 ? -1 : 0))
#define UTIL_clamp(x, m, M) (x < m ? m : (x > M ? M : x))
#define FRAC0(x) (x - floorf(x))
#define FRAC1(x) (1 - x + floorf(x))

#define UTIL_cross(u, v) \
    (vec3) { (u).y *(v).z - (u).z *(v).y, (u).z *(v).x - (u).x *(v).z, (u).x *(v).y - (u).y *(v).x }
#define UTIL_dot(u, v) ((u).x * (v).x + (u).y * (v).y + (u).z * (v).z)
// #define norm(v) sqrt(dot(v, v))// norm = length of  vector

F32 math_perlin(F32 x, F32 y, F32 z, int x_wrap = 0, int y_wrap = 0, int z_wrap = 0);

static_inline ImVec4 vec4_to_imvec4(const vec4 &v4) { return {v4.x, v4.y, v4.z, v4.w}; }

template <class T>
constexpr T pi = T(3.1415926535897932385L);

#pragma region NewMATH

namespace NewMaths {

constexpr std::size_t hash_combine(std::size_t l, std::size_t r) noexcept { return l ^ (r + 0x9e3779b9 + (l << 6) + (l >> 2)); }

F32 vec22angle(vec2 v2);
F32 clamp(F32 input, F32 min, F32 max);
int rand_range(int min, int max);
uint64_t rand_XOR();
inline F64 random_double();
inline F64 random_double(F64 min, F64 max);
struct v2;
struct RandState;
F32 v2_distance_2Points(v2 A, v2 B);
v2 unitvec_AtoB(v2 A, v2 B);
F32 signed_angle_v2(v2 A, v2 B);
v2 Rotate2D(v2 P, F32 sine, F32 cosine);
v2 Rotate2D(v2 P, F32 Angle);
v2 Rotate2D(v2 p, v2 o, F32 angle);
v2 Reflection2D(v2 P, v2 N);
bool PointInRectangle(v2 P, v2 A, v2 B, v2 C);
int sign(F32 x);
static F32 dot(v2 A, v2 B);
static F32 perpdot(v2 A, v2 B);
static bool operator==(v2 A, v2 B);

vec3 NormalizeVector(vec3 v);
vec3 Add(vec3 a, vec3 b);
vec3 Subtract(vec3 a, vec3 b);
vec3 ScalarMult(vec3 v, F32 s);
F64 Distance(vec3 a, vec3 b);
vec3 VectorProjection(vec3 a, vec3 b);
vec3 Reflection(vec3 *v1, vec3 *v2);

F64 DistanceFromPointToLine2D(vec3 lP1, vec3 lP2, vec3 p);

typedef struct Matrix3x3 {
    F32 m[3][3];
} Matrix3x3;

Matrix3x3 Transpose(Matrix3x3 m);
Matrix3x3 Identity();
vec3 Matrix3x3ToEulerAngles(Matrix3x3 m);
Matrix3x3 EulerAnglesToMatrix3x3(vec3 rotation);
vec3 RotateVector(vec3 v, Matrix3x3 m);
vec3 RotatePoint(vec3 p, vec3 r, vec3 pivot);
Matrix3x3 MultiplyMatrix3x3(Matrix3x3 a, Matrix3x3 b);

typedef struct Matrix4x4 {
    F32 m[4][4];
} Matrix4x4;

Matrix4x4 Identity4x4();
Matrix4x4 GetProjectionMatrix(F32 right, F32 left, F32 top, F32 bottom, F32 near, F32 far);

F32 Lerp(F64 t, F32 a, F32 b);
int Step(F32 edge, F32 x);
F32 Smoothstep(F32 edge0, F32 edge1, F32 x);
int Modulus(int a, int b);
F32 fModulus(F32 a, F32 b);
}  // namespace NewMaths

#pragma endregion NewMATH

#pragma region PCG

struct pcg_state_setseq_64 {  // Internals are *Private*.
    uint64_t state;           // RNG state.  All values are possible.
    uint64_t inc;             // Controls which RNG sequence (stream) is
                              // selected. Must *always* be odd.
};
typedef struct pcg_state_setseq_64 pcg32_random_t;

// If you *must* statically initialize it, here's one.

#define PCG32_INITIALIZER \
    { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL }

// pcg32_srandom(initstate, initseq)
// pcg32_srandom_r(rng, initstate, initseq):
//     Seed the rng.  Specified in two parts, state initializer and a
//     sequence selection constant (a.k.a. stream id)

void pcg32_srandom(uint64_t initstate, uint64_t initseq);
void pcg32_srandom_r(pcg32_random_t *rng, uint64_t initstate, uint64_t initseq);

// pcg32_random()
// pcg32_random_r(rng)
//     Generate a uniformly distributed 32-bit random number

uint32_t pcg32_random(void);
uint32_t pcg32_random_r(pcg32_random_t *rng);

// pcg32_boundedrand(bound):
// pcg32_boundedrand_r(rng, bound):
//     Generate a uniformly distributed number, r, where 0 <= r < bound

uint32_t pcg32_boundedrand(uint32_t bound);
uint32_t pcg32_boundedrand_r(pcg32_random_t *rng, uint32_t bound);

#pragma endregion PCG

#pragma region PORO

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "core/const.h"

namespace MetaEngine {
namespace math {

//=============================================================================
// Still in the works templated vector class

template <class Type>
class CVector2 {
public:
    typedef Type unit_type;
    //=========================================================================

    CVector2() : x(Type()), y(Type()) {}

    CVector2(Type x, Type y) : x(x), y(y) {}

    template <class T>
    explicit CVector2(const T &other) : x((Type)(other.x)), y((Type)(other.y)) {}

    void Set(Type x_, Type y_) {
        x = x_;
        y = y_;
    }

    //=========================================================================

    bool operator==(const CVector2<Type> &other) const { return (this->x == other.x && this->y == other.y); }
    bool operator!=(const CVector2<Type> &other) const { return !operator==(other); }

    //=========================================================================

    CVector2<Type> operator-() const { return CVector2<Type>(-x, -y); }

    CVector2<Type> &operator+=(const CVector2<Type> &v) {
        x += v.x;
        y += v.y;

        return *this;
    }

    CVector2<Type> &operator-=(const CVector2<Type> &v) {
        x -= v.x;
        y -= v.y;

        return *this;
    }

    CVector2<Type> &operator*=(float a) {
        x = (Type)(x * a);
        y = (Type)(y * a);

        return *this;
    }

    CVector2<Type> &operator/=(float a) {
        x = (Type)(x / a);
        y = (Type)(y / a);

        return *this;
    }

    //-------------------------------------------------------------------------

    CVector2<Type> operator+(const CVector2<Type> &other) const { return CVector2<Type>(this->x + other.x, this->y + other.y); }

    CVector2<Type> operator-(const CVector2<Type> &other) const { return CVector2<Type>(this->x - other.x, this->y - other.y); }

    CVector2<Type> operator*(float t) const { return CVector2<Type>((Type)(this->x * t), (Type)(this->y * t)); }

    CVector2<Type> operator/(float t) const { return CVector2<Type>((Type)(this->x / t), (Type)(this->y / t)); }

    //=========================================================================

    Type Dot(const CVector2<Type> &a, const CVector2<Type> &b) const { return a.x * b.x + a.y * b.y; }

    Type Cross(const CVector2<Type> &a, const CVector2<Type> &b) const { return a.x * b.y - a.y * b.x; }

    CVector2<Type> Cross(const CVector2<Type> &a, const Type &s) const { return CVector2<Type>(s * a.y, -s * a.x); }

    CVector2<Type> Cross(const Type &s, const CVector2<Type> &a) const { return CVector2<Type>(-s * a.y, s * a.x); }

    //=========================================================================

    Type LengthSquared() const { return (x * x + y * y); }

    float Length() const { return sqrtf((float)LengthSquared()); }

    CVector2<Type> Normalize() const {
        float d = Length();

        if (d > 0)
            return CVector2<Type>(Type(x / d), Type(y / d));
        else
            return CVector2<Type>(0, 0);
    }

    //=========================================================================
    // ripped from:
    // http://forums.indiegamer.com/showthread.php?t=10459

    Type Angle() const {
        CVector2<Type> normal = Normalize();
        Type angle = atan2(normal.y, normal.x);

        return angle;
    }

    Type Angle(const CVector2<Type> &x) const {
        Type dot = Dot(*this, x);
        Type cross = Cross(*this, x);

        // angle between segments
        Type angle = (Type)atan2(cross, dot);

        return angle;
    }

    CVector2<Type> &Rotate(Type angle_rad) {
        Type tx = x;
        x = (Type)x * (Type)cos(angle_rad) - y * (Type)sin(angle_rad);
        y = (Type)tx * (Type)sin(angle_rad) + y * (Type)cos(angle_rad);

        return *this;
    }

    CVector2<Type> &Rotate(const CVector2<Type> &centre, Type angle_rad) {
        CVector2<Type> D = *this - centre;
        D.Rotate(angle_rad);

        // *this = xCentre + D;
        D += centre;
        Set(D.x, D.y);

        return *this;
    }

    //=========================================================================

    Type x;
    Type y;
};

}  // namespace math
}  // end of namespace MetaEngine

// ---------- types ----------
namespace types {
typedef MetaEngine::math::CVector2<float> vector2;
typedef MetaEngine::math::CVector2<double> dvector2;
typedef MetaEngine::math::CVector2<int> ivector2;

typedef MetaEngine::math::CVector2<int> point;

}  // end of namespace types

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>

namespace MetaEngine {
namespace math {

// typedef CVector2< float > Vec2;

//-----------------------------------------------------------------------------
template <class Type>
struct CMat22 {
    CMat22() { SetIdentity(); }
    CMat22(float angle) {
        Type c = (Type)cos(angle), s = (Type)sin(angle);
        col1.x = c;
        col2.x = -s;
        col1.y = s;
        col2.y = c;
    }

    CMat22(const CVector2<Type> &col1, const CVector2<Type> &col2) : col1(col1), col2(col2) {}

    CMat22(Type c1x, Type c1y, Type c2x, Type c2y) : col1(c1x, c1y), col2(c2x, c2y) {}

    CMat22<Type> Transpose() const { return CMat22<Type>(CVector2<Type>(col1.x, col2.x), CVector2<Type>(col1.y, col2.y)); }

    /// Initialize this matrix using columns.
    void Set(const CVector2<Type> &c1, const CVector2<Type> &c2) {
        col1 = c1;
        col2 = c2;
    }

    /// Initialize this matrix using an angle. This matrix becomes
    /// an orthonormal rotation matrix.
    void Set(float angle) {
        float c = cosf((float)angle), s = sinf((float)angle);
        col1.x = (Type)c;
        col2.x = (Type)(-s);
        col1.y = (Type)s;
        col2.y = (Type)c;
    }

    /// Set this to the identity matrix.
    void SetIdentity() {
        col1.x = (Type)1;
        col2.x = (Type)0;
        col1.y = (Type)0;
        col2.y = (Type)1;
    }

    /// Set this matrix to all zeros.
    void SetZero() {
        col1.x = 0;
        col2.x = 0;
        col1.y = 0;
        col2.y = 0;
    }

    /// Extract the angle from this matrix (assumed to be
    /// a rotation matrix).
    float GetAngle() const { return static_cast<float>(atan2(col1.y, col1.x)); }

    void SetAngle(float angle) { Set(angle); }

    CMat22<Type> Invert() const {
        Type a = col1.x, b = col2.x, c = col1.y, d = col2.y;
        CMat22<Type> B;
        Type det = a * d - b * c;
        assert(det != 0.0f);
        float fdet = 1.0f / (float)det;
        B.col1.x = Type(fdet * d);
        B.col2.x = Type(-fdet * b);
        B.col1.y = Type(-fdet * c);
        B.col2.y = Type(fdet * a);
        return B;
    }

    CVector2<Type> col1, col2;
};

}  // end of namespace math
}  // end of namespace MetaEngine

//----------------- types --------------------------------------------

namespace types {
typedef MetaEngine::math::CMat22<float> mat22;
}  // end of namespace types

namespace MetaEngine {
namespace math {

/// A transform contains translation and rotation. It is used to represent
/// the position and orientation of rigid frames.
template <typename Type>
struct CXForm {
    /// The default constructor does nothing (for performance).
    CXForm() : scale((Type)1, (Type)1) {}

    /// Initialize using a position vector and a rotation matrix.
    CXForm(const CVector2<Type> &position, const CMat22<Type> &R, const CVector2<Type> &scale) : position(position), R(R), scale(scale) {}

    const CXForm<Type> &operator=(const CXForm<Type> &other) {
        position = other.position;
        R = other.R;
        scale = other.scale;
        return *this;
    }

    /// Set this to the identity transform.
    void SetIdentity() {
        position.Set((Type)0, (Type)0);
        R.SetIdentity();
        scale.Set((Type)1, (Type)1);
    }

    CVector2<Type> position;
    CMat22<Type> R;
    CVector2<Type> scale;
};

}  // end of namespace math
}  // End of namespace MetaEngine

// -------------- types --------------------------

namespace types {
typedef MetaEngine::math::CXForm<float> xform;
}  // end of namespace types

namespace MetaEngine {
namespace math {

//-----------------------------------------------------------------------------
// returns a rounded value, if the value is less than 0, rounds it "down"
// otherwise rounds it rounds it up by 0.5f
float Round(float value);
double Round(double value);

inline float Round(float value) {
    if (value < 0)
        value -= 0.5f;
    else
        value += 0.5f;
    return std::floor(value);
}

inline double Round(double value) {
    if (value < 0)
        value -= 0.5;
    else
        value += 0.5;
    return std::floor(value);
}

//=============================================================================
// Min -function returns the smaller one
//
// Required operators: < -operator with the same type
//
// Basically done because VC6 breaks the std::min and std::max, that fucker
//.............................................................................

template <typename Type>
inline const Type &Min(const Type &c1, const Type &c2) {
    return (c1 < c2) ? c1 : c2;
}

template <typename Type>
inline const Type &Min(const Type &c1, const Type &c2, const Type &c3) {
    return (c1 < Min(c2, c3)) ? c1 : Min(c2, c3);
}

template <typename Type>
inline const Type &Min(const Type &c1, const Type &c2, const Type &c3, const Type &c4) {
    return (c1 < Min(c2, c3, c4)) ? c1 : Min(c2, c3, c4);
}

//=============================================================================
// Max -function returns the bigger one
//
// Required operators: < -operator with the same type
//
// Basically done because VC6 breaks the std::min and std::max, that fucker
//.............................................................................

template <typename Type>
inline const Type &Max(const Type &c1, const Type &c2) {
    return (c1 < c2) ? c2 : c1;
}

template <typename Type>
inline const Type &Max(const Type &c1, const Type &c2, const Type &c3) {
    return (c1 < Max(c2, c3)) ? Max(c2, c3) : c1;
}

template <typename Type>
inline const Type &Max(const Type &c1, const Type &c2, const Type &c3, const Type &c4) {
    return (c1 < Max(c2, c3, c4)) ? Max(c2, c3, c4) : c1;
}

//=============================================================================
// Returns the absolute value of the variable
//
// Required operators: < operator and - operator.
//.............................................................................

template <typename Type>
inline Type Absolute(const Type &n) {
    /*
if( n < 0 )
return -n;
else
return n;
*/

    return MetaEngine::math::Max(n, -n);
}

//=============================================================================
// Clams a give value to the desired
//
// Required operators: < operator
//.............................................................................

template <typename Type>
inline Type Clamp(const Type &a, const Type &low, const Type &high) {
    return MetaEngine::math::Max(low, MetaEngine::math::Min(a, high));
}

//=============================================================================
// Swaps to elements with each other.
//
// Required operators: = operator
//.............................................................................

template <typename Type>
inline void Swap(Type &a, Type &b) {
    Type temp = a;
    a = b;
    b = temp;
}

//=============================================================================
// Returns Square value of the value
//
// Required operators: * operator
//.............................................................................

template <typename Type>
inline Type Square(Type x) {
    return x * x;
}

//=============================================================================

template <typename Type>
inline Type ConvertRadToAngle(const Type &rad) {
    return rad * (180.0f / PI);
}

template <typename Type>
inline Type ConvertAngleToRad(const Type &angle) {
    return angle * (PI / 180.0f);
}

//=============================================================================

template <typename Type>
inline float Distance(const Type &vector1, const Type &vector2) {
    Type d;
    d.x = vector1.x - vector2.x;
    d.y = vector1.y - vector2.y;

    return sqrtf((float)(d.x * d.x + d.y * d.y));
}

template <typename Type>
inline float Distance(const Type &vector1) {
    return sqrtf((float)(vector1.x * vector1.x + vector1.y * vector1.y));
}

template <typename Type>
inline float DistanceSquared(const Type &vector1, const Type &vector2) {
    Type d;
    d.x = vector1.x - vector2.x;
    d.y = vector1.y - vector2.y;

    return (float)(d.x * d.x + d.y * d.y);
}

template <typename Type>
inline float DistanceSquared(const Type &vector1) {
    return (float)(vector1.x * vector1.x + vector1.y * vector1.y);
}

//=============================================================================

template <typename Type>
inline Type Lerp(const Type &a, const Type &b, float t) {
    return (Type)(a + t * (b - a));
}

//=============================================================================

//-----------------------------------------------------------------------------

template <class Type>
Type Dot(const CVector2<Type> &a, const CVector2<Type> &b) {
    return a.x * b.x + a.y * b.y;
}

template <class Type>
Type Cross(const CVector2<Type> &a, const CVector2<Type> &b) {
    return a.x * b.y - a.y * b.x;
}

template <class Type>
CVector2<Type> Cross(const CVector2<Type> &a, Type s) {
    return CVector2<Type>(s * a.y, -s * a.x);
}
template <class Type>
CVector2<Type> Cross(Type s, const CVector2<Type> &a) {
    return CVector2<Type>(-s * a.y, s * a.x);
}

// ----------------------------------------------------------------------------

template <class Type>
CVector2<Type> operator*(const CMat22<Type> &A, const CVector2<Type> &v) {
    return CVector2<Type>(A.col1.x * v.x + A.col2.x * v.y, A.col1.y * v.x + A.col2.y * v.y);
}

template <class Type>
CVector2<Type> operator*(Type s, const CVector2<Type> &v) {
    return CVector2<Type>(s * v.x, s * v.y);
}

template <class Type>
CMat22<Type> operator+(const CMat22<Type> &A, const CMat22<Type> &B) {
    return CMat22<Type>(A.col1 + B.col1, A.col2 + B.col2);
}

template <class Type>
CMat22<Type> operator*(const CMat22<Type> &A, const CMat22<Type> &B) {
    return CMat22<Type>(A * B.col1, A * B.col2);
}

// ----------------------------------------------------------------------------

template <class Type>
CVector2<Type> Abs(const CVector2<Type> &a) {
    return CVector2<Type>(fabsf(a.x), fabsf(a.y));
}

template <class Type>
CMat22<Type> Abs(const CMat22<Type> &A) {
    return CMat22<Type>(Abs(A.col1), Abs(A.col2));
}

// ----------------------------------------------------------------------------

template <class Type>
inline Type Sign(Type value) {
    return value < (Type)0 ? (Type)-1 : (Type)1;
}

// ----------------------------------------------------------------------------

template <class Type>
CVector2<Type> Mul(const CVector2<Type> &a, const CVector2<Type> &b) {
    CVector2<Type> result(a.x * b.x, a.y * b.y);
    return result;
}

/// Multiply a matrix times a vector. If a rotation matrix is provided,
/// then this transforms the vector from one frame to another.
template <class Type, class Vector2>
Vector2 Mul(const CMat22<Type> &A, const Vector2 &v) {
    Vector2 u;
    u.x = A.col1.x * v.x + A.col2.x * v.y;
    u.y = A.col1.y * v.x + A.col2.y * v.y;
    // u.Set( A.col1.x * v.x + A.col2.x * v.y, A.col1.y * v.x + A.col2.y * v.y );
    return u;
}

/// Multiply a matrix transpose times a vector. If a rotation matrix is provided,
/// then this transforms the vector from one frame to another (inverse transform).
template <class Type>
CVector2<Type> MulT(const CMat22<Type> &A, const CVector2<Type> &v) {
    CVector2<Type> u;
    u.Set(Dot(v, A.col1), Dot(v, A.col2));
    return u;
}

template <class Type>
CVector2<Type> Mul(const CXForm<Type> &T, const CVector2<Type> &v) {
    return T.position + Mul(T.R, v);
}

#if 0
template< class Type, class Vector2 >
Vector2 Mul( const CXForm< Type >& T, const Vector2& v )
{
	Vector2 result;
	result.x = v.x * T.scale.x;
	result.y = v.y * T.scale.y;
	result = Mul( T.R, result );
	result.x += T.position.x;
	result.y += T.position.y;
	return result;
}
#endif

template <class Type, class Vector2>
Vector2 MulWithScale(const CXForm<Type> &T, const Vector2 &v) {
    Vector2 result;
    result.x = v.x * T.scale.x;
    result.y = v.y * T.scale.y;
    result = Mul(T.R, result);
    result.x += T.position.x;
    result.y += T.position.y;
    return result;
}

template <class Type>
CXForm<Type> Mul(const CXForm<Type> &T, const CXForm<Type> &v) {

    CXForm<Type> result;

    result.position.x = T.scale.x * v.position.x;
    result.position.y = T.scale.y * v.position.y;
    result.position = Mul(T.R, result.position);
    result.position += T.position;

    result.R = Mul(T.R, v.R);
    result.scale.x = T.scale.x * v.scale.x;
    result.scale.y = T.scale.y * v.scale.y;

    return result;
}

template <class Type>
CVector2<Type> MulT(const CXForm<Type> &T, const CVector2<Type> &v) {
    return MulT(T.R, v - T.position);
}

template <class Type>
CXForm<Type> MulT(const CXForm<Type> &T, const CXForm<Type> &v) {

    CXForm<Type> result;

    result.scale.x = v.scale.x / T.scale.x;
    result.scale.y = v.scale.y / T.scale.y;
    result.R = Mul(T.R.Invert(), v.R);

    result.position.x = (v.position.x - T.position.x) / T.scale.x;
    result.position.y = (v.position.y - T.position.y) / T.scale.y;
    // result.position = v.position - T.position;
    result.position = MulT(T.R, result.position);

    return result;
}

template <class Type>
CVector2<Type> MulTWithScale(const CXForm<Type> &T, const CVector2<Type> &v) {
    CVector2<Type> result(v);
    if (T.scale.x != 0) result.x /= T.scale.x;
    if (T.scale.y != 0) result.y /= T.scale.y;
    return MulT(T.R, result - T.position);
}

// A * B
template <class Type>
inline CMat22<Type> Mul(const CMat22<Type> &A, const CMat22<Type> &B) {
    CMat22<Type> C;
    C.Set(Mul(A, B.col1), Mul(A, B.col2));
    return C;
}

// ----------------------------------------------------------------------------

template <class Type>
inline CVector2<float> ClosestPointOnLineSegment(const CVector2<Type> &a, const CVector2<Type> &b, const CVector2<Type> &p) {
    CVector2<float> c(p - a);
    CVector2<float> v(b - a);
    float distance = v.Length();

    // optimized normalized
    // v = v.Normalise();
    if (distance != 0) {
        v.x /= distance;
        v.y /= distance;
    }

    float t = (float)Dot(v, c);

    if (t < 0) return CVector2<float>(a);

    if (t > distance) return CVector2<float>(b);

    v *= t;

    return CVector2<float>(a) + v;
}

// ----------------------------------------------------------------------------

template <class Type>
inline float DistanceFromLineSquared(const CVector2<Type> &a, const CVector2<Type> &b, const CVector2<Type> &p) {
    CVector2<float> delta = ClosestPointOnLineSegment(a, b, p) - CVector2<float>(p);

    return delta.LengthSquared();
}

// ----------------------------------------------------------------------------

template <class Type>
inline float DistanceFromLine(const CVector2<Type> &a, const CVector2<Type> &b, const CVector2<Type> &p) {
    return sqrtf((float)DistanceFromLineSquared(a, b, p));
}

// ----------------------------------------------------------------------------

template <class T>
bool LineIntersection(const CVector2<T> &startA, const CVector2<T> &endA, const CVector2<T> &startB, const CVector2<T> &endB, CVector2<T> &result) {

    // TODO: reuse mathutil.intersect
    float d = (endB.y - startB.y) * (endA.x - startA.x) - (endB.x - startB.x) * (endA.y - startA.y);

    if (d == 0)  // parallel lines
        return false;

    float uA = (endB.x - startB.x) * (startA.y - startB.y) - (endB.y - startB.y) * (startA.x - startB.x);
    uA /= d;
    float uB = (endA.x - startA.x) * (startA.y - startB.y) - (endA.y - startA.y) * (startA.x - startB.x);
    uB /= d;

    if (uA < 0 || uA > 1 || uB < 0 || uB > 1) return false;  // intersection point isn't between the start and endpoints

    result.Set(startA.x + uA * (endA.x - startA.x), startA.y + uA * (endA.y - startA.y));

    // result = position;
    return true;
}

// ----------------------------------------------------------------------------

template <class PType>
float DistanceFromAABB(const CVector2<PType> &point, const CVector2<PType> &aabb_min, const CVector2<PType> &aabb_max) {
    if (IsPointInsideAABB(point, aabb_min, aabb_max)) return 0;

    float lowest = 0;
    float temp = MetaEngine::math::DistanceFromLineSquared(aabb_min, CVector2<PType>(aabb_max.x, aabb_min.y), point);
    if (temp < lowest) lowest = temp;

    temp = MetaEngine::math::DistanceFromLineSquared(CVector2<PType>(aabb_max.x, aabb_min.y), CVector2<PType>(aabb_max.x, aabb_max.y), point);
    if (temp < lowest) lowest = temp;

    temp = MetaEngine::math::DistanceFromLineSquared(CVector2<PType>(aabb_max.x, aabb_max.y), CVector2<PType>(aabb_min.x, aabb_max.y), point);
    if (temp < lowest) lowest = temp;

    temp = MetaEngine::math::DistanceFromLineSquared(CVector2<PType>(aabb_min.x, aabb_max.y), CVector2<PType>(aabb_min.x, aabb_min.y), point);
    if (temp < lowest) lowest = temp;

    return sqrtf(lowest);
}

template <class T>
class CAverager {
public:
    CAverager() : value(T()), current_value(T()), count(0) {}

    virtual ~CAverager() {}

    virtual void Reset() {
        value = T();
        count = 0;
    }

    virtual T Add(const T &other) {
        current_value = other;
        value += other;
        count++;

        return GetAverage();
    }

    T operator+=(const T &other) { return Add(other); }

    T GetAverage() const {
        if (count == 0) return T();

        return (T)(value / (T)count);
    }

    T GetCurrent() const { return current_value; }
    T GetTotal() const { return value; }
    unsigned int GetCount() const { return count; }

private:
    T value;
    T current_value;
    unsigned int count;
};

///////////////////////////////////////////////////////////////////////////////

typedef float PointType;

bool IsPointInsideRect(const CVector2<PointType> &point, const CVector2<PointType> &position, const CVector2<PointType> &width, float rotation);

bool IsPointInsideCircle(const CVector2<PointType> &point, const CVector2<PointType> &position, float d);

bool IsPointInsidePolygon(const CVector2<PointType> &point, const std::vector<CVector2<PointType>> &polygon);
bool IsPointInsidePolygon(const CVector2<int> &point, const std::vector<CVector2<int>> &polygon);

bool IsPointInsidePolygon_Better(const CVector2<PointType> &point, const std::vector<CVector2<PointType>> &pgon);

template <class Type>
bool IsPointInsideAABB(const Type &point, const Type &rect_low, const Type &rect_high) {
    return (rect_low.x <= point.x && rect_high.x > point.x && rect_low.y <= point.y && rect_high.y > point.y);
}

bool DoesLineAndBoxCollide(const CVector2<PointType> &p1, const CVector2<PointType> &p2, const CVector2<PointType> &rect_low, const CVector2<PointType> &rect_high);

bool TestLineAABB(const CVector2<PointType> &p1, const CVector2<PointType> &p2, const CVector2<PointType> &rect_low, const CVector2<PointType> &rect_high);

template <class PType>
bool TestAABBAABBIntersection(const CVector2<PType> &a_min, const CVector2<PType> &a_max, const CVector2<PType> &b_min, const CVector2<PType> &b_max) {
    if (a_max.x < b_min.x || a_max.y < b_min.y || a_min.x > b_max.x || a_min.y > b_max.y) {
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////

template <class T>
class CStatisticsHelper : public math::CAverager<T> {
public:
    CStatisticsHelper() : CAverager<T>(), minValue(std::numeric_limits<T>::max()), maxValue(std::numeric_limits<T>::min()) {}

    virtual void Reset() {
        minValue = std::numeric_limits<T>::max();
        maxValue = std::numeric_limits<T>::min();

        CAverager<T>::Reset();
    }

    virtual T Add(const T &other) {
        if (other < minValue) minValue = other;
        if (other > maxValue) maxValue = other;

        return CAverager<T>::Add(other);
    }

    T GetMin() const { return minValue; }

    T GetMax() const { return maxValue; }

private:
    T minValue;
    T maxValue;
};
///////////////////////////////////////////////////////////////////////////////

template <typename T>
class CAngle {
    static const T TWO_PI;

    T AngleMod(T value) {
        while (value < 0) {
            value += TWO_PI;
        }

        value = static_cast<T>(fmod((double)value, (double)TWO_PI));

        return value;
    }

public:
    CAngle() : mAng(T()) {}
    CAngle(const CAngle &other) : mAng(other.mAng) {}
    CAngle(T value) { SetValue(value); }

    T GetValue() const { return mAng; }
    void SetValue(T value) { mAng = AngleMod(value); }

    void operator+=(const CAngle &other) {
        mAng += other.mAng;
        if (mAng >= TWO_PI) mAng -= TWO_PI;

        METADOT_ASSERT_E(mAng >= 0 && mAng <= TWO_PI);
    }

    void operator-=(const CAngle &other) {
        mAng -= other.mAng;
        if (mAng < 0) mAng += TWO_PI;

        METADOT_ASSERT_E(mAng >= 0 && mAng <= TWO_PI);
    }

    CAngle operator+(const CAngle &other) const {
        CAngle result;
        result.mAng = mAng + other.mAng;
        if (result.mAng >= TWO_PI) result.mAng -= TWO_PI;

        return result;
    }

    CAngle operator-(const CAngle &other) const {
        CAngle result;
        result.mAng = mAng - other.mAng;
        if (result.mAng < 0) result.mAng += TWO_PI;

        return result;
    }

    bool operator==(const CAngle &other) const { return GetValue() == other.GetValue(); }

private:
    T mAng;
};

template <typename T>
const T CAngle<T>::TWO_PI = (T)(2 * PI);

}  // end of namespace math
}  // namespace MetaEngine

// ---------------- types ---------------------

namespace types {
typedef MetaEngine::math::CAngle<float> angle;
}  // end of namespace types

#pragma endregion PORO

#pragma region c2

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// 2d vector.
typedef struct METAENGINE_V2 {
    float x;
    float y;
} METAENGINE_V2;

// Use this to create a v2 struct.
// The C++ API uses V2(x, y).
METADOT_INLINE METAENGINE_V2 metadot_v2(float x, float y) {
    METAENGINE_V2 result;
    result.x = x;
    result.y = y;
    return result;
}

// Rotation about an axis composed of cos/sin pair.
typedef struct METAENGINE_SinCos {
    float s;
    float c;
} METAENGINE_SinCos;

// 2x2 matrix.
typedef struct METAENGINE_M2x2 {
    METAENGINE_V2 x;
    METAENGINE_V2 y;
} METAENGINE_M2x2;

// 2d transformation, mostly useful for graphics and not physics colliders, since it supports scale.
typedef struct METAENGINE_M3x2 {
    METAENGINE_M2x2 m;
    METAENGINE_V2 p;
} METAENGINE_M3x2;

// 2d transformation, mostly useful for physics colliders since there's no scale.
typedef struct METAENGINE_Transform {
    METAENGINE_SinCos r;
    METAENGINE_V2 p;
} METAENGINE_Transform;

// 2d plane, aka line.
typedef struct METAENGINE_Halfspace {
    METAENGINE_V2 n;  // normal
    float d;          // distance to origin; d = ax + by = dot(n, p)
} METAENGINE_Halfspace;

// A ray is a directional line segment. It starts at an endpoint and extends into another direction
// for a specified distance (defined by t).
typedef struct METAENGINE_Ray {
    METAENGINE_V2 p;  // position
    METAENGINE_V2 d;  // direction (normalized)
    float t;          // distance along d from position p to find endpoint of ray
} METAENGINE_Ray;

// The results for a raycast query.
typedef struct METAENGINE_Raycast {
    float t;          // time of impact
    METAENGINE_V2 n;  // normal of surface at impact (unit length)
} METAENGINE_Raycast;

typedef struct METAENGINE_Circle {
    METAENGINE_V2 p;
    float r;
} METAENGINE_Circle;

// Axis-aligned bounding box. A box that cannot rotate.
typedef struct METAENGINE_Aabb {
    METAENGINE_V2 min;
    METAENGINE_V2 max;
} METAENGINE_Aabb;

// Box that cannot rotate defined with integers instead of floats. Not used for collision detection,
// but still sometimes useful.
typedef struct METAENGINE_Rect {
    int w, h, x, y;
} METAENGINE_Rect;

#define METAENGINE_PI 3.14159265f

//--------------------------------------------------------------------------------------------------
// Scalar float ops.

METADOT_INLINE float metadot_min(float a, float b) { return a < b ? a : b; }
METADOT_INLINE float metadot_max(float a, float b) { return b < a ? a : b; }
METADOT_INLINE float metadot_clamp(float a, float lo, float hi) { return metadot_max(lo, metadot_min(a, hi)); }
METADOT_INLINE float metadot_clamp01(float a) { return metadot_max(0.0f, metadot_min(a, 1.0f)); }
METADOT_INLINE float metadot_sign(float a) { return a < 0 ? -1.0f : 1.0f; }
METADOT_INLINE float metadot_intersect(float da, float db) { return da / (da - db); }
METADOT_INLINE float metadot_safe_invert(float a) { return a != 0 ? 1.0f / a : 0; }
METADOT_INLINE float metadot_lerp(float a, float b, float t) { return a + (b - a) * t; }
METADOT_INLINE float metadot_remap(float t, float lo, float hi) { return (hi - lo) != 0 ? (t - lo) / (hi - lo) : 0; }
METADOT_INLINE float metadot_mod(float x, float m) { return x - (int)(x / m) * m; }
METADOT_INLINE float metadot_fract(float x) { return x - floorf(x); }

METADOT_INLINE int metadot_sign_int(int a) { return a < 0 ? -1 : 1; }
#define metadot_min(a, b) ((a) < (b) ? (a) : (b))
#define metadot_max(a, b) ((b) < (a) ? (a) : (b))
METADOT_INLINE float metadot_abs(float a) { return fabsf(a); }
METADOT_INLINE int metadot_abs_int(int a) {
    int mask = a >> ((sizeof(int) * 8) - 1);
    return (a + mask) ^ mask;
}
METADOT_INLINE int metadot_clamp_int(int a, int lo, int hi) { return metadot_max(lo, metadot_min(a, hi)); }
METADOT_INLINE int metadot_clamp01_int(int a) { return metadot_max(0, metadot_min(a, 1)); }
METADOT_INLINE bool metadot_is_even(int x) { return (x % 2) == 0; }
METADOT_INLINE bool metadot_is_odd(int x) { return !metadot_is_even(x); }

//--------------------------------------------------------------------------------------------------
// Bit manipulation.

METADOT_INLINE bool metadot_is_power_of_two(int a) { return a != 0 && (a & (a - 1)) == 0; }
METADOT_INLINE bool metadot_is_power_of_two_uint(uint64_t a) { return a != 0 && (a & (a - 1)) == 0; }
METADOT_INLINE int metadot_fit_power_of_two(int a) {
    a--;
    a |= a >> 1;
    a |= a >> 2;
    a |= a >> 4;
    a |= a >> 8;
    a |= a >> 16;
    a++;
    return a;
}

//--------------------------------------------------------------------------------------------------
// Easing functions.
// Adapted from Noel Berry: https://github.com/NoelFB/blah/blob/master/include/blah_ease.h

METADOT_INLINE float metadot_smoothstep(float x) { return x * x * (3.0f - 2.0f * x); }
METADOT_INLINE float metadot_quad_in(float x) { return x * x; }
METADOT_INLINE float metadot_quad_out(float x) { return -(x * (x - 2.0f)); }
METADOT_INLINE float metadot_quad_in_out(float x) {
    if (x < 0.5f)
        return 2.0f * x * x;
    else
        return (-2.0f * x * x) + (4.0f * x) - 1.0f;
}
METADOT_INLINE float metadot_cube_in(float x) { return x * x * x; }
METADOT_INLINE float metadot_cube_out(float x) {
    float f = (x - 1);
    return f * f * f + 1.0f;
}
METADOT_INLINE float metadot_cube_in_out(float x) {
    if (x < 0.5f)
        return 4.0f * x * x * x;
    else {
        float f = ((2.0f * x) - 2.0f);
        return 0.5f * x * x * x + 1.0f;
    }
}
METADOT_INLINE float metadot_quart_in(float x) { return x * x * x * x; }
METADOT_INLINE float metadot_quart_out(float x) {
    float f = (x - 1.0f);
    return f * f * f * (1.0f - x) + 1.0f;
}
METADOT_INLINE float metadot_quart_in_out(float x) {
    if (x < 0.5f)
        return 8.0f * x * x * x * x;
    else {
        float f = (x - 1);
        return -8.0f * f * f * f * f + 1.0f;
    }
}
METADOT_INLINE float metadot_quint_in(float x) { return x * x * x * x * x; }
METADOT_INLINE float metadot_quint_out(float x) {
    float f = (x - 1);
    return f * f * f * f * f + 1.0f;
}
METADOT_INLINE float metadot_quint_in_out(float x) {
    if (x < 0.5f)
        return 16.0f * x * x * x * x * x;
    else {
        float f = ((2.0f * x) - 2.0f);
        return 0.5f * f * f * f * f * f + 1.0f;
    }
}
METADOT_INLINE float metadot_sin_in(float x) { return sinf((x - 1.0f) * METAENGINE_PI * 0.5f) + 1.0f; }
METADOT_INLINE float metadot_sin_out(float x) { return sinf(x * (METAENGINE_PI * 0.5f)); }
METADOT_INLINE float metadot_sin_in_out(float x) { return 0.5f * (1.0f - cosf(x * METAENGINE_PI)); }
METADOT_INLINE float metadot_circle_in(float x) { return 1.0f - sqrtf(1.0f - (x * x)); }
METADOT_INLINE float metadot_circle_out(float x) { return sqrtf((2.0f - x) * x); }
METADOT_INLINE float metadot_circle_in_out(float x) {
    if (x < 0.5f)
        return 0.5f * (1.0f - sqrtf(1.0f - 4.0f * (x * x)));
    else
        return 0.5f * (sqrtf(-((2.0f * x) - 3.0f) * ((2.0f * x) - 1.0f)) + 1.0f);
}
METADOT_INLINE float metadot_back_in(float x) { return x * x * x - x * sinf(x * METAENGINE_PI); }
METADOT_INLINE float metadot_back_out(float x) {
    float f = (1.0f - x);
    return 1.0f - (x * x * x - x * sinf(f * METAENGINE_PI));
}
METADOT_INLINE float metadot_back_in_out(float x) {
    if (x < 0.5f) {
        float f = 2.0f * x;
        return 0.5f * (f * f * f - f * sinf(f * METAENGINE_PI));
    } else {
        float f = (1.0f - (2.0f * x - 1.0f));
        return 0.5f * (1.0f - (f * f * f - f * sinf(f * METAENGINE_PI))) + 0.5f;
    }
}

//--------------------------------------------------------------------------------------------------
// 2D vector ops.

METADOT_INLINE METAENGINE_V2 metadot_add_v2(METAENGINE_V2 a, METAENGINE_V2 b) { return metadot_v2(a.x + b.x, a.y + b.y); }
METADOT_INLINE METAENGINE_V2 metadot_sub_v2(METAENGINE_V2 a, METAENGINE_V2 b) { return metadot_v2(a.x - b.x, a.y - b.y); }

METADOT_INLINE float metadot_dot(METAENGINE_V2 a, METAENGINE_V2 b) { return a.x * b.x + a.y * b.y; }

METADOT_INLINE METAENGINE_V2 metadot_mul_v2_f(METAENGINE_V2 a, float b) { return metadot_v2(a.x * b, a.y * b); }
METADOT_INLINE METAENGINE_V2 metadot_mul_v2(METAENGINE_V2 a, METAENGINE_V2 b) { return metadot_v2(a.x * b.x, a.y * b.y); }
METADOT_INLINE METAENGINE_V2 metadot_div_v2_f(METAENGINE_V2 a, float b) { return metadot_v2(a.x / b, a.y / b); }

METADOT_INLINE METAENGINE_V2 metadot_skew(METAENGINE_V2 a) { return metadot_v2(-a.y, a.x); }
METADOT_INLINE METAENGINE_V2 metadot_cw90(METAENGINE_V2 a) { return metadot_v2(a.y, -a.x); }
METADOT_INLINE float metadot_det2(METAENGINE_V2 a, METAENGINE_V2 b) { return a.x * b.y - a.y * b.x; }
METADOT_INLINE float metadot_cross(METAENGINE_V2 a, METAENGINE_V2 b) { return metadot_det2(a, b); }
METADOT_INLINE METAENGINE_V2 metadot_cross_v2_f(METAENGINE_V2 a, float b) { return metadot_v2(b * a.y, -b * a.x); }
METADOT_INLINE METAENGINE_V2 metadot_cross_f_v2(float a, METAENGINE_V2 b) { return metadot_v2(-a * b.y, a * b.x); }
METADOT_INLINE METAENGINE_V2 metadot_min_v2(METAENGINE_V2 a, METAENGINE_V2 b) { return metadot_v2(metadot_min(a.x, b.x), metadot_min(a.y, b.y)); }
METADOT_INLINE METAENGINE_V2 metadot_max_v2(METAENGINE_V2 a, METAENGINE_V2 b) { return metadot_v2(metadot_max(a.x, b.x), metadot_max(a.y, b.y)); }
METADOT_INLINE METAENGINE_V2 metadot_clamp_v2(METAENGINE_V2 a, METAENGINE_V2 lo, METAENGINE_V2 hi) { return metadot_max_v2(lo, metadot_min_v2(a, hi)); }
METADOT_INLINE METAENGINE_V2 metadot_clamp01_v2(METAENGINE_V2 a) { return metadot_max_v2(metadot_v2(0, 0), metadot_min_v2(a, metadot_v2(1, 1))); }
METADOT_INLINE METAENGINE_V2 metadot_abs_v2(METAENGINE_V2 a) { return metadot_v2(fabsf(a.x), fabsf(a.y)); }
METADOT_INLINE float metadot_hmin(METAENGINE_V2 a) { return metadot_min(a.x, a.y); }
METADOT_INLINE float metadot_hmax(METAENGINE_V2 a) { return metadot_max(a.x, a.y); }
METADOT_INLINE float metadot_len(METAENGINE_V2 a) { return sqrtf(metadot_dot(a, a)); }
METADOT_INLINE float metadot_distance(METAENGINE_V2 a, METAENGINE_V2 b) {
    METAENGINE_V2 d = metadot_sub_v2(b, a);
    return sqrtf(metadot_dot(d, d));
}
METADOT_INLINE METAENGINE_V2 metadot_norm(METAENGINE_V2 a) { return metadot_div_v2_f(a, metadot_len(a)); }
METADOT_INLINE METAENGINE_V2 metadot_safe_norm(METAENGINE_V2 a) {
    float sq = metadot_dot(a, a);
    return sq ? metadot_div_v2_f(a, sqrtf(sq)) : metadot_v2(0, 0);
}
METADOT_INLINE float metadot_safe_norm_f(float a) { return a == 0 ? 0 : metadot_sign(a); }
METADOT_INLINE int metadot_safe_norm_int(int a) { return a == 0 ? 0 : metadot_sign_int(a); }
METADOT_INLINE METAENGINE_V2 metadot_neg_v2(METAENGINE_V2 a) { return metadot_v2(-a.x, -a.y); }
METADOT_INLINE METAENGINE_V2 metadot_lerp_v2(METAENGINE_V2 a, METAENGINE_V2 b, float t) { return metadot_add_v2(a, metadot_mul_v2_f(metadot_sub_v2(b, a), t)); }
METADOT_INLINE METAENGINE_V2 metadot_bezier(METAENGINE_V2 a, METAENGINE_V2 c0, METAENGINE_V2 b, float t) { return metadot_lerp_v2(metadot_lerp_v2(a, c0, t), metadot_lerp_v2(c0, b, t), t); }
METADOT_INLINE METAENGINE_V2 metadot_bezier2(METAENGINE_V2 a, METAENGINE_V2 c0, METAENGINE_V2 c1, METAENGINE_V2 b, float t) {
    return metadot_bezier(metadot_lerp_v2(a, c0, t), metadot_lerp_v2(c0, c1, t), metadot_lerp_v2(c1, b, t), t);
}
METADOT_INLINE int metadot_lesser_v2(METAENGINE_V2 a, METAENGINE_V2 b) { return a.x < b.x && a.y < b.y; }
METADOT_INLINE int metadot_greater_v2(METAENGINE_V2 a, METAENGINE_V2 b) { return a.x > b.x && a.y > b.y; }
METADOT_INLINE int metadot_lesser_equal_v2(METAENGINE_V2 a, METAENGINE_V2 b) { return a.x <= b.x && a.y <= b.y; }
METADOT_INLINE int metadot_greater_equal_v2(METAENGINE_V2 a, METAENGINE_V2 b) { return a.x >= b.x && a.y >= b.y; }
METADOT_INLINE METAENGINE_V2 metadot_floor(METAENGINE_V2 a) { return metadot_v2(floorf(a.x), floorf(a.y)); }
METADOT_INLINE METAENGINE_V2 metadot_round(METAENGINE_V2 a) { return metadot_v2(roundf(a.x), roundf(a.y)); }
METADOT_INLINE METAENGINE_V2 metadot_safe_invert_v2(METAENGINE_V2 a) { return metadot_v2(metadot_safe_invert(a.x), metadot_safe_invert(a.y)); }
METADOT_INLINE METAENGINE_V2 metadot_sign_v2(METAENGINE_V2 a) { return metadot_v2(metadot_sign(a.x), metadot_sign(a.y)); }

METADOT_INLINE int metadot_parallel(METAENGINE_V2 a, METAENGINE_V2 b, float tol) {
    float k = metadot_len(a) / metadot_len(b);
    b = metadot_mul_v2_f(b, k);
    if (fabs(a.x - b.x) < tol && fabs(a.y - b.y) < tol) return 1;
    return 0;
}

//--------------------------------------------------------------------------------------------------
// METAENGINE_SinCos rotation ops.

METADOT_INLINE METAENGINE_SinCos metadot_sincos_f(float radians) {
    METAENGINE_SinCos r;
    r.s = sinf(radians);
    r.c = cosf(radians);
    return r;
}
METADOT_INLINE METAENGINE_SinCos metadot_sincos() {
    METAENGINE_SinCos r;
    r.c = 1.0f;
    r.s = 0;
    return r;
}
METADOT_INLINE METAENGINE_V2 metadot_x_axis(METAENGINE_SinCos r) { return metadot_v2(r.c, r.s); }
METADOT_INLINE METAENGINE_V2 metadot_y_axis(METAENGINE_SinCos r) { return metadot_v2(-r.s, r.c); }
METADOT_INLINE METAENGINE_V2 metadot_mul_sc_v2(METAENGINE_SinCos a, METAENGINE_V2 b) { return metadot_v2(a.c * b.x - a.s * b.y, a.s * b.x + a.c * b.y); }
METADOT_INLINE METAENGINE_V2 metadot_mulT_sc_v2(METAENGINE_SinCos a, METAENGINE_V2 b) { return metadot_v2(a.c * b.x + a.s * b.y, -a.s * b.x + a.c * b.y); }
METADOT_INLINE METAENGINE_SinCos metadot_mul_sc(METAENGINE_SinCos a, METAENGINE_SinCos b) {
    METAENGINE_SinCos c;
    c.c = a.c * b.c - a.s * b.s;
    c.s = a.s * b.c + a.c * b.s;
    return c;
}
METADOT_INLINE METAENGINE_SinCos metadot_mulT_sc(METAENGINE_SinCos a, METAENGINE_SinCos b) {
    METAENGINE_SinCos c;
    c.c = a.c * b.c + a.s * b.s;
    c.s = a.c * b.s - a.s * b.c;
    return c;
}

// Remaps the result from atan2f to the continuous range of 0, 2*PI.
METADOT_INLINE float metadot_atan2_360(float y, float x) { return atan2f(-y, -x) + METAENGINE_PI; }
METADOT_INLINE float metadot_atan2_360_sc(METAENGINE_SinCos r) { return metadot_atan2_360(r.s, r.c); }
METADOT_INLINE float metadot_atan2_360_v2(METAENGINE_V2 v) { return atan2f(-v.y, -v.x) + METAENGINE_PI; }

// Computes the shortest angle to rotate the vector a to the vector b.
METADOT_INLINE float metadot_shortest_arc(METAENGINE_V2 a, METAENGINE_V2 b) {
    float c = metadot_dot(a, b);
    float s = metadot_det2(a, b);
    float theta = acosf(c);
    if (s > 0) {
        return theta;
    } else {
        return -theta;
    }
}

METADOT_INLINE float metadot_angle_diff(float radians_a, float radians_b) { return metadot_mod((radians_b - radians_a) + METAENGINE_PI, 2.0f * METAENGINE_PI) - METAENGINE_PI; }
METADOT_INLINE METAENGINE_V2 metadot_from_angle(float radians) { return metadot_v2(cosf(radians), sinf(radians)); }

//--------------------------------------------------------------------------------------------------
// m2 ops.
// 2D graphics matrix for only scale + rotate.

METADOT_INLINE METAENGINE_M2x2 metadot_mul_m2_f(METAENGINE_M2x2 a, float b) {
    METAENGINE_M2x2 c;
    c.x = metadot_mul_v2_f(a.x, b);
    c.y = metadot_mul_v2_f(a.y, b);
    return c;
}
METADOT_INLINE METAENGINE_V2 metadot_mul_m2_v2(METAENGINE_M2x2 a, METAENGINE_V2 b) {
    METAENGINE_V2 c;
    c.x = a.x.x * b.x + a.y.x * b.y;
    c.y = a.x.y * b.x + a.y.y * b.y;
    return c;
}
METADOT_INLINE METAENGINE_M2x2 metadot_mul_m2(METAENGINE_M2x2 a, METAENGINE_M2x2 b) {
    METAENGINE_M2x2 c;
    c.x = metadot_mul_m2_v2(a, b.x);
    c.y = metadot_mul_m2_v2(a, b.y);
    return c;
}

//--------------------------------------------------------------------------------------------------
// m3x2 ops.
// General purpose 2D graphics matrix; scale + rotate + translate.

METADOT_INLINE METAENGINE_V2 metadot_mul_m32_v2(METAENGINE_M3x2 a, METAENGINE_V2 b) { return metadot_add_v2(metadot_mul_m2_v2(a.m, b), a.p); }
METADOT_INLINE METAENGINE_M3x2 metadot_mul_m32(METAENGINE_M3x2 a, METAENGINE_M3x2 b) {
    METAENGINE_M3x2 c;
    c.m = metadot_mul_m2(a.m, b.m);
    c.p = metadot_add_v2(metadot_mul_m2_v2(a.m, b.p), a.p);
    return c;
}
METADOT_INLINE METAENGINE_M3x2 metadot_make_identity() {
    METAENGINE_M3x2 m;
    m.m.x = metadot_v2(1, 0);
    m.m.y = metadot_v2(0, 1);
    m.p = metadot_v2(0, 0);
    return m;
}
METADOT_INLINE METAENGINE_M3x2 metadot_make_translation_f(float x, float y) {
    METAENGINE_M3x2 m;
    m.m.x = metadot_v2(1, 0);
    m.m.y = metadot_v2(0, 1);
    m.p = metadot_v2(x, y);
    return m;
}
METADOT_INLINE METAENGINE_M3x2 metadot_make_translation(METAENGINE_V2 p) { return metadot_make_translation_f(p.x, p.y); }
METADOT_INLINE METAENGINE_M3x2 metadot_make_scale(METAENGINE_V2 s) {
    METAENGINE_M3x2 m;
    m.m.x = metadot_v2(s.x, 0);
    m.m.y = metadot_v2(0, s.y);
    m.p = metadot_v2(0, 0);
    return m;
}
METADOT_INLINE METAENGINE_M3x2 metadot_make_scale_f(float s) { return metadot_make_scale(metadot_v2(s, s)); }
METADOT_INLINE METAENGINE_M3x2 metadot_make_scale_translation(METAENGINE_V2 s, METAENGINE_V2 p) {
    METAENGINE_M3x2 m;
    m.m.x = metadot_v2(s.x, 0);
    m.m.y = metadot_v2(0, s.y);
    m.p = p;
    return m;
}
METADOT_INLINE METAENGINE_M3x2 metadot_make_scale_translation_f(float s, METAENGINE_V2 p) { return metadot_make_scale_translation(metadot_v2(s, s), p); }
METADOT_INLINE METAENGINE_M3x2 metadot_make_scale_translation_f_f(float sx, float sy, METAENGINE_V2 p) { return metadot_make_scale_translation(metadot_v2(sx, sy), p); }
METADOT_INLINE METAENGINE_M3x2 metadot_make_rotation(float radians) {
    METAENGINE_SinCos sc = metadot_sincos_f(radians);
    METAENGINE_M3x2 m;
    m.m.x = metadot_v2(sc.c, -sc.s);
    m.m.y = metadot_v2(sc.s, sc.c);
    m.p = metadot_v2(0, 0);
    return m;
}
METADOT_INLINE METAENGINE_M3x2 metadot_make_transform_TSR(METAENGINE_V2 p, METAENGINE_V2 s, float radians) {
    METAENGINE_SinCos sc = metadot_sincos_f(radians);
    METAENGINE_M3x2 m;
    m.m.x = metadot_mul_v2_f(metadot_v2(sc.c, -sc.s), s.x);
    m.m.y = metadot_mul_v2_f(metadot_v2(sc.s, sc.c), s.y);
    m.p = p;
    return m;
}

METADOT_INLINE METAENGINE_M3x2 metadot_invert(METAENGINE_M3x2 a) {
    float id = metadot_safe_invert(metadot_det2(a.m.x, a.m.y));
    METAENGINE_M3x2 b;
    b.m.x = metadot_v2(a.m.y.y * id, -a.m.x.y * id);
    b.m.y = metadot_v2(-a.m.y.x * id, a.m.x.x * id);
    b.p.x = (a.m.y.x * a.p.y - a.p.x * a.m.y.y) * id;
    b.p.y = (a.p.x * a.m.x.y - a.m.x.x * a.p.y) * id;
    return b;
}

//--------------------------------------------------------------------------------------------------
// Transform ops.
// No scale factor allowed here, good for physics + colliders.

METADOT_INLINE METAENGINE_Transform metadot_make_transform() {
    METAENGINE_Transform x;
    x.p = metadot_v2(0, 0);
    x.r = metadot_sincos();
    return x;
}
METADOT_INLINE METAENGINE_Transform metadot_make_transform_TR(METAENGINE_V2 p, float radians) {
    METAENGINE_Transform x;
    x.r = metadot_sincos_f(radians);
    x.p = p;
    return x;
}
METADOT_INLINE METAENGINE_V2 metadot_mul_tf_v2(METAENGINE_Transform a, METAENGINE_V2 b) { return metadot_add_v2(metadot_mul_sc_v2(a.r, b), a.p); }
METADOT_INLINE METAENGINE_V2 metadot_mulT_tf_v2(METAENGINE_Transform a, METAENGINE_V2 b) { return metadot_mulT_sc_v2(a.r, metadot_sub_v2(b, a.p)); }
METADOT_INLINE METAENGINE_Transform metadot_mul_tf(METAENGINE_Transform a, METAENGINE_Transform b) {
    METAENGINE_Transform c;
    c.r = metadot_mul_sc(a.r, b.r);
    c.p = metadot_add_v2(metadot_mul_sc_v2(a.r, b.p), a.p);
    return c;
}
METADOT_INLINE METAENGINE_Transform metadot_mulT_tf(METAENGINE_Transform a, METAENGINE_Transform b) {
    METAENGINE_Transform c;
    c.r = metadot_mulT_sc(a.r, b.r);
    c.p = metadot_mulT_sc_v2(a.r, metadot_sub_v2(b.p, a.p));
    return c;
}

//--------------------------------------------------------------------------------------------------
// Halfspace (plane/line) ops.
// Functions for infinite lines.

METADOT_INLINE METAENGINE_Halfspace metadot_plane(METAENGINE_V2 n, float d) {
    METAENGINE_Halfspace h;
    h.n = n;
    h.d = d;
    return h;
}
METADOT_INLINE METAENGINE_Halfspace metadot_plane2(METAENGINE_V2 n, METAENGINE_V2 p) {
    METAENGINE_Halfspace h;
    h.n = n;
    h.d = metadot_dot(n, p);
    return h;
}
METADOT_INLINE METAENGINE_V2 metadot_origin(METAENGINE_Halfspace h) { return metadot_mul_v2_f(h.n, h.d); }
METADOT_INLINE float metadot_distance_hs(METAENGINE_Halfspace h, METAENGINE_V2 p) { return metadot_dot(h.n, p) - h.d; }
METADOT_INLINE METAENGINE_V2 metadot_project(METAENGINE_Halfspace h, METAENGINE_V2 p) { return metadot_sub_v2(p, metadot_mul_v2_f(h.n, metadot_distance_hs(h, p))); }
METADOT_INLINE METAENGINE_Halfspace metadot_mul_tf_hs(METAENGINE_Transform a, METAENGINE_Halfspace b) {
    METAENGINE_Halfspace c;
    c.n = metadot_mul_sc_v2(a.r, b.n);
    c.d = metadot_dot(metadot_mul_tf_v2(a, metadot_origin(b)), c.n);
    return c;
}
METADOT_INLINE METAENGINE_Halfspace metadot_mulT_tf_hs(METAENGINE_Transform a, METAENGINE_Halfspace b) {
    METAENGINE_Halfspace c;
    c.n = metadot_mulT_sc_v2(a.r, b.n);
    c.d = metadot_dot(metadot_mulT_tf_v2(a, metadot_origin(b)), c.n);
    return c;
}
METADOT_INLINE METAENGINE_V2 metadot_intersect_halfspace(METAENGINE_V2 a, METAENGINE_V2 b, float da, float db) { return metadot_add_v2(a, metadot_mul_v2_f(metadot_sub_v2(b, a), (da / (da - db)))); }
METADOT_INLINE METAENGINE_V2 metadot_intersect_halfspace2(METAENGINE_Halfspace h, METAENGINE_V2 a, METAENGINE_V2 b) {
    return metadot_intersect_halfspace(a, b, metadot_distance_hs(h, a), metadot_distance_hs(h, b));
}

//--------------------------------------------------------------------------------------------------
// AABB helpers.

METADOT_INLINE METAENGINE_Aabb metadot_make_aabb(METAENGINE_V2 min, METAENGINE_V2 max) {
    METAENGINE_Aabb bb;
    bb.min = min;
    bb.max = max;
    return bb;
}
METADOT_INLINE METAENGINE_Aabb metadot_make_aabb_pos_w_h(METAENGINE_V2 pos, float w, float h) {
    METAENGINE_Aabb bb;
    METAENGINE_V2 he = metadot_mul_v2_f(metadot_v2(w, h), 0.5f);
    bb.min = metadot_sub_v2(pos, he);
    bb.max = metadot_add_v2(pos, he);
    return bb;
}
METADOT_INLINE METAENGINE_Aabb metadot_make_aabb_center_half_extents(METAENGINE_V2 center, METAENGINE_V2 half_extents) {
    METAENGINE_Aabb bb;
    bb.min = metadot_sub_v2(center, half_extents);
    bb.max = metadot_add_v2(center, half_extents);
    return bb;
}
METADOT_INLINE METAENGINE_Aabb metadot_make_aabb_from_top_left(METAENGINE_V2 top_left, float w, float h) {
    return metadot_make_aabb(metadot_add_v2(top_left, metadot_v2(0, -h)), metadot_add_v2(top_left, metadot_v2(w, 0)));
}
METADOT_INLINE float metadot_width(METAENGINE_Aabb bb) { return bb.max.x - bb.min.x; }
METADOT_INLINE float metadot_height(METAENGINE_Aabb bb) { return bb.max.y - bb.min.y; }
METADOT_INLINE float metadot_half_width(METAENGINE_Aabb bb) { return metadot_width(bb) * 0.5f; }
METADOT_INLINE float metadot_half_height(METAENGINE_Aabb bb) { return metadot_height(bb) * 0.5f; }
METADOT_INLINE METAENGINE_V2 metadot_half_extents(METAENGINE_Aabb bb) { return (metadot_mul_v2_f(metadot_sub_v2(bb.max, bb.min), 0.5f)); }
METADOT_INLINE METAENGINE_V2 metadot_extents(METAENGINE_Aabb aabb) { return metadot_sub_v2(aabb.max, aabb.min); }
METADOT_INLINE METAENGINE_Aabb metadot_expand_aabb(METAENGINE_Aabb aabb, METAENGINE_V2 v) { return metadot_make_aabb(metadot_sub_v2(aabb.min, v), metadot_add_v2(aabb.max, v)); }
METADOT_INLINE METAENGINE_Aabb metadot_expand_aabb_f(METAENGINE_Aabb aabb, float v) {
    METAENGINE_V2 factor = metadot_v2(v, v);
    return metadot_make_aabb(metadot_sub_v2(aabb.min, factor), metadot_add_v2(aabb.max, factor));
}
METADOT_INLINE METAENGINE_V2 metadot_min_aabb(METAENGINE_Aabb bb) { return bb.min; }
METADOT_INLINE METAENGINE_V2 metadot_max_aabb(METAENGINE_Aabb bb) { return bb.max; }
METADOT_INLINE METAENGINE_V2 metadot_midpoint(METAENGINE_Aabb bb) { return metadot_mul_v2_f(metadot_add_v2(bb.min, bb.max), 0.5f); }
METADOT_INLINE METAENGINE_V2 metadot_center(METAENGINE_Aabb bb) { return metadot_mul_v2_f(metadot_add_v2(bb.min, bb.max), 0.5f); }
METADOT_INLINE METAENGINE_V2 metadot_top_left(METAENGINE_Aabb bb) { return metadot_v2(bb.min.x, bb.max.y); }
METADOT_INLINE METAENGINE_V2 metadot_top_right(METAENGINE_Aabb bb) { return metadot_v2(bb.max.x, bb.max.y); }
METADOT_INLINE METAENGINE_V2 metadot_bottom_left(METAENGINE_Aabb bb) { return metadot_v2(bb.min.x, bb.min.y); }
METADOT_INLINE METAENGINE_V2 metadot_bottom_right(METAENGINE_Aabb bb) { return metadot_v2(bb.max.x, bb.min.y); }
METADOT_INLINE bool metadot_contains_point(METAENGINE_Aabb bb, METAENGINE_V2 p) { return metadot_greater_equal_v2(p, bb.min) && metadot_lesser_equal_v2(p, bb.max); }
METADOT_INLINE bool metadot_contains_aabb(METAENGINE_Aabb a, METAENGINE_Aabb b) { return metadot_lesser_equal_v2(a.min, b.min) && metadot_greater_equal_v2(a.max, b.max); }
METADOT_INLINE float metadot_surface_area_aabb(METAENGINE_Aabb bb) { return 2.0f * metadot_width(bb) * metadot_height(bb); }
METADOT_INLINE float metadot_area_aabb(METAENGINE_Aabb bb) { return metadot_width(bb) * metadot_height(bb); }
METADOT_INLINE METAENGINE_V2 metadot_clamp_aabb_v2(METAENGINE_Aabb bb, METAENGINE_V2 p) { return metadot_clamp_v2(p, bb.min, bb.max); }
METADOT_INLINE METAENGINE_Aabb metadot_clamp_aabb(METAENGINE_Aabb a, METAENGINE_Aabb b) { return metadot_make_aabb(metadot_clamp_v2(a.min, b.min, b.max), metadot_clamp_v2(a.max, b.min, b.max)); }
METADOT_INLINE METAENGINE_Aabb metadot_combine(METAENGINE_Aabb a, METAENGINE_Aabb b) { return metadot_make_aabb(metadot_min_v2(a.min, b.min), metadot_max_v2(a.max, b.max)); }

METADOT_INLINE int metadot_overlaps(METAENGINE_Aabb a, METAENGINE_Aabb b) {
    int d0 = b.max.x < a.min.x;
    int d1 = a.max.x < b.min.x;
    int d2 = b.max.y < a.min.y;
    int d3 = a.max.y < b.min.y;
    return !(d0 | d1 | d2 | d3);
}

METADOT_INLINE int metadot_collide_aabb(METAENGINE_Aabb a, METAENGINE_Aabb b) { return metadot_overlaps(a, b); }

METADOT_INLINE METAENGINE_Aabb metadot_make_aabb_verts(METAENGINE_V2 *verts, int count) {
    METAENGINE_V2 vmin = verts[0];
    METAENGINE_V2 vmax = vmin;
    for (int i = 0; i < count; ++i) {
        vmin = metadot_min_v2(vmin, verts[i]);
        vmax = metadot_max_v2(vmax, verts[i]);
    }
    return metadot_make_aabb(vmin, vmax);
}

METADOT_INLINE void metadot_aabb_verts(METAENGINE_V2 *out, METAENGINE_Aabb bb) {
    out[0] = bb.min;
    out[1] = metadot_v2(bb.max.x, bb.min.y);
    out[2] = bb.max;
    out[3] = metadot_v2(bb.min.x, bb.max.y);
}

//--------------------------------------------------------------------------------------------------
// Circle helpers.

METADOT_INLINE float metadot_area_circle(METAENGINE_Circle c) { return 3.14159265f * c.r * c.r; }
METADOT_INLINE float metadot_surface_area_circle(METAENGINE_Circle c) { return 2.0f * 3.14159265f * c.r; }
METADOT_INLINE METAENGINE_Circle metadot_mul_tf_circle(METAENGINE_Transform tx, METAENGINE_Circle a) {
    METAENGINE_Circle b;
    b.p = metadot_mul_tf_v2(tx, a.p);
    b.r = a.r;
    return b;
}

//--------------------------------------------------------------------------------------------------
// Ray ops.
// Full raycasting suite is farther down below in this file.

METADOT_INLINE METAENGINE_V2 metadot_impact(METAENGINE_Ray r, float t) { return metadot_add_v2(r.p, metadot_mul_v2_f(r.d, t)); }
METADOT_INLINE METAENGINE_V2 metadot_endpoint(METAENGINE_Ray r) { return metadot_add_v2(r.p, metadot_mul_v2_f(r.d, r.t)); }

METADOT_INLINE int metadot_ray_to_halfpsace(METAENGINE_Ray A, METAENGINE_Halfspace B, METAENGINE_Raycast *out) {
    float da = metadot_distance_hs(B, A.p);
    float db = metadot_distance_hs(B, metadot_impact(A, A.t));
    if (da * db > 0) return 0;
    out->n = metadot_mul_v2_f(B.n, metadot_sign(da));
    out->t = metadot_intersect(da, db);
    return 1;
}

// http://www.randygaul.net/2014/07/23/distance-point-to-line-segment/
METADOT_INLINE float metadot_distance_sq(METAENGINE_V2 a, METAENGINE_V2 b, METAENGINE_V2 p) {
    METAENGINE_V2 n = metadot_sub_v2(b, a);
    METAENGINE_V2 pa = metadot_sub_v2(a, p);
    float c = metadot_dot(n, pa);

    // Closest point is a
    if (c > 0.0f) return metadot_dot(pa, pa);

    // Closest point is b
    METAENGINE_V2 bp = metadot_sub_v2(p, b);
    if (metadot_dot(n, bp) > 0.0f) return metadot_dot(bp, bp);

    // Closest point is between a and b
    METAENGINE_V2 e = metadot_sub_v2(pa, metadot_mul_v2_f(n, (c / metadot_dot(n, n))));
    return metadot_dot(e, e);
}

//--------------------------------------------------------------------------------------------------
// Collision detection.

// It's quite common to limit the number of verts on polygons to a low number. Feel free to adjust
// this number if needed, but be warned: higher than 8 and shapes generally start to look more like
// circles/ovals; it becomes pointless beyond a certain point.
#define METAENGINE_POLY_MAX_VERTS 8

// 2D polygon. Verts are ordered in counter-clockwise order (CCW).
typedef struct METAENGINE_Poly {
    int count;
    METAENGINE_V2 verts[METAENGINE_POLY_MAX_VERTS];
    METAENGINE_V2 norms[METAENGINE_POLY_MAX_VERTS];  // Pointing perpendicular along the poly's surface.
                                                     // Rotated vert[i] to vert[i + 1] 90 degrees CCW + normalized.
} METAENGINE_Poly;

// 2D capsule shape. It's like a shrink-wrap of 2 circles connected by a rod.
typedef struct METAENGINE_Capsule {
    METAENGINE_V2 a;
    METAENGINE_V2 b;
    float r;
} METAENGINE_Capsule;

// Contains all information necessary to resolve a collision, or in other words
// this is the information needed to separate shapes that are colliding. Doing
// the resolution step is *not* included.
typedef struct METAENGINE_Manifold {
    int count;
    float depths[2];
    METAENGINE_V2 contact_points[2];

    // Always points from shape A to shape B.
    METAENGINE_V2 n;
} METAENGINE_Manifold;

// Boolean collision detection functions.
// These versions are slightly faster/simpler than the manifold versions, but only give a YES/NO result.
bool METADOT_CDECL metadot_circle_to_circle(METAENGINE_Circle A, METAENGINE_Circle B);
bool METADOT_CDECL metadot_circle_to_aabb(METAENGINE_Circle A, METAENGINE_Aabb B);
bool METADOT_CDECL metadot_circle_to_capsule(METAENGINE_Circle A, METAENGINE_Capsule B);
bool METADOT_CDECL metadot_aabb_to_aabb(METAENGINE_Aabb A, METAENGINE_Aabb B);
bool METADOT_CDECL metadot_aabb_to_capsule(METAENGINE_Aabb A, METAENGINE_Capsule B);
bool METADOT_CDECL metadot_capsule_to_capsule(METAENGINE_Capsule A, METAENGINE_Capsule B);
bool METADOT_CDECL metadot_circle_to_poly(METAENGINE_Circle A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx);
bool METADOT_CDECL metadot_aabb_to_poly(METAENGINE_Aabb A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx);
bool METADOT_CDECL metadot_capsule_to_poly(METAENGINE_Capsule A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx);
bool METADOT_CDECL metadot_poly_to_poly(const METAENGINE_Poly *A, const METAENGINE_Transform *ax, const METAENGINE_Poly *B, const METAENGINE_Transform *bx);

// Ray casting.
// Output is placed into the `METAENGINE_Raycast` struct, which represents the hit location
// of the ray. The out param contains no meaningful information if these funcs
// return false.
bool METADOT_CDECL metadot_ray_to_circle(METAENGINE_Ray A, METAENGINE_Circle B, METAENGINE_Raycast *out);
bool METADOT_CDECL metadot_ray_to_aabb(METAENGINE_Ray A, METAENGINE_Aabb B, METAENGINE_Raycast *out);
bool METADOT_CDECL metadot_ray_to_capsule(METAENGINE_Ray A, METAENGINE_Capsule B, METAENGINE_Raycast *out);
bool METADOT_CDECL metadot_ray_to_poly(METAENGINE_Ray A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx_ptr, METAENGINE_Raycast *out);

// Manifold generation.
// These functions are (generally) slower + more complex than bool versions, but compute one
// or two points that represent the plane of contact. This information is
// is usually needed to resolve and prevent shapes from colliding. If no coll-
// ision occured the `count` member of the manifold typedef struct is set to 0.
void METADOT_CDECL metadot_circle_to_circle_manifold(METAENGINE_Circle A, METAENGINE_Circle B, METAENGINE_Manifold *m);
void METADOT_CDECL metadot_circle_to_aabb_manifold(METAENGINE_Circle A, METAENGINE_Aabb B, METAENGINE_Manifold *m);
void METADOT_CDECL metadot_circle_to_capsule_manifold(METAENGINE_Circle A, METAENGINE_Capsule B, METAENGINE_Manifold *m);
void METADOT_CDECL metadot_aabb_to_aabb_manifold(METAENGINE_Aabb A, METAENGINE_Aabb B, METAENGINE_Manifold *m);
void METADOT_CDECL metadot_aabb_to_capsule_manifold(METAENGINE_Aabb A, METAENGINE_Capsule B, METAENGINE_Manifold *m);
void METADOT_CDECL metadot_capsule_to_capsule_manifold(METAENGINE_Capsule A, METAENGINE_Capsule B, METAENGINE_Manifold *m);
void METADOT_CDECL metadot_circle_to_poly_manifold(METAENGINE_Circle A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx, METAENGINE_Manifold *m);
void METADOT_CDECL metadot_aabb_to_poly_manifold(METAENGINE_Aabb A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx, METAENGINE_Manifold *m);
void METADOT_CDECL metadot_capsule_to_poly_manifold(METAENGINE_Capsule A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx, METAENGINE_Manifold *m);
void METADOT_CDECL metadot_poly_to_poly_manifold(const METAENGINE_Poly *A, const METAENGINE_Transform *ax, const METAENGINE_Poly *B, const METAENGINE_Transform *bx, METAENGINE_Manifold *m);

#define METAENGINE_SHAPE_TYPE_DEFS         \
    METAENGINE_ENUM(SHAPE_TYPE_NONE, 0)    \
    METAENGINE_ENUM(SHAPE_TYPE_CIRCLE, 1)  \
    METAENGINE_ENUM(SHAPE_TYPE_AABB, 2)    \
    METAENGINE_ENUM(SHAPE_TYPE_CAPSULE, 3) \
    METAENGINE_ENUM(SHAPE_TYPE_POLY, 4)

typedef enum METAENGINE_ShapeType {
#define METAENGINE_ENUM(K, V) METAENGINE_##K = V,
    METAENGINE_SHAPE_TYPE_DEFS
#undef METAENGINE_ENUM
} METAENGINE_ShapeType;

METADOT_INLINE const char *metadot_shape_type_to_string(METAENGINE_ShapeType type) {
    switch (type) {
#define METAENGINE_ENUM(K, V) \
    case METAENGINE_##K:      \
        return METAENGINE_STRINGIZE(METAENGINE_##K);
        METAENGINE_SHAPE_TYPE_DEFS
#undef METAENGINE_ENUM
        default:
            return NULL;
    }
}

// This typedef struct is only for advanced usage of the `metadot_gjk` function. See comments inside of the
// `metadot_gjk` function for more details.
typedef struct METAENGINE_GjkCache {
    float metric;
    int count;
    int iA[3];
    int iB[3];
    float div;
} METAENGINE_GjkCache;

// This is an advanced function, intended to be used by people who know what they're doing.
//
// Runs the GJK algorithm to find closest points, returns distance between closest points.
// outA and outB can be NULL, in this case only distance is returned. ax_ptr and bx_ptr
// can be NULL, and represent local to world transformations for shapes A and B respectively.
// use_radius will apply radii for capsules and circles (if set to false, spheres are
// treated as points and capsules are treated as line segments i.e. rays). The cache parameter
// should be NULL, as it is only for advanced usage (unless you know what you're doing, then
// go ahead and use it). iterations is an optional parameter.
//
//
// IMPORTANT NOTE
//
//     The GJK function is sensitive to large shapes, since it internally will compute signed area
//     values. `metadot_gjk` is called throughout this file in many ways, so try to make sure all of your
//     collision shapes are not gigantic. For example, try to keep the volume of all your shapes
//     less than 100.0f. If you need large shapes, you should use tiny collision geometry for all
//     function here, and simply render the geometry larger on-screen by scaling it up.
//
float METADOT_CDECL metadot_gjk(const void *A, METAENGINE_ShapeType typeA, const METAENGINE_Transform *ax_ptr, const void *B, METAENGINE_ShapeType typeB, const METAENGINE_Transform *bx_ptr,
                                METAENGINE_V2 *outA, METAENGINE_V2 *outB, int use_radius, int *iterations, METAENGINE_GjkCache *cache);

// Stores results of a time of impact calculation done by `metadot_toi`.
typedef struct METAENGINE_ToiResult {
    int hit;          // 1 if shapes were touching at the TOI, 0 if they never hit.
    float toi;        // The time of impact between two shapes.
    METAENGINE_V2 n;  // Surface normal from shape A to B at the time of impact.
    METAENGINE_V2 p;  // Point of contact between shapes A and B at time of impact.
    int iterations;   // Number of iterations the solver underwent.
} METAENGINE_ToiResult;

// This is an advanced function, intended to be used by people who know what they're doing.
//
// Computes the time of impact from shape A and shape B. The velocity of each shape is provided
// by vA and vB respectively. The shapes are *not* allowed to rotate over time. The velocity is
// assumed to represent the change in motion from time 0 to time 1, and so the return value will
// be a number from 0 to 1. To move each shape to the colliding configuration, multiply vA and vB
// each by the return value. ax_ptr and bx_ptr are optional parameters to transforms for each shape,
// and are typically used for polygon shapes to transform from model to world space. Set these to
// NULL to represent identity transforms. iterations is an optional parameter. use_radius
// will apply radii for capsules and circles (if set to false, spheres are treated as points and
// capsules are treated as line segments i.e. rays).
//
// IMPORTANT NOTE:
// The metadot_toi function can be used to implement a "swept character controller", but it can be
// difficult to do so. Say we compute a time of impact with `metadot_toi` and move the shapes to the
// time of impact, and adjust the velocity by zeroing out the velocity along the surface normal.
// If we then call `metadot_toi` again, it will fail since the shapes will be considered to start in
// a colliding configuration. There are many styles of tricks to get around this problem, and
// all of them involve giving the next call to `metadot_toi` some breathing room. It is recommended
// to use some variation of the following algorithm:
//
// 1. Call metadot_toi.
// 2. Move the shapes to the TOI.
// 3. Slightly inflate the size of one, or both, of the shapes so they will be intersecting.
//    The purpose is to make the shapes numerically intersecting, but not visually intersecting.
//    Another option is to call metadot_toi with slightly deflated shapes.
//    See the function `metadot_inflate` for some more details.
// 4. Compute the collision manifold between the inflated shapes (for example, use poly_ttoPolyManifold).
// 5. Gently push the shapes apart. This will give the next call to metadot_toi some breathing room.
METAENGINE_ToiResult METADOT_CDECL metadot_toi(const void *A, METAENGINE_ShapeType typeA, const METAENGINE_Transform *ax_ptr, METAENGINE_V2 vA, const void *B, METAENGINE_ShapeType typeB,
                                               const METAENGINE_Transform *bx_ptr, METAENGINE_V2 vB, int use_radius);

// Inflating a shape.
//
// This is useful to numerically grow or shrink a polytope. For example, when calling
// a time of impact function it can be good to use a slightly smaller shape. Then, once
// both shapes are moved to the time of impact a collision manifold can be made from the
// slightly larger (and now overlapping) shapes.
//
// IMPORTANT NOTE
// Inflating a shape with sharp corners can cause those corners to move dramatically.
// Deflating a shape can avoid this problem, but deflating a very small shape can invert
// the planes and result in something that is no longer convex. Make sure to pick an
// appropriately small skin factor, for example 1.0e-6f.
void METADOT_CDECL metadot_inflate(void *shape, METAENGINE_ShapeType type, float skin_factor);

// Computes 2D convex hull. Will not do anything if less than two verts supplied. If
// more than METAENGINE_POLY_MAX_VERTS are supplied extras are ignored.
int METADOT_CDECL metadot_hull(METAENGINE_V2 *verts, int count);
void METADOT_CDECL metadot_norms(METAENGINE_V2 *verts, METAENGINE_V2 *norms, int count);

// runs metadot_hull and metadot_norms, assumes p->verts and p->count are both set to valid values
void METADOT_CDECL metadot_make_poly(METAENGINE_Poly *p);
METAENGINE_V2 METADOT_CDECL metadot_centroid(const METAENGINE_V2 *verts, int count);

// Generic collision detection routines, useful for games that want to use some poly-
// morphism to write more generic-styled code. Internally calls various above functions.
// For AABBs/Circles/Capsules ax and bx are ignored. For polys ax and bx can define
// model to world transformations (for polys only), or be NULL for identity transforms.
int METADOT_CDECL metadot_collided(const void *A, const METAENGINE_Transform *ax, METAENGINE_ShapeType typeA, const void *B, const METAENGINE_Transform *bx, METAENGINE_ShapeType typeB);
void METADOT_CDECL metadot_collide(const void *A, const METAENGINE_Transform *ax, METAENGINE_ShapeType typeA, const void *B, const METAENGINE_Transform *bx, METAENGINE_ShapeType typeB,
                                   METAENGINE_Manifold *m);
bool METADOT_CDECL metadot_cast_ray(METAENGINE_Ray A, const void *B, const METAENGINE_Transform *bx, METAENGINE_ShapeType typeB, METAENGINE_Raycast *out);

#ifdef __cplusplus
}
#endif  // __cplusplus

//--------------------------------------------------------------------------------------------------
// C++ API

namespace MetaEngine {

using v2 = METAENGINE_V2;

METADOT_INLINE v2 V2(float x, float y) {
    v2 result;
    result.x = x;
    result.y = y;
    return result;
}

METADOT_INLINE v2 operator+(v2 a, v2 b) { return V2(a.x + b.x, a.y + b.y); }
METADOT_INLINE v2 operator-(v2 a, v2 b) { return V2(a.x - b.x, a.y - b.y); }
METADOT_INLINE v2 &operator+=(v2 &a, v2 b) { return a = a + b; }
METADOT_INLINE v2 &operator-=(v2 &a, v2 b) { return a = a - b; }

METADOT_INLINE v2 operator*(v2 a, float b) { return V2(a.x * b, a.y * b); }
METADOT_INLINE v2 operator*(v2 a, v2 b) { return V2(a.x * b.x, a.y * b.y); }
METADOT_INLINE v2 &operator*=(v2 &a, float b) { return a = a * b; }
METADOT_INLINE v2 &operator*=(v2 &a, v2 b) { return a = a * b; }
METADOT_INLINE v2 operator/(v2 a, float b) { return V2(a.x / b, a.y / b); }
METADOT_INLINE v2 operator/(v2 a, v2 b) { return V2(a.x / b.x, a.y / b.y); }
METADOT_INLINE v2 &operator/=(v2 &a, float b) { return a = a / b; }
METADOT_INLINE v2 &operator/=(v2 &a, v2 b) { return a = a / b; }

METADOT_INLINE v2 operator-(v2 a) { return V2(-a.x, -a.y); }

METADOT_INLINE int operator<(v2 a, v2 b) { return a.x < b.x && a.y < b.y; }
METADOT_INLINE int operator>(v2 a, v2 b) { return a.x > b.x && a.y > b.y; }
METADOT_INLINE int operator<=(v2 a, v2 b) { return a.x <= b.x && a.y <= b.y; }
METADOT_INLINE int operator>=(v2 a, v2 b) { return a.x >= b.x && a.y >= b.y; }

using SinCos = METAENGINE_SinCos;
using M2x2 = METAENGINE_M2x2;
using M3x2 = METAENGINE_M3x2;
using MTransform = METAENGINE_Transform;
using Halfspace = METAENGINE_Halfspace;
using Ray = METAENGINE_Ray;
using Raycast = METAENGINE_Raycast;
using Circle = METAENGINE_Circle;
using Aabb = METAENGINE_Aabb;
using Rect = METAENGINE_Rect;
using Poly = METAENGINE_Poly;
using Capsule = METAENGINE_Capsule;
using Manifold = METAENGINE_Manifold;
using GjkCache = METAENGINE_GjkCache;
using ToiResult = METAENGINE_ToiResult;

using ShapeType = METAENGINE_ShapeType;
#define METAENGINE_ENUM(K, V) METADOT_INLINE constexpr ShapeType K = METAENGINE_##K;
METAENGINE_SHAPE_TYPE_DEFS
#undef METAENGINE_ENUM

METADOT_INLINE const char *to_string(ShapeType type) {
    switch (type) {
#define METAENGINE_ENUM(K, V) \
    case METAENGINE_##K:      \
        return #K;
        METAENGINE_SHAPE_TYPE_DEFS
#undef METAENGINE_ENUM
        default:
            return NULL;
    }
}

METADOT_INLINE float min(float a, float b) { return metadot_min(a, b); }
METADOT_INLINE float max(float a, float b) { return metadot_max(a, b); }
METADOT_INLINE float clamp(float a, float lo, float hi) { return metadot_clamp(a, lo, hi); }
METADOT_INLINE float clamp01(float a) { return metadot_clamp01(a); }
METADOT_INLINE float sign(float a) { return metadot_sign(a); }
METADOT_INLINE float intersect(float da, float db) { return metadot_intersect(da, db); }
METADOT_INLINE float safe_invert(float a) { return metadot_safe_invert(a); }
METADOT_INLINE float lerp_f(float a, float b, float t) { return metadot_lerp(a, b, t); }
METADOT_INLINE float remap(float t, float lo, float hi) { return metadot_remap(t, lo, hi); }
METADOT_INLINE float mod(float x, float m) { return metadot_mod(x, m); }
METADOT_INLINE float fract(float x) { return metadot_fract(x); }

METADOT_INLINE int sign(int a) { return metadot_sign_int(a); }
METADOT_INLINE int min(int a, int b) { return metadot_min(a, b); }
METADOT_INLINE int max(int a, int b) { return metadot_max(a, b); }
METADOT_INLINE uint64_t min(uint64_t a, uint64_t b) { return metadot_min(a, b); }
METADOT_INLINE uint64_t max(uint64_t a, uint64_t b) { return metadot_max(a, b); }
METADOT_INLINE float abs(float a) { return metadot_abs(a); }
METADOT_INLINE int abs(int a) { return metadot_abs_int(a); }
METADOT_INLINE int clamp(int a, int lo, int hi) { return metadot_clamp_int(a, lo, hi); }
METADOT_INLINE int clamp01(int a) { return metadot_clamp01_int(a); }
METADOT_INLINE bool is_even(int x) { return metadot_is_even(x); }
METADOT_INLINE bool is_odd(int x) { return metadot_is_odd(x); }

METADOT_INLINE bool is_power_of_two(int a) { return metadot_is_power_of_two(a); }
METADOT_INLINE bool is_power_of_two(uint64_t a) { return metadot_is_power_of_two_uint(a); }
METADOT_INLINE int fit_power_of_two(int a) { return metadot_fit_power_of_two(a); }

METADOT_INLINE float smoothstep(float x) { return metadot_smoothstep(x); }
METADOT_INLINE float quad_in(float x) { return metadot_quad_in(x); }
METADOT_INLINE float quad_out(float x) { return metadot_quad_out(x); }
METADOT_INLINE float quad_in_out(float x) { return metadot_quad_in_out(x); }
METADOT_INLINE float cube_in(float x) { return metadot_cube_in(x); }
METADOT_INLINE float cube_out(float x) { return metadot_cube_out(x); }
METADOT_INLINE float cube_in_out(float x) { return metadot_cube_in_out(x); }
METADOT_INLINE float quart_in(float x) { return metadot_quart_in(x); }
METADOT_INLINE float quart_out(float x) { return metadot_quart_out(x); }
METADOT_INLINE float quart_in_out(float x) { return metadot_quart_in_out(x); }
METADOT_INLINE float quint_in(float x) { return metadot_quint_in(x); }
METADOT_INLINE float quint_out(float x) { return metadot_quint_out(x); }
METADOT_INLINE float quint_in_out(float x) { return metadot_quint_in_out(x); }
METADOT_INLINE float sin_in(float x) { return metadot_sin_in(x); }
METADOT_INLINE float sin_out(float x) { return metadot_sin_out(x); }
METADOT_INLINE float sin_in_out(float x) { return metadot_sin_in_out(x); }
METADOT_INLINE float circle_in(float x) { return metadot_circle_in(x); }
METADOT_INLINE float circle_out(float x) { return metadot_circle_out(x); }
METADOT_INLINE float circle_in_out(float x) { return metadot_circle_in_out(x); }
METADOT_INLINE float back_in(float x) { return metadot_back_in(x); }
METADOT_INLINE float back_out(float x) { return metadot_back_out(x); }
METADOT_INLINE float back_in_out(float x) { return metadot_back_in_out(x); }

METADOT_INLINE float dot(v2 a, v2 b) { return metadot_dot(a, b); }

METADOT_INLINE v2 skew(v2 a) { return metadot_skew(a); }
METADOT_INLINE v2 cw90(v2 a) { return metadot_cw90(a); }
METADOT_INLINE float det2(v2 a, v2 b) { return metadot_det2(a, b); }
METADOT_INLINE float cross(v2 a, v2 b) { return metadot_cross(a, b); }
METADOT_INLINE v2 cross(v2 a, float b) { return metadot_cross_v2_f(a, b); }
METADOT_INLINE v2 cross(float a, v2 b) { return metadot_cross_f_v2(a, b); }
METADOT_INLINE v2 min(v2 a, v2 b) { return metadot_min_v2(a, b); }
METADOT_INLINE v2 max(v2 a, v2 b) { return metadot_max_v2(a, b); }
METADOT_INLINE v2 clamp(v2 a, v2 lo, v2 hi) { return metadot_clamp_v2(a, lo, hi); }
METADOT_INLINE v2 clamp01(v2 a) { return metadot_clamp01_v2(a); }
METADOT_INLINE v2 abs(v2 a) { return metadot_abs_v2(a); }
METADOT_INLINE float hmin(v2 a) { return metadot_hmin(a); }
METADOT_INLINE float hmax(v2 a) { return metadot_hmax(a); }
METADOT_INLINE float len(v2 a) { return metadot_len(a); }
METADOT_INLINE float distance(v2 a, v2 b) { return metadot_distance(a, b); }
METADOT_INLINE v2 norm(v2 a) { return metadot_norm(a); }
METADOT_INLINE v2 safe_norm(v2 a) { return metadot_safe_norm(a); }
METADOT_INLINE float safe_norm(float a) { return metadot_safe_norm_f(a); }
METADOT_INLINE int safe_norm(int a) { return metadot_safe_norm_int(a); }

METADOT_INLINE v2 lerp_v2(v2 a, v2 b, float t) { return metadot_lerp_v2(a, b, t); }
METADOT_INLINE v2 bezier(v2 a, v2 c0, v2 b, float t) { return metadot_bezier(a, c0, b, t); }
METADOT_INLINE v2 bezier(v2 a, v2 c0, v2 c1, v2 b, float t) { return metadot_bezier2(a, c0, c1, b, t); }
METADOT_INLINE v2 floor(v2 a) { return metadot_floor(a); }
METADOT_INLINE v2 round(v2 a) { return metadot_round(a); }
METADOT_INLINE v2 safe_invert(v2 a) { return metadot_safe_invert_v2(a); }
METADOT_INLINE v2 sign(v2 a) { return metadot_sign_v2(a); }

METADOT_INLINE int parallel(v2 a, v2 b, float tol) { return metadot_parallel(a, b, tol); }

METADOT_INLINE SinCos sincos(float radians) { return metadot_sincos_f(radians); }
METADOT_INLINE SinCos sincos() { return metadot_sincos(); }
METADOT_INLINE v2 x_axis(SinCos r) { return metadot_x_axis(r); }
METADOT_INLINE v2 y_axis(SinCos r) { return metadot_y_axis(r); }
METADOT_INLINE v2 mul(SinCos a, v2 b) { return metadot_mul_sc_v2(a, b); }
METADOT_INLINE v2 mulT(SinCos a, v2 b) { return metadot_mulT_sc_v2(a, b); }
METADOT_INLINE SinCos mul(SinCos a, SinCos b) { return metadot_mul_sc(a, b); }
METADOT_INLINE SinCos mulT(SinCos a, SinCos b) { return metadot_mulT_sc(a, b); }

METADOT_INLINE float atan2_360(float y, float x) { return metadot_atan2_360(y, x); }
METADOT_INLINE float atan2_360(v2 v) { return metadot_atan2_360_v2(v); }
METADOT_INLINE float atan2_360(SinCos r) { return metadot_atan2_360_sc(r); }

METADOT_INLINE float shortest_arc(v2 a, v2 b) { return metadot_shortest_arc(a, b); }

METADOT_INLINE float angle_diff(float radians_a, float radians_b) { return metadot_angle_diff(radians_a, radians_b); }
METADOT_INLINE v2 from_angle(float radians) { return metadot_from_angle(radians); }

METADOT_INLINE v2 mul(M2x2 a, v2 b) { return metadot_mul_m2_v2(a, b); }
METADOT_INLINE M2x2 mul(M2x2 a, M2x2 b) { return metadot_mul_m2(a, b); }

METADOT_INLINE v2 mul(M3x2 a, v2 b) { return metadot_mul_m32_v2(a, b); }
METADOT_INLINE M3x2 mul(M3x2 a, M3x2 b) { return metadot_mul_m32(a, b); }
METADOT_INLINE M3x2 make_identity() { return metadot_make_identity(); }
METADOT_INLINE M3x2 make_translation(float x, float y) { return metadot_make_translation_f(x, y); }
METADOT_INLINE M3x2 make_translation(v2 p) { return metadot_make_translation(p); }
METADOT_INLINE M3x2 make_scale(v2 s) { return metadot_make_scale(s); }
METADOT_INLINE M3x2 make_scale(float s) { return metadot_make_scale_f(s); }
METADOT_INLINE M3x2 make_scale(v2 s, v2 p) { return metadot_make_scale_translation(s, p); }
METADOT_INLINE M3x2 make_scale(float s, v2 p) { return metadot_make_scale_translation_f(s, p); }
METADOT_INLINE M3x2 make_scale(float sx, float sy, v2 p) { return metadot_make_scale_translation_f_f(sx, sy, p); }
METADOT_INLINE M3x2 make_rotation(float radians) { return metadot_make_rotation(radians); }
METADOT_INLINE M3x2 make_transform(v2 p, v2 s, float radians) { return metadot_make_transform_TSR(p, s, radians); }
METADOT_INLINE M3x2 invert(M3x2 m) { return metadot_invert(m); }

METADOT_INLINE MTransform make_transform() { return metadot_make_transform(); }
METADOT_INLINE MTransform make_transform(v2 p, float radians) { return metadot_make_transform_TR(p, radians); }
METADOT_INLINE v2 mul(MTransform a, v2 b) { return metadot_mul_tf_v2(a, b); }
METADOT_INLINE v2 mulT(MTransform a, v2 b) { return metadot_mulT_tf_v2(a, b); }
METADOT_INLINE MTransform mul(MTransform a, MTransform b) { return metadot_mul_tf(a, b); }
METADOT_INLINE MTransform mulT(MTransform a, MTransform b) { return metadot_mulT_tf(a, b); }

METADOT_INLINE Halfspace plane(v2 n, float d) { return metadot_plane(n, d); }
METADOT_INLINE Halfspace plane(v2 n, v2 p) { return metadot_plane2(n, p); }
METADOT_INLINE v2 origin(Halfspace h) { return metadot_origin(h); }
METADOT_INLINE float distance(Halfspace h, v2 p) { return metadot_distance_hs(h, p); }
METADOT_INLINE v2 project(Halfspace h, v2 p) { return metadot_project(h, p); }
METADOT_INLINE Halfspace mul(MTransform a, Halfspace b) { return metadot_mul_tf_hs(a, b); }
METADOT_INLINE Halfspace mulT(MTransform a, Halfspace b) { return metadot_mulT_tf_hs(a, b); }
METADOT_INLINE v2 intersect(v2 a, v2 b, float da, float db) { return metadot_intersect_halfspace(a, b, da, db); }
METADOT_INLINE v2 intersect(Halfspace h, v2 a, v2 b) { return metadot_intersect_halfspace2(h, a, b); }

METADOT_INLINE Aabb make_aabb(v2 min, v2 max) { return metadot_make_aabb(min, max); }
METADOT_INLINE Aabb make_aabb(v2 pos, float w, float h) { return metadot_make_aabb_pos_w_h(pos, w, h); }
METADOT_INLINE Aabb make_aabb_center_half_extents(v2 center, v2 half_extents) { return metadot_make_aabb_center_half_extents(center, half_extents); }
METADOT_INLINE Aabb make_aabb_from_top_left(v2 top_left, float w, float h) { return metadot_make_aabb_from_top_left(top_left, w, h); }
METADOT_INLINE float width(Aabb bb) { return metadot_width(bb); }
METADOT_INLINE float height(Aabb bb) { return metadot_height(bb); }
METADOT_INLINE float half_width(Aabb bb) { return metadot_half_width(bb); }
METADOT_INLINE float half_height(Aabb bb) { return metadot_half_height(bb); }
METADOT_INLINE v2 half_extents(Aabb bb) { return metadot_half_extents(bb); }
METADOT_INLINE v2 extents(Aabb aabb) { return metadot_extents(aabb); }
METADOT_INLINE Aabb expand(Aabb aabb, v2 v) { return metadot_expand_aabb(aabb, v); }
METADOT_INLINE Aabb expand(Aabb aabb, float v) { return metadot_expand_aabb_f(aabb, v); }
METADOT_INLINE v2 min(Aabb bb) { return metadot_min_aabb(bb); }
METADOT_INLINE v2 max(Aabb bb) { return metadot_max_aabb(bb); }
METADOT_INLINE v2 midpoint(Aabb bb) { return metadot_midpoint(bb); }
METADOT_INLINE v2 center(Aabb bb) { return metadot_center(bb); }
METADOT_INLINE v2 top_left(Aabb bb) { return metadot_top_left(bb); }
METADOT_INLINE v2 top_right(Aabb bb) { return metadot_top_right(bb); }
METADOT_INLINE v2 bottom_left(Aabb bb) { return metadot_bottom_left(bb); }
METADOT_INLINE v2 bottom_right(Aabb bb) { return metadot_bottom_right(bb); }
METADOT_INLINE bool contains(Aabb bb, v2 p) { return metadot_contains_point(bb, p); }
METADOT_INLINE bool contains(Aabb a, Aabb b) { return metadot_contains_aabb(a, b); }
METADOT_INLINE float surface_area(Aabb bb) { return metadot_surface_area_aabb(bb); }
METADOT_INLINE float area(Aabb bb) { return metadot_area_aabb(bb); }
METADOT_INLINE v2 clamp(Aabb bb, v2 p) { return metadot_clamp_aabb_v2(bb, p); }
METADOT_INLINE Aabb clamp(Aabb a, Aabb b) { return metadot_clamp_aabb(a, b); }
METADOT_INLINE Aabb combine(Aabb a, Aabb b) { return metadot_combine(a, b); }

METADOT_INLINE int overlaps(Aabb a, Aabb b) { return metadot_overlaps(a, b); }
METADOT_INLINE int collide(Aabb a, Aabb b) { return metadot_collide_aabb(a, b); }

METADOT_INLINE Aabb make_aabb(v2 *verts, int count) { return metadot_make_aabb_verts((METAENGINE_V2 *)verts, count); }
METADOT_INLINE void aabb_verts(v2 *out, Aabb bb) { return metadot_aabb_verts((METAENGINE_V2 *)out, bb); }

METADOT_INLINE float area(Circle c) { return metadot_area_circle(c); }
METADOT_INLINE float surface_area(Circle c) { return metadot_surface_area_circle(c); }
METADOT_INLINE Circle mul(MTransform tx, Circle a) { return metadot_mul_tf_circle(tx, a); }

METADOT_INLINE v2 impact(Ray r, float t) { return metadot_impact(r, t); }
METADOT_INLINE v2 endpoint(Ray r) { return metadot_endpoint(r); }

METADOT_INLINE int ray_to_halfpsace(Ray A, Halfspace B, Raycast *out) { return metadot_ray_to_halfpsace(A, B, out); }
METADOT_INLINE float distance_sq(v2 a, v2 b, v2 p) { return metadot_distance_sq(a, b, p); }

METADOT_INLINE bool circle_to_circle(Circle A, Circle B) { return metadot_circle_to_circle(A, B); }
METADOT_INLINE bool circle_to_aabb(Circle A, Aabb B) { return metadot_circle_to_aabb(A, B); }
METADOT_INLINE bool circle_to_capsule(Circle A, Capsule B) { return metadot_circle_to_capsule(A, B); }
METADOT_INLINE bool aabb_to_aabb(Aabb A, Aabb B) { return metadot_aabb_to_aabb(A, B); }
METADOT_INLINE bool aabb_to_capsule(Aabb A, Capsule B) { return metadot_aabb_to_capsule(A, B); }
METADOT_INLINE bool capsule_to_capsule(Capsule A, Capsule B) { return metadot_capsule_to_capsule(A, B); }
METADOT_INLINE bool circle_to_poly(Circle A, const Poly *B, const MTransform *bx) { return metadot_circle_to_poly(A, B, bx); }
METADOT_INLINE bool aabb_to_poly(Aabb A, const Poly *B, const MTransform *bx) { return metadot_aabb_to_poly(A, B, bx); }
METADOT_INLINE bool capsule_to_poly(Capsule A, const Poly *B, const MTransform *bx) { return metadot_capsule_to_poly(A, B, bx); }
METADOT_INLINE bool poly_to_poly(const Poly *A, const MTransform *ax, const Poly *B, const MTransform *bx) { return metadot_poly_to_poly(A, ax, B, bx); }

METADOT_INLINE bool ray_to_circle(Ray A, Circle B, Raycast *out) { return metadot_ray_to_circle(A, B, out); }
METADOT_INLINE bool ray_to_aabb(Ray A, Aabb B, Raycast *out) { return metadot_ray_to_aabb(A, B, out); }
METADOT_INLINE bool ray_to_capsule(Ray A, Capsule B, Raycast *out) { return metadot_ray_to_capsule(A, B, out); }
METADOT_INLINE bool ray_to_poly(Ray A, const Poly *B, const MTransform *bx_ptr, Raycast *out) { return metadot_ray_to_poly(A, B, bx_ptr, out); }

METADOT_INLINE void circle_to_circle_manifold(Circle A, Circle B, Manifold *m) { return metadot_circle_to_circle_manifold(A, B, m); }
METADOT_INLINE void circle_to_aabb_manifold(Circle A, Aabb B, Manifold *m) { return metadot_circle_to_aabb_manifold(A, B, m); }
METADOT_INLINE void circle_to_capsule_manifold(Circle A, Capsule B, Manifold *m) { return metadot_circle_to_capsule_manifold(A, B, m); }
METADOT_INLINE void aabb_to_aabb_manifold(Aabb A, Aabb B, Manifold *m) { return metadot_aabb_to_aabb_manifold(A, B, m); }
METADOT_INLINE void aabb_to_capsule_manifold(Aabb A, Capsule B, Manifold *m) { return metadot_aabb_to_capsule_manifold(A, B, m); }
METADOT_INLINE void capsule_to_capsule_manifold(Capsule A, Capsule B, Manifold *m) { return metadot_capsule_to_capsule_manifold(A, B, m); }
METADOT_INLINE void circle_to_poly_manifold(Circle A, const Poly *B, const MTransform *bx, Manifold *m) { return metadot_circle_to_poly_manifold(A, B, bx, m); }
METADOT_INLINE void aabb_to_poly_manifold(Aabb A, const Poly *B, const MTransform *bx, Manifold *m) { return metadot_aabb_to_poly_manifold(A, B, bx, m); }
METADOT_INLINE void capsule_to_poly_manifold(Capsule A, const Poly *B, const MTransform *bx, Manifold *m) { return metadot_capsule_to_poly_manifold(A, B, bx, m); }
METADOT_INLINE void poly_to_poly_manifold(const Poly *A, const MTransform *ax, const Poly *B, const MTransform *bx, Manifold *m) { return metadot_poly_to_poly_manifold(A, ax, B, bx, m); }

METADOT_INLINE float gjk(const void *A, ShapeType typeA, const MTransform *ax_ptr, const void *B, ShapeType typeB, const MTransform *bx_ptr, v2 *outA, v2 *outB, int use_radius, int *iterations,
                         GjkCache *cache) {
    return metadot_gjk(A, typeA, ax_ptr, B, typeB, bx_ptr, (METAENGINE_V2 *)outA, (METAENGINE_V2 *)outB, use_radius, iterations, cache);
}

METADOT_INLINE ToiResult toi(const void *A, ShapeType typeA, const MTransform *ax_ptr, v2 vA, const void *B, ShapeType typeB, const MTransform *bx_ptr, v2 vB, int use_radius, int *iterations) {
    return metadot_toi(A, typeA, ax_ptr, vA, B, typeB, bx_ptr, vB, use_radius);
}

METADOT_INLINE void inflate(void *shape, ShapeType type, float skin_factor) { return metadot_inflate(shape, type, skin_factor); }

METADOT_INLINE int hull(v2 *verts, int count) { return metadot_hull((METAENGINE_V2 *)verts, count); }
METADOT_INLINE void norms(v2 *verts, v2 *norms, int count) { return metadot_norms((METAENGINE_V2 *)verts, (METAENGINE_V2 *)norms, count); }

METADOT_INLINE void make_poly(Poly *p) { return metadot_make_poly(p); }
METADOT_INLINE v2 centroid(const v2 *verts, int count) { return metadot_centroid((METAENGINE_V2 *)verts, count); }

METADOT_INLINE int collided(const void *A, const MTransform *ax, ShapeType typeA, const void *B, const MTransform *bx, ShapeType typeB) { return metadot_collided(A, ax, typeA, B, bx, typeB); }
METADOT_INLINE void collide(const void *A, const MTransform *ax, ShapeType typeA, const void *B, const MTransform *bx, ShapeType typeB, Manifold *m) {
    return metadot_collide(A, ax, typeA, B, bx, typeB, m);
}
METADOT_INLINE bool cast_ray(Ray A, const void *B, const MTransform *bx, ShapeType typeB, Raycast *out) { return metadot_cast_ray(A, B, bx, typeB, out); }

}  // namespace MetaEngine

#pragma endregion c2

#pragma region liner

#define METADOT_USE_SSE 0
#if METADOT_USE_SSE
#include <pmmintrin.h>
#include <xmmintrin.h>
#endif

// if you wish NOT to include iostream, simply change the
// #define to 0
#define METADOT_INCLUDE_IOSTREAM 1
#if METADOT_INCLUDE_IOSTREAM
#include <iostream>
#endif

#include <cmath>

#define METADOT_SQRTF sqrtf
#define METADOT_SINF sinf
#define METADOT_COSF cosf
#define METADOT_TANF tanf
#define METADOT_ACOSF acosf

#define METADOT_MIN(x, y) ((x) < (y) ? (x) : (y))
#define METADOT_MAX(x, y) ((x) > (y) ? (x) : (y))
#define METADOT_ABS(x) ((x) > 0 ? (x) : -(x))

inline float rad_to_deg(float rad) { return rad * 57.2957795131f; }
inline float deg_to_rad(float deg) { return deg * 0.01745329251f; }

#if METADOT_INCLUDE_IOSTREAM

inline std::ostream &operator<<(std::ostream &os, const vec2 &v) {
    os << v.x << ", " << v.y;
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const vec3 &v) {
    os << v.x << ", " << v.y << ", " << v.z;
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const vec4 &v) {
    os << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
    return os;
}

// input:

inline std::istream &operator>>(std::istream &is, vec2 &v) {
    is >> v.x >> v.y;
    return is;
}

inline std::istream &operator>>(std::istream &is, vec3 &v) {
    is >> v.x >> v.y >> v.z;
    return is;
}

inline std::istream &operator>>(std::istream &is, vec4 &v) {
    is >> v.x >> v.y >> v.z >> v.w;
    return is;
}

#endif

// addition:

inline vec2 operator+(const vec2 &v1, const vec2 &v2) {
    vec2 result;

    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;

    return result;
}

inline vec3 operator+(const vec3 &v1, const vec3 &v2) {
    vec3 result;

    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;

    return result;
}

inline vec4 operator+(const vec4 &v1, const vec4 &v2) {
    vec4 result;

    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;
    result.w = v1.w + v2.w;
    return result;
}

// subtraction:

inline vec2 operator-(const vec2 &v1, const vec2 &v2) {
    vec2 result;

    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;

    return result;
}

inline vec3 operator-(const vec3 &v1, const vec3 &v2) {
    vec3 result;

    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;

    return result;
}

inline vec4 operator-(const vec4 &v1, const vec4 &v2) {
    vec4 result;

    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    result.w = v1.w - v2.w;

    return result;
}

// multiplication:

inline vec2 operator*(const vec2 &v1, const vec2 &v2) {
    vec2 result;

    result.x = v1.x * v2.x;
    result.y = v1.y * v2.y;

    return result;
}

inline vec3 operator*(const vec3 &v1, const vec3 &v2) {
    vec3 result;

    result.x = v1.x * v2.x;
    result.y = v1.y * v2.y;
    result.z = v1.z * v2.z;

    return result;
}

inline vec4 operator*(const vec4 &v1, const vec4 &v2) {
    vec4 result;

    result.x = v1.x * v2.x;
    result.y = v1.y * v2.y;
    result.z = v1.z * v2.z;
    result.w = v1.w * v2.w;

    return result;
}

// division:

inline vec2 operator/(const vec2 &v1, const vec2 &v2) {
    vec2 result;

    result.x = v1.x / v2.x;
    result.y = v1.y / v2.y;

    return result;
}

inline vec3 operator/(const vec3 &v1, const vec3 &v2) {
    vec3 result;

    result.x = v1.x / v2.x;
    result.y = v1.y / v2.y;
    result.z = v1.z / v2.z;

    return result;
}

inline vec4 operator/(const vec4 &v1, const vec4 &v2) {
    vec4 result;

#if METADOT_USE_SSE

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

inline vec2 operator*(const vec2 &v, float s) {
    vec2 result;

    result.x = v.x * s;
    result.y = v.y * s;

    return result;
}

inline vec3 operator*(const vec3 &v, float s) {
    vec3 result;

    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;

    return result;
}

inline vec4 operator*(const vec4 &v, float s) {
    vec4 result;

#if METADOT_USE_SSE

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

inline vec2 operator*(float s, const vec2 &v) { return v * s; }

inline vec3 operator*(float s, const vec3 &v) { return v * s; }

inline vec4 operator*(float s, const vec4 &v) { return v * s; }

// scalar division:

inline vec2 operator/(const vec2 &v, float s) {
    vec2 result;

    result.x = v.x / s;
    result.y = v.y / s;

    return result;
}

inline vec3 operator/(const vec3 &v, float s) {
    vec3 result;

    result.x = v.x / s;
    result.y = v.y / s;
    result.z = v.z / s;

    return result;
}

inline vec4 operator/(const vec4 &v, float s) {
    vec4 result;

#if METADOT_USE_SSE

    __m128 scale = _mm_set1_ps(s);
    result.packed = _mm_div_ps(v.packed, scale);

#else

    result.x = v.x / s;
    result.y = v.y / s;
    result.z = v.z / s;
    result.w = v.w / s;

#endif

    return result;
}

inline vec2 operator/(float s, const vec2 &v) {
    vec2 result;

    result.x = s / v.x;
    result.y = s / v.y;

    return result;
}

inline vec3 operator/(float s, const vec3 &v) {
    vec3 result;

    result.x = s / v.x;
    result.y = s / v.y;
    result.z = s / v.z;

    return result;
}

inline vec4 operator/(float s, const vec4 &v) {
    vec4 result;

#if METADOT_USE_SSE

    __m128 scale = _mm_set1_ps(s);
    result.packed = _mm_div_ps(scale, v.packed);

#else

    result.x = s / v.x;
    result.y = s / v.y;
    result.z = s / v.z;
    result.w = s / v.w;

#endif

    return result;
}

// dot product:

inline float dot(const vec2 &v1, const vec2 &v2) {
    float result;

    result = v1.x * v2.x + v1.y * v2.y;

    return result;
}

inline float dot(const vec3 &v1, const vec3 &v2) {
    float result;

    result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;

    return result;
}

inline float dot(const vec4 &v1, const vec4 &v2) {
    float result;

#if METADOT_USE_SSE

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

inline vec3 cross(const vec3 &v1, const vec3 &v2) {
    vec3 result;

    result.x = (v1.y * v2.z) - (v1.z * v2.y);
    result.y = (v1.z * v2.x) - (v1.x * v2.z);
    result.z = (v1.x * v2.y) - (v1.y * v2.x);

    return result;
}

// length:

inline float length(const vec2 &v) {
    float result;

    result = METADOT_SQRTF(dot(v, v));

    return result;
}

inline float length(const vec3 &v) {
    float result;

    result = METADOT_SQRTF(dot(v, v));

    return result;
}

inline float length(const vec4 &v) {
    float result;

    result = METADOT_SQRTF(dot(v, v));

    return result;
}

// normalize:

inline vec2 normalize(const vec2 &v) {
    vec2 result;

    float len = length(v);
    if (len != 0.0f) result = v / len;

    return result;
}

inline vec3 normalize(const vec3 &v) {
    vec3 result;

    float len = length(v);
    if (len != 0.0f) result = v / len;

    return result;
}

inline vec4 normalize(const vec4 &v) {
    vec4 result;

    float len = length(v);
    result = v / len;

    return result;
}

// distance:

inline float distance(const vec2 &v1, const vec2 &v2) {
    float result;

    vec2 to = v1 - v2;
    result = length(to);

    return result;
}

inline float distance(const vec3 &v1, const vec3 &v2) {
    float result;

    vec3 to = v1 - v2;
    result = length(to);

    return result;
}

inline float distance(const vec4 &v1, const vec4 &v2) {
    float result;

    vec4 to = v1 - v2;
    result = length(to);

    return result;
}

// equality:

inline bool operator==(const vec2 &v1, const vec2 &v2) {
    bool result;

    result = (v1.x == v2.x) && (v1.y == v2.y);

    return result;
}

inline bool operator==(const vec3 &v1, const vec3 &v2) {
    bool result;

    result = (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z);

    return result;
}

inline bool operator==(const vec4 &v1, const vec4 &v2) {
    bool result;

    // TODO: there are SIMD instructions for floating point equality, find a way to get a single bool from them
    result = (v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z) && (v1.w == v2.w);

    return result;
}

inline bool operator!=(const vec2 &v1, const vec2 &v2) {
    bool result;

    result = (v1.x != v2.x) || (v1.y != v2.y);

    return result;
}

inline bool operator!=(const vec3 &v1, const vec3 &v2) {
    bool result;

    result = (v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z);

    return result;
}

inline bool operator!=(const vec4 &v1, const vec4 &v2) {
    bool result;

    result = (v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z) || (v1.w != v2.w);

    return result;
}

// min:

inline vec2 min(const vec2 &v1, const vec2 &v2) {
    vec2 result;

    result.x = METADOT_MIN(v1.x, v2.x);
    result.y = METADOT_MIN(v1.y, v2.y);

    return result;
}

inline vec3 min(const vec3 &v1, const vec3 &v2) {
    vec3 result;

    result.x = METADOT_MIN(v1.x, v2.x);
    result.y = METADOT_MIN(v1.y, v2.y);
    result.z = METADOT_MIN(v1.z, v2.z);

    return result;
}

inline vec4 min(const vec4 &v1, const vec4 &v2) {
    vec4 result;

#if METADOT_USE_SSE

    result.packed = _mm_min_ps(v1.packed, v2.packed);

#else

    result.x = METADOT_MIN(v1.x, v2.x);
    result.y = METADOT_MIN(v1.y, v2.y);
    result.z = METADOT_MIN(v1.z, v2.z);
    result.w = METADOT_MIN(v1.w, v2.w);

#endif

    return result;
}

// max:

inline vec2 max(const vec2 &v1, const vec2 &v2) {
    vec2 result;

    result.x = METADOT_MAX(v1.x, v2.x);
    result.y = METADOT_MAX(v1.y, v2.y);

    return result;
}

inline vec3 max(const vec3 &v1, const vec3 &v2) {
    vec3 result;

    result.x = METADOT_MAX(v1.x, v2.x);
    result.y = METADOT_MAX(v1.y, v2.y);
    result.z = METADOT_MAX(v1.z, v2.z);

    return result;
}

inline vec4 max(const vec4 &v1, const vec4 &v2) {
    vec4 result;

#if METADOT_USE_SSE

    result.packed = _mm_max_ps(v1.packed, v2.packed);

#else

    result.x = METADOT_MAX(v1.x, v2.x);
    result.y = METADOT_MAX(v1.y, v2.y);
    result.z = METADOT_MAX(v1.z, v2.z);
    result.w = METADOT_MAX(v1.w, v2.w);

#endif

    return result;
}

//----------------------------------------------------------------------//
// MATRIX FUNCTIONS:

#if METADOT_INCLUDE_IOSTREAM

// output:

inline std::ostream &operator<<(std::ostream &os, const mat3 &m) {
    os << m.v[0] << std::endl << m.v[1] << std::endl << m.v[2];
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const mat4 &m) {
    os << m.v[0] << std::endl << m.v[1] << std::endl << m.v[2] << std::endl << m.v[3];
    return os;
}

// input:

inline std::istream &operator>>(std::istream &is, mat3 &m) {
    is >> m.v[0] >> m.v[1] >> m.v[2];
    return is;
}

inline std::istream &operator>>(std::istream &is, mat4 &m) {
    is >> m.v[0] >> m.v[1] >> m.v[2] >> m.v[3];
    return is;
}

#endif

// initialization:

inline mat3 mat3_identity() {
    mat3 result;

    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;

    return result;
}

inline mat4 mat4_identity() {
    mat4 result;

    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;
    result.m[3][3] = 1.0f;

    return result;
}

// addition:

inline mat3 operator+(const mat3 &m1, const mat3 &m2) {
    mat3 result;

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

inline mat4 operator+(const mat4 &m1, const mat4 &m2) {
    mat4 result;

#if METADOT_USE_SSE

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

inline mat3 operator-(const mat3 &m1, const mat3 &m2) {
    mat3 result;

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

inline mat4 operator-(const mat4 &m1, const mat4 &m2) {
    mat4 result;

#if METADOT_USE_SSE

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

inline mat3 operator*(const mat3 &m1, const mat3 &m2) {
    mat3 result;

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

inline mat4 operator*(const mat4 &m1, const mat4 &m2) {
    mat4 result;

#if METADOT_USE_SSE

    result.packed[0] = mat4_mult_column_sse(m2.packed[0], m1);
    result.packed[1] = mat4_mult_column_sse(m2.packed[1], m1);
    result.packed[2] = mat4_mult_column_sse(m2.packed[2], m1);
    result.packed[3] = mat4_mult_column_sse(m2.packed[3], m1);

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

inline vec3 operator*(const mat3 &m, const vec3 &v) {
    vec3 result;

    result.x = m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z;
    result.y = m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z;
    result.z = m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z;

    return result;
}

inline vec4 operator*(const mat4 &m, const vec4 &v) {
    vec4 result;

#if METADOT_USE_SSE

    result.packed = mat4_mult_column_sse(v.packed, m);

#else

    result.x = m.m[0][0] * v.x + m.m[1][0] * v.y + m.m[2][0] * v.z + m.m[3][0] * v.w;
    result.y = m.m[0][1] * v.x + m.m[1][1] * v.y + m.m[2][1] * v.z + m.m[3][1] * v.w;
    result.z = m.m[0][2] * v.x + m.m[1][2] * v.y + m.m[2][2] * v.z + m.m[3][2] * v.w;
    result.w = m.m[0][3] * v.x + m.m[1][3] * v.y + m.m[2][3] * v.z + m.m[3][3] * v.w;

#endif

    return result;
}

// transpose:

inline mat3 transpose(const mat3 &m) {
    mat3 result;

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

inline mat4 transpose(const mat4 &m) {
    mat4 result = m;

#if METADOT_USE_SSE

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

inline mat3 inverse(const mat3 &m) {
    mat3 result;

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

inline mat4 inverse(const mat4 &mat) {
    // TODO: this function is not SIMD optimized, figure out how to do it

    mat4 result;

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

#if METADOT_USE_SSE

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

inline mat3 translate(const vec2 &t) {
    mat3 result = mat3_identity();

    result.m[2][0] = t.x;
    result.m[2][1] = t.y;

    return result;
}

inline mat4 translate(const vec3 &t) {
    mat4 result = mat4_identity();

    result.m[3][0] = t.x;
    result.m[3][1] = t.y;
    result.m[3][2] = t.z;

    return result;
}

// scaling:

inline mat3 scale(const vec2 &s) {
    mat3 result = mat3_identity();

    result.m[0][0] = s.x;
    result.m[1][1] = s.y;

    return result;
}

inline mat4 scale(const vec3 &s) {
    mat4 result = mat4_identity();

    result.m[0][0] = s.x;
    result.m[1][1] = s.y;
    result.m[2][2] = s.z;

    return result;
}

// rotation:

inline mat3 rotate(float angle) {
    mat3 result = mat3_identity();

    float radians = deg_to_rad(angle);
    float sine = METADOT_SINF(radians);
    float cosine = METADOT_COSF(radians);

    result.m[0][0] = cosine;
    result.m[1][0] = sine;
    result.m[0][1] = -sine;
    result.m[1][1] = cosine;

    return result;
}

inline mat4 rotate(const vec3 &axis, float angle) {
    mat4 result = mat4_identity();

    vec3 normalized = normalize(axis);

    float radians = deg_to_rad(angle);
    float sine = METADOT_SINF(radians);
    float cosine = METADOT_COSF(radians);
    float cosine2 = 1.0f - cosine;

    result.m[0][0] = normalized.x * normalized.x * cosine2 + cosine;
    result.m[0][1] = normalized.x * normalized.y * cosine2 + normalized.z * sine;
    result.m[0][2] = normalized.x * normalized.z * cosine2 - normalized.y * sine;
    result.m[1][0] = normalized.y * normalized.x * cosine2 - normalized.z * sine;
    result.m[1][1] = normalized.y * normalized.y * cosine2 + cosine;
    result.m[1][2] = normalized.y * normalized.z * cosine2 + normalized.x * sine;
    result.m[2][0] = normalized.z * normalized.x * cosine2 + normalized.y * sine;
    result.m[2][1] = normalized.z * normalized.y * cosine2 - normalized.x * sine;
    result.m[2][2] = normalized.z * normalized.z * cosine2 + cosine;

    return result;
}

inline mat4 rotate(const vec3 &euler) {
    mat4 result = mat4_identity();

    vec3 radians;
    radians.x = deg_to_rad(euler.x);
    radians.y = deg_to_rad(euler.y);
    radians.z = deg_to_rad(euler.z);

    float sinX = METADOT_SINF(radians.x);
    float cosX = METADOT_COSF(radians.x);
    float sinY = METADOT_SINF(radians.y);
    float cosY = METADOT_COSF(radians.y);
    float sinZ = METADOT_SINF(radians.z);
    float cosZ = METADOT_COSF(radians.z);

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

inline mat3 top_left(const mat4 &m) {
    mat3 result;

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

inline mat4 perspective(float fov, float aspect, float near, float far) {
    mat4 result;

    float scale = METADOT_TANF(deg_to_rad(fov * 0.5f)) * near;

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

inline mat4 orthographic(float left, float right, float bot, float top, float near, float far) {
    mat4 result = mat4_identity();

    result.m[0][0] = 2.0f / (right - left);
    result.m[1][1] = 2.0f / (top - bot);
    result.m[2][2] = 2.0f / (near - far);

    result.m[3][0] = (left + right) / (left - right);
    result.m[3][1] = (bot + top) / (bot - top);
    result.m[3][2] = (near + far) / (near - far);

    return result;
}

// view matrix:

inline mat4 look(const vec3 &pos, const vec3 &dir, const vec3 &up) {
    mat4 result;

    vec3 r = normalize(cross(up, dir));
    vec3 u = cross(dir, r);

    mat4 RUD = mat4_identity();
    RUD.m[0][0] = r.x;
    RUD.m[1][0] = r.y;
    RUD.m[2][0] = r.z;
    RUD.m[0][1] = u.x;
    RUD.m[1][1] = u.y;
    RUD.m[2][1] = u.z;
    RUD.m[0][2] = dir.x;
    RUD.m[1][2] = dir.y;
    RUD.m[2][2] = dir.z;

    vec3 oppPos = {-pos.x, -pos.y, -pos.z};
    result = RUD * translate(oppPos);

    return result;
}

inline mat4 lookat(const vec3 &pos, const vec3 &target, const vec3 &up) {
    mat4 result;

    vec3 dir = normalize(pos - target);
    result = look(pos, dir, up);

    return result;
}

//----------------------------------------------------------------------//
// QUATERNION FUNCTIONS:

#if METADOT_INCLUDE_IOSTREAM

inline std::ostream &operator<<(std::ostream &os, const quaternion &q) {
    os << q.x << ", " << q.y << ", " << q.z << ", " << q.w;
    return os;
}

inline std::istream &operator>>(std::istream &is, quaternion &q) {
    is >> q.x >> q.y >> q.z >> q.w;
    return is;
}

#endif

inline quaternion quaternion_identity() {
    quaternion result;

    result.x = 0.0f;
    result.y = 0.0f;
    result.z = 0.0f;
    result.w = 1.0f;

    return result;
}

inline quaternion operator+(const quaternion &q1, const quaternion &q2) {
    quaternion result;

#if METADOT_USE_SSE

    result.packed = _mm_add_ps(q1.packed, q2.packed);

#else

    result.x = q1.x + q2.x;
    result.y = q1.y + q2.y;
    result.z = q1.z + q2.z;
    result.w = q1.w + q2.w;

#endif

    return result;
}

inline quaternion operator-(const quaternion &q1, const quaternion &q2) {
    quaternion result;

#if METADOT_USE_SSE

    result.packed = _mm_sub_ps(q1.packed, q2.packed);

#else

    result.x = q1.x - q2.x;
    result.y = q1.y - q2.y;
    result.z = q1.z - q2.z;
    result.w = q1.w - q2.w;

#endif

    return result;
}

inline quaternion operator*(const quaternion &q1, const quaternion &q2) {
    quaternion result;

#if METADOT_USE_SSE

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

inline quaternion operator*(const quaternion &q, float s) {
    quaternion result;

#if METADOT_USE_SSE

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

inline quaternion operator*(float s, const quaternion &q) { return q * s; }

inline quaternion operator/(const quaternion &q, float s) {
    quaternion result;

#if METADOT_USE_SSE

    __m128 scale = _mm_set1_ps(s);
    result.packed = _mm_div_ps(q.packed, scale);

#else

    result.x = q.x / s;
    result.y = q.y / s;
    result.z = q.z / s;
    result.w = q.w / s;

#endif

    return result;
}

inline quaternion operator/(float s, const quaternion &q) {
    quaternion result;

#if METADOT_USE_SSE

    __m128 scale = _mm_set1_ps(s);
    result.packed = _mm_div_ps(scale, q.packed);

#else

    result.x = s / q.x;
    result.y = s / q.y;
    result.z = s / q.z;
    result.w = s / q.w;

#endif

    return result;
}

inline float dot(const quaternion &q1, const quaternion &q2) {
    float result;

#if METADOT_USE_SSE

    __m128 r = _mm_mul_ps(q1.packed, q2.packed);
    r = _mm_hadd_ps(r, r);
    r = _mm_hadd_ps(r, r);
    _mm_store_ss(&result, r);

#else

    result = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;

#endif

    return result;
}

inline float length(const quaternion &q) {
    float result;

    result = METADOT_SQRTF(dot(q, q));

    return result;
}

inline quaternion normalize(const quaternion &q) {
    quaternion result;

    float len = length(q);
    if (len != 0.0f) result = q / len;

    return result;
}

inline quaternion conjugate(const quaternion &q) {
    quaternion result;

    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = q.w;

    return result;
}

inline quaternion inverse(const quaternion &q) {
    quaternion result;

    result.x = -q.x;
    result.y = -q.y;
    result.z = -q.z;
    result.w = q.w;

#if METADOT_USE_SSE

    __m128 scale = _mm_set1_ps(dot(q, q));
    _mm_div_ps(result.packed, scale);

#else

    float invLen2 = 1.0f / dot(q, q);

    result.x *= invLen2;
    result.y *= invLen2;
    result.z *= invLen2;
    result.w *= invLen2;

#endif

    return result;
}

inline quaternion slerp(const quaternion &q1, const quaternion &q2, float a) {
    quaternion result;

    float cosine = dot(q1, q2);
    float angle = METADOT_ACOSF(cosine);

    float sine1 = METADOT_SINF((1.0f - a) * angle);
    float sine2 = METADOT_SINF(a * angle);
    float invSine = 1.0f / METADOT_SINF(angle);

    quaternion q1scaled = q1 * sine1;
    quaternion q2scaled = q2 * sine2;

    result = q1scaled + q2scaled;
    result = result * invSine;

    return result;
}

inline bool operator==(const quaternion &q1, const quaternion &q2) {
    bool result;

    // TODO: there are SIMD instructions for floating point equality, find a way to get a single bool from them
    result = (q1.x == q2.x) && (q1.y == q2.y) && (q1.z == q2.z) && (q1.w == q2.w);

    return result;
}

inline bool operator!=(const quaternion &q1, const quaternion &q2) {
    bool result;

    result = (q1.x != q2.x) || (q1.y != q2.y) || (q1.z != q2.z) || (q1.w != q2.w);

    return result;
}

inline quaternion quaternion_from_axis_angle(const vec3 &axis, float angle) {
    quaternion result;

    float radians = deg_to_rad(angle * 0.5f);
    vec3 normalized = normalize(axis);
    float sine = METADOT_SINF(radians);

    result.x = normalized.x * sine;
    result.y = normalized.y * sine;
    result.z = normalized.z * sine;
    result.w = METADOT_COSF(radians);

    return result;
}

inline quaternion quaternion_from_euler(const vec3 &angles) {
    quaternion result;

    vec3 radians;
    radians.x = deg_to_rad(angles.x * 0.5f);
    radians.y = deg_to_rad(angles.y * 0.5f);
    radians.z = deg_to_rad(angles.z * 0.5f);

    float sinx = METADOT_SINF(radians.x);
    float cosx = METADOT_COSF(radians.x);
    float siny = METADOT_SINF(radians.y);
    float cosy = METADOT_COSF(radians.y);
    float sinz = METADOT_SINF(radians.z);
    float cosz = METADOT_COSF(radians.z);

#if METADOT_USE_SSE

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

inline mat4 quaternion_to_mat4(const quaternion &q) {
    mat4 result = mat4_identity();

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

#pragma endregion liner

#ifdef METADOT_PLATFORM_WIN32
#define far
#define near
#endif

#endif
