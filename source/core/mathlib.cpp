// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <list>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/const.h"
#include "libs/external/stb_perlin.h"
#include "mathlib.hpp"

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



F32 math_perlin(F32 x, F32 y, F32 z, int x_wrap, int y_wrap, int z_wrap) { return stb_perlin_noise3(x, y, z, x_wrap, y_wrap, z_wrap); }

#pragma region NewMATH

F32 NewMaths::vec22angle(MEvec2 v2) { return atan2f(v2.y, v2.x); }

F32 NewMaths::clamp(F32 input, F32 min, F32 max) {
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
    // return (F32)rand()/ RAND_MAX * (max - min + 1) + min;
}

inline F64 NewMaths::random_double() {
    // Returns a random real in [0,1).
    return rand() / (RAND_MAX + 1.0);
}

inline F64 NewMaths::random_double(F64 min, F64 max) {
    // Returns a random real in [min,max).
    return min + (max - min) * random_double();
}
struct NewMaths::v2 {
    union {
        F32 e[2];
        struct {
            F32 x, y;
        };
    };

    v2() : e{0, 0} {}
    v2(F32 e0, F32 e1) : e{e0, e1} {}

    NewMaths::v2 operator-() const { return NewMaths::v2(-e[0], -e[1]); }
    F32 operator[](int i) const { return e[i]; }
    F32 &operator[](int i) { return e[i]; }

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

    NewMaths::v2 &operator*=(const F32 t) {
        e[0] *= t;
        e[1] *= t;
        return *this;
    }

    NewMaths::v2 &operator/=(const F32 t) { return *this *= 1 / t; }

    F32 length_squared() const { return e[0] * e[0] + e[1] * e[1]; }

    F32 length() const { return sqrt(length_squared()); }

    NewMaths::v2 normalize() {
        F32 lengf = length();

        if (lengf > 0)
            return NewMaths::v2(x / lengf, y / lengf);
        else
            return NewMaths::v2(0, 0);
    }

    F32 dot(NewMaths::v2 V) {
        F32 result = x * V.x + y * V.y;
        return result;
    }
    NewMaths::v2 perpendicular() { return NewMaths::v2(y, -x); }
    F32 perpdot(NewMaths::v2 V) {
        F32 result = x * V.y - y * V.x;
        return result;
    }
    F32 cross(NewMaths::v2 V) {
        F32 result = x * V.y - y * V.x;
        return result;
    }

    NewMaths::v2 hadamard(NewMaths::v2 V) {
        NewMaths::v2 result(x * V.x, y * V.y);
        return result;
    }

    F32 angle(NewMaths::v2 V)  // returns signed angle in radians
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
NewMaths::v2 operator-(NewMaths::v2 a, F32 b) {
    NewMaths::v2 tojesus(a.x - b, a.y - b);
    return tojesus;
}

NewMaths::v2 operator+(NewMaths::v2 a, F32 b) {
    NewMaths::v2 tojesus(a.x + b, a.y + b);
    return tojesus;
}

NewMaths::v2 operator*(NewMaths::v2 a, F32 b) {
    NewMaths::v2 tojesus(a.x * b, a.y * b);
    return tojesus;
}
NewMaths::v2 operator*(NewMaths::v2 a, NewMaths::v2 b) {
    NewMaths::v2 tojesus(a.x * b.x, a.y * b.y);
    return tojesus;
}

NewMaths::v2 operator*(F32 b, NewMaths::v2 a) {
    NewMaths::v2 tojesus(a.x * b, a.y * b);
    return tojesus;
}

NewMaths::v2 operator/(NewMaths::v2 a, F32 b) {
    NewMaths::v2 tojesus(a.x / b, a.y / b);
    return tojesus;
}
NewMaths::v2 operator/(NewMaths::v2 a, NewMaths::v2 b) {
    NewMaths::v2 tojesus(a.x / b.x, a.y / b.y);
    return tojesus;
}

F32 NewMaths::v2_distance_2Points(NewMaths::v2 A, NewMaths::v2 B) { return sqrt((A.x - B.x) * (A.x - B.x) + (A.y - B.y) * (A.y - B.y)); }

NewMaths::v2 NewMaths::unitvec_AtoB(NewMaths::v2 A, NewMaths::v2 B) {
    F32 n = NewMaths::v2_distance_2Points(A, B);
    return ((B - A) / n);
}

F32 NewMaths::signed_angle_v2(NewMaths::v2 A, NewMaths::v2 B) { return atan2(A.x * B.y - A.y * B.x, A.x * B.x + A.y * B.y); }

NewMaths::v2 NewMaths::Rotate2D(NewMaths::v2 P, F32 sine, F32 cosine) { return NewMaths::v2(NewMaths::v2(cosine, -sine).dot(P), NewMaths::v2(sine, cosine).dot(P)); }
NewMaths::v2 NewMaths::Rotate2D(NewMaths::v2 P, F32 Angle) {
    F32 sine = sin(Angle);
    F32 cosine = cos(Angle);
    return NewMaths::Rotate2D(P, sine, cosine);
}
NewMaths::v2 NewMaths::Rotate2D(NewMaths::v2 p, NewMaths::v2 o, F32 angle) {
    // Demonstration: https://www.desmos.com/calculator/8aaegifsba
    F32 s = sin(angle);
    F32 c = cos(angle);

    F32 x = (p.x - o.x) * c - (p.y - o.y) * s + o.x;
    F32 y = (p.x - o.x) * s + (p.y - o.y) * c + o.y;

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

int NewMaths::sign(F32 x) {
    if (x > 0)
        return 1;
    else if (x < 0)
        return -1;
    else
        return 0;
}

static F32 NewMaths::dot(NewMaths::v2 A, NewMaths::v2 B) { return A.x * B.x + A.y * B.y; }

static F32 NewMaths::perpdot(NewMaths::v2 A, NewMaths::v2 B) { return A.x * B.y - A.y * B.x; }

static bool operator==(NewMaths::v2 A, NewMaths::v2 B) { return A.x == B.x && A.y == B.y; }

static F32 abso(F32 F) { return F > 0 ? F : -F; };

static NewMaths::v2 rand_vector(F32 length) { return NewMaths::v2(NewMaths::rand_range(-100, 100) * 0.01f * length, NewMaths::rand_range(-100, 100) * 0.01f * length); }

// --------------- Vector Functions ---------------

MEvec3 NewMaths::NormalizeVector(MEvec3 v) {
    F32 l = sqrt((v.x * v.x) + (v.y * v.y) + (v.z * v.z));
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

MEvec3 NewMaths::ScalarMult(MEvec3 v, F32 s) { return MEvec3{v.x * s, v.y * s, v.z * s}; }

F64 NewMaths::Distance(MEvec3 a, MEvec3 b) {
    MEvec3 AMinusB = Subtract(a, b);
    return sqrt(UTIL_dot(AMinusB, AMinusB));
}

MEvec3 NewMaths::VectorProjection(MEvec3 a, MEvec3 b) {
    // https://en.wikipedia.org/wiki/Vector_projection
    MEvec3 normalizedB = NormalizeVector(b);
    F64 a1 = UTIL_dot(a, normalizedB);
    return ScalarMult(normalizedB, a1);
}

MEvec3 NewMaths::Reflection(MEvec3 *v1, MEvec3 *v2) {
    F32 dotpr = UTIL_dot(*v2, *v1);
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

F64 NewMaths::DistanceFromPointToLine2D(MEvec3 lP1, MEvec3 lP2, MEvec3 p) {
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

    F32 c2 = sqrt(m.m[0][0] * m.m[0][0] + m.m[0][1] * m.m[0][1]);
    rotation.y = atan2(-m.m[0][2], c2);

    F32 s1 = sin(rotation.x);
    F32 c1 = cos(rotation.x);
    rotation.z = atan2(s1 * m.m[2][0] - c1 * m.m[1][0], c1 * m.m[1][1] - s1 * m.m[2][1]);

    return ScalarMult(rotation, 180.0 / PI);
}

NewMaths::Matrix3x3 NewMaths::EulerAnglesToMatrix3x3(MEvec3 rotation) {

    F32 s1 = sin(rotation.x * PI / 180.0);
    F32 c1 = cos(rotation.x * PI / 180.0);
    F32 s2 = sin(rotation.y * PI / 180.0);
    F32 c2 = cos(rotation.y * PI / 180.0);
    F32 s3 = sin(rotation.z * PI / 180.0);
    F32 c3 = cos(rotation.z * PI / 180.0);

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

NewMaths::Matrix4x4 NewMaths::GetProjectionMatrix(F32 rightPlane, F32 leftPlane, F32 topPlane, F32 bottomPlane, F32 nearPlane, F32 farPlane) {
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

F32 NewMaths::Lerp(F64 t, F32 a, F32 b) { return (1 - t) * a + t * b; }

int NewMaths::Step(F32 edge, F32 x) { return x < edge ? 0 : 1; }

F32 NewMaths::Smoothstep(F32 edge0, F32 edge1, F32 x) {
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

F32 NewMaths::fModulus(F32 a, F32 b) {
    F32 r = fmod(a, b);
    return r < 0 ? r + b : r;
}

#pragma endregion NewMATH

#pragma region PCG

// PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

static pcg32_random_t pcg32_global = PCG32_INITIALIZER;

// pcg32_srandom(initstate, initseq)
// pcg32_srandom_r(rng, initstate, initseq):
//     Seed the rng.  Specified in two parts, state initializer and a
//     sequence selection constant (a.k.a. stream id)

void pcg32_srandom_r(pcg32_random_t *rng, uint64_t initstate, uint64_t initseq) {
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_random_r(rng);
    rng->state += initstate;
    pcg32_random_r(rng);
}

void pcg32_srandom(uint64_t seed, uint64_t seq) { pcg32_srandom_r(&pcg32_global, seed, seq); }

// pcg32_random()
// pcg32_random_r(rng)
//     Generate a uniformly distributed 32-bit random number

uint32_t pcg32_random_r(pcg32_random_t *rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

uint32_t pcg32_random() { return pcg32_random_r(&pcg32_global); }

// pcg32_boundedrand(bound):
// pcg32_boundedrand_r(rng, bound):
//     Generate a uniformly distributed number, r, where 0 <= r < bound

uint32_t pcg32_boundedrand_r(pcg32_random_t *rng, uint32_t bound) {
    // To avoid bias, we need to make the range of the RNG a multiple of
    // bound, which we do by dropping output less than a threshold.
    // A naive scheme to calculate the threshold would be to do
    //
    //     uint32_t threshold = 0x100000000ull % bound;
    //
    // but 64-bit div/mod is slower than 32-bit div/mod (especially on
    // 32-bit platforms).  In essence, we do
    //
    //     uint32_t threshold = (0x100000000ull-bound) % bound;
    //
    // because this version will calculate the same modulus, but the LHS
    // value is less than 2^32.

    uint32_t threshold = -bound % bound;

    // Uniformity guarantees that this loop will terminate.  In practice, it
    // should usually terminate quickly; on average (assuming all bounds are
    // equally likely), 82.25% of the time, we can expect it to require just
    // one iteration.  In the worst case, someone passes a bound of 2^31 + 1
    // (i.e., 2147483649), which invalidates almost 50% of the range.  In
    // practice, bounds are typically small and only a tiny amount of the range
    // is eliminated.
    for (;;) {
        uint32_t r = pcg32_random_r(rng);
        if (r >= threshold) return r % bound;
    }
}

uint32_t pcg32_boundedrand(uint32_t bound) { return pcg32_boundedrand_r(&pcg32_global, bound); }

#pragma endregion PCG

#pragma region Poro

namespace MetaEngine {
namespace math {

///////////////////////////////////////////////////////////////////////////////
// ripped from
// http://www.programmersheaven.com/download/33146/download.aspx
// Point in Polygon test by Hesham Ebrahimi
namespace {
template <class TType>
inline TType isLeft(const CVector2<TType> &p0, const CVector2<TType> &p1, const CVector2<TType> &p2) {
    return ((p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y));
}
}  // namespace

//=============================================================================

bool IsPointInsidePolygon(const CVector2<PointType> &point, const std::vector<CVector2<PointType>> &polygon) {
    int wn = 0;  // the winding number counter

    // std::vector<CPoint *>::iterator it;
    unsigned int i = 0;
    // loop through all edges of the polygon
    for (i = 0; i < polygon.size() - 1; i++)  // edge from V[i] to V[i+1]
    {
        if (polygon[i].y <= point.y) {                              // start y <= pt->y
            if (polygon[i + 1].y > point.y)                         // an upward crossing
                if (isLeft(polygon[i], polygon[i + 1], point) > 0)  // P left of edge
                    ++wn;                                           // have a valid up intersect
        } else {                                                    // start y > P.y (no test needed)
            if (polygon[i + 1].y <= point.y)                        // a downward crossing
                if (isLeft(polygon[i], polygon[i + 1], point) < 0)  // P right of edge
                    --wn;                                           // have a valid down intersect
        }
    }
    if (wn == 0) return false;

    return true;
}

bool IsPointInsidePolygon(const CVector2<int> &point, const std::vector<CVector2<int>> &polygon) {
    int wn = 0;  // the winding number counter

    // std::vector<CPoint *>::iterator it;
    unsigned int i = 0;
    // loop through all edges of the polygon
    for (i = 0; i < polygon.size() - 1; i++)  // edge from V[i] to V[i+1]
    {
        if (polygon[i].y <= point.y) {                              // start y <= pt->y
            if (polygon[i + 1].y > point.y)                         // an upward crossing
                if (isLeft(polygon[i], polygon[i + 1], point) > 0)  // P left of edge
                    ++wn;                                           // have a valid up intersect
        } else {                                                    // start y > P.y (no test needed)
            if (polygon[i + 1].y <= point.y)                        // a downward crossing
                if (isLeft(polygon[i], polygon[i + 1], point) < 0)  // P right of edge
                    --wn;                                           // have a valid down intersect
        }
    }
    if (wn == 0) return false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////

// Stolen from http://erich.realtimerendering.com/ptinpoly/
//
/* ======= Crossings algorithm ============================================ */

/* Shoot a test ray along +X axis.  The strategy, from MacMartin, is to
 * compare vertex Y values to the testing point's Y and quickly discard
 * edges which are entirely to one side of the test ray.
 *
 * Input 2D polygon _pgon_ with _numverts_ number of vertices and test point
 * _point_, returns 1 if inside, 0 if outside.  WINDING and CONVEX can be
 * defined for this test.
 */
// int CrossingsTest( const std::vector< types::vector2 >& pgon, numverts, point )
bool IsPointInsidePolygon_Better(const CVector2<PointType> &point, const std::vector<CVector2<PointType>> &pgon) {
    int numverts = (int)pgon.size();
    bool inside_flag = false;
    bool yflag0, yflag1, xflag0;
    // float ty, tx; // , vtx0, vtx1 ;

    PointType tx = point.x;
    PointType ty = point.y;

    CVector2<PointType> vtx0 = pgon[numverts - 1];

    /* get test bit for above/below X axis */
    yflag0 = (vtx0.y >= ty);
    CVector2<PointType> vtx1 = pgon[0];

    for (int j = 1; j < numverts + 1; ++j) {
        // for ( j = numverts+1 ; --j ; ) {

        yflag1 = (vtx1.y >= ty);
        /* check if endpoints straddle (are on opposite sides) of X axis
         * (i.e. the Y's differ); if so, +X ray could intersect this edge.
         */
        if (yflag0 != yflag1) {
            xflag0 = (vtx0.x >= tx);
            /* check if endpoints are on same side of the Y axis (i.e. X's
             * are the same); if so, it's easy to test if edge hits or misses.
             */
            if (xflag0 == (vtx1.x >= tx)) {

                /* if edge's X values both right of the point, must hit */
                if (xflag0) inside_flag = !inside_flag;
            } else {
                /* compute intersection of pgon segment with +X ray, note
                 * if >= point's X; if so, the ray hits it.
                 */
                if ((vtx1.x - (vtx1.y - ty) * (vtx0.x - vtx1.x) / (vtx0.y - vtx1.y)) >= tx) {
                    inside_flag = !inside_flag;
                }
            }
        }

        /* move to next pair of vertices, retaining info as possible */
        yflag0 = yflag1;
        vtx0 = vtx1;
        if (j < numverts)
            vtx1 = pgon[j];
        else
            vtx1 = pgon[j - numverts];
    }

    return (inside_flag);
}

///////////////////////////////////////////////////////////////////////////////

namespace {
void extractXAxisFromAngle(CVector2<PointType> &x, float a) {
    x.x = -cos(a);
    x.y = -sin(a);
}

void extractYAxisFromAngle(CVector2<PointType> &y, float a) {
    y.x = -sin(a);
    y.y = cos(a);
}
}  // namespace

//=============================================================================

bool IsPointInsideRect(const CVector2<PointType> &point, const CVector2<PointType> &position, const CVector2<PointType> &width, float rotation) {
    CVector2<PointType> boxWidth = 0.5f * width;

    // quick and dirty collision check
    /*
{
CVector2< PointType > delta = position - point;
float dis = delta.Length();
if( dis > boxWidth.Length()  )
    return 0;
}*/

    CVector2<PointType> axis[2];

    // Pura tahojen normaalit asennosta
    extractXAxisFromAngle(axis[0], rotation);
    extractYAxisFromAngle(axis[1], rotation);

    CVector2<PointType> offset = point - position;

    CVector2<PointType> point_pos;  // = mat * offset;
    point_pos.x = Dot(offset, axis[0]);
    point_pos.y = Dot(offset, axis[1]);

    return (-boxWidth.x <= point_pos.x && boxWidth.x > point_pos.x && -boxWidth.y <= point_pos.y && boxWidth.y > point_pos.y);
}

///////////////////////////////////////////////////////////////////////////////

bool IsPointInsideCircle(const CVector2<PointType> &point, const CVector2<PointType> &position, float d) { return ((point - position).LengthSquared() <= Square(d * 0.5f)); }

///////////////////////////////////////////////////////////////////////////////

bool IsPointInsideRect(const CVector2<PointType> &point, const CVector2<PointType> &rect_low, const CVector2<PointType> &rect_high) {
    return (rect_low.x <= point.x && rect_high.x > point.x && rect_low.y <= point.y && rect_high.y > point.y);
}

//-----------------------------------------------------------------------------

bool DoesLineAndBoxCollide(const CVector2<PointType> &p1, const CVector2<PointType> &p2, const CVector2<PointType> &rect_low, const CVector2<PointType> &rect_high) {
    if (IsPointInsideRect(p1, rect_low, rect_high)) return true;
    if (IsPointInsideRect(p2, rect_low, rect_high)) return true;

    CVector2<PointType> result;
    typedef CVector2<PointType> vec2;

    if (LineIntersection(p1, p2, rect_low, vec2(rect_low.x, rect_high.y), result)) return true;

    if (LineIntersection(p1, p2, vec2(rect_low.x, rect_high.y), rect_high, result)) return true;

    if (LineIntersection(p1, p2, rect_high, vec2(rect_high.x, rect_low.x), result)) return true;

    if (LineIntersection(p1, p2, vec2(rect_high.x, rect_low.x), rect_low, result)) return true;

    return false;
}

///////////////////////////////////////////////////////////////////////////////

// Implementation from the book Real Time Collision Detection by Christer Ericson
// found on page 183, implemented in 2D by removing extra cross products
bool TestLineAABB(const CVector2<PointType> &p0, const CVector2<PointType> &p1, const CVector2<PointType> &rect_min, const CVector2<PointType> &rect_max) {
    typedef CVector2<PointType> Point;
    typedef CVector2<PointType> Vector;

    // Point    rect_center = ( rect_min + rect_max ) * 0.5f;
    const Vector rect_extents(rect_max - rect_min);
    const Vector line_halfwidth(p1 - p0);
    const Point line_midpoint(p0 + p1 - rect_min - rect_max);
    // line_midpoint = line_midpoint - rect_center;     // Translate box and segment to origin

    // Try world coordinate axes as separating axes
    float adx = abs(line_halfwidth.x);
    if (abs(line_midpoint.x) > rect_extents.x + adx) return false;

    float ady = abs(line_halfwidth.y);
    if (abs(line_midpoint.y) > rect_extents.y + ady) return false;

    // Add in an epsilon term to counteract arithmetic errors when segment is
    // (near) parallel to a coordinate axis
    adx += FLT_EPSILON;
    ady += FLT_EPSILON;

    // Try cross products of segment direction vector with coordinate axes
    if (abs(line_midpoint.x * line_halfwidth.y - line_midpoint.y * line_halfwidth.x) > rect_extents.x * ady + rect_extents.y * adx) return false;

    return true;
}

///////////////////////////////////////////////////////////////////////////////

}  // end of namespace math
}  // end of namespace MetaEngine

#pragma endregion Poro

#pragma region c2

#define CUTE_C2_IMPLEMENTATION
#include "libs/cute/cute_c2.h"

METAENGINE_STATIC_ASSERT(METAENGINE_POLY_MAX_VERTS == C2_MAX_POLYGON_VERTS, "Must be equal.");

METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_V2) == sizeof(c2v), "Must be equal.");
METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_SinCos) == sizeof(c2r), "Must be equal.");
METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_Transform) == sizeof(c2x), "Must be equal.");
METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_M2x2) == sizeof(c2m), "Must be equal.");
METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_Halfspace) == sizeof(c2h), "Must be equal.");
METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_Ray) == sizeof(c2Ray), "Must be equal.");
METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_Raycast) == sizeof(c2Raycast), "Must be equal.");
METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_Manifold) == sizeof(c2Manifold), "Must be equal.");
METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_GjkCache) == sizeof(c2GJKCache), "Must be equal.");
METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_Circle) == sizeof(c2Circle), "Must be equal.");
METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_Aabb) == sizeof(c2AABB), "Must be equal.");
METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_Capsule) == sizeof(c2Capsule), "Must be equal.");
METAENGINE_STATIC_ASSERT(sizeof(METAENGINE_Poly) == sizeof(c2Poly), "Must be equal.");

bool metadot_circle_to_circle(METAENGINE_Circle A, METAENGINE_Circle B) { return !!c2CircletoCircle(*(c2Circle *)&A, *(c2Circle *)&B); }

bool metadot_circle_to_aabb(METAENGINE_Circle A, METAENGINE_Aabb B) { return !!c2CircletoAABB(*(c2Circle *)&A, *(c2AABB *)&B); }

bool metadot_circle_to_capsule(METAENGINE_Circle A, METAENGINE_Capsule B) { return !!c2CircletoCapsule(*(c2Circle *)&A, *(c2Capsule *)&B); }

bool metadot_aabb_to_aabb(METAENGINE_Aabb A, METAENGINE_Aabb B) { return !!c2AABBtoAABB(*(c2AABB *)&A, *(c2AABB *)&B); }

bool metadot_aabb_to_capsule(METAENGINE_Aabb A, METAENGINE_Capsule B) { return !!c2AABBtoCapsule(*(c2AABB *)&A, *(c2Capsule *)&B); }

bool metadot_capsule_to_capsule(METAENGINE_Capsule A, METAENGINE_Capsule B) { return !!c2CapsuletoCapsule(*(c2Capsule *)&A, *(c2Capsule *)&B); }

bool metadot_circle_to_poly(METAENGINE_Circle A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx) { return !!c2CircletoPoly(*(c2Circle *)&A, (c2Poly *)B, (c2x *)bx); }

bool metadot_aabb_to_poly(METAENGINE_Aabb A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx) { return !!c2AABBtoPoly(*(c2AABB *)&A, (c2Poly *)B, (c2x *)bx); }

bool metadot_capsule_to_poly(METAENGINE_Capsule A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx) { return !!c2CapsuletoPoly(*(c2Capsule *)&A, (c2Poly *)B, (c2x *)bx); }

bool metadot_poly_to_poly(const METAENGINE_Poly *A, const METAENGINE_Transform *ax, const METAENGINE_Poly *B, const METAENGINE_Transform *bx) {
    return !!c2PolytoPoly((c2Poly *)A, (c2x *)ax, (c2Poly *)B, (c2x *)bx);
}

bool metadot_ray_to_circle(METAENGINE_Ray A, METAENGINE_Circle B, METAENGINE_Raycast *out) { return !!c2RaytoCircle(*(c2Ray *)&A, *(c2Circle *)&B, (c2Raycast *)out); }

bool metadot_ray_to_aabb(METAENGINE_Ray A, METAENGINE_Aabb B, METAENGINE_Raycast *out) { return !!c2RaytoAABB(*(c2Ray *)&A, *(c2AABB *)&B, (c2Raycast *)out); }

bool metadot_ray_to_capsule(METAENGINE_Ray A, METAENGINE_Capsule B, METAENGINE_Raycast *out) { return !!c2RaytoCapsule(*(c2Ray *)&A, *(c2Capsule *)&B, (c2Raycast *)out); }

bool metadot_ray_to_poly(METAENGINE_Ray A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx_ptr, METAENGINE_Raycast *out) {
    return !!c2RaytoPoly(*(c2Ray *)&A, (c2Poly *)B, (c2x *)bx_ptr, (c2Raycast *)out);
}

void metadot_circle_to_circle_manifold(METAENGINE_Circle A, METAENGINE_Circle B, METAENGINE_Manifold *m) { c2CircletoCircleManifold(*(c2Circle *)&A, *(c2Circle *)&B, (c2Manifold *)m); }

void metadot_circle_to_aabb_manifold(METAENGINE_Circle A, METAENGINE_Aabb B, METAENGINE_Manifold *m) { c2CircletoAABBManifold(*(c2Circle *)&A, *(c2AABB *)&B, (c2Manifold *)m); }

void metadot_circle_to_capsule_manifold(METAENGINE_Circle A, METAENGINE_Capsule B, METAENGINE_Manifold *m) { c2CircletoCapsuleManifold(*(c2Circle *)&A, *(c2Capsule *)&B, (c2Manifold *)m); }

void metadot_aabb_to_aabb_manifold(METAENGINE_Aabb A, METAENGINE_Aabb B, METAENGINE_Manifold *m) { c2AABBtoAABBManifold(*(c2AABB *)&A, *(c2AABB *)&B, (c2Manifold *)m); }

void metadot_aabb_to_capsule_manifold(METAENGINE_Aabb A, METAENGINE_Capsule B, METAENGINE_Manifold *m) { c2AABBtoCapsuleManifold(*(c2AABB *)&A, *(c2Capsule *)&B, (c2Manifold *)m); }

void metadot_capsule_to_capsule_manifold(METAENGINE_Capsule A, METAENGINE_Capsule B, METAENGINE_Manifold *m) { c2CapsuletoCapsuleManifold(*(c2Capsule *)&A, *(c2Capsule *)&B, (c2Manifold *)m); }

void metadot_circle_to_poly_manifold(METAENGINE_Circle A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx, METAENGINE_Manifold *m) {
    c2CircletoPolyManifold(*(c2Circle *)&A, (c2Poly *)B, (c2x *)bx, (c2Manifold *)m);
}

void metadot_aabb_to_poly_manifold(METAENGINE_Aabb A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx, METAENGINE_Manifold *m) {
    c2AABBtoPolyManifold(*(c2AABB *)&A, (c2Poly *)B, (c2x *)bx, (c2Manifold *)m);
}

void metadot_capsule_to_poly_manifold(METAENGINE_Capsule A, const METAENGINE_Poly *B, const METAENGINE_Transform *bx, METAENGINE_Manifold *m) {
    c2CapsuletoPolyManifold(*(c2Capsule *)&A, (c2Poly *)B, (c2x *)bx, (c2Manifold *)m);
}

void metadot_poly_to_poly_manifold(const METAENGINE_Poly *A, const METAENGINE_Transform *ax, const METAENGINE_Poly *B, const METAENGINE_Transform *bx, METAENGINE_Manifold *m) {
    c2PolytoPolyManifold((c2Poly *)A, (c2x *)ax, (c2Poly *)B, (c2x *)bx, (c2Manifold *)m);
}

float metadot_gjk(const void *A, METAENGINE_ShapeType typeA, const METAENGINE_Transform *ax_ptr, const void *B, METAENGINE_ShapeType typeB, const METAENGINE_Transform *bx_ptr, METAENGINE_V2 *outA,
                  METAENGINE_V2 *outB, int use_radius, int *iterations, METAENGINE_GjkCache *cache) {
    return c2GJK(A, (C2_TYPE)typeA, (c2x *)ax_ptr, B, (C2_TYPE)typeB, (c2x *)bx_ptr, (c2v *)outA, (c2v *)outB, use_radius, iterations, (c2GJKCache *)cache);
}

METAENGINE_ToiResult metadot_toi(const void *A, METAENGINE_ShapeType typeA, const METAENGINE_Transform *ax_ptr, METAENGINE_V2 vA, const void *B, METAENGINE_ShapeType typeB,
                                 const METAENGINE_Transform *bx_ptr, METAENGINE_V2 vB, int use_radius) {
    METAENGINE_ToiResult result;
    c2TOIResult c2result = c2TOI(A, (C2_TYPE)typeA, (c2x *)ax_ptr, *(c2v *)&vA, B, (C2_TYPE)typeB, (c2x *)bx_ptr, *(c2v *)&vB, use_radius);
    result = *(METAENGINE_ToiResult *)&c2result;
    return result;
}

void metadot_inflate(void *shape, METAENGINE_ShapeType type, float skin_factor) { c2Inflate(shape, (C2_TYPE)type, skin_factor); }

int metadot_hull(METAENGINE_V2 *verts, int count) { return c2Hull((c2v *)verts, count); }

void metadot_norms(METAENGINE_V2 *verts, METAENGINE_V2 *norms, int count) { c2Norms((c2v *)verts, (c2v *)norms, count); }

void metadot_make_poly(METAENGINE_Poly *p) { c2MakePoly((c2Poly *)p); }

METAENGINE_V2 metadot_centroid(const METAENGINE_V2 *metadot_verts, int count) {
    using namespace MetaEngine;
    const v2 *verts = (const v2 *)metadot_verts;
    if (count == 0)
        return metadot_v2(0, 0);
    else if (count == 1)
        return verts[0];
    else if (count == 2)
        return (verts[0] + verts[1]) * 0.5f;
    METAENGINE_V2 c = metadot_v2(0, 0);
    float area_sum = 0;
    METAENGINE_V2 p0 = verts[0];
    for (int i = 0; i < count; ++i) {
        METAENGINE_V2 p1 = verts[0] - p0;
        METAENGINE_V2 p2 = verts[i] - p0;
        METAENGINE_V2 p3 = (i + 1 == count ? verts[0] : verts[i + 1]) - p0;
        METAENGINE_V2 e1 = p2 - p1;
        METAENGINE_V2 e2 = p3 - p1;
        float area = 0.5f * metadot_cross(e1, e2);
        area_sum += area;
        c = c + (p1 + p2 + p3) * area * (1.0f / 3.0f);
    }
    return c * (1.0f / area_sum) + p0;
}

int metadot_collided(const void *A, const METAENGINE_Transform *ax, METAENGINE_ShapeType typeA, const void *B, const METAENGINE_Transform *bx, METAENGINE_ShapeType typeB) {
    return c2Collided(A, (c2x *)ax, (C2_TYPE)typeA, B, (c2x *)bx, (C2_TYPE)typeB);
}

void metadot_collide(const void *A, const METAENGINE_Transform *ax, METAENGINE_ShapeType typeA, const void *B, const METAENGINE_Transform *bx, METAENGINE_ShapeType typeB, METAENGINE_Manifold *m) {
    c2Collide(A, (c2x *)ax, (C2_TYPE)typeA, B, (c2x *)bx, (C2_TYPE)typeB, (c2Manifold *)m);
}

bool metadot_cast_ray(METAENGINE_Ray A, const void *B, const METAENGINE_Transform *bx, METAENGINE_ShapeType typeB, METAENGINE_Raycast *out) {
    return c2CastRay(*(c2Ray *)&A, B, (c2x *)bx, (C2_TYPE)typeB, (c2Raycast *)out);
}

#pragma endregion c2