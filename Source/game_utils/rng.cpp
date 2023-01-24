// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "rng.h"

#include "core/cpp/utils.hpp"
#include "mathlib.hpp"

RNG* RNG_Create() {
    pcg32_random_t rng;
    pcg32_srandom_r(&rng, Time::millis(), 1);
    unsigned int seed = pcg32_random_r(&rng);

    RNG* sRNG = new RNG;
    sRNG->rng = rng;
    sRNG->root_seed = seed;

    return sRNG;
}

void RNG_Delete(RNG* rng) {
    if (NULL != rng) delete rng;
}

U32 RNG_Next(RNG* rng) { return pcg32_random_r(&rng->rng); }
