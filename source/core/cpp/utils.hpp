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
#include "core/math/mathlib.hpp"
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
//	: std::type_identity<typename_template_type<IValue_of<Ints>...>> {};

// 1 0
template <template <auto, typename> class T, auto Int, typename U>
struct MetaEngine::to_typename_template_type<T<Int, U>> : std::type_identity<typename_template_type<IValue_of<Int>, U>> {};

// 1 0...
// template<template<auto, typename...>class T, auto Int, typename... Us>
// struct MetaEngine::to_typename_template_type<T<Int, Us...>>
//	: std::type_identity<typename_template_type<IValue_of<Int>, Us...>> {};

// 0 1
template <template <typename, auto> class T, typename U, auto Int>
struct MetaEngine::to_typename_template_type<T<U, Int>> : std::type_identity<typename_template_type<U, IValue_of<Int>>> {};

// 0 1...
// template<template<typename, auto...>class T, typename U, auto... Ints>
// struct MetaEngine::to_typename_template_type<T<U, Ints...>>
//	: std::type_identity<typename_template_type<U, IValue_of<Ints>...>> {};

// 1 1
template <template <auto, auto> class T, auto Int0, auto Int1>
struct MetaEngine::to_typename_template_type<T<Int0, Int1>> : std::type_identity<typename_template_type<IValue_of<Int0>, IValue_of<Int1>>> {};

// 1 1...
// template<template<auto, auto, typename...>class T, auto Int, auto... Ints>
// struct MetaEngine::to_typename_template_type<T<Int, Ints...>>
//	: std::type_identity<typename_template_type<IValue_of<Int>, IValue_of<Ints>...>> {};

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

std::vector<std::string> split(std::string strToSplit, char delimeter);
std::vector<std::string> string_split(std::string s, const char delimiter);
std::vector<std::string> split2(std::string const& original, char separator);

struct String {

    String() = default;

    String(const char* str [[maybe_unused]]) : m_String(str ? str : "") {}

    String(std::string str) : m_String(std::move(str)) {}

    operator const char*() { return m_String.c_str(); }

    operator std::string() { return m_String; }

    std::pair<size_t, size_t> NextPoi(size_t& start) const {
        size_t end = m_String.size();
        std::pair<size_t, size_t> range(end + 1, end);
        size_t pos = start;

        for (; pos < end; ++pos)
            if (!std::isspace(m_String[pos])) {
                range.first = pos;
                break;
            }

        for (; pos < end; ++pos)
            if (std::isspace(m_String[pos])) {
                range.second = pos;
                break;
            }

        start = range.second;
        return range;
    }

    [[nodiscard]] size_t End() const { return m_String.size() + 1; }

    std::string m_String;
};

long long metadot_gettime();
double metadot_gettime_d();
time_t metadot_gettime_mkgmtime(struct tm* unixdate);

class Timer {
public:
    typedef U32 Ticks;

    Timer();
    virtual ~Timer();

    virtual Timer& operator=(const Timer& other);
    virtual Timer operator-(const Timer& other);
    virtual Timer operator-(Ticks time);
    virtual Timer& operator-=(const Timer& other);
    virtual Timer& operator-=(Ticks time);

    //! Returns the time in ms (int)
    virtual Ticks GetTime() const;

    //! Returns the time in seconds (float)
    virtual float GetSeconds() const;

    //! Returns a time the time step, between the moment now and the last
    //! time Updated was called
    //! returns ms ( int )
    /*!
        This is useful in some
    */
    virtual Ticks GetDerivate() const;

    //! Returns the same thing as GetDerivate \sa GetDerivate()
    virtual float GetDerivateSeconds() const;

    //! called to reset the last updated, used with
    //! GetDerivate() and GetDerivateInSeconds()
    //! \sa GetDerivate()
    //! \sa GetDerivateInSeconds()
    virtual void Updated();

    //! Pauses the timer, so nothing is runnning
    virtual void Pause();

    //! Resumes the timer from a pause
    virtual void Resume();

    //! Resets the timer, starting from 0 ms
    virtual void Reset();

    //! Sets the time to the given time
    virtual void SetTime(Ticks time);

    //! tells us if something is paused
    virtual bool IsPaused() const { return myPause; }

protected:
    I64 myOffSet;
    I64 myPauseTime;
    bool myPause;
    Ticks myLastUpdate;
};

namespace SUtil {
typedef std::string String;
typedef std::string_view StringView;

inline std::string ws2s(const std::wstring& wstr) {
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

inline bool is_chinese_c(const char str) { return str & 0x80; }

inline bool is_chinese_str(const std::string& str) {
    for (int i = 0; i < str.length(); i++)
        if (is_chinese_c(str[i])) return true;
    return false;
}

inline bool equals(const char* a, const char* c) { return strcmp(a, c) == 0; }

inline bool startsWith(StringView s, StringView prefix) { return prefix.size() <= s.size() && (strncmp(prefix.data(), s.data(), prefix.size()) == 0); }

inline bool startsWith(StringView s, char prefix) { return !s.empty() && s[0] == prefix; }

inline bool startsWith(const char* s, const char* prefix) { return strncmp(s, prefix, strlen(prefix)) == 0; }

inline bool endsWith(StringView s, StringView suffix) { return suffix.size() <= s.size() && strncmp(suffix.data(), s.data() + s.size() - suffix.size(), suffix.size()) == 0; }

inline bool endsWith(StringView s, char suffix) { return !s.empty() && s[s.size() - 1] == suffix; }

inline bool endsWith(const char* s, const char* suffix) {
    auto sizeS = strlen(s);
    auto sizeSuf = strlen(suffix);

    return sizeSuf <= sizeS && strncmp(suffix, s + sizeS - sizeSuf, sizeSuf) == 0;
}

inline void toLower(char* s) {
    int l = strlen(s);
    int ind = 0;
    // spec of "simd"
    for (int i = 0; i < l / 4; i++) {
        s[ind] = std::tolower(s[ind]);
        s[ind + 1] = std::tolower(s[ind + 1]);
        s[ind + 2] = std::tolower(s[ind + 2]);
        s[ind + 3] = std::tolower(s[ind + 3]);
        ind += 4;
    }
    // do the rest linearly
    for (int i = 0; i < (l & 3); ++i) {
        s[ind++] = std::tolower(s[ind]);
    }
}

inline void toLower(String& ss) {
    int l = ss.size();
    auto s = ss.data();
    int ind = 0;
    // spec of "simd"
    for (int i = 0; i < l / 4; i++) {
        s[ind] = std::tolower(s[ind]);
        s[ind + 1] = std::tolower(s[ind + 1]);
        s[ind + 2] = std::tolower(s[ind + 2]);
        s[ind + 3] = std::tolower(s[ind + 3]);
        ind += 4;
    }
    // do the rest linearly
    for (int i = 0; i < (l & 3); ++i) s[ind++] = std::tolower(s[ind]);
}

inline void toUpper(char* s) {
    int l = strlen(s);
    int ind = 0;
    // spec of "simd"
    for (int i = 0; i < l / 4; i++) {
        s[ind] = std::toupper(s[ind]);
        s[ind + 1] = std::toupper(s[ind + 1]);
        s[ind + 2] = std::toupper(s[ind + 2]);
        s[ind + 3] = std::toupper(s[ind + 3]);
        ind += 4;
    }
    // do the rest linearly
    for (int i = 0; i < (l & 3); ++i) {
        s[ind++] = std::toupper(s[ind]);
    }
}

inline void toUpper(String& ss) {
    int l = ss.size();
    auto s = ss.data();
    int ind = 0;
    // spec of "simd"
    for (int i = 0; i < l / 4; i++) {
        s[ind] = std::toupper(s[ind]);
        s[ind + 1] = std::toupper(s[ind + 1]);
        s[ind + 2] = std::toupper(s[ind + 2]);
        s[ind + 3] = std::toupper(s[ind + 3]);
        ind += 4;
    }
    // do the rest linearly
    for (int i = 0; i < (l & 3); ++i) s[ind++] = std::toupper(s[ind]);
}

inline bool replaceWith(char* src, char what, char with) {
    for (int i = 0; true; ++i) {
        auto& id = src[i];
        if (id == '\0') return true;
        bool isWhat = id == what;
        id = isWhat * with + src[i] * (!isWhat);
    }
}

inline bool replaceWith(String& src, char what, char with) {
    for (int i = 0; i < src.size(); ++i) {
        auto& id = src.data()[i];
        bool isWhat = id == what;
        id = isWhat * with + src[i] * (!isWhat);
    }
    return true;
}

inline bool replaceWith(String& src, const char* what, const char* with) {
    String out;
    size_t whatlen = strlen(what);
    out.reserve(src.size());
    size_t ind = 0;
    size_t lastInd = 0;
    while (true) {
        ind = src.find(what, ind);
        if (ind == String::npos) {
            out += src.substr(lastInd);
            break;
        }
        out += src.substr(lastInd, ind - lastInd) + with;
        ind += whatlen;
        lastInd = ind;
    }
    src = out;
    return true;
}

inline bool replaceWith(String& src, const char* what, const char* with, int times) {
    for (int i = 0; i < times; ++i) replaceWith(src, what, with);
    return true;
}

// populates words with words that were crated by splitting line with one char of dividers
// NOTE!: when using views
//	if you delete the line (or src of the line)
//		-> string_views will no longer be valid!!!!!
/*template <typename StringOrView>
void splitString(const Viewo& line, std::vector<StringOrView>& words, const char* divider = " \n\t")
{
    auto beginIdx = line.find_first_not_of(divider, 0);
    while (beginIdx != Stringo::npos)
    {
        auto endIdx = line.find_first_of(divider, beginIdx);
        if (endIdx != Stringo::npos)
        {
            words.emplace_back(line.begin() + beginIdx, endIdx - beginIdx);
            beginIdx = line.find_first_not_of(divider, endIdx);
        }
        else {
            words.emplace_back(line.begin() + beginIdx, line.size() - beginIdx);
            break;
        }
    }
}*/
inline void splitString(const String& line, std::vector<String>& words, const char* divider = " \n\t") {
    auto beginIdx = line.find_first_not_of(divider, 0);
    while (beginIdx != String::npos) {
        auto endIdx = line.find_first_of(divider, beginIdx);
        if (endIdx != String::npos) {
            words.emplace_back(line.data() + beginIdx, endIdx - beginIdx);
            beginIdx = line.find_first_not_of(divider, endIdx);
        } else {
            words.emplace_back(line.data() + beginIdx, line.size() - beginIdx);
            break;
        }
    }
}

/*inline void splitString(const Viewo& line, std::vector<Viewo>& words, const char* divider = " \n\t")
{
    auto beginIdx = line.find_first_not_of(divider, 0);
    while (beginIdx != Stringo::npos)
    {
        auto endIdx = line.find_first_of(divider, beginIdx);
        if (endIdx != Stringo::npos)
        {
            words.emplace_back(line.begin() + beginIdx, endIdx - beginIdx);
            beginIdx = line.find_first_not_of(divider, endIdx);
        }
        else {
            words.emplace_back(line.begin() + beginIdx, line.size() - beginIdx);
            break;
        }
    }
}*/
// iterates over a string_view divided by "divider" (any chars in const char*)
// Example:
//		if(!ignoreBlanks)
//			"\n\nBoo\n" -> "","","Boo",""
//		else
//			"\n\nBoo\n" -> "Boo"
//
//
//	enable throwException to throw Stringo("No Word Left")
//	if(operator bool()==false)
//		-> on calling operator*() or operator->()
//
template <bool ignoreBlanks = false, typename DividerType = const char*, bool throwException = false>
class SplitIterator {
    static_assert(std::is_same<DividerType, const char*>::value || std::is_same<DividerType, char>::value);

private:
    StringView m_source;
    StringView m_pointer;
    // this fixes 1 edge case when m_source end with m_divider
    DividerType m_divider;
    bool m_ending = false;

    void step() {
        if (!operator bool()) return;

        if constexpr (ignoreBlanks) {
            bool first = true;
            while (m_pointer.empty() || first) {
                first = false;

                if (!operator bool()) return;
                // check if this is ending
                // the next one will be false
                if (m_source.size() == m_pointer.size()) {
                    m_source = StringView(nullptr, 0);
                } else {
                    auto viewSize = m_pointer.size();
                    m_source = StringView(m_source.data() + viewSize + 1, m_source.size() - viewSize - 1);
                    // shift source by viewSize

                    auto nextDivi = m_source.find_first_of(m_divider, 0);
                    if (nextDivi != String::npos)
                        m_pointer = StringView(m_source.data(), nextDivi);
                    else
                        m_pointer = StringView(m_source.data(), m_source.size());
                }
            }
        } else {
            // check if this is ending
            // the next one will be false
            if (m_source.size() == m_pointer.size()) {
                m_source = StringView(nullptr, 0);
                m_ending = false;
            } else {
                auto viewSize = m_pointer.size();
                m_source = StringView(m_source.data() + viewSize + 1, m_source.size() - viewSize - 1);
                // shift source by viewSize
                if (m_source.empty()) m_ending = true;
                auto nextDivi = m_source.find_first_of(m_divider, 0);
                if (nextDivi != String::npos)
                    m_pointer = StringView(m_source.data(), nextDivi);
                else
                    m_pointer = StringView(m_source.data(), m_source.size());
            }
        }
    }

public:
    SplitIterator(StringView src, DividerType divider) : m_source(src), m_divider(divider) {
        auto div = m_source.find_first_of(m_divider, 0);
        m_pointer = StringView(m_source.data(), div == String::npos ? m_source.size() : div);
        if constexpr (ignoreBlanks) {
            if (m_pointer.empty()) step();
        }
    }

    SplitIterator(const SplitIterator& s) = default;

    operator bool() const { return !m_source.empty() || m_ending; }

    SplitIterator& operator+=(size_t delta) {
        while (this->operator bool() && delta) {
            delta--;
            step();
        }
        return (*this);
    }

    SplitIterator& operator++() {
        if (this->operator bool()) step();
        return (*this);
    }

    SplitIterator operator++(int) {
        auto temp(*this);
        step();
        return temp;
    }

    const StringView& operator*() {
        if (throwException && !operator bool())  // Attempt to access* it or it->when no word is left in the iterator
            throw String("No Word Left");
        return m_pointer;
    }

    const StringView* operator->() {
        if (throwException && !operator bool())  // Attempt to access *it or it-> when no word is left in the iterator
            throw String("No Word Left");
        return &m_pointer;
    }
};

// removes comments: (replaces with ' ')
//	1. one liner starting with "//"
//	2. block comment bounded by "/*" and "*/"
inline void removeComments(String& src) {
    size_t offset = 0;
    bool opened = false;  // multiliner opened
    size_t openedStart = 0;
    while (true) {
        auto slash = src.find_first_of('/', offset);
        if (slash != String::npos) {
            String s = src.substr(slash);
            if (!opened) {
                if (src.size() == slash - 1) return;

                char next = src[slash + 1];
                if (next == '/')  // one liner
                {
                    auto end = src.find_first_of('\n', slash + 1);
                    if (end == String::npos) {
                        memset(src.data() + slash, ' ', src.size() - 1 - slash);
                        return;
                    }
                    memset(src.data() + slash, ' ', end - slash);
                    offset = end;
                } else if (next == '*') {
                    opened = true;
                    offset = slash + 1;
                    openedStart = slash;
                } else
                    offset = slash + 1;
            } else {
                if (src[slash - 1] == '*') {
                    opened = false;
                    memset(src.data() + openedStart, ' ', slash - openedStart);
                    offset = slash + 1;
                }
                offset = slash + 1;
            }
        } else if (opened) {
            memset(src.data() + openedStart, ' ', src.size() - 1 - openedStart);
            return;
        } else
            return;
    }
}

// converts utf8 encoded string to zero terminated int array of codePoints
// transfers ownership of returned array (don't forget free())
// length will be set to size returned array (excluding zero terminator)
const int* utf8toCodePointsArray(const char* c, int* length = nullptr);

std::u32string utf8toCodePoints(const char* c);

inline std::u32string utf8toCodePoints(const String& c) { return utf8toCodePoints(c.c_str()); }

// converts ascii u32 string to string
// use only if you know that there are only ascii characters
String u32StringToString(std::u32string_view s);

// returns first occurrence of digit or nullptr
inline const char* skipToNextDigit(const char* c) {
    c--;
    while (*(++c)) {
        if (*c >= '0' && *c <= '9') return c;
    }
    return nullptr;
}

// keeps parsing numbers until size is reached or until there are no numbers
// actualSize is set to number of numbers actually parsed
template <int size, typename numberType>
void parseNumbers(const char* c, numberType ray[size], int* actualSize = nullptr) {
    size_t chars = 0;

    for (int i = 0; i < size; ++i) {
        if ((c = skipToNextDigit(c + chars)) != nullptr) {
            if (std::is_same<numberType, int>::value)
                ray[i] = std::stoi(c, &chars);
            else if (std::is_same<numberType, float>::value)
                ray[i] = std::stof(c, &chars);
            else if (std::is_same<numberType, double>::value)
                ray[i] = std::stod(c, &chars);
            else
                static_assert("invalid type");
        } else {
            if (actualSize) *actualSize = i;
            return;
        }
    }
    if (actualSize) *actualSize = size;
}

// keeps parsing numbers until size is reached or until there are no numbers
// actualSize is set to number of numbers actually parsed
template <int size, typename numberType>
void parseNumbers(const String& s, numberType ray[size], int* actualSize = nullptr) {
    parseNumbers<size>(s.c_str(), ray, actualSize);
}

}  // namespace SUtil

#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#ifndef FILENAME_MAX
#define FILENAME_MAX 256
#endif

// #define max(a, b)                                                                                  \
//     ({                                                                                             \
//         __typeof__(a) _a = (a);                                                                    \
//         __typeof__(b) _b = (b);                                                                    \
//         _a > _b ? _a : _b;                                                                         \
//     })

// #define min(a, b)                                                                                  \
//     ({                                                                                             \
//         __typeof__(a) _a = (a);                                                                    \
//         __typeof__(b) _b = (b);                                                                    \
//         _a < _b ? _a : _b;                                                                         \
//     })

/* --- PRINTF_BYTE_TO_BINARY macro's --- */
#define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT8(i)                                                                                                                                       \
    (((i)&0x80ll) ? '1' : '0'), (((i)&0x40ll) ? '1' : '0'), (((i)&0x20ll) ? '1' : '0'), (((i)&0x10ll) ? '1' : '0'), (((i)&0x08ll) ? '1' : '0'), (((i)&0x04ll) ? '1' : '0'), \
            (((i)&0x02ll) ? '1' : '0'), (((i)&0x01ll) ? '1' : '0')

#define PRINTF_BINARY_PATTERN_INT16 PRINTF_BINARY_PATTERN_INT8 PRINTF_BINARY_PATTERN_INT8
#define PRINTF_BYTE_TO_BINARY_INT16(i) PRINTF_BYTE_TO_BINARY_INT8((i) >> 8), PRINTF_BYTE_TO_BINARY_INT8(i)
#define PRINTF_BINARY_PATTERN_INT32 PRINTF_BINARY_PATTERN_INT16 PRINTF_BINARY_PATTERN_INT16
#define PRINTF_BYTE_TO_BINARY_INT32(i) PRINTF_BYTE_TO_BINARY_INT16((i) >> 16), PRINTF_BYTE_TO_BINARY_INT16(i)
#define PRINTF_BINARY_PATTERN_INT64 PRINTF_BINARY_PATTERN_INT32 PRINTF_BINARY_PATTERN_INT32
#define PRINTF_BYTE_TO_BINARY_INT64(i) PRINTF_BYTE_TO_BINARY_INT32((i) >> 32), PRINTF_BYTE_TO_BINARY_INT32(i)
/* --- end macros --- */

// vsnprintf replacement from Valentin Milea:
// http://stackoverflow.com/questions/2915672/snprintf-and-visual-studio-2010
#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

METADOT_INLINE int c99_vsnprintf(char* outBuf, size_t size, const char* format, va_list ap) {
    int count = -1;

    if (size != 0) count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1) count = _vscprintf(format, ap);

    return count;
}

METADOT_INLINE int c99_snprintf(char* outBuf, size_t size, const char* format, ...) {
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}

#endif

// Trie structures and interface

// Add all supported types here, as 'Trie_type'
typedef enum TrieType { Trie_None, Trie_Pointer, Trie_String, Trie_vec3, Trie_double, Trie_float, Trie_char, Trie_int } TrieType;

// Structure used to retrieve all the data from the trie
// Add all supported types inside the union, as 'type* typeValue'
typedef struct TrieElement {
    char* key;
    TrieType type;
    unsigned size;
    union {
        void* pointerValue;
        char* stringValue;
        vec3* vector3Value;
        F64* doubleValue;
        F32* floatValue;
        char* charValue;
        int* intValue;
    };
} TrieElement;

#define TRIE_ALPHABET_SIZE 256
typedef struct TrieCell {
    TrieType elementType;
    unsigned maxKeySize;
    union {
        unsigned numberOfElements;
        unsigned elementSize;
    };
    // In a leaf and branch, the '\0' points to the data stored, while in a trunk it points to NULL
    // In both branch and trunk, all the other characters points to other Tries
    void* branch[TRIE_ALPHABET_SIZE];
} Trie;

Trie InitTrie();
void FreeTrie(Trie* trie);

void InsertTrie(Trie* trie, const char* key, const void* value, int size, TrieType valueType);
void InsertTrieString(Trie* trie, const char* key, const char* value);

int TrieContainsKey(Trie trie, const char* key);
void* GetTrieElement(Trie trie, const char* key);
void* GetTrieElementWithProperties(Trie trie, const char* key, int* sizeOut, TrieType* typeOut);
void* GetTrieElementAsPointer(Trie trie, const char* key, void* defaultValue);
char* GetTrieElementAsString(Trie trie, const char* key, char* defaultValue);

// Macro to generate headers for the insertion and retrieval functions
// Remember to call the function template macro on utils.c and to modify the TrieType enum when adding more types
#define TRIE_TYPE_FUNCTION_HEADER_MACRO(type)                        \
    void InsertTrie_##type(Trie* trie, const char* key, type value); \
    type GetTrieElementAs_##type(Trie trie, const char* key, type defaultValue);

TRIE_TYPE_FUNCTION_HEADER_MACRO(vec3)
TRIE_TYPE_FUNCTION_HEADER_MACRO(F64)
TRIE_TYPE_FUNCTION_HEADER_MACRO(F32)
TRIE_TYPE_FUNCTION_HEADER_MACRO(char)
TRIE_TYPE_FUNCTION_HEADER_MACRO(int)

// Functions to obtain all data inside a trie
// The data returned should be used before any new replace modifications
// are made in the trie, as the data pointed can be freed when replaced
TrieElement* GetTrieElementsArray(Trie trie, int* outElementsCount);
void FreeTrieElementsArray(TrieElement* elementsArray, int elementsCount);

cJSON* OpenJSON(char path[], char name[]);
F64 JSON_GetObjectDouble(cJSON* object, char* string, F64 defaultValue);
vec3 JSON_GetObjectVector3(cJSON* object, char* string, vec3 defaultValue);
cJSON* JSON_CreateVector3(vec3 value);

void Vector3ToTable(lua_State* L, vec3 vector);

int StringCompareEqual(char* stringA, char* stringB);
int StringCompareEqualCaseInsensitive(char* stringA, char* stringB);

inline R_bool startsWith(const char* s, const char* prefix) { return strlen(prefix) <= strlen(s) && (strncmp(prefix, s, strlen(prefix)) == 0); }

#endif
