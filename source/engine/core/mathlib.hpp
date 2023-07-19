// Copyright(c) 2022-2023, KaoruXun All rights reserved.

// This source file may include
// Polypartition (MIT) by Ivan Fratric
// MarchingSquares(Tom Gibara) (MIT) by Juha Reunanen

#ifndef ME_MATH_HPP
#define ME_MATH_HPP

#include <cassert>
#include <list>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/core/mathlib_base.hpp"

namespace ME {

typedef struct U16Point {
    u16 x;
    u16 y;
} U16Point;

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

f32 math_perlin(f32 x, f32 y, f32 z, int x_wrap = 0, int y_wrap = 0, int z_wrap = 0);

namespace NewMaths {

constexpr std::size_t hash_combine(std::size_t l, std::size_t r) noexcept { return l ^ (r + 0x9e3779b9 + (l << 6) + (l >> 2)); }

f32 vec22angle(MEvec2 v2);
f32 clamp(f32 input, f32 min, f32 max);
int rand_range(int min, int max);
uint64_t rand_XOR();
inline f64 random_double();
inline f64 random_double(f64 min, f64 max);
struct v2;
struct RandState;
f32 v2_distance_2Points(v2 A, v2 B);
v2 unitvec_AtoB(v2 A, v2 B);
f32 signed_angle_v2(v2 A, v2 B);
v2 Rotate2D(v2 P, f32 sine, f32 cosine);
v2 Rotate2D(v2 P, f32 Angle);
v2 Rotate2D(v2 p, v2 o, f32 angle);
v2 Reflection2D(v2 P, v2 N);
bool PointInRectangle(v2 P, v2 A, v2 B, v2 C);
int sign(f32 x);
static f32 dot(v2 A, v2 B);
static f32 perpdot(v2 A, v2 B);
static bool operator==(v2 A, v2 B);

MEvec3 NormalizeVector(MEvec3 v);
MEvec3 Add(MEvec3 a, MEvec3 b);
MEvec3 Subtract(MEvec3 a, MEvec3 b);
MEvec3 ScalarMult(MEvec3 v, f32 s);
f64 Distance(MEvec3 a, MEvec3 b);
MEvec3 VectorProjection(MEvec3 a, MEvec3 b);
MEvec3 Reflection(MEvec3 *v1, MEvec3 *v2);

f64 DistanceFromPointToLine2D(MEvec3 lP1, MEvec3 lP2, MEvec3 p);

typedef struct Matrix3x3 {
    f32 m[3][3];
} Matrix3x3;

Matrix3x3 Transpose(Matrix3x3 m);
Matrix3x3 Identity();
MEvec3 Matrix3x3ToEulerAngles(Matrix3x3 m);
Matrix3x3 EulerAnglesToMatrix3x3(MEvec3 rotation);
MEvec3 RotateVector(MEvec3 v, Matrix3x3 m);
MEvec3 RotatePoint(MEvec3 p, MEvec3 r, MEvec3 pivot);
Matrix3x3 MultiplyMatrix3x3(Matrix3x3 a, Matrix3x3 b);

typedef struct Matrix4x4 {
    f32 m[4][4];
} Matrix4x4;

Matrix4x4 Identity4x4();
Matrix4x4 GetProjectionMatrix(f32 right, f32 left, f32 top, f32 bottom, f32 near, f32 far);

f32 Lerp(f64 t, f32 a, f32 b);
int Step(f32 edge, f32 x);
f32 Smoothstep(f32 edge0, f32 edge1, f32 x);
int Modulus(int a, int b);
f32 fModulus(f32 a, f32 b);
}  // namespace NewMaths
}  // namespace ME

#endif
