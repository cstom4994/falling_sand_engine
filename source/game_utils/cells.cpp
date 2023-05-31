#include "cells.h"

#include <algorithm>
#include <string>

#include "core/core.hpp"
#include "core/math/mathlib.hpp"
#include "game_resources.hpp"

inline float Deg2Rad(float a) { return a * 0.01745329252f; }

inline float Rad2Deg(float a) { return a * 57.29577951f; }

inline float clampf(float value, float min_inclusive, float max_inclusive) {
    if (min_inclusive > max_inclusive) {
        std::swap(min_inclusive, max_inclusive);
    }
    return value < min_inclusive ? min_inclusive : value < max_inclusive ? value : max_inclusive;
}

inline void normalize_point(float x, float y, MEvec2* out) {
    float n = x * x + y * y;
    // Already normalized.
    if (n == 1.0f) {
        return;
    }

    n = sqrt(n);
    // Too close to zero.
    if (n < 1e-5) {
        return;
    }

    n = 1.0f / n;
    out->x = x * n;
    out->y = y * n;
}

/**
A more effect random number getter function, get from ejoy2d.
*/
inline static float RANDOM_M11(unsigned int* seed) {
    *seed = *seed * 134775813 + 1;
    union {
        uint32_t d;
        float f;
    } u;
    u.d = (((uint32_t)(*seed) & 0x7fff) << 8) | 0x40000000;
    return u.f - 3.0f;
}