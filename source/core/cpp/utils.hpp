// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_CPP_TUTIL_HPP_
#define _METADOT_CPP_TUTIL_HPP_

#include <algorithm>
#include <bitset>
#include <cassert>
#include <codecvt>
#include <cstdint>
#include <locale>
#include <span>
#include <string_view>
#include <type_traits>
#include <unordered_map>

#include "core/alloc.hpp"
#include "core/const.h"
#include "core/core.hpp"
#include "core/mathlib.hpp"
#include "core/stl/list.h"
// #include "scripting/lua/lua_wrapper.hpp"
#include "libs/cJSON.h"
// #include "sdl_wrapper.h"

struct lua_State;

#pragma region Template

namespace MetaEngine {
template <typename T>
constexpr bool always_false = false;

template <typename T, T V>
struct IValue {
    static constexpr T value = V;
};
template <typename T>
struct IsIValue;
template <typename T>
constexpr bool IsIValue_v = IsIValue<T>::value;
template <auto V>
using IValue_of = IValue<decltype(V), V>;

template <typename...>
struct typename_template_type;

template <typename T>
struct is_typename_template_type;
template <typename T>
static constexpr bool is_typename_template_type_v = is_typename_template_type<T>::value;

// use IValue to replace integral value in template arguments
// we provide some partial template specializations (see details/ToTTType.inl for more details)
// [example]
// template<typename T, std::size_t N>
// struct Array;
// to_typename_template_type_t<Array<T, N>> == typename_template_type<T, IValue<std::size_t, N>>
template <typename T>
struct to_typename_template_type : std::type_identity<T> {};
template <typename T>
using to_typename_template_type_t = typename to_typename_template_type<T>::type;

// type object
// type value
template <typename T>
struct member_pointer_traits;
template <typename T>
using member_pointer_traits_object = typename member_pointer_traits<T>::object;
template <typename T>
using member_pointer_traits_value = typename member_pointer_traits<T>::value;

template <template <typename...> typename T, typename... Ts>
struct is_instantiable;
template <template <typename...> typename T, typename... Ts>
constexpr bool is_instantiable_v = is_instantiable<T, Ts...>::value;

template <template <typename...> class TA, template <typename...> class TB>
struct is_same_typename_template;
template <template <typename...> class TA, template <typename...> class TB>
constexpr bool is_same_typename_template_v = is_same_typename_template<TA, TB>::value;

template <typename Instance, template <typename...> class T>
struct is_instance_of;
template <typename Instance, template <typename...> class T>
constexpr bool is_instance_of_v = is_instance_of<Instance, T>::value;

template <typename T, typename... Args>
struct is_list_initializable;
template <typename T, typename... Args>
static constexpr bool is_list_initializable_v = is_list_initializable<T, Args...>::value;

template <typename T>
struct is_defined;
template <typename T>
static constexpr bool is_defined_v = is_defined<T>::value;

template <typename T>
struct has_virtual_base;
template <typename T>
constexpr bool has_virtual_base_v = has_virtual_base<T>::value;

template <typename Base, typename Derived>
struct is_virtual_base_of;
template <typename Base, typename Derived>
constexpr bool is_virtual_base_of_v = is_virtual_base_of<Base, Derived>::value;

template <size_t N>
constexpr std::size_t lengthof(const char (&str)[N]) noexcept;

constexpr std::size_t string_hash_seed(std::size_t seed, const char* str, std::size_t N) noexcept;
constexpr std::size_t string_hash_seed(std::size_t seed, std::string_view str) noexcept { return string_hash_seed(seed, str.data(), str.size()); }
template <std::size_t N>
constexpr std::size_t string_hash_seed(std::size_t seed, const char (&str)[N]) noexcept {
    return string_hash_seed(seed, str, N - 1);
}
constexpr std::size_t string_hash_seed(std::size_t seed, const char* str) noexcept;

constexpr std::size_t string_hash(const char* str, std::size_t N) noexcept;
constexpr std::size_t string_hash(std::string_view str) noexcept { return string_hash(str.data(), str.size()); }
template <std::size_t N>
constexpr std::size_t string_hash(const char (&str)[N]) noexcept {
    return string_hash(str, N - 1);
}
constexpr std::size_t string_hash(const char* str) noexcept;

template <typename T>
struct is_function_pointer;
template <typename T>
constexpr bool is_function_pointer_v = is_function_pointer<T>::value;

template <template <class...> class Op, class... Args>
struct is_valid;
template <template <class...> class Op, class... Args>
constexpr bool is_valid_v = is_valid<Op, Args...>::value;

template <typename V1, typename Obj1, typename V2, typename Obj2>
constexpr bool member_pointer_equal(V1 Obj1::*p1, V2 Obj2::*p2) noexcept;

template <typename Y>
struct is_same_with {
    template <typename X>
    struct Ttype : std::is_same<X, Y> {};
};

enum class ReferenceMode { None, Left, Right };

enum class CVRefMode : std::uint8_t {
    None = 0b0000,
    Left = 0b0001,
    Right = 0b0010,
    Const = 0b0100,
    ConstLeft = 0b0101,
    ConstRight = 0b0110,
    Volatile = 0b1000,
    VolatileLeft = 0b1001,
    VolatileRight = 0b1010,
    CV = 0b1100,
    CVLeft = 0b1101,
    CVRight = 0b1110,
};

constexpr bool CVRefMode_IsLeft(CVRefMode mode) noexcept { return static_cast<std::uint8_t>(mode) & 0b0001; }
constexpr bool CVRefMode_IsRight(CVRefMode mode) noexcept { return static_cast<std::uint8_t>(mode) & 0b0010; }
constexpr bool CVRefMode_IsConst(CVRefMode mode) noexcept { return static_cast<std::uint8_t>(mode) & 0b0100; }
constexpr bool CVRefMode_IsVolatile(CVRefMode mode) noexcept { return static_cast<std::uint8_t>(mode) & 0b1000; }

template <typename T, std::size_t N>
class TempArray {
public:
    template <typename... Elems>
    constexpr TempArray(Elems&&... elems) : data{static_cast<T>(elems)...} {}

    constexpr operator std::add_lvalue_reference_t<T[N]>() & { return data; }
    constexpr operator std::add_lvalue_reference_t<const T[N]>() const& { return data; }
    constexpr operator std::add_rvalue_reference_t<T[N]>() && { return std::move(data); }
    constexpr operator std::add_rvalue_reference_t<const T[N]>() const&& { return std::move(data); }

    constexpr operator std::span<T>() { return data; }
    constexpr operator std::span<const T>() const { return data; }
    constexpr operator std::span<T, N>() { return data; }
    constexpr operator std::span<const T, N>() const { return data; }

private:
    T data[N];
};

template <typename T, typename... Ts>
TempArray(T, Ts...) -> TempArray<T, sizeof...(Ts) + 1>;
}  // namespace MetaEngine

// To Typename Template Type

// 1
template <template <auto> class T, auto Int>
struct MetaEngine::to_typename_template_type<T<Int>> : std::type_identity<typename_template_type<IValue_of<Int>>> {};

// 1...
// template<template<auto...>class T, auto... Ints>
// struct MetaEngine::to_typename_template_type<T<Ints...>>
//  : std::type_identity<typename_template_type<IValue_of<Ints>...>> {};

// 1 0
template <template <auto, typename> class T, auto Int, typename U>
struct MetaEngine::to_typename_template_type<T<Int, U>> : std::type_identity<typename_template_type<IValue_of<Int>, U>> {};

// 1 0...
// template<template<auto, typename...>class T, auto Int, typename... Us>
// struct MetaEngine::to_typename_template_type<T<Int, Us...>>
//  : std::type_identity<typename_template_type<IValue_of<Int>, Us...>> {};

// 0 1
template <template <typename, auto> class T, typename U, auto Int>
struct MetaEngine::to_typename_template_type<T<U, Int>> : std::type_identity<typename_template_type<U, IValue_of<Int>>> {};

// 0 1...
// template<template<typename, auto...>class T, typename U, auto... Ints>
// struct MetaEngine::to_typename_template_type<T<U, Ints...>>
//  : std::type_identity<typename_template_type<U, IValue_of<Ints>...>> {};

// 1 1
template <template <auto, auto> class T, auto Int0, auto Int1>
struct MetaEngine::to_typename_template_type<T<Int0, Int1>> : std::type_identity<typename_template_type<IValue_of<Int0>, IValue_of<Int1>>> {};

// 1 1...
// template<template<auto, auto, typename...>class T, auto Int, auto... Ints>
// struct MetaEngine::to_typename_template_type<T<Int, Ints...>>
//  : std::type_identity<typename_template_type<IValue_of<Int>, IValue_of<Ints>...>> {};

// 1 0 0
template <template <auto, typename, typename> class T, auto Int, typename U0, typename U1>
struct MetaEngine::to_typename_template_type<T<Int, U0, U1>> : std::type_identity<typename_template_type<IValue_of<Int>, U0, U1>> {};

// 1 0 0 0...
template <template <auto, typename, typename, typename...> class T, auto Int, typename U0, typename U1, typename... Us>
struct MetaEngine::to_typename_template_type<T<Int, U0, U1, Us...>> : std::type_identity<typename_template_type<IValue_of<Int>, U0, U1, Us...>> {};

// 0 1 0
template <template <typename, auto, typename> class T, typename U0, auto Int, typename U1>
struct MetaEngine::to_typename_template_type<T<U0, Int, U1>> : std::type_identity<typename_template_type<U0, IValue_of<Int>, U1>> {};

// 0 1 0 0...
template <template <typename, auto, typename, typename...> class T, typename U0, auto Int, typename U1, typename... Us>
struct MetaEngine::to_typename_template_type<T<U0, Int, U1, Us...>> : std::type_identity<typename_template_type<U0, IValue_of<Int>, U1, Us...>> {};

// 0 0 1
template <template <typename, typename, auto> class T, typename U0, typename U1, auto Int>
struct MetaEngine::to_typename_template_type<T<U0, U1, Int>> : std::type_identity<typename_template_type<U0, U1, IValue_of<Int>>> {};

// 0 0 1 1...
template <template <typename, typename, auto, auto...> class T, typename U0, typename U1, auto Int, auto... Ints>
struct MetaEngine::to_typename_template_type<T<U0, U1, Int, Ints...>> : std::type_identity<typename_template_type<U0, U1, IValue_of<Int>, IValue_of<Ints>...>> {};

// 1 1 0
template <template <auto, auto, typename> class T, auto Int0, auto Int1, typename U>
struct MetaEngine::to_typename_template_type<T<Int0, Int1, U>> : std::type_identity<typename_template_type<IValue_of<Int0>, IValue_of<Int1>, U>> {};

// 1 1 0 0...
template <template <auto, auto, typename, typename...> class T, auto Int0, auto Int1, typename U, typename... Us>
struct MetaEngine::to_typename_template_type<T<Int0, Int1, U, Us...>> : std::type_identity<typename_template_type<IValue_of<Int0>, IValue_of<Int1>, U, Us...>> {};

// 1 0 1
template <template <auto, typename, auto> class T, auto Int0, typename U, auto Int1>
struct MetaEngine::to_typename_template_type<T<Int0, U, Int1>> : std::type_identity<typename_template_type<IValue_of<Int0>, U, IValue_of<Int1>>> {};

// 1 0 1 1...
template <template <auto, typename, auto, auto...> class T, auto Int0, typename U, auto Int1, auto... Ints>
struct MetaEngine::to_typename_template_type<T<Int0, U, Int1, Ints...>> : std::type_identity<typename_template_type<IValue_of<Int0>, U, IValue_of<Int1>, IValue_of<Ints>...>> {};

// 0 1 1
template <template <typename, auto, auto> class T, typename U, auto Int0, auto Int1>
struct MetaEngine::to_typename_template_type<T<U, Int0, Int1>> : std::type_identity<typename_template_type<U, IValue_of<Int0>, IValue_of<Int1>>> {};

// 0 1 1 1...
template <template <typename, auto, auto, auto...> class T, typename U, auto Int0, auto Int1, auto... Ints>
struct MetaEngine::to_typename_template_type<T<U, Int0, Int1, Ints...>> : std::type_identity<typename_template_type<U, IValue_of<Int0>, IValue_of<Int1>, IValue_of<Ints>...>> {};

// 1 1 1
template <template <auto, auto, auto> class T, auto Int0, auto Int1, auto Int2>
struct MetaEngine::to_typename_template_type<T<Int0, Int1, Int2>> : std::type_identity<typename_template_type<IValue_of<Int0>, IValue_of<Int1>, IValue_of<Int2>>> {};

// 1 1 1 1...
template <template <auto, auto, auto, auto...> class T, auto Int0, auto Int1, auto Int2, auto... Ints>
struct MetaEngine::to_typename_template_type<T<Int0, Int1, Int2, Ints...>> : std::type_identity<typename_template_type<IValue_of<Int0>, IValue_of<Int1>, IValue_of<Int2>, IValue_of<Ints>...>> {};

namespace MetaEngine::details {
template <typename Void, template <typename...> typename T, typename... Ts>
struct is_instantiable : std::false_type {};
template <template <typename...> typename T, typename... Ts>
struct is_instantiable<std::void_t<T<Ts...>>, T, Ts...> : std::true_type {};

template <typename Void, typename T, typename... Args>
struct is_list_initializable : std::false_type {};

template <typename T, typename... Args>
struct is_list_initializable<std::void_t<decltype(T{std::declval<Args>()...})>, T, Args...> : std::true_type {};

template <typename Void, typename T>
struct is_defined_helper : std::false_type {};

template <typename T>
struct is_defined_helper<std::void_t<decltype(sizeof(T))>, T> : std::true_type {};

template <std::size_t size>
struct fnv1a_traits;

template <>
struct fnv1a_traits<4> {
    using type = std::uint32_t;
    static constexpr std::uint32_t offset = 2166136261;
    static constexpr std::uint32_t prime = 16777619;
};

template <>
struct fnv1a_traits<8> {
    using type = std::uint64_t;
    static constexpr std::uint64_t offset = 14695981039346656037ull;
    static constexpr std::uint64_t prime = 1099511628211ull;
};

template <typename Void, typename T>
struct is_function_pointer : std::false_type {};

template <typename T>
struct is_function_pointer<std::enable_if_t<std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>>, T> : std::true_type {};

struct has_virtual_base_void {};
template <typename Void, typename Obj>
struct has_virtual_base_helper : std::true_type {};
template <typename Obj>
struct has_virtual_base_helper<std::void_t<decltype(reinterpret_cast<has_virtual_base_void has_virtual_base_void::*>(std::declval<has_virtual_base_void Obj::*>()))>, Obj> : std::false_type {};

template <typename Void, typename Base, typename Derived>
struct is_virtual_base_of_helper : std::is_base_of<Base, Derived> {};
template <typename Base, typename Derived>
struct is_virtual_base_of_helper<std::void_t<decltype(static_cast<Derived*>(std::declval<Base*>()))>, Base, Derived> : std::false_type {};

template <class Void, template <class...> class Op, class... Args>
struct is_valid : std::false_type {};
template <template <class...> class Op, class... Args>
struct is_valid<std::void_t<Op<Args...>>, Op, Args...> : std::true_type {};
}  // namespace MetaEngine::details

template <template <typename...> typename T, typename... Ts>
struct MetaEngine::is_instantiable : details::is_instantiable<void, T, Ts...> {};

template <typename Instance, template <typename...> class T>
struct MetaEngine::is_instance_of : std::false_type {};

template <typename... Args, template <typename...> class T>
struct MetaEngine::is_instance_of<T<Args...>, T> : std::true_type {};

template <typename T, typename... Args>
struct MetaEngine::is_list_initializable : details::is_list_initializable<void, T, Args...> {};

template <template <typename...> class TA, template <typename...> class TB>
struct MetaEngine::is_same_typename_template : std::false_type {};

template <template <typename...> class T>
struct MetaEngine::is_same_typename_template<T, T> : std::true_type {};

template <typename T>
struct MetaEngine::is_defined : details::is_defined_helper<void, T> {};

template <typename T, typename U>
struct MetaEngine::member_pointer_traits<T U::*> {
    using object = U;
    using value = T;
};

template <typename T>
struct MetaEngine::is_typename_template_type : std::false_type {};

template <template <typename...> class T, typename... Ts>
struct MetaEngine::is_typename_template_type<T<Ts...>> : std::true_type {};

template <typename T>
struct MetaEngine::IsIValue : std::false_type {};
template <typename T, T v>
struct MetaEngine::IsIValue<MetaEngine::IValue<T, v>> : std::true_type {};

template <size_t N>
constexpr std::size_t MetaEngine::lengthof(const char (&str)[N]) noexcept {
    static_assert(N > 0);
    assert(str[N - 1] == 0);  // c-style string
    return N - 1;
}

constexpr std::size_t MetaEngine::string_hash_seed(std::size_t seed, const char* str, std::size_t N) noexcept {
    using Traits = details::fnv1a_traits<sizeof(std::size_t)>;
    std::size_t value = seed;

    for (std::size_t i = 0; i < N; i++) value = (value ^ static_cast<Traits::type>(str[i])) * Traits::prime;

    return value;
}

constexpr std::size_t MetaEngine::string_hash_seed(std::size_t seed, const char* curr) noexcept {
    using Traits = details::fnv1a_traits<sizeof(std::size_t)>;
    std::size_t value = seed;

    while (*curr) {
        value = (value ^ static_cast<Traits::type>(*(curr++))) * Traits::prime;
    }

    return value;
}

constexpr std::size_t MetaEngine::string_hash(const char* str, std::size_t N) noexcept {
    using Traits = details::fnv1a_traits<sizeof(std::size_t)>;
    return string_hash_seed(Traits::offset, str, N);
}

constexpr std::size_t MetaEngine::string_hash(const char* str) noexcept {
    using Traits = details::fnv1a_traits<sizeof(std::size_t)>;
    return string_hash_seed(Traits::offset, str);
}

template <typename T>
struct MetaEngine::is_function_pointer : MetaEngine::details::is_function_pointer<void, T> {};

template <typename T>
struct MetaEngine::has_virtual_base : MetaEngine::details::has_virtual_base_helper<void, T> {};

template <typename Base, typename Derived>
struct MetaEngine::is_virtual_base_of : MetaEngine::details::is_virtual_base_of_helper<void, Base, Derived> {};

template <template <class...> class Op, class... Args>
struct MetaEngine::is_valid : MetaEngine::details::is_valid<void, Op, Args...> {};

template <typename V1, typename Obj1, typename V2, typename Obj2>
constexpr bool MetaEngine::member_pointer_equal(V1 Obj1::*p1, V2 Obj2::*p2) noexcept {
    if constexpr (std::is_same_v<Obj1, Obj2> && std::is_same_v<V1, V2>)
        return p1 == p2;
    else
        return false;
}

#pragma endregion Template

#endif
