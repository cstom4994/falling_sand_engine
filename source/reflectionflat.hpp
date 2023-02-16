// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_REFLECTIONFLAT_HPP_
#define _METADOT_REFLECTIONFLAT_HPP_

#include <array>
#include <vector>

#include "core/core.h"
#include "core/macros.h"
#include "console.hpp"
#include "game_basic.hpp"

METADOT_INLINE Command::ItemLog &operator<<(Command::ItemLog &log, ImVec4 &vec) {
    log << "ImVec4: [" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << "]";
    return log;
}

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

namespace Meta {
struct Msg {
    float a;
    float b;
};
}  // namespace Meta

template <>
struct MetaEngine::StaticRefl::TypeInfo<Meta::Msg> : TypeInfoBase<Meta::Msg> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("a"), &Type::a},
            Field{TSTR("b"), &Type::b},
    };
};

TYPEOF_REGISTER(Meta::Msg);

#endif
