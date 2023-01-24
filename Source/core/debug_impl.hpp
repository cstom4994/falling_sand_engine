// Copyright(c) 2022-2023, KaoruXun All rights reserved.

#ifndef _METADOT_DEBUGIMPL_HPP_
#define _METADOT_DEBUGIMPL_HPP_

#include "core/core.hpp"
#include "core/macros.h"
#include "core/cpp/vector.hpp"
#include "engine/imgui/imgui_impl.hpp"

extern int metadot_buildnum(void);
extern const std::string metadot_metadata(void);

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <ios>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#define DBG_MACRO_CXX_STANDARD 17

#if DBG_MACRO_CXX_STANDARD >= 17
#include <optional>
#include <variant>
#endif

namespace dbg {

struct time {};

namespace pretty_function {

// Compiler-agnostic version of __PRETTY_FUNCTION__ and constants to
// extract the template argument in `type_name_impl`

#if defined(__clang__)
#define DBG_MACRO_PRETTY_FUNCTION __PRETTY_FUNCTION__
static constexpr size_t PREFIX_LENGTH = sizeof("const char *dbg::type_name_impl() [T = ") - 1;
static constexpr size_t SUFFIX_LENGTH = sizeof("]") - 1;
#elif defined(__GNUC__) && !defined(__clang__)
#define DBG_MACRO_PRETTY_FUNCTION __PRETTY_FUNCTION__
static constexpr size_t PREFIX_LENGTH = sizeof("const char* dbg::type_name_impl() [with T = ") - 1;
static constexpr size_t SUFFIX_LENGTH = sizeof("]") - 1;
#elif defined(_MSC_VER)
#define DBG_MACRO_PRETTY_FUNCTION __FUNCSIG__
static constexpr size_t PREFIX_LENGTH = sizeof("const char *__cdecl dbg::type_name_impl<") - 1;
static constexpr size_t SUFFIX_LENGTH = sizeof(">(void)") - 1;
#else
#error "This compiler is currently not supported by dbg_macro."
#endif

}  // namespace pretty_function

// Formatting helpers

template <typename T>
struct print_formatted {
    static_assert(std::is_integral<T>::value, "Only integral types are supported.");

    print_formatted(T value, int numeric_base) : inner(value), base(numeric_base) {}

    operator T() const { return inner; }

    const char *prefix() const {
        switch (base) {
            case 8:
                return "0o";
            case 16:
                return "0x";
            case 2:
                return "0b";
            default:
                return "";
        }
    }

    T inner;
    int base;
};

template <typename T>
print_formatted<T> hex(T value) {
    return print_formatted<T>{value, 16};
}

template <typename T>
print_formatted<T> oct(T value) {
    return print_formatted<T>{value, 8};
}

template <typename T>
print_formatted<T> bin(T value) {
    return print_formatted<T>{value, 2};
}

// Implementation of 'type_name<T>()'

template <typename T>
const char *type_name_impl() {
    return DBG_MACRO_PRETTY_FUNCTION;
}

template <typename T>
struct type_tag {};

template <int &...ExplicitArgumentBarrier, typename T>
std::string get_type_name(type_tag<T>) {
    namespace pf = pretty_function;

    std::string type = type_name_impl<T>();
    return type.substr(pf::PREFIX_LENGTH, type.size() - pf::PREFIX_LENGTH - pf::SUFFIX_LENGTH);
}

template <typename T>
std::string type_name() {
    if (std::is_volatile<T>::value) {
        if (std::is_pointer<T>::value) {
            return type_name<typename std::remove_volatile<T>::type>() + " volatile";
        } else {
            return "volatile " + type_name<typename std::remove_volatile<T>::type>();
        }
    }
    if (std::is_const<T>::value) {
        if (std::is_pointer<T>::value) {
            return type_name<typename std::remove_const<T>::type>() + " const";
        } else {
            return "const " + type_name<typename std::remove_const<T>::type>();
        }
    }
    if (std::is_pointer<T>::value) {
        return type_name<typename std::remove_pointer<T>::type>() + "*";
    }
    if (std::is_lvalue_reference<T>::value) {
        return type_name<typename std::remove_reference<T>::type>() + "&";
    }
    if (std::is_rvalue_reference<T>::value) {
        return type_name<typename std::remove_reference<T>::type>() + "&&";
    }
    return get_type_name(type_tag<T>{});
}

inline std::string get_type_name(type_tag<short>) { return "short"; }

inline std::string get_type_name(type_tag<unsigned short>) { return "unsigned short"; }

inline std::string get_type_name(type_tag<long>) { return "long"; }

inline std::string get_type_name(type_tag<unsigned long>) { return "unsigned long"; }

inline std::string get_type_name(type_tag<std::string>) { return "std::string"; }

template <typename T>
std::string get_type_name(type_tag<std::vector<T, std::allocator<T>>>) {
    return "std::vector<" + type_name<T>() + ">";
}
template <typename T>
std::string get_type_name(type_tag<MetaEngine::vector<T, std::allocator<T>>>) {
    return "MetaEngine::vector<" + type_name<T>() + ">";
}

template <typename T1, typename T2>
std::string get_type_name(type_tag<std::pair<T1, T2>>) {
    return "std::pair<" + type_name<T1>() + ", " + type_name<T2>() + ">";
}

template <typename... T>
std::string type_list_to_string() {
    std::string result;
    auto unused = {(result += type_name<T>() + ", ", 0)..., 0};
    static_cast<void>(unused);

#if DBG_MACRO_CXX_STANDARD >= 17
    if constexpr (sizeof...(T) > 0) {
#else
    if (sizeof...(T) > 0) {
#endif
        result.pop_back();
        result.pop_back();
    }
    return result;
}

template <typename... T>
std::string get_type_name(type_tag<std::tuple<T...>>) {
    return "std::tuple<" + type_list_to_string<T...>() + ">";
}

template <typename T>
inline std::string get_type_name(type_tag<print_formatted<T>>) {
    return type_name<T>();
}

// Implementation of 'is_detected' to specialize for container-like types

namespace detail_detector {

struct nonesuch {
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(nonesuch const &) = delete;
    void operator=(nonesuch const &) = delete;
};

template <typename...>
using void_t = void;

template <class Default, class AlwaysVoid, template <class...> class Op, class... Args>
struct detector {
    using value_t = std::false_type;
    using type = Default;
};

template <class Default, template <class...> class Op, class... Args>
struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
    using value_t = std::true_type;
    using type = Op<Args...>;
};

}  // namespace detail_detector

template <template <class...> class Op, class... Args>
using is_detected = typename detail_detector::detector<detail_detector::nonesuch, void, Op, Args...>::value_t;

namespace detail {

namespace {
using std::begin;
using std::end;
#if DBG_MACRO_CXX_STANDARD < 17
template <typename T>
constexpr auto size(const T &c) -> decltype(c.size()) {
    return c.size();
}
template <typename T, std::size_t N>
constexpr std::size_t size(const T (&)[N]) {
    return N;
}
#else
using std::size;
#endif
}  // namespace

template <typename T>
using detect_begin_t = decltype(detail::begin(std::declval<T>()));

template <typename T>
using detect_end_t = decltype(detail::end(std::declval<T>()));

template <typename T>
using detect_size_t = decltype(detail::size(std::declval<T>()));

template <typename T>
struct is_container {
    static constexpr bool value = is_detected<detect_begin_t, T>::value && is_detected<detect_end_t, T>::value && is_detected<detect_size_t, T>::value &&
                                  !std::is_same<std::string, typename std::remove_cv<typename std::remove_reference<T>::type>::type>::value;
};

template <typename T>
using ostream_operator_t = decltype(std::declval<std::ostream &>() << std::declval<T>());

template <typename T>
struct has_ostream_operator : is_detected<ostream_operator_t, T> {};

}  // namespace detail

// Helper to dbg(…)-print types
template <typename T>
struct print_type {};

template <typename T>
print_type<T> type() {
    return print_type<T>{};
}

// Forward declarations of "pretty_print"

template <typename T>
inline void pretty_print(std::ostream &stream, const T &value, std::true_type);

template <typename T>
inline void pretty_print(std::ostream &, const T &, std::false_type);

template <typename T>
inline typename std::enable_if<!detail::is_container<const T &>::value && !std::is_enum<T>::value, bool>::type pretty_print(std::ostream &stream, const T &value);

inline bool pretty_print(std::ostream &stream, const bool &value);

inline bool pretty_print(std::ostream &stream, const char &value);

template <typename P>
inline bool pretty_print(std::ostream &stream, P *const &value);

template <typename T, typename Deleter>
inline bool pretty_print(std::ostream &stream, std::unique_ptr<T, Deleter> &value);

template <typename T>
inline bool pretty_print(std::ostream &stream, std::shared_ptr<T> &value);

template <size_t N>
inline bool pretty_print(std::ostream &stream, const char (&value)[N]);

template <>
inline bool pretty_print(std::ostream &stream, const char *const &value);

template <typename... Ts>
inline bool pretty_print(std::ostream &stream, const std::tuple<Ts...> &value);

template <>
inline bool pretty_print(std::ostream &stream, const std::tuple<> &);

template <>
inline bool pretty_print(std::ostream &stream, const time &);

template <typename T>
inline bool pretty_print(std::ostream &stream, const print_formatted<T> &value);

template <typename T>
inline bool pretty_print(std::ostream &stream, const print_type<T> &);

template <typename Enum>
inline typename std::enable_if<std::is_enum<Enum>::value, bool>::type pretty_print(std::ostream &stream, Enum const &value);

inline bool pretty_print(std::ostream &stream, const std::string &value);

#if DBG_MACRO_CXX_STANDARD >= 17

inline bool pretty_print(std::ostream &stream, const std::string_view &value);

#endif

template <typename T1, typename T2>
inline bool pretty_print(std::ostream &stream, const std::pair<T1, T2> &value);

#if DBG_MACRO_CXX_STANDARD >= 17

template <typename T>
inline bool pretty_print(std::ostream &stream, const std::optional<T> &value);

template <typename... Ts>
inline bool pretty_print(std::ostream &stream, const std::variant<Ts...> &value);

#endif

template <typename Container>
inline typename std::enable_if<detail::is_container<const Container &>::value, bool>::type pretty_print(std::ostream &stream, const Container &value);

// Specializations of "pretty_print"

template <typename T>
inline void pretty_print(std::ostream &stream, const T &value, std::true_type) {
    stream << value;
}

template <typename T>
inline void pretty_print(std::ostream &, const T &, std::false_type) {
    static_assert(detail::has_ostream_operator<const T &>::value, "Type does not support the << ostream operator");
}

template <typename T>
inline typename std::enable_if<!detail::is_container<const T &>::value && !std::is_enum<T>::value, bool>::type pretty_print(std::ostream &stream, const T &value) {
    pretty_print(stream, value, typename detail::has_ostream_operator<const T &>::type{});
    return true;
}

inline bool pretty_print(std::ostream &stream, const bool &value) {
    stream << std::boolalpha << value;
    return true;
}

inline bool pretty_print(std::ostream &stream, const char &value) {
    const bool printable = value >= 0x20 && value <= 0x7E;

    if (printable) {
        stream << "'" << value << "'";
    } else {
        stream << "'\\x" << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << (0xFF & value) << "'";
    }
    return true;
}

template <typename P>
inline bool pretty_print(std::ostream &stream, P *const &value) {
    if (value == nullptr) {
        stream << "nullptr";
    } else {
        stream << value;
    }
    return true;
}

template <typename T, typename Deleter>
inline bool pretty_print(std::ostream &stream, std::unique_ptr<T, Deleter> &value) {
    pretty_print(stream, value.get());
    return true;
}

template <typename T>
inline bool pretty_print(std::ostream &stream, std::shared_ptr<T> &value) {
    pretty_print(stream, value.get());
    stream << " (use_count = " << value.use_count() << ")";

    return true;
}

template <size_t N>
inline bool pretty_print(std::ostream &stream, const char (&value)[N]) {
    stream << value;
    return false;
}

template <>
inline bool pretty_print(std::ostream &stream, const char *const &value) {
    stream << '"' << value << '"';
    return true;
}

template <size_t Idx>
struct pretty_print_tuple {
    template <typename... Ts>
    static void print(std::ostream &stream, const std::tuple<Ts...> &tuple) {
        pretty_print_tuple<Idx - 1>::print(stream, tuple);
        stream << ", ";
        pretty_print(stream, std::get<Idx>(tuple));
    }
};

template <>
struct pretty_print_tuple<0> {
    template <typename... Ts>
    static void print(std::ostream &stream, const std::tuple<Ts...> &tuple) {
        pretty_print(stream, std::get<0>(tuple));
    }
};

template <typename... Ts>
inline bool pretty_print(std::ostream &stream, const std::tuple<Ts...> &value) {
    stream << "{";
    pretty_print_tuple<sizeof...(Ts) - 1>::print(stream, value);
    stream << "}";

    return true;
}

template <>
inline bool pretty_print(std::ostream &stream, const std::tuple<> &) {
    stream << "{}";

    return true;
}

template <>
inline bool pretty_print(std::ostream &stream, const time &) {
    using namespace std::chrono;

    const auto now = system_clock::now();
    const auto us = duration_cast<microseconds>(now.time_since_epoch()).count() % 1000000;
    const auto hms = system_clock::to_time_t(now);
#if _MSC_VER >= 1600
    struct tm t;
    localtime_s(&t, &hms);
    const std::tm *tm = &t;
#else
    const std::tm *tm = std::localtime(&hms);
#endif
    stream << "current time = " << std::put_time(tm, "%H:%M:%S") << '.' << std::setw(6) << std::setfill('0') << us;
    return false;
}

// Converts decimal integer to binary string
template <typename T>
std::string decimalToBinary(T n) {
    const size_t length = 8 * sizeof(T);
    std::string toRet;
    toRet.resize(length);

    for (size_t i = 0; i < length; ++i) {
        const auto bit_at_index_i = static_cast<char>((n >> i) & 1);
        toRet[length - 1 - i] = bit_at_index_i + '0';
    }

    return toRet;
}

template <typename T>
inline bool pretty_print(std::ostream &stream, const print_formatted<T> &value) {
    if (value.inner < 0) {
        stream << "-";
    }
    stream << value.prefix();

    // Print using setbase
    if (value.base != 2) {
        stream << std::setw(sizeof(T)) << std::setfill('0') << std::setbase(value.base) << std::uppercase;

        if (value.inner >= 0) {
            // The '+' sign makes sure that a uint_8 is printed as a number
            stream << +value.inner;
        } else {
            using unsigned_type = typename std::make_unsigned<T>::type;
            stream << +(static_cast<unsigned_type>(-(value.inner + 1)) + 1);
        }
    } else {
        // Print for binary
        if (value.inner >= 0) {
            stream << decimalToBinary(value.inner);
        } else {
            using unsigned_type = typename std::make_unsigned<T>::type;
            stream << decimalToBinary<unsigned_type>(static_cast<unsigned_type>(-(value.inner + 1)) + 1);
        }
    }

    return true;
}

template <typename T>
inline bool pretty_print(std::ostream &stream, const print_type<T> &) {
    stream << type_name<T>();

    stream << " [sizeof: " << sizeof(T) << " byte, ";

    stream << "trivial: ";
    if (std::is_trivial<T>::value) {
        stream << "yes";
    } else {
        stream << "no";
    }

    stream << ", standard layout: ";
    if (std::is_standard_layout<T>::value) {
        stream << "yes";
    } else {
        stream << "no";
    }
    stream << "]";

    return false;
}

template <typename Enum>
inline typename std::enable_if<std::is_enum<Enum>::value, bool>::type pretty_print(std::ostream &stream, Enum const &value) {
    using UnderlyingType = typename std::underlying_type<Enum>::type;
    stream << static_cast<UnderlyingType>(value);

    return true;
}

inline bool pretty_print(std::ostream &stream, const std::string &value) {
    stream << '"' << value << '"';
    return true;
}

#if DBG_MACRO_CXX_STANDARD >= 17

inline bool pretty_print(std::ostream &stream, const std::string_view &value) {
    stream << '"' << std::string(value) << '"';
    return true;
}

#endif

template <typename T1, typename T2>
inline bool pretty_print(std::ostream &stream, const std::pair<T1, T2> &value) {
    stream << "{";
    pretty_print(stream, value.first);
    stream << ", ";
    pretty_print(stream, value.second);
    stream << "}";
    return true;
}

#if DBG_MACRO_CXX_STANDARD >= 17

template <typename T>
inline bool pretty_print(std::ostream &stream, const std::optional<T> &value) {
    if (value) {
        stream << '{';
        pretty_print(stream, *value);
        stream << '}';
    } else {
        stream << "nullopt";
    }

    return true;
}

template <typename... Ts>
inline bool pretty_print(std::ostream &stream, const std::variant<Ts...> &value) {
    stream << "{";
    std::visit([&stream](auto &&arg) { pretty_print(stream, arg); }, value);
    stream << "}";

    return true;
}

#endif

template <typename Container>
inline typename std::enable_if<detail::is_container<const Container &>::value, bool>::type pretty_print(std::ostream &stream, const Container &value) {
    stream << "{";
    const size_t size = detail::size(value);
    const size_t n = std::min(size_t{10}, size);
    size_t i = 0;
    using std::begin;
    using std::end;
    for (auto it = begin(value); it != end(value) && i < n; ++it, ++i) {
        pretty_print(stream, *it);
        if (i != n - 1) {
            stream << ", ";
        }
    }

    if (size > n) {
        stream << ", ...";
        stream << " size:" << size;
    }

    stream << "}";
    return true;
}

template <typename T, typename... U>
struct last {
    using type = typename last<U...>::type;
};

template <typename T>
struct last<T> {
    using type = T;
};

template <typename... T>
using last_t = typename last<T...>::type;

class DebugOutput {
public:
    // Helper alias to avoid obscure type `const char* const*` in signature.
    using expr_t = const char *;

    DebugOutput(const char *filepath, int line, const char *function_name) {
        std::string path = filepath;
        const std::size_t path_length = path.length();
        if (path_length > MAX_PATH_LENGTH) {
            path = ".." + path.substr(path_length - MAX_PATH_LENGTH, MAX_PATH_LENGTH);
        }
        std::stringstream ss;
        ss << "[" << path << ":" << line << " (" << function_name << ")] ";
        m_location = ss.str();
    }

    template <typename... T>
    auto print(std::initializer_list<expr_t> exprs, std::initializer_list<std::string> types, T &&...values) -> last_t<T...> {
        if (exprs.size() != sizeof...(values)) {
            std::cout << m_location << "The number of arguments mismatch, please check unprotected comma" << std::endl;
        }
        return print_impl(exprs.begin(), types.begin(), std::forward<T>(values)...);
    }

private:
    template <typename T>
    T &&print_impl(const expr_t *expr, const std::string *type, T &&value) {
        const T &ref = value;
        std::stringstream stream_value;
        const bool print_expr_and_type = pretty_print(stream_value, ref);

        std::stringstream output;
        output << m_location;
        if (print_expr_and_type) {
            output << *expr << " = ";
        }
        output << stream_value.str();
        if (print_expr_and_type) {
            output << " (" << *type << ")";
        }
        output << std::endl;
        std::cout << output.str();

        return std::forward<T>(value);
    }

    template <typename T, typename... U>
    auto print_impl(const expr_t *exprs, const std::string *types, T &&value, U &&...rest) -> last_t<T, U...> {
        print_impl(exprs, types, std::forward<T>(value));
        return print_impl(exprs + 1, types + 1, std::forward<U>(rest)...);
    }

    std::string m_location;

    static constexpr std::size_t MAX_PATH_LENGTH = 20;
};

// Identity function to suppress "-Wunused-value" warnings in DBG_MACRO_DISABLE
// mode
template <typename T>
T &&identity(T &&t) {
    return std::forward<T>(t);
}

template <typename T, typename... U>
auto identity(T &&, U &&...u) -> last_t<U...> {
    return identity(std::forward<U>(u)...);
}

}  // namespace dbg

#ifndef DBG_MACRO_DISABLE

// Force expanding argument with commas for MSVC, ref:
// https://stackoverflow.com/questions/35210637/macro-expansion-argument-with-commas
// Note that "args" should be a tuple with parentheses, such as "(e1, e2, ...)".
#define DBG_IDENTITY(x) x
#define DBG_CALL(fn, args) DBG_IDENTITY(fn args)

#define DBG_CAT_IMPL(_1, _2) _1##_2
#define DBG_CAT(_1, _2) DBG_CAT_IMPL(_1, _2)

#define DBG_16TH_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, ...) _16
#define DBG_16TH(args) DBG_CALL(DBG_16TH_IMPL, args)
#define DBG_NARG(...) DBG_16TH((__VA_ARGS__, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

// DBG_VARIADIC_CALL(fn, data, e1, e2, ...) => fn_N(data, (e1, e2, ...))
#define DBG_VARIADIC_CALL(fn, data, ...)  \
    DBG_CAT(fn##_, DBG_NARG(__VA_ARGS__)) \
    (data, (__VA_ARGS__))

// (e1, e2, e3, ...) => e1
#define DBG_HEAD_IMPL(_1, ...) _1
#define DBG_HEAD(args) DBG_CALL(DBG_HEAD_IMPL, args)

// (e1, e2, e3, ...) => (e2, e3, ...)
#define DBG_TAIL_IMPL(_1, ...) (__VA_ARGS__)
#define DBG_TAIL(args) DBG_CALL(DBG_TAIL_IMPL, args)

#define DBG_MAP_1(fn, args) DBG_CALL(fn, args)
#define DBG_MAP_2(fn, args) fn(DBG_HEAD(args)), DBG_MAP_1(fn, DBG_TAIL(args))
#define DBG_MAP_3(fn, args) fn(DBG_HEAD(args)), DBG_MAP_2(fn, DBG_TAIL(args))
#define DBG_MAP_4(fn, args) fn(DBG_HEAD(args)), DBG_MAP_3(fn, DBG_TAIL(args))
#define DBG_MAP_5(fn, args) fn(DBG_HEAD(args)), DBG_MAP_4(fn, DBG_TAIL(args))
#define DBG_MAP_6(fn, args) fn(DBG_HEAD(args)), DBG_MAP_5(fn, DBG_TAIL(args))
#define DBG_MAP_7(fn, args) fn(DBG_HEAD(args)), DBG_MAP_6(fn, DBG_TAIL(args))
#define DBG_MAP_8(fn, args) fn(DBG_HEAD(args)), DBG_MAP_7(fn, DBG_TAIL(args))
#define DBG_MAP_9(fn, args) fn(DBG_HEAD(args)), DBG_MAP_8(fn, DBG_TAIL(args))
#define DBG_MAP_10(fn, args) fn(DBG_HEAD(args)), DBG_MAP_9(fn, DBG_TAIL(args))
#define DBG_MAP_11(fn, args) fn(DBG_HEAD(args)), DBG_MAP_10(fn, DBG_TAIL(args))
#define DBG_MAP_12(fn, args) fn(DBG_HEAD(args)), DBG_MAP_11(fn, DBG_TAIL(args))
#define DBG_MAP_13(fn, args) fn(DBG_HEAD(args)), DBG_MAP_12(fn, DBG_TAIL(args))
#define DBG_MAP_14(fn, args) fn(DBG_HEAD(args)), DBG_MAP_13(fn, DBG_TAIL(args))
#define DBG_MAP_15(fn, args) fn(DBG_HEAD(args)), DBG_MAP_14(fn, DBG_TAIL(args))
#define DBG_MAP_16(fn, args) fn(DBG_HEAD(args)), DBG_MAP_15(fn, DBG_TAIL(args))

// DBG_MAP(fn, e1, e2, e3, ...) => fn(e1), fn(e2), fn(e3), ...
#define DBG_MAP(fn, ...) DBG_VARIADIC_CALL(DBG_MAP, fn, __VA_ARGS__)

#define DBG_STRINGIFY_IMPL(x) #x
#define DBG_STRINGIFY(x) DBG_STRINGIFY_IMPL(x)

#define DBG_TYPE_NAME(x) dbg::type_name<decltype(x)>()

#define METADOT_DBG(...) dbg::DebugOutput(__FILE__, __LINE__, __func__).print({DBG_MAP(DBG_STRINGIFY, __VA_ARGS__)}, {DBG_MAP(DBG_TYPE_NAME, __VA_ARGS__)}, __VA_ARGS__)
#else
#define METADOT_DBG(...) dbg::identity(__VA_ARGS__)
#endif  // DBG_MACRO_DISABLE

// Unit-testing framework. zlib/libpng licensed.

#ifndef METADOT_UNIT_TESING
#define METADOT_UNIT_TESING

#define METADOT_UNIT(...) (!!(__VA_ARGS__) ? (METADOT_UNIT::suite(#__VA_ARGS__, __FILE__, __LINE__, 1) < __VA_ARGS__) : (METADOT_UNIT::suite(#__VA_ARGS__, __FILE__, __LINE__, 0) < __VA_ARGS__))
#define METADOT_UNITS(...)                                                        \
    static void METADOT_UNIT$line(METADOT_UNIT)();                                \
    static const bool METADOT_UNIT$line(dsstSuite_) = METADOT_UNIT::suite::queue( \
            []() {                                                                \
                std::string title = "" __VA_ARGS__;                               \
                if (title.empty()) title = "Suite";                               \
                fprintf(stderr, "------  %s\n", title.c_str());                   \
                METADOT_UNIT$line(METADOT_UNIT)();                                \
            },                                                                    \
            "" #__VA_ARGS__);                                                     \
    void METADOT_UNIT$line(METADOT_UNIT)()
#define throws(...)      \
    ([&]() {             \
        try {            \
            __VA_ARGS__; \
        } catch (...) {  \
            return true; \
        }                \
        return false;    \
    }())

/* API details following */
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <functional>
#include <sstream>
#include <string>

namespace METADOT_UNIT {
using namespace std;
using timer = chrono::high_resolution_clock;
template <typename T>
inline string to_str(const T &t) {
    stringstream ss;
    return (ss << t) ? ss.str() : string("??");
}
template <>
inline string to_str(const timer::time_point &start) {
    return to_str(double((timer::now() - start).count()) * timer::period::num / timer::period::den);
}
class suite {
    timer::time_point start = timer::now();
    deque<string> xpr;
    int ok = false, has_bp = false;
    enum { BREAKPOINT, BREAKPOINTS, PASSED, FAILED, TESTNO };
    static unsigned &get(int i) {
        static unsigned var[TESTNO + 1] = {};
        return var[i];
    }

public:
    static bool queue(const function<void()> &fn, const string &text) {
        static auto start = timer::now();
        static struct install : public deque<function<void()>> {
            install() : deque<function<void()>>() { get(BREAKPOINT) = stoul(getenv("BREAKON") ? getenv("BREAKON") : "0"); }
            ~install() {
                for (auto &fn : *this) fn();
                string ss, run = to_str(get(PASSED) + get(FAILED)), res = get(FAILED) ? "[FAIL]  " : "[ OK ]  ";
                if (get(FAILED))
                    ss += res + "Failure! " + to_str(get(FAILED)) + '/' + run + " tests failed :(\n";
                else
                    ss += res + "Success: " + run + " tests passed :)\n";
                ss += "        Breakpoints: " + to_str(get(BREAKPOINTS)) + " (*)\n";
                ss += "        Total time: " + to_str(start) + " seconds.\n";
                fprintf(stderr, "\n%s", ss.c_str());
                if (get(FAILED)) std::exit(get(FAILED));
            }
        } queue;
        return text.find("before main()") == string::npos ? (queue.push_back(fn), 0) : (fn(), 1);
    }
    suite(const char *const text, const char *const file, int line, bool result) : xpr({string(file) + ':' + to_str(line), " - ", text, ""}), ok(result) {
        xpr[0] = "Test " + to_str(++get(TESTNO)) + " at " + xpr[0];
        if (0 != (has_bp = (get(TESTNO) == get(BREAKPOINT)))) {
            get(BREAKPOINTS)++;
            fprintf(stderr, "MetaDotUnitTest : breaking on test #%d\n\t", get(TESTNO));
            assert(!"MetaDotUnitTest : breakpoint requested");
            fprintf(stderr, "%s", "\nMetaDotUnitTest : breakpoint failed!\n");
        }
    }
    ~suite() {
        if (xpr.empty()) return;
        operator bool(), queue([&]() { get(ok ? PASSED : FAILED)++; }, "before main()");
        string res[] = {"[FAIL]", "[ OK ]", "  ", " *", "        ", ""}, *bp = &res[2], *tab = &res[4];
        xpr[0] = res[ok] + bp[has_bp] + xpr[0] + " (" + to_str(start) + " s)" + (xpr[1].size() > 3 && !ok ? xpr[1] : tab[1]);
        xpr.erase(xpr.begin() + 1);
        if (ok)
            xpr = {xpr[0]};
        else {
            xpr[1].resize((xpr[1] != (xpr[2] = xpr[2].substr(xpr[2][2] == ' ' ? 3 : 4))) * xpr[1].size());
            xpr.push_back("(unexpected)");
        }
        for (auto end = xpr.size(), it = end - end; it < end; ++it) {
            fprintf(stderr, &"\0%s%s\n"[!xpr[it].empty()], tab[!it].c_str(), xpr[it].c_str());
        }
    }
#define METADOT_UNIT$join(str, num) str##num
#define METADOT_UNIT$glue(str, num) METADOT_UNIT$join(str, num)
#define METADOT_UNIT$line(str) METADOT_UNIT$glue(str, __LINE__)
#define METADOT_UNIT$(OP)                                  \
    template <typename T>                                  \
    suite &operator OP(const T &rhs) {                     \
        return xpr[3] += " " #OP " " + to_str(rhs), *this; \
    }                                                      \
    template <unsigned N>                                  \
    suite &operator OP(const char(&rhs)[N]) {              \
        return xpr[3] += " " #OP " " + to_str(rhs), *this; \
    }
    template <typename T>
    suite &operator<<(const T &t) {
        return xpr[1] += to_str(t), *this;
    }
    template <unsigned N>
    suite &operator<<(const char (&str)[N]) {
        return xpr[1] += to_str(str), *this;
    }
    operator bool() {
        return xpr.size() >= 3 && xpr[3].size() >= 6 && [&]() -> bool {
            char signR = xpr[3].at(2);
            bool equal = xpr[3].substr(4 + xpr[3].size() / 2) == xpr[3].substr(3, xpr[3].size() / 2 - 3);
            return ok = (signR == '=' ? equal : (signR == '!' ? !equal : ok));
        }(),
               ok;
    }
    METADOT_UNIT$(<)
    METADOT_UNIT$(<=)
    METADOT_UNIT$(>)
    METADOT_UNIT$(>=)
    METADOT_UNIT$(!=)
    METADOT_UNIT$(==) METADOT_UNIT$(&&) METADOT_UNIT$(||)
};
}  // namespace METADOT_UNIT
#endif

class Profiler {
public:
    enum Stage {
        SdlInput,
        GameTick,
        Rendering,
        RenderEarly,
        RenderLate,

        _StageCount,
    };

    struct Scope {
        ImU8 _level;
        std::chrono::system_clock::time_point _start;
        std::chrono::system_clock::time_point _end;
        bool _finalized = false;
    };

    struct Entry {
        std::chrono::system_clock::time_point _frameStart;
        std::chrono::system_clock::time_point _frameEnd;
        std::array<Scope, _StageCount> _stages;
    };

    void Frame() {
        auto &prevEntry = _entries[_currentEntry];
        _currentEntry = (_currentEntry + 1) % _bufferSize;
        prevEntry._frameEnd = _entries[_currentEntry]._frameStart = std::chrono::system_clock::now();
    }

    void Begin(Stage stage) {
        assert(_currentLevel < 255);
        auto &entry = _entries[_currentEntry]._stages[stage];
        entry._level = _currentLevel;
        _currentLevel++;
        entry._start = std::chrono::system_clock::now();
        entry._finalized = false;
    }

    void End(Stage stage) {
        assert(_currentLevel > 0);
        auto &entry = _entries[_currentEntry]._stages[stage];
        assert(!entry._finalized);
        _currentLevel--;
        assert(entry._level == _currentLevel);
        entry._end = std::chrono::system_clock::now();
        entry._finalized = true;
    }

    ImU8 GetCurrentEntryIndex() const { return (_currentEntry + _bufferSize - 1) % _bufferSize; }

    static const ImU8 _bufferSize = 100;
    std::array<Entry, _bufferSize> _entries;

private:
    ImU8 _currentEntry = _bufferSize - 1;
    ImU8 _currentLevel = 0;
};

static const std::array<const char *, Profiler::_StageCount> stageNames = {
        "SDL Input", "Game Tick", "Rendering", "Render Early", "Render Late",
};

static void ProfilerValueGetter(float *startTimestamp, float *endTimestamp, ImU8 *level, const char **caption, const void *data, int idx) {
    auto entry = reinterpret_cast<const Profiler::Entry *>(data);
    auto &stage = entry->_stages[idx];
    if (startTimestamp) {
        std::chrono::duration<float, std::milli> fltStart = stage._start - entry->_frameStart;
        *startTimestamp = fltStart.count();
    }
    if (endTimestamp) {
        *endTimestamp = stage._end.time_since_epoch().count() / 1000000.0f;

        std::chrono::duration<float, std::milli> fltEnd = stage._end - entry->_frameStart;
        *endTimestamp = fltEnd.count();
    }
    if (level) {
        *level = stage._level;
    }
    if (caption) {
        *caption = stageNames[idx];
    }
}

int metadot_buildnum(void);

#endif
