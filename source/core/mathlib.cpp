// Copyright(c) 2023, KaoruXun All rights reserved.

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
