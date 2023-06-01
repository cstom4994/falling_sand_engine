// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "core/core.hpp"
#include "core/mathlib.hpp"

// Random number generation
typedef struct RNG {
    pcg32_random_t rng;
    unsigned int root_seed;
} RNG;

RNG* RNG_Create();
void RNG_Delete(RNG* rng);
u32 RNG_Next(RNG* rng);