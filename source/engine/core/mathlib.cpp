// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "mathlib.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <list>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/core/const.h"
#include "libs/external/stb_perlin.h"

namespace ME {

// ========================================================
// Fast approximations of math functions used by Debug Draw
// ========================================================

union Float2UInt {
    f32 asFloat;
    u32 asUInt;
};

static inline f32 floatRound(f32 x) {
    // Probably slower than std::floor(), also depends of FPU settings,
    // but we only need this for that special sin/cos() case anyways...
    const int i = static_cast<int>(x);
    return (x >= 0.0f) ? static_cast<f32>(i) : static_cast<f32>(i - 1);
}

static inline f32 floatAbs(f32 x) {
    // Mask-off the sign bit
    Float2UInt i;
    i.asFloat = x;
    i.asUInt &= 0x7FFFFFFF;
    return i.asFloat;
}

static inline f32 floatInvSqrt(f32 x) {
    // Modified version of the emblematic Q_rsqrt() from Quake 3.
    // See: http://en.wikipedia.org/wiki/Fast_inverse_square_root
    Float2UInt i;
    f32 y, r;
    y = x * 0.5f;
    i.asFloat = x;
    i.asUInt = 0x5F3759DF - (i.asUInt >> 1);
    r = i.asFloat;
    r = r * (1.5f - (r * r * y));
    return r;
}

static inline f32 floatSin(f32 radians) {
    static const f32 A = -2.39e-08;
    static const f32 B = 2.7526e-06;
    static const f32 C = 1.98409e-04;
    static const f32 D = 8.3333315e-03;
    static const f32 E = 1.666666664e-01;

    if (radians < 0.0f || radians >= TAU) {
        radians -= floatRound(radians / TAU) * TAU;
    }

    if (radians < PI) {
        if (radians > HalfPI) {
            radians = PI - radians;
        }
    } else {
        radians = (radians > (PI + HalfPI)) ? (radians - TAU) : (PI - radians);
    }

    const f32 s = radians * radians;
    return radians * (((((A * s + B) * s - C) * s + D) * s - E) * s + 1.0f);
}

static inline f32 floatCos(f32 radians) {
    static const f32 A = -2.605e-07;
    static const f32 B = 2.47609e-05;
    static const f32 C = 1.3888397e-03;
    static const f32 D = 4.16666418e-02;
    static const f32 E = 4.999999963e-01;

    if (radians < 0.0f || radians >= TAU) {
        radians -= floatRound(radians / TAU) * TAU;
    }

    f32 d;
    if (radians < PI) {
        if (radians > HalfPI) {
            radians = PI - radians;
            d = -1.0f;
        } else {
            d = 1.0f;
        }
    } else {
        if (radians > (PI + HalfPI)) {
            radians = radians - TAU;
            d = 1.0f;
        } else {
            radians = PI - radians;
            d = -1.0f;
        }
    }

    const f32 s = radians * radians;
    return d * (((((A * s + B) * s - C) * s + D) * s - E) * s + 1.0f);
}

f32 math_perlin(f32 x, f32 y, f32 z, int x_wrap, int y_wrap, int z_wrap) { return stb_perlin_noise3(x, y, z, x_wrap, y_wrap, z_wrap); }

f32 NewMaths::vec22angle(MEvec2 v2) { return atan2f(v2.y, v2.x); }

f32 NewMaths::clamp(f32 input, f32 min, f32 max) {
    if (input < min)
        return min;
    else if (input > max)
        return max;
    else
        return input;
}

struct NewMaths::RandState {
    uint64_t seed;
    bool initialized = false;
};
NewMaths::RandState RANDSTATE;

uint64_t NewMaths::rand_XOR() {
    if (!RANDSTATE.initialized) {
        RANDSTATE.seed = time(NULL);
        RANDSTATE.initialized = true;
    }
    uint64_t x = RANDSTATE.seed;
    x ^= x << 9;
    x ^= x >> 5;
    x ^= x << 15;
    return RANDSTATE.seed = x;
}
int NewMaths::rand_range(int min, int max) {
    // return min + rand() / (RAND_MAX / (max - min + 1) + 1);
    return min + NewMaths::rand_XOR() % (max - min + 1);
    // return (f32)rand()/ RAND_MAX * (max - min + 1) + min;
}

inline f64 NewMaths::random_double() {
    // Returns a random real in [0,1).
    return rand() / (RAND_MAX + 1.0);
}

inline f64 NewMaths::random_double(f64 min, f64 max) {
    // Returns a random real in [min,max).
    return min + (max - min) * random_double();
}
struct NewMaths::v2 {
    union {
        f32 e[2];
        struct {
            f32 x, y;
        };
    };

    v2() : e{0, 0} {}
    v2(f32 e0, f32 e1) : e{e0, e1} {}

    NewMaths::v2 operator-() const { return NewMaths::v2(-e[0], -e[1]); }
    f32 operator[](int i) const { return e[i]; }
    f32 &operator[](int i) { return e[i]; }

    NewMaths::v2 &operator+=(const NewMaths::v2 &v) {
        e[0] += v.e[0];
        e[1] += v.e[1];
        return *this;
    }
    NewMaths::v2 &operator-=(const NewMaths::v2 &v) {
        e[0] -= v.e[0];
        e[1] -= v.e[1];
        return *this;
    }

    NewMaths::v2 &operator*=(const f32 t) {
        e[0] *= t;
        e[1] *= t;
        return *this;
    }

    NewMaths::v2 &operator/=(const f32 t) { return *this *= 1 / t; }

    f32 length_squared() const { return e[0] * e[0] + e[1] * e[1]; }

    f32 length() const { return sqrt(length_squared()); }

    NewMaths::v2 normalize() {
        f32 lengf = length();

        if (lengf > 0)
            return NewMaths::v2(x / lengf, y / lengf);
        else
            return NewMaths::v2(0, 0);
    }

    f32 dot(NewMaths::v2 V) {
        f32 result = x * V.x + y * V.y;
        return result;
    }
    NewMaths::v2 perpendicular() { return NewMaths::v2(y, -x); }
    f32 perpdot(NewMaths::v2 V) {
        f32 result = x * V.y - y * V.x;
        return result;
    }
    f32 cross(NewMaths::v2 V) {
        f32 result = x * V.y - y * V.x;
        return result;
    }

    NewMaths::v2 hadamard(NewMaths::v2 V) {
        NewMaths::v2 result(x * V.x, y * V.y);
        return result;
    }

    f32 angle(NewMaths::v2 V)  // returns signed angle in radians
    {
        return atan2(x * V.y - y * V.x, x * V.x + y * V.y);
    }
};
NewMaths::v2 operator-(NewMaths::v2 a, NewMaths::v2 b) {
    NewMaths::v2 tojesus(a.x - b.x, a.y - b.y);
    return tojesus;
}

NewMaths::v2 operator+(NewMaths::v2 a, NewMaths::v2 b) {
    NewMaths::v2 tojesus(a.x + b.x, a.y + b.y);
    return tojesus;
}
NewMaths::v2 operator-(NewMaths::v2 a, f32 b) {
    NewMaths::v2 tojesus(a.x - b, a.y - b);
    return tojesus;
}

NewMaths::v2 operator+(NewMaths::v2 a, f32 b) {
    NewMaths::v2 tojesus(a.x + b, a.y + b);
    return tojesus;
}

NewMaths::v2 operator*(NewMaths::v2 a, f32 b) {
    NewMaths::v2 tojesus(a.x * b, a.y * b);
    return tojesus;
}
NewMaths::v2 operator*(NewMaths::v2 a, NewMaths::v2 b) {
    NewMaths::v2 tojesus(a.x * b.x, a.y * b.y);
    return tojesus;
}

NewMaths::v2 operator*(f32 b, NewMaths::v2 a) {
    NewMaths::v2 tojesus(a.x * b, a.y * b);
    return tojesus;
}

NewMaths::v2 operator/(NewMaths::v2 a, f32 b) {
    NewMaths::v2 tojesus(a.x / b, a.y / b);
    return tojesus;
}
NewMaths::v2 operator/(NewMaths::v2 a, NewMaths::v2 b) {
    NewMaths::v2 tojesus(a.x / b.x, a.y / b.y);
    return tojesus;
}

f32 NewMaths::v2_distance_2Points(NewMaths::v2 A, NewMaths::v2 B) { return sqrt((A.x - B.x) * (A.x - B.x) + (A.y - B.y) * (A.y - B.y)); }

NewMaths::v2 NewMaths::unitvec_AtoB(NewMaths::v2 A, NewMaths::v2 B) {
    f32 n = NewMaths::v2_distance_2Points(A, B);
    return ((B - A) / n);
}

f32 NewMaths::signed_angle_v2(NewMaths::v2 A, NewMaths::v2 B) { return atan2(A.x * B.y - A.y * B.x, A.x * B.x + A.y * B.y); }

NewMaths::v2 NewMaths::Rotate2D(NewMaths::v2 P, f32 sine, f32 cosine) { return NewMaths::v2(NewMaths::v2(cosine, -sine).dot(P), NewMaths::v2(sine, cosine).dot(P)); }
NewMaths::v2 NewMaths::Rotate2D(NewMaths::v2 P, f32 Angle) {
    f32 sine = sin(Angle);
    f32 cosine = cos(Angle);
    return NewMaths::Rotate2D(P, sine, cosine);
}
NewMaths::v2 NewMaths::Rotate2D(NewMaths::v2 p, NewMaths::v2 o, f32 angle) {
    // Demonstration: https://www.desmos.com/calculator/8aaegifsba
    f32 s = sin(angle);
    f32 c = cos(angle);

    f32 x = (p.x - o.x) * c - (p.y - o.y) * s + o.x;
    f32 y = (p.x - o.x) * s + (p.y - o.y) * c + o.y;

    return NewMaths::v2(x, y);
}

NewMaths::v2 NewMaths::Reflection2D(NewMaths::v2 P, NewMaths::v2 N) { return P - 2 * N.dot(P) * N; }

bool NewMaths::PointInRectangle(NewMaths::v2 P, NewMaths::v2 A, NewMaths::v2 B, NewMaths::v2 C) {
    NewMaths::v2 M = P;
    NewMaths::v2 AB = B - A;
    NewMaths::v2 BC = C - B;
    NewMaths::v2 AM = M - A;
    NewMaths::v2 BM = M - B;
    if (0 <= AB.dot(AM) && AB.dot(AM) <= AB.dot(AB) && 0 <= BC.dot(BM) && BC.dot(BM) <= BC.dot(BC))
        return true;
    else
        return false;
}

int NewMaths::sign(f32 x) {
    if (x > 0)
        return 1;
    else if (x < 0)
        return -1;
    else
        return 0;
}

static f32 NewMaths::dot(NewMaths::v2 A, NewMaths::v2 B) { return A.x * B.x + A.y * B.y; }

static f32 NewMaths::perpdot(NewMaths::v2 A, NewMaths::v2 B) { return A.x * B.y - A.y * B.x; }

static bool operator==(NewMaths::v2 A, NewMaths::v2 B) { return A.x == B.x && A.y == B.y; }

static f32 abso(f32 F) { return F > 0 ? F : -F; };

static NewMaths::v2 rand_vector(f32 length) { return NewMaths::v2(NewMaths::rand_range(-100, 100) * 0.01f * length, NewMaths::rand_range(-100, 100) * 0.01f * length); }

// --------------- Vector Functions ---------------

MEvec3 NewMaths::NormalizeVector(MEvec3 v) {
    f32 l = sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
    if (l == 0) return VECTOR3_ZERO;

    v.x *= 1 / l;
    v.y *= 1 / l;
    v.z *= 1 / l;
    return v;
}

MEvec3 NewMaths::Add(MEvec3 a, MEvec3 b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
    return a;
}

MEvec3 NewMaths::Subtract(MEvec3 a, MEvec3 b) {
    a.x -= b.x;
    a.y -= b.y;
    a.z -= b.z;
    return a;
}

MEvec3 NewMaths::ScalarMult(MEvec3 v, f32 s) { return MEvec3{v.x * s, v.y * s, v.z * s}; }

f64 NewMaths::Distance(MEvec3 a, MEvec3 b) {
    MEvec3 AMinusB = Subtract(a, b);
    return sqrt(UTIL_dot(AMinusB, AMinusB));
}

MEvec3 NewMaths::VectorProjection(MEvec3 a, MEvec3 b) {
    // https://en.wikipedia.org/wiki/Vector_projection
    MEvec3 normalizedB = NormalizeVector(b);
    f64 a1 = UTIL_dot(a, normalizedB);
    return ScalarMult(normalizedB, a1);
}

MEvec3 NewMaths::Reflection(MEvec3 *v1, MEvec3 *v2) {
    f32 dotpr = UTIL_dot(*v2, *v1);
    MEvec3 result;
    result.x = v2->x * 2 * dotpr;
    result.y = v2->y * 2 * dotpr;
    result.z = v2->z * 2 * dotpr;

    result.x = v1->x - result.x;
    result.y = v1->y - result.y;
    result.z = v1->z - result.z;

    return result;
}

MEvec3 NewMaths::RotatePoint(MEvec3 p, MEvec3 r, MEvec3 pivot) { return Add(RotateVector(Subtract(p, pivot), EulerAnglesToMatrix3x3(r)), pivot); }

f64 NewMaths::DistanceFromPointToLine2D(MEvec3 lP1, MEvec3 lP2, MEvec3 p) {
    // https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
    return fabsf((lP2.y - lP1.y) * p.x - (lP2.x - lP1.x) * p.y + lP2.x * lP1.y - lP2.y * lP1.x) / Distance(lP1, lP2);
}

// --------------- Matrix3x3 type ---------------

inline NewMaths::Matrix3x3 NewMaths::Transpose(Matrix3x3 m) {
    Matrix3x3 t;

    t.m[0][1] = m.m[1][0];
    t.m[1][0] = m.m[0][1];

    t.m[0][2] = m.m[2][0];
    t.m[2][0] = m.m[0][2];

    t.m[1][2] = m.m[2][1];
    t.m[2][1] = m.m[1][2];

    t.m[0][0] = m.m[0][0];
    t.m[1][1] = m.m[1][1];
    t.m[2][2] = m.m[2][2];

    return t;
}

NewMaths::Matrix3x3 NewMaths::Identity() {
    Matrix3x3 m = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    return m;
}

// Based on the article: Extracting Euler Angles from a Rotation Matrix - Mike Day, Insomniac Games
MEvec3 NewMaths::Matrix3x3ToEulerAngles(Matrix3x3 m) {
    MEvec3 rotation = VECTOR3_ZERO;
    rotation.x = atan2(m.m[1][2], m.m[2][2]);

    f32 c2 = sqrt(m.m[0][0] * m.m[0][0] + m.m[0][1] * m.m[0][1]);
    rotation.y = atan2(-m.m[0][2], c2);

    f32 s1 = sin(rotation.x);
    f32 c1 = cos(rotation.x);
    rotation.z = atan2(s1 * m.m[2][0] - c1 * m.m[1][0], c1 * m.m[1][1] - s1 * m.m[2][1]);

    return ScalarMult(rotation, 180.0 / PI);
}

NewMaths::Matrix3x3 NewMaths::EulerAnglesToMatrix3x3(MEvec3 rotation) {

    f32 s1 = sin(rotation.x * PI / 180.0);
    f32 c1 = cos(rotation.x * PI / 180.0);
    f32 s2 = sin(rotation.y * PI / 180.0);
    f32 c2 = cos(rotation.y * PI / 180.0);
    f32 s3 = sin(rotation.z * PI / 180.0);
    f32 c3 = cos(rotation.z * PI / 180.0);

    Matrix3x3 m = {{{c2 * c3, c2 * s3, -s2}, {s1 * s2 * c3 - c1 * s3, s1 * s2 * s3 + c1 * c3, s1 * c2}, {c1 * s2 * c3 + s1 * s3, c1 * s2 * s3 - s1 * c3, c1 * c2}}};

    return m;
}

// Vectors are interpreted as rows
inline MEvec3 NewMaths::RotateVector(MEvec3 v, Matrix3x3 m) {
    return MEvec3{v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0], v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1], v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2]};
}

NewMaths::Matrix3x3 NewMaths::MultiplyMatrix3x3(Matrix3x3 a, Matrix3x3 b) {
    Matrix3x3 r;
    int i, j, k;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            r.m[i][j] = 0;
            for (k = 0; k < 3; k++) {
                r.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }

    return r;
}

NewMaths::Matrix4x4 NewMaths::Identity4x4() { return Matrix4x4{{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}}; }

NewMaths::Matrix4x4 NewMaths::GetProjectionMatrix(f32 rightPlane, f32 leftPlane, f32 topPlane, f32 bottomPlane, f32 nearPlane, f32 farPlane) {
    Matrix4x4 matrix = {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}};

    matrix.m[0][0] = 2.0f / (rightPlane - leftPlane);
    matrix.m[1][1] = 2.0f / (topPlane - bottomPlane);
    matrix.m[2][2] = -2.0f / (farPlane - nearPlane);
    matrix.m[3][3] = 1;
    matrix.m[3][0] = -(rightPlane + leftPlane) / (rightPlane - leftPlane);
    matrix.m[3][1] = -(topPlane + bottomPlane) / (topPlane - bottomPlane);
    matrix.m[3][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);

    return matrix;
}

// --------------- Numeric functions ---------------

f32 NewMaths::Lerp(f64 t, f32 a, f32 b) { return (1 - t) * a + t * b; }

int NewMaths::Step(f32 edge, f32 x) { return x < edge ? 0 : 1; }

f32 NewMaths::Smoothstep(f32 edge0, f32 edge1, f32 x) {
    // Scale, bias and saturate x to 0..1 range
    x = UTIL_clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    // Evaluate polynomial
    return x * x * (3 - 2 * x);
}

// Modulus function, returning only positive values
int NewMaths::Modulus(int a, int b) {
    int r = a % b;
    return r < 0 ? r + b : r;
}

f32 NewMaths::fModulus(f32 a, f32 b) {
    f32 r = fmod(a, b);
    return r < 0 ? r + b : r;
}

}  // namespace ME