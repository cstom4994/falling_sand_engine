// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_REFLECTIONFLAT_HPP
#define ME_REFLECTIONFLAT_HPP

#include <array>
#include <vector>

#include "engine/core/core.hpp"
#include "engine/core/macros.hpp"
#include "game_basic.hpp"

// ME_INLINE Command::ItemLog &operator<<(Command::ItemLog &log, ImVec4 &vec) {
//     log << "ImVec4: [" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << "]";
//     return log;
// }

static void int_setter(int &my_type, int v) { my_type = v; }

static void float_setter(float &my_type, float vec) {
    // if (vec.size() != 1) return;
    my_type = vec;
}

static void imvec4_setter(ImVec4 &my_type, std::vector<int> vec) {
    if (vec.size() < 4) return;

    my_type.x = vec[0] / 255.f;
    my_type.y = vec[1] / 255.f;
    my_type.z = vec[2] / 255.f;
    my_type.w = vec[3] / 255.f;
}

#endif
