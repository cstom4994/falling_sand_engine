// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef ME_REFLECTIONFLAT_HPP
#define ME_REFLECTIONFLAT_HPP

#include <array>
#include <vector>

#include "core/core.hpp"
#include "core/macros.hpp"
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

namespace Meta {
template <typename Str>
struct Typeof;
template <typename Str>
using Typeof_t = typename Meta::Typeof<Str>::type;
}  // namespace Meta

template <typename T>
struct is_std_tuple : std::false_type {};
template <typename... Ts>
struct is_std_tuple<std::tuple<Ts...>> : std::true_type {};
template <typename T>
constexpr bool is_std_tuple_v = is_std_tuple<T>::value;

template <typename T, typename Tuple>
constexpr auto tuple_init(Tuple &&t) {
    return std::apply([](auto &&...elems) { return T{std::forward<decltype(elems)>(elems)...}; }, std::forward<Tuple>(t));
}

template <typename Str, typename Value>
constexpr auto attr_init(Str, Value &&v) {
    using T = Meta::Typeof_t<Str>;
    if constexpr (is_std_tuple_v<std::decay_t<Value>>)
        return tuple_init<T>(std::forward<Value>(v));
    else
        return T{std::forward<Value>(v)};
}

#define TYPEOF_REGISTER(X)                                                    \
    template <>                                                               \
    struct Meta::Typeof<typename ME::meta::static_refl::TypeInfo<X>::TName> { \
        using type = X;                                                       \
    }

namespace Meta {
struct Msg {
    float a;
    float b;
};
}  // namespace Meta

template <>
struct ME::meta::static_refl::TypeInfo<Meta::Msg> : TypeInfoBase<Meta::Msg> {
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
            Field{TSTR("a"), &Type::a},
            Field{TSTR("b"), &Type::b},
    };
};

TYPEOF_REGISTER(Meta::Msg);

#endif
