// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#include "rng.h"

#include "engine/core/cpp/utils.hpp"
#include "engine/core/mathlib.hpp"
#include "engine/core/utils/utility.hpp"

RNG* RNG_Create() {
    srand(time(NULL));
    RNG* sRNG = new RNG;
    sRNG->root_seed = rand();
    return sRNG;
}

void RNG_Delete(RNG* rng) {
    if (NULL != rng) delete rng;
}

u32 RNG_Next(RNG* rng) { return rand(); }
